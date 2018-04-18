#include "saturn/feature_engine.h"
#include "mars/featurizer.h"

#include <set>
#include <string>
#include <vector>

namespace saturn {

FeatureEngine::FeatureEngine(std::string root_path)
{
    std::vector<std::string> columns;
    columns.insert(columns.end(), STRING_FIELDS.cbegin(), STRING_FIELDS.cend());
    columns.insert(columns.end(), INT_FIELDS.cbegin(), INT_FIELDS.cend());
    columns.insert(columns.end(), FLOAT_FIELDS.cbegin(), FLOAT_FIELDS.cend());

    if (std::set<std::string>(columns.cbegin(), columns.cend()).size() != columns.size()) {
        throw SaturnError("field names for `FeatureEngine` are not all unique");
    }

    _mars_feature_engine = static_cast<void *>(new mars::FeatureEngine>(columns));
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
