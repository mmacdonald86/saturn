#include "saturn/common.h"
#include "saturn/svr_model.h"
#include "saturn/feature_engine.h"
#include "mars/catalog_model.h"
#include "mars/single_model.h"
#include "mars/featurizer.h"
#include "mars/utils.h"

#include <any>
#include <tuple>

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
        if ("CatalogModel" != class_name)
        {
            throw SaturnError(mars::make_string(
                "expecting a `CatalogModel` for `SvrModel`; get a `",
                class_name,
                "`"));
        }

        auto model = mars::CatalogModel::from_avro(areader);
        _mars_model = static_cast<void *>(model.release());
    }

    SvrModel::~SvrModel()
    {
        delete static_cast<mars::CatalogModel *>(_mars_model);
    }

    int SvrModel::run(std::string const & brand_id, double user_brand_svr)
    {
        try {
            _feature_engine.update_field(FeatureEngine::FloatField::kUserExtlba, user_brand_svr);

            auto f = static_cast<mars::FeatureEngine *>(_feature_engine._mars_feature_engine);
            auto x = f->render(_composer_id);

            auto m = static_cast<mars::CatalogModel *>(_mars_model);
            auto z = m->run(x, brand_id);

            auto zz = std::any_cast<mars::ChainModel::result_type>(z);
            _bid_multiplier = std::any_cast<double>(std::get<0>(zz));
            _svr = std::get<1>(zz)[0][0];
            _message = "";
            return 0;
        } catch (std::exception& e) {
            _bid_multiplier = 0.;
            _svr = 0.;
            _message = e.what();
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
