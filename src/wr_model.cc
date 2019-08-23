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
    auto const class_name = areader.get_scalar<std::string>("class_name");
    if ("BinaryRandomForestClassifier" != class_name) {
        throw SaturnError(mars::make_string(
        "expecting a `BinaryRandomForestClassifier` for `WrModel`; get a `", class_name, "`"));
    }


    auto model = mars::BinaryRandomForestClassifier::from_avro(areader);
    _mars_model = static_cast<void *>(model.release());

    // Read in `adgroup_quantile_cutoff.txt` file.
    // If file does not exist, no adgroup is using the 'placed' strategy.

}


WrModel::~WrModel()
{
    delete static_cast<mars::BinaryRandomForestClassifier *>(_mars_model);
}


double WrModel::get_prob(std::vector<std::string> input)
{
    _feature_engine.update_field(FeatureEngine::StringField::kOs, input[0]);
    _feature_engine.update_field(FeatureEngine::StringField::kGender, input[1]);
    _feature_engine.update_field(FeatureEngine::StringField::kBundle, input[2]);

    auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
    auto x = f->render(_composer_id);

    std::cout << "  feature: " << std::endl;
    for (auto i = x.begin(); i != x.end(); ++i){
        std::cout << *i << ' ';
    }
    std::cout << std::endl;
    auto m = static_cast<mars::BinaryRandomForestClassifier *>(_mars_model);

    auto z = m->predict_one(x);

    // Version 0:
    //   CatalogModel contains ChainModel's.
    //
    // auto zz = std::any_cast<mars::ChainModel::result_type>(z);
    // _bid_multiplier = std::any_cast<double>(std::get<0>(zz));
    // _svr = std::get<1>(zz)[0][0];

    // Version 1:
    //   CatalogModel contains IsotonicRegression or IsotonicLinearInterpolation.
    //

    double prob = std::any_cast<double>(z);
    _prob = prob;
    return 0;
}


std::string const & WrModel::message() const
{
    return _message;
}

double WrModel::prob() const
{
    return _prob;
}
}  // namespace
