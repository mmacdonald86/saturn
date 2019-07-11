#include "saturn/common.h"
#include "saturn/svr_model.h"
#include "saturn/feature_engine.h"
#include "saturn/utils.h"
#include "mars/mars.h"
#include "mars/numeric.h"
#include "mars/utils.h"

#include <any>
#include <cassert>
#include <fstream>
#include <tuple>

#include <iostream>

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
    jreader.seek("/", "features");
    _composer_id = f->add_composer(jreader);

    if (jreader.has_member("/", "default_multiplier_curve")) {
        jreader.seek("/", "default_multiplier_curve");
        _default_multiplier_curve_mu = jreader.get_scalar<double>("mu");
        _default_multiplier_curve_sigma = jreader.get_scalar<double>("sigma");
    }

    if (jreader.has_member("/", "default_multiplier_cap")) {
        _default_multiplier_cap = jreader.get_scalar<double>("/", "default_multiplier_cap");
    }

    if (jreader.has_member("/", "adjust_multiplier_curve_for_pacing")) {
        double z = jreader.get_scalar<double>("/", "adjust_multiplier_curve_for_pacing");
        if (z > 0.01) {
            if (z > 2.0) {
                z = 2.0;
            }
            _adjust_multiplier_curve_for_pacing = z;
        }
    }

    if (jreader.has_member("/", "default_nonlba_svr")) {
        _default_nonlba_svr = jreader.get_scalar<double>("/", "default_nonlba_svr");
    }

    if (jreader.has_member("/", "default_lba_svr")) {
        _default_lba_svr = jreader.get_scalar<double>("/", "default_lba_svr");
    }

    if (jreader.has_member("/", "adgroup_default_svr")) {
        jreader.seek("/", "adgroup_default_svr");
        std::string adgroup_id;
        double nonlba, lba;
        auto n = jreader.get_array_size();
        for (size_t i = 0; i < n; i++) {
            jreader.save_cursor();
            jreader.seek_in_array(i);
            adgroup_id = jreader.get_scalar<std::string>("adgroup_id");
            nonlba = jreader.get_scalar<double>("nonlba");
            lba = jreader.get_scalar<double>("lba");
            _adgroup_default_svr.emplace(adgroup_id, std::make_tuple(nonlba, lba));
            jreader.restore_cursor();
        }
    }

    if (jreader.has_member("/", "adgroup_multiplier_curve")) {
        jreader.seek("/", "adgroup_multiplier_curve");
        std::string adgroup_id;
        double mu, sigma;
        auto n = jreader.get_array_size();
        for (size_t i = 0; i < n; i++) {
            jreader.save_cursor();
            jreader.seek_in_array(i);
            adgroup_id = jreader.get_scalar<std::string>("adgroup_id");
            mu = jreader.get_scalar<double>("mu");
            sigma = jreader.get_scalar<double>("sigma");
            _adgroup_multiplier_curve.emplace(adgroup_id, std::make_tuple(mu, sigma));
            jreader.restore_cursor();
        }
    }

    if (jreader.has_member("/", "adgroup_multiplier_cap")) {
        jreader.seek("/", "adgroup_multiplier_cap");
        std::string adgroup_id;
        double cap;
        auto n = jreader.get_array_size();
        for (size_t i = 0; i < n; i++) {
            jreader.save_cursor();
            jreader.seek_in_array(i);
            adgroup_id = jreader.get_scalar<std::string>("adgroup_id");
            cap = jreader.get_scalar<double>("cap");
            _adgroup_multiplier_cap.emplace(adgroup_id, cap);
            jreader.restore_cursor();
        }
    }

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

    // Read in brand default svr file. If file does not exist, default values will be used.
    auto infile_svr = std::ifstream(_path + "/brand_default_svr.txt");
    if (infile_svr.is_open()) {
        std::string brand_id;
        double nonlba_svr, lba_svr;
        while (infile_svr >> brand_id >> nonlba_svr >> lba_svr) {
            _brand_default_svr.emplace(brand_id, std::make_tuple(nonlba_svr, lba_svr));
        }
        infile_svr.close();
    }

    // Read in `adgroup_quantile_cutoff.txt` file.
    // If file does not exist, no adgroup is using the 'placed' strategy.
    auto infile_quant = std::ifstream(_path + "/adgroup_quantile_cutoff.txt");
    if (infile_quant.is_open()) {
        std::string adgroup_id;
        double cutoff;
        while (infile_quant >> adgroup_id >> cutoff) {
            if (cutoff < 0.0) {
                cutoff = 0.0;
            } else if (cutoff > 1.0) {
                cutoff = 1.0;
            }
            _adgroup_quantile_cutoff.emplace(adgroup_id, cutoff);
        }
        infile_quant.close();
    }
}


