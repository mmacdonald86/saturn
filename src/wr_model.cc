#include "saturn/common.h"
#include "saturn/wr_model.h"
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


WrModel::WrModel(FeatureEngine & feature_engine, std::string path)
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

    mars::AvroReader areader((_path + "/wr_model_object.data").c_str());
    mars::AvroReader areader1((_path + "/delivery_model_object.data").c_str());
    auto const class_name = areader.get_scalar<std::string>("class_name");
    if ("ChainModel" != class_name) {
        throw SaturnError(mars::make_string(
        "expecting a `ChainModel` for `WrModel`; get a `", class_name, "`"));
    }

    auto const class_name1 = areader1.get_scalar<std::string>("class_name");
    if ("ChainModel" != class_name1) {
        throw SaturnError(mars::make_string(
        "expecting a `ChainModel` for `DeliveryModel`; get a `", class_name, "`"));
    }

    auto model = mars::ChainModel::from_avro(areader);
    auto model1 = mars::ChainModel::from_avro(areader1);
    _mars_model = static_cast<void *>(model.release());
    _deliver_model = static_cast<void *>(model1.release());

    // Read in `adgroup_quantile_cutoff.txt` file.
    // If file does not exist, no adgroup is using the 'placed' strategy.

}


WrModel::~WrModel()
{
    delete static_cast<mars::ChainModel *>(_mars_model);
    delete static_cast<mars::ChainModel *>(_deliver_model);
}


double WrModel::get_prob(std::vector<std::string> input)
{
    _feature_engine.update_field(FeatureEngine::StringField::wr_Uidtype, input[0]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Devicetype, input[1]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Os, input[2]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Devicemake, input[3]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Bundle, input[4]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Bannersize, input[5]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Spusergender, input[6]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Pubbidrate, input[7]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Isp, input[8]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Adomain, input[9]);
    _feature_engine.update_field(FeatureEngine::StringField::wr_Devicemodel, input[10]);
    _feature_engine.update_field(FeatureEngine::IntField::wr_Hour, std::stoi(input[11]));
    _feature_engine.update_field(FeatureEngine::IntField::wr_Sladjustedconfidence, std::stoi(input[12]));
    _feature_engine.update_field(FeatureEngine::IntField::wr_Weekday, std::stoi(input[13]));

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    auto x = f->render(_composer_id);

    std::cout << "  feature: " << std::endl;
//    for (auto i = x.begin(); i != x.end(); ++i){
//        std::cout << *i << ' ';
//    }
//    std::cout << std::endl;
    auto m = static_cast<mars::ChainModel *>(_mars_model);

    auto z = m->predict_one(x);

    auto m1 = static_cast<mars::ChainModel *>(_deliver_model);

    auto z1 = m1->predict_one(x);
//    std::cout << typeid(z).name() << std::endl;
    // Version 0:
    //   CatalogModel contains ChainModel's.
    //
    // auto zz = std::any_cast<mars::ChainModel::result_type>(z);
    // _bid_multiplier = std::any_cast<double>(std::get<0>(zz));
    // _svr = std::get<1>(zz)[0][0];

    // Version 1:
    //   CatalogModel contains IsotonicRegression or IsotonicLinearInterpolation.
    //
//    auto zz = std::any_cast<mars::ChainModel::result_type>(z);
    double prob = std::any_cast<double>(std::get<0>(z));
    double dev_prob = std::any_cast<double>(std::get<0>(z1));
    std::cout << "  win rate prediction: " << prob << std::endl;
    std::cout << "  delivery rate prediction: " << dev_prob << std::endl;
    std::cout << "  final prediction: " << prob * dev_prob << std::endl;
    _win_prob = prob;
    _dev_prob = dev_prob;
    _final_prob = prob * dev_prob;
    return 0;
}


std::string const & WrModel::message() const
{
    return _message;
}

double WrModel::final_prob() const
{
    return _final_prob;
}

double WrModel::win_prob() const
{
    return _win_prob;
}

double WrModel::dev_prob() const
{
    return _dev_prob;
}
}  // namespace
