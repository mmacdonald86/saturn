#include "saturn/common.h"
#include "saturn/svr_model.h"
#include "saturn/feature_engine.h"
#include "saturn/utils.h"
#include "mars/mars.h"
#include "mars/numeric.h"
#include "mars/utils.h"

#include <any>
#include <fstream>
#include <tuple>

// #include <iostream>

namespace saturn
{

SvrModel::SvrModel(FeatureEngine & feature_engine, std::string path)
    : _feature_engine(feature_engine)
{
    // Removing trailing '/'.
    while (path.back() == '/') {
        path.pop_back();
    }
    if (path.length() == 0) {
        throw SaturnError("can not use root directory as `path` for model data");
    }

    _path = path;
    _model_id = path;  // TODO: improve this later, adding more info

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    mars::JsonReader jreader((_path + "/model_config.json").c_str());
    jreader.seek("features");
    _composer_id = f->add_composer(jreader);

    mars::AvroReader areader((_path + "/model_object.data").c_str());
    auto const class_name = areader.get_scalar<std::string>("class_name");
    if ("CatalogModel" != class_name) {
        throw SaturnError(mars::make_string(
                              "expecting a `CatalogModel` for `SvrModel`; get a `",
                              class_name,
                              "`"));
    }

    auto model = mars::CatalogModel::from_avro(areader);
    _mars_model = static_cast<void *>(model.release());

    // Read in default svr file.
    auto infile_svr = std::ifstream(_path + "/default_svr.txt");
    if (infile_svr.is_open()) {
        std::string brand_id;
        double nonlba_svr, lba_svr;
        while (infile_svr >> brand_id >> nonlba_svr >> lba_svr) {
            _default_svr.emplace(brand_id, std::make_tuple(nonlba_svr, lba_svr));
        }
        infile_svr.close();
    } else {
        throw SaturnError(mars::make_string(
                              "failed to read file `",
                              _path + "/default_svr.txt",
                              "`"));
    }

    // Read in parameters for multiplier curve.
    auto infile_curve = std::ifstream(_path + "/multiplier_curve.txt");
    if (infile_curve.is_open()) {
        std::string adgroup_id;
        double mu, sigma;
        while (infile_curve >> adgroup_id >> mu >> sigma) {
            _multiplier_curve.emplace(adgroup_id, std::make_tuple(mu, sigma));
        }
        infile_curve.close();
    }
}


SvrModel::~SvrModel()
{
    delete static_cast<mars::CatalogModel *>(_mars_model);
}


double SvrModel::_calc_multiplier(std::string const & brand_id, std::string const & adgroup_id, double user_brand_svr,
                                  double pacing)
{
    _feature_engine.update_field(FeatureEngine::FloatField::kUserExtlba, user_brand_svr);

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    auto x = f->render(_composer_id);

    auto m = static_cast<mars::CatalogModel *>(_mars_model);

    std::vector<std::string> tag{brand_id};
    if (adgroup_id.length() > 0 && m->n_tags() > 1) {
        tag.push_back(adgroup_id);
    }

    auto z = m->run(x, tag);

    // Version 0:
    //   CatalogModel contains ChainModel's.
    //
    // auto zz = std::any_cast<mars::ChainModel::result_type>(z);
    // _bid_multiplier = std::any_cast<double>(std::get<0>(zz));
    // _svr = std::get<1>(zz)[0][0];

    // Version 1:
    //   CatalogModel contains IsotonicRegression or IsotonicLinearInterpolation.
    //

    double quantile = std::any_cast<double>(z);

    double mu, sigma;
    auto it = _multiplier_curve.find(adgroup_id);
    if (it != _multiplier_curve.end()) {
        mu = std::get<0>(std::get<1>(*it));
        sigma = std::get<1>(std::get<1>(*it));
    } else {
        sigma = 0.5;
        if (pacing < 0.0) {
            mu = 0.;
        } else {
            if (pacing > 1.0) {
                throw SaturnError(mars::make_string(
                                    "argument `pacing` must be in {-1, [0, 1]}; got ",
                                    pacing
                                ));
            }
            mu = pacing * pacing * 2 - 1.;
            // Square, stretch to [0, 2], shift to [-1, 1].
        }
    }
    return mars::logitnormal_cdf(quantile, mu, sigma);
}


double SvrModel::_get_default_svr(std::string const & brand_id, int flag) const
{
    try {
        auto val = _default_svr.at(brand_id);
        if (flag == 0) {
            return std::get<0>(val);
        } else if (flag == 1) {
            return std::get<1>(val);
        } else {
            throw SaturnError(mars::make_string(
                                  "unknown flag value `", flag, "`"
                              ));
        }
    } catch (std::exception& e) {
        throw SaturnError(mars::make_string(
                              "failed to get default ",
                              flag == 0 ? "non-LBA" : "LBA",
                              " SVR for brand `",
                              brand_id,
                              "`; additional error message: ",
                              e.what()));
    }
}


int SvrModel::run(std::string const & brand_id, std::string const & adgroup_id, double user_brand_svr, double pacing)
{
    // When `user_brand_svr` is -1, this function provides a brand-aware
    // appropriately small multiplier.
    // If a campaign does not want to bid on -1 traffic, the logic is in Neptune.
    //
    // In other words, Neptune needs to know whether a specific campaign wants
    // to bid on '-1' traffic.
    // If yes, call this function as usual.
    // If no, do not call this function; just use multiplier 0.

    // For now, LBA default svr and multiplier are not used;
    // only the non-LBA ones are used.

    // If want to get a result for the brand 'overall' (in some sense)
    // without regard to the specific adgroup, pass in an empty string
    // for `adgroup_id`. This requires that the CatalogModel contains
    // an entry tagged by the brand ID.

    try {
        _svr = user_brand_svr;
        _message = "";

        if (user_brand_svr < 0.0) {
            // Caching default multipliers is because right now
            // we do not use request-level features.
            // Once we do use request-level features,
            // we'll need to re-calculate the multiplier using
            // the default SVR along with the request-level features.

            std::string multikey = brand_id + "-" + adgroup_id;
            // `adgroup_id` may be empty.

            auto it = _default_multiplier.find(multikey);
            if (it == _default_multiplier.end()) {
                double nonlba_svr = this->_get_default_svr(brand_id, 0);
                double nonlba_multiplier = this->_calc_multiplier(brand_id, adgroup_id, nonlba_svr, pacing);
                _default_multiplier[multikey] = std::make_tuple(nonlba_multiplier, -1.0);
                _bid_multiplier = nonlba_multiplier;
            } else {
                auto [nonlba_multiplier, lba_multiplier] = std::get<1>(*it);
                if (nonlba_multiplier < 0.0) {

                    // Should not fall in this branch now,
                    // because LBA default svr is not used.
                    throw SaturnError(mars::make_string(
                                          "you are not supposed to get here in this testing"
                                      ));

                    double nonlba_svr = this->_get_default_svr(brand_id, 0);
                    nonlba_multiplier = this->_calc_multiplier(brand_id, adgroup_id, nonlba_svr, pacing);
                    _default_multiplier[multikey] = std::make_tuple(nonlba_multiplier, lba_multiplier);
                }
                _bid_multiplier = nonlba_multiplier;
            }

            // std::cout << "default multiplier for brand `" << brand_id << "`, adgroup `" << adgroup_id << "`: " << _bid_multiplier << std::endl;
        } else {
            _bid_multiplier = this->_calc_multiplier(brand_id, adgroup_id, user_brand_svr, pacing);
        }

        return 0;
    } catch (std::exception& e) {
        _svr = 0.;
        _message = e.what();
        _bid_multiplier = 0.;
        return 2;
    }
}

double SvrModel::svr() const
{
    return _svr;
}

double SvrModel::bid_multiplier() const
{
    return _bid_multiplier;
}

std::string const & SvrModel::message() const
{
    return _message;
}

}  // namespace
