#include "saturn/feature_engine.h"
#include "mars/mars.h"

#include <set>

namespace saturn
{


const std::vector<std::string> FeatureEngine::STRING_FIELDS({
    "gender",
    "carrier",
    "device_type",
    "device_make",
    "device_model",
    "hour",
    "state",
    "zipcode",
    "os",
    "ctr_campaign_id",
    "ctr_adgroup_id",
    "ctr_creative_id",
    "ctr_publisher_id",
    "ctr_traffic_name",
    "ctr_age",
    "ctr_gender",
    "ctr_banner_size",
    "ctr_os",
    "ctr_carrier",
    "ctr_pub_type",
    "ctr_device_type",
    "ctr_creative_type",
    "ctr_adomain",
    "ctr_uid_type",
    "ctr_device_make",
    "ctr_device_model",
    "ctr_device_year",
    "ctr_isp",
    "ctr_hour",
    "ctr_sic",
    "ctr_dt",
    "wr_uid_type",
    "wr_device_type",
    "wr_os",
    "wr_device_make",
    "wr_bundle",
    "wr_banner_size",
    "wr_sp_user_gender",
    "wr_pub_bid_rates",
    "wr_isp",
    "wr_adomain",
    "wr_device_model",
});


const std::vector<std::string> FeatureEngine::INT_FIELDS({
    "pub_id",
    "age",
    "sl_adjusted_confidence",
    "device_year",
    "ctr_sl_adjusted_confidence",
    "wr_hour",
    "wr_sl_adjusted_confidence",
    "wr_weekday"
});


const std::vector<std::string> FeatureEngine::FLOAT_FIELDS({
    "latitude",
    "longitude",
    "pub_bid_floor",
    "user_extlba",
});


FeatureEngine::FeatureEngine()
{
    std::vector<std::string> columns;
    columns.insert(columns.end(), STRING_FIELDS.cbegin(), STRING_FIELDS.cend());
    columns.insert(columns.end(), INT_FIELDS.cbegin(), INT_FIELDS.cend());
    columns.insert(columns.end(), FLOAT_FIELDS.cbegin(), FLOAT_FIELDS.cend());

    if (std::set<std::string>(columns.cbegin(), columns.cend()).size() != columns.size()) {
        throw SaturnError("field names for `FeatureEngine` are not all unique");
    }

    _mars_feature_engine = static_cast<void *>(new mars::FeatureEngine(columns));
}


FeatureEngine::~FeatureEngine()
{
    delete static_cast<mars::FeatureEngine *>(_mars_feature_engine);
}

void FeatureEngine::update_field(StringField idx, std::string const & value)
{
    auto f = static_cast<mars::FeatureEngine *>(_mars_feature_engine);
    f->ingest_column(_string_field_idx_base + static_cast<size_t>(idx), mars::Column(value));
}

void FeatureEngine::update_field(StringField idx, std::string const * value)
{
    this->update_field(idx, *value);
}

void FeatureEngine::update_field(StringField idx, char const * c_str)
{
    auto f = static_cast<mars::FeatureEngine *>(_mars_feature_engine);
    f->ingest_column(_string_field_idx_base + static_cast<size_t>(idx), mars::Column(c_str));
}

void FeatureEngine::update_field(IntField idx, int value)
{
    auto f = static_cast<mars::FeatureEngine *>(_mars_feature_engine);
    f->ingest_column(_int_field_idx_base + static_cast<size_t>(idx), mars::Column(value));
}

void FeatureEngine::update_field(FloatField idx, double value)
{
    auto f = static_cast<mars::FeatureEngine *>(_mars_feature_engine);
    f->ingest_column(_float_field_idx_base + static_cast<size_t>(idx), mars::Column(value));
}

void FeatureEngine::reset_fields()
{
    auto f = static_cast<mars::FeatureEngine *>(_mars_feature_engine);
    for (size_t idx = 0; idx < STRING_FIELDS.size(); idx++) {
        f->ingest_column(_string_field_idx_base + idx, mars::Column(""));
    }
    for (size_t idx = 0; idx < INT_FIELDS.size(); idx++) {
        f->ingest_column(_int_field_idx_base + idx, mars::Column(0));
    }
    for (size_t idx = 0; idx < FLOAT_FIELDS.size(); idx++) {
        f->ingest_column(_float_field_idx_base + idx, mars::Column(0.0));
    }
}

} // namespace
