#ifndef _SATURN_FEATURE_ENGINE_H_
#define _SATURN_FEATURE_ENGINE_H_

#include "common.h"

namespace saturn {

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

    public:

        // DO PROOFREAD over and over again to make sure
        // the field names and ID enums are in sync.

        // The order of the fields in each type group is not important.
        // It's OK to re-arrange them.

        // All feature names, across the 3 types, must be unique.

        // A 'field' here is a 'column' in Mars terminology.

        const std::vector<std::string> STRING_FIELDS = {
            "gender",
            "carrier",
            "device_type",
            "device_make",
            "device_model",
            "hour",
            "state",
            "zipcode",
            "os",
        };

        enum class StringField : size_t
        {
            kGender = 0,
            kCarrier,
            kDevice,
            kDeviceMake,
            kDeviceModel,
            kHour,
            kState,
            kZip,
            kOs,
        };

        const std::vector<std::string> INT_FIELDS = {
            "pub_id",
            "age",
            "sl_adjusted_confidence",
            "device_year",
        };

        enum class IntField : size_t
        {
            kPubId = 0,
            kAge,
            kSconf,
            kDeviceYear,
        };

        const std::vector<std::string> FLOAT_FIELDS = {
            "latitude",
            "longitude",
            "pub_bid_floor",
            "user_extlba",
        };

        enum class FloatField : size_t
        {
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