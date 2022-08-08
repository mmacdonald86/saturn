#ifndef _SATURN_CTR_MODEL_H_
#define _SATURN_CTR_MODEL_H_

#include "common.h"
#include "feature_engine.h"

#include <map>
#include <tuple>


namespace saturn
{
class ctrModel
{
  public:
    ctrModel(FeatureEngine & feature_engine, std::string path);

    ~ctrModel();

    std::string const & model_id() const;

    double get_prob(std::vector<std::string> input);

    // Get output probability after setting features directly
    double get_prob();

    FeatureEngine& get_features() {
      return _feature_engine;
    }

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
    std::string const & message() const;
    double prob() const;

  private:
    FeatureEngine & _feature_engine;
    void * _mars_model = nullptr;
    double _prob = 0.;

    std::string _path;
    std::string _model_id;
    std::string _composer_id;

    std::string _message = "This is a sample message.";

};

}  // namespace
#endif  // include guard