SvrModel::~SvrModel()
{
    delete static_cast<mars::CatalogModel *>(_mars_model);
}


double SvrModel::_calc_multiplier(std::string const & adgroup_id, double user_adgroup_svr, double pacing)
{
    _feature_engine.update_field(FeatureEngine::FloatField::kUserExtlba, user_adgroup_svr);

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    auto x = f->render(_composer_id);

    auto m = static_cast<mars::CatalogModel *>(_mars_model);

    std::string const & tag = adgroup_id;

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

    auto it_q = _adgroup_quantile_cutoff.find(adgroup_id);
    if (it_q != _adgroup_quantile_cutoff.end()) {
        double cutoff = std::get<1>(*it_q);
        if (quantile >= cutoff) {
            return 1.0;
        }
        return 0.0;
    }

    double mu, sigma;
    auto it = _adgroup_multiplier_curve.find(adgroup_id);
    if (it != _adgroup_multiplier_curve.end()) {
        mu = std::get<0>(std::get<1>(*it));
        sigma = std::get<1>(std::get<1>(*it));
    } else {
        sigma = _default_multiplier_curve_sigma;
        if (pacing < 0.0 || _adjust_multiplier_curve_for_pacing == 0.0) {  // No pacing info; use default
            mu = _default_multiplier_curve_mu;
        } else {
            if (pacing > 1.0) {
                throw SaturnError(mars::make_string(
                                      "argument `pacing` must be in {-1, [0, 1]}; got ",
                                      pacing
                                  ));
            }
            mu = (pacing * pacing * 2 - 1.) * _adjust_multiplier_curve_for_pacing;
            // Square, stretch to [0, 2], shift to [-1, 1], scale to
            // [- _adjust_multiplier_curve_for_pacing, _adjust_multiplier_curve_for_pacing]
        }
    }
    return mars::logitnormal_cdf(quantile, mu, sigma);
}


double SvrModel::_get_default_svr(std::string const & brand_id, std::string const & adgroup_id, int flag) const
{
    assert(flag == 0 || flag == 1);

    auto iit = _adgroup_default_svr.find(adgroup_id);
    if (iit != _adgroup_default_svr.end()) {
        auto [nonlba_svr, lba_svr] = std::get<1>(*iit);
        if (flag == 0) {
            return nonlba_svr;
        } else {
            return lba_svr;
        }
    }

    auto it = _brand_default_svr.find(brand_id);
    if (it != _brand_default_svr.end()) {
        auto [nonlba_svr, lba_svr] = std::get<1>(*it);
        if (flag == 0) {
            return nonlba_svr;
        } else {
            return lba_svr;
        }
    }
    
    if (flag == 0) {
        return _default_nonlba_svr;
    } else {
        return _default_lba_svr;
    }
}


bool SvrModel::has_model(std::string const & adgroup_id) const
{
    auto m = static_cast<mars::CatalogModel *>(_mars_model);
    return m->has_model(adgroup_id.substr(1));
}

enum class Mode{brand, location_group};

int SvrModel::get_multiplier(std::string const & id, std::string const & adgroup_id, double user_adgroup_svr,
                             Mode mode)
{
    try {
        if (user_adgroup_svr < 0.) {
            _svr = user_adgroup_svr;
            _bid_multiplier = 0.;
            return 0;
        }

        std::string keys = "";
        switch(mode) {
            case SvrModel::Mode::brand:
                keys = "/" + adgroup_id + "/b_" + id; break;
            case SvrModel::Mode::location_group:
                keys = "/" + adgroup_id + "/t_" + id; break;
        }

        if (!this->has_model(keys)) {
            _svr = user_adgroup_svr;
            _bid_multiplier = 0.;
            return 0;
        }

        auto m = static_cast<mars::CatalogModel *>(_mars_model);
        std::vector<double> x(1);
        x[0] = user_adgroup_svr;
        double percent = std::any_cast<double>(m->run(x, keys.substr(1)));

        auto it_q = _adgroup_quantile_cutoff.find(adgroup_id);
        if (it_q != _adgroup_quantile_cutoff.end()) {
            double cutoff = std::get<1>(*it_q);
            if (percent >= cutoff) {
                _bid_multiplier = 1.;
            } else {
                _bid_multiplier = 0.;
            }
        } else {
            _bid_multiplier = percent;
        }

        _svr = user_adgroup_svr;
        return 0;

    } catch (std::exception& e) {
        _message = e.what();
        _bid_multiplier = 0.;
        return 2;
    }
}


