#ifndef _SATURN_SVR_MODEL_H_
#define _SATURN_SVR_MODEL_H_

#include "common.h"
#include "feature_engine.h"

#include <map>
#include <tuple>


namespace saturn
{
class SvrModel
{
  public:
    SvrModel(FeatureEngine & feature_engine, std::string path);
    // `path`: this absolute path contains the trained model object as well as supporting data.
    //         The following files must exist in this folder:
    //
    //            model_object.data
    //            model_config.json
    //            default_svr.txt
    //
    // The file `model_object.data` is created by Python code that trains the model.
    // (Specifically, `mars.BaseModel.cc_dump`).
    //
    // The content of the file `model_config.json` looks like this:
    //
    // {
    //   "type": "BinaryLogisticRegression",
    //   "features": [
    //         {
    //             "type": "OneHot",
    //             "args": {"column": "country", "values": ["US", "UK", "DE", "JP"]}
    //         },
    //         {
    //             "type": "DirectNumber",
    //             "args": {"column": "age"}
    //         },
    //         {
    //             "type": "DirectNumber",
    //             "args": {"column": "purchase_amount"}
    //         },
    //         {
    //             "type": "HashedColumn",
    //             "args": {"column": "domain_name", "n_predictors": 10}
    //         },
    //         {
    //             "type": "HashedColumnCross",
    //             "args": {"columns": ["country", "age"], "n_predictors": 10}
    //         },
    //         {
    //             "type": "HashedColumnBundle",
    //             "args": {"columns": ["age", "purchase_amount", "domain_name"], "n_predictors": 12}
    //         }
    //     ]
    //  }
    //
    // In this config, `type` is the exact class name of a subclass of `mars::Model`.
    //
    // The listing of `features` implicitly defines a `FeatureComposer` for this model to use.
    // If any of these features is already present in `feature_engine`, then the existing one is re-used.
    // If the composer (the whole list including the order of the elements) is already present in `feature_engine`,
    // (that is, a previously created model associated with the same `feature_engine` specified exactly
    // the same features, including their order), then the existing composer is re-used.
    //
    // All the column names must have been specified in `feature_engine`.
    //
    // When there are a large number of models, the developer should try to use the same features and composers
    // across models, unless there are good reasons to differ.
    // Such sharing improves efficiency of internal feature processing.
    //
    // The file `default_svr.txt` is a plain text file containing three columns on each line:
    //
    // brand_id non_lba_default_svr lba_default_svr
    //
    // The file does not contain a header line. The columns are separated by spaces.
    // This file must contain all the brands that are going to be called on this class object.

    ~SvrModel();

    std::string const & model_id() const;

    int run(std::string const & brand_id, std::string const & adgroup_id, double user_brand_svr, double pacing = -1.);
    // 0 is success; usually no need to check `message()`.
    // Other values indicate problems; check `message()`.
    //
    // `pacing` is an indicator of how capable the adgroup has been
    // of spending the allocated budget **in recent times**.
    // This may not be identical to the 'pacing' concept in Neptune.
    // This can be a metric based on data of the preceding hour.
    // For example, in the preceding hour,
    // the adgroup had an allocated budget of $100 (i.e. had an allowance to spend $100,
    // unrelated to how the adgroup had been spending before the preceding hour),
    // and it actually spent $82, then `pacing` is 0.82.
    //
    // `pacing` is between 0 and 1, or -1 to use an internal default.
    //
    // The default value of `pacing` corresponds to a default curve.
    // Skip this argument (hence use the default) when the caller is not doing
    // dynamic multiplier based on pacing.

    // The following methods provide results after a call to `run`.
    double svr() const;
    double bid_multiplier() const;
    std::string const & message() const;

  private:
    FeatureEngine & _feature_engine;
    void * _mars_model = nullptr;

    std::string _path;
    std::string _model_id;
    std::string _composer_id;

    double _svr = 0.;
    double _bid_multiplier = 0.;
    std::string _message = "";

    double _calc_multiplier(std::string const & brand_id, std::string const & adgroup_id, double user_brand_svr, double pacing);

    std::map<std::string, std::tuple<double, double>> _default_svr;
    // Key is brand ID; value is default SVR value for non-LBA traffic and LBA traffic,
    // in that order.
    std::map<std::string, std::tuple<double, double>> _default_multiplier;
    // Key is brandid-adgroupid; value is default multiplier for non-LBA traffic and LBA traffic,
    // in that order.

    double _get_default_svr(std::string const & brand_id, int flag) const;
    // flag:
    // if 0, get non-LBA svr;
    // if 1, get LBA svr.
};

}  // namespace
#endif  // include guard