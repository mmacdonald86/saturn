#include "saturn/common.h"
#include "saturn/ctr_model.h"
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


ctrModel::ctrModel(FeatureEngine & feature_engine, std::string path)
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

    mars::AvroReader areader((_path + "/ctr_model_object.data").c_str());
    auto const class_name = areader.get_scalar<std::string>("class_name");
    if ("ChainModel" != class_name) {
        throw SaturnError(mars::make_string(
        "expecting a `ChainModel` for `ctrModel`; got a `", class_name, "`"));
    }


    auto model = mars::ChainModel::from_avro(areader);
    _mars_model = static_cast<void *>(model.release());

    // Read in `adgroup_quantile_cutoff.txt` file.
    // If file does not exist, no adgroup is using the 'placed' strategy.

}


ctrModel::~ctrModel()
{
    delete static_cast<mars::ChainModel *>(_mars_model);
}


double ctrModel::get_prob(std::vector<std::string> input)
{

    _feature_engine.update_field(FeatureEngine::StringField::ctr_campaign_id, input[0]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_creative_id, input[1]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_creative_type, input[2]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_adomain, input[3]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_sic, input[4]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_gender, input[5]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_banner_size, input[6]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_carrier, input[7]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_device_type, input[8]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_publisher_id, input[9]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_traffic_name, input[10]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_uid_type, input[11]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_device_model, input[12]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_isp, input[13]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_hour, input[14]);
    _feature_engine.update_field(FeatureEngine::StringField::ctr_age, input[15]);
    _feature_engine.update_field(FeatureEngine::IntField::ctr_sl_adjusted_confidence, std::stoi(input[16]));

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    auto x = f->render(_composer_id);
    auto m = static_cast<mars::ChainModel *>(_mars_model);

    auto z = m->predict_one(x);
    
    _prob = std::any_cast<double>(std::get<0>(z));
    return 0;
}


std::string const & ctrModel::message() const
{
    return _message;
}

double ctrModel::prob() const
{
    return _prob;
}
}  // namespace