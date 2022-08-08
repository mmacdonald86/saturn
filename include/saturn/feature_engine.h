#ifndef _SATURN_FEATURE_ENGINE_H_
#define _SATURN_FEATURE_ENGINE_H_

#include "common.h"

namespace saturn
{

class SvrModel;

class FeatureEngine
{
    // `FeatureEngine` manages a group of features, or 'fields' or 'columns' for models to use.
    //
    // Some features are expected to be used by multiple models.
    // Such feature sharing typically avoids some duplicate computation.
    // If such sharing is not a concern, it's perfectly fine to create multiple `FeatureEngine` instances
    // in a program and use them independently.

    friend class SvrModel;  // Needs to access `_mars_feature_engine`.
    friend class WrModel;  // Needs to access `_mars_feature_engine`.
    friend class ctrModel;  // Needs to access `_mars_feature_engine`.

  public:

    // DO PROOFREAD over and over again to make sure
    // the field names and ID enums are in sync.

    // The order of the fields in each type group is not important.
    // It's OK to re-arrange them.

    // All feature names, across the 3 types, must be unique.

    // A 'field' here is a 'column' in Mars terminology.

    static const std::vector<std::string> STRING_FIELDS;

    enum class StringField : size_t {
        kGender = 0,
        kCarrier,
        kDevice,
        kDeviceMake,
        kDeviceModel,
        kHour,
        kState,
        kZip,
        kOs,
        ctr_campaign_id, 
        ctr_adgroup_id,
        ctr_creative_id,
        ctr_publisher_id,
        ctr_traffic_name,
        ctr_age,
        ctr_gender,
        ctr_banner_size,
        ctr_os,
        ctr_carrier,
        ctr_pub_type,
        ctr_device_type,
        ctr_creative_type,
        ctr_adomain,
        ctr_uid_type,
        ctr_device_make,
        ctr_device_model,
        ctr_device_year,
        ctr_isp,
        ctr_hour,
        ctr_sic,
        ctr_dt,
        ctr_bundle,
        wr_Uidtype,
        wr_Devicetype,
        wr_Os,
        wr_Devicemake,
        wr_Bundle,
        wr_Bannersize,
        wr_Spusergender,
        wr_Pubbidrate,
        wr_Isp,
        wr_Adomain,
        wr_Devicemodel,
    };

    static const std::vector<std::string> INT_FIELDS;

    enum class IntField : size_t {
        kPubId = 0,
        kAge,
        kSconf,
        kDeviceYear,
        ctr_sl_adjusted_confidence,
        wr_Hour,
        wr_Sladjustedconfidence,
        wr_Weekday,
    };

    static const std::vector<std::string> FLOAT_FIELDS;

    enum class FloatField : size_t {
        kLat = 0,
        kLon,
        kBidFloor,
        kUserExtlba,
    };

    FeatureEngine();
    ~FeatureEngine();

    // Use these functions to update one field at a time,
    // in no particular order.
    // If some fields are not actually used by the models that you will subsequently run,
    // those fields do not need to be updated.
    void update_field(StringField idx, std::string const & value);
    void update_field(StringField idx, std::string const * value);
    void update_field(StringField idx, char const * c_str);
    void update_field(IntField idx, int value);
    void update_field(FloatField idx, double value);

    // Update each field to a default, non-informative value, that is,
    // number becomes zero and string becomes empty.
    // If you make sure all needed fields are updated individually by calling
    // `update_field`, then you don't need to call `reset_fields` beforehand.
    void reset_fields();

  private:
    void * _mars_feature_engine = nullptr;

    const size_t _string_field_idx_base = 0;
    const size_t _int_field_idx_base = STRING_FIELDS.size();
    const size_t _float_field_idx_base = STRING_FIELDS.size() + INT_FIELDS.size();
};


}  // namespace
#endif  // include guard