int SvrModel::get_cpsvr(std::string const & id, std::string const & adgroup_id, double user_adgroup_svr, Mode mode)
{
    try {
        if (user_adgroup_svr < 0.) {
            _cpsvr = 0.;
            _bid_multiplier = 0.;
            _svr = user_adgroup_svr;
            return 0;
        }
        std::string keys = "";
        switch(mode) {
            case SvrModel::Mode::brand:
                keys = "/" + adgroup_id + "/b_" + id; break;
            case SvrModel::Mode::location_group:
                keys = "/" + adgroup_id + "/t_" + id; break;
        }
        if (!this->has_model(keys)) {
            _svr = user_adgroup_svr;
            _cpsvr = user_adgroup_svr;
            _bid_multiplier = user_adgroup_svr;
            return 0;
        }

        auto m = static_cast<mars::CatalogModel *>(_mars_model);
        std::vector<double> x(1);
        x[0] = user_adgroup_svr;
        _cpsvr = std::any_cast<double>(m->run(x, keys.substr(1)));
        _bid_multiplier = _cpsvr;
        _svr = user_adgroup_svr;
        return 0;

    } catch (std::exception& e) {
        _message = e.what();
        _cpsvr = 0.;
        _bid_multiplier = 0.;
        return 2;
    }

}


int SvrModel::run(std::string const & brand_id, std::string const & adgroup_id, double user_adgroup_svr, double pacing)
{
    // When `user_adgroup_svr` is -1, this function provides a brand-aware
    // appropriately small multiplier.
    // If a campaign does not want to bid on -1 traffic, the logic is in Neptune.
    //
    // In other words, Neptune needs to know whether a specific campaign wants
    // to bid on '-1' traffic.
    // If yes, call this function as usual.
    // If no, do not call this function; just use multiplier 0.

    // For now, LBA default svr and multiplier are not used;
    // only the non-LBA ones are used.

    try {
        _svr = user_adgroup_svr;
        _message = "";

        if (!this->has_model(adgroup_id)) {
            _bid_multiplier = 1.0;
            return 0;
        }

        if (user_adgroup_svr < 0.0) {
            // '-1' traffic
            
            // Caching default multipliers because right now
            // we do not use request-level features.
            // Once we do use request-level features,
            // we'll need to re-calculate the multiplier using
            // the default SVR along with the request-level features.

            auto it = _adgroup_default_multiplier.find(adgroup_id);
            if (it == _adgroup_default_multiplier.end()) {
                double nonlba_svr = this->_get_default_svr(brand_id, adgroup_id, 0);
                double nonlba_multiplier = this->_calc_multiplier(adgroup_id, nonlba_svr, pacing);
                // TODO: not quite right here if `pacing` is provided, in which case
                // this multiplier should be re-calculated every time.

                _adgroup_default_multiplier[adgroup_id] = std::make_tuple(nonlba_multiplier, -1.0);
                _bid_multiplier = nonlba_multiplier;
            } else {
                auto [nonlba_multiplier, lba_multiplier] = std::get<1>(*it);
                if (nonlba_multiplier < 0.0) {

                    // Should not fall in this branch now,
                    // because LBA default svr is not used.
                    throw SaturnError(mars::make_string(
                                          "you are not supposed to get here in this testing"
                                      ));

                    double nonlba_svr = this->_get_default_svr(brand_id, adgroup_id, 0);
                    nonlba_multiplier = this->_calc_multiplier(adgroup_id, nonlba_svr, pacing);
                    _adgroup_default_multiplier[adgroup_id] = std::make_tuple(nonlba_multiplier, lba_multiplier);
                }
                _bid_multiplier = nonlba_multiplier;
            }
        } else {
            _bid_multiplier = this->_calc_multiplier(adgroup_id, user_adgroup_svr, pacing);
        }

        auto it = _adgroup_multiplier_cap.find(adgroup_id);
        if (it == _adgroup_multiplier_cap.end()) {
            _bid_multiplier *= _default_multiplier_cap;
        } else {
            _bid_multiplier *= std::get<1>(*it);
        }
        return 0;

    } catch (std::exception& e) {
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

double SvrModel::cpsvr() const
{
    return _cpsvr;
}

std::string const & SvrModel::message() const
{
    return _message;
}

}  // namespace
