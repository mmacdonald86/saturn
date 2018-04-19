#include "saturn/saturn.h"
#include "saturn/utils.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
// #include <memory>
#include <stdexcept>
#include <string>
// #include <sstream>
#include <vector>
// #include <tuple>
// #include <utility>  // as_const

using namespace saturn;


struct ColumnInfo {
    char type;  // 's', 'i', 'f'
    size_t idx_in_type;

    ColumnInfo(char type_, size_t idx_) : type(type_), idx_in_type(idx_) {}
};


struct ColumnValue {
    std::string s_value = "";
    int i_value = 0;
    double f_value = 0.0;
};


size_t find_column_idx(FeatureEngine const & feature_engine, std::string const & name, std::string const & type)
{
    if ("str" == type) {
        auto it = std::find(feature_engine.STRING_FIELDS.cbegin(), feature_engine.STRING_FIELDS.cend(), name);
        return static_cast<size_t>(std::distance(feature_engine.STRING_FIELDS.cbegin(), it));
    }
    if ("int" == type) {
        auto it = std::find(feature_engine.INT_FIELDS.cbegin(), feature_engine.INT_FIELDS.cend(), name);
        return static_cast<size_t>(std::distance(feature_engine.INT_FIELDS.cbegin(), it));
    }
    if ("float" == type) {
        auto it = std::find(feature_engine.FLOAT_FIELDS.cbegin(), feature_engine.FLOAT_FIELDS.cend(), name);
        return static_cast<size_t>(std::distance(feature_engine.FLOAT_FIELDS.cbegin(), it));
    }
    throw std::runtime_error(std::string("unrecognized type '" + type + "'"));
}


std::vector<ColumnInfo> read_column_list(std::string const & filename, FeatureEngine const & feature_engine)
{
    std::vector<ColumnInfo> columns;
    auto infile = std::ifstream(filename);
    std::string col_name, col_type;
    while (infile >> col_name >> col_type) {
        columns.emplace_back(col_type[0], find_column_idx(feature_engine, col_name, col_type));
    }
    infile.close();
    return columns;
}


std::vector<std::string> read_brand_list(std::string filename)
{
    std::vector<std::string> values;
    auto infile = std::ifstream(filename);
    std::string brand_id;
    while (infile >> brand_id) {
        values.push_back(brand_id);
    }
    infile.close();
    return values;
}


std::vector<double> read_user_brand_svr(std::string filename)
{
    std::vector<double> values;
    auto infile = std::ifstream(filename);
    double x;
    while (infile >> x) {
        values.push_back(x);
    }
    infile.close();
    return values;
}


std::vector<std::vector<ColumnValue>> read_request_data(std::string const & filename, std::vector<ColumnInfo> const & col_info)
{
    std::vector<std::vector<ColumnValue>> data;
    const std::string delimiter = "\t";
    std::ifstream infile(filename);
    std::string line;
    size_t pos;
    const size_t ll = delimiter.length();
    const size_t ncols = col_info.size();
    while (std::getline(infile, line)) {
        std::vector<ColumnValue> oneline;
        for (size_t idx = 0; idx < ncols; idx++) {
            pos = line.find(delimiter);
            auto substr = line.substr(0, pos);
            ColumnValue value;
            if (col_info[idx].type == 'i') {
                value.i_value = std::stoi(std::string(substr));
            } else if (col_info[idx].type == 'f') {
                value.f_value = std::stod(std::string(substr));
            } else {
                value.s_value = substr;
            }
            oneline.push_back(value);
            line = line.substr(pos + ll);
        }
        data.push_back(oneline);
    }
    infile.close();
    return data;
}


void timeit(long n, Timer const & timer)
{
    std::cout << "  total time: " << timer.seconds() << " seconds" << std::endl;
    std::cout << "  qps: " << static_cast<int>((double)n / timer.seconds()) << std::endl;
    std::cout << "  latency: " << (double)timer.milliseconds() / n << " milliseconds" << std::endl;
}


void run(
    FeatureEngine & feature_engine, 
    SvrModel & svr_model, 
    std::vector<ColumnInfo> const & col_info,
    std::vector<std::vector<ColumnValue>> const & request_data,
    std::vector<std::string> const & brand_ids,
    std::vector<std::vector<double>> user_brand_svr)
{
    auto timer = Timer();
    timer.start();

    for (size_t i_req = 0; i_req < request_data.size(); i_req++) {
        // Ingest request-level fields into feature engine.
        for (size_t i_col = 0; i_col < col_info.size(); i_col++) {
            if (col_info[i_col].type == 's') {
                feature_engine.update_field(
                    static_cast<FeatureEngine::StringField>(col_info[i_col].idx_in_type),
                    request_data[i_req][i_col].s_value);
            } else if (col_info[i_col].type == 'i') {
                feature_engine.update_field(
                    static_cast<FeatureEngine::IntField>(col_info[i_col].idx_in_type),
                    request_data[i_req][i_col].i_value);
            } else if (col_info[i_col].type == 'f') {
                feature_engine.update_field(
                    static_cast<FeatureEngine::FloatField>(col_info[i_col].idx_in_type),
                    request_data[i_req][i_col].f_value);
            } else {
                throw std::runtime_error("wrong column type!");  // this should never happen
            }
        }

        // Run SVR model for each brand.
        for (size_t i_brand = 0; i_brand < brand_ids.size(); i_brand++) {
            std::string brand_id = brand_ids[i_brand];
            double user_svr = user_brand_svr[i_brand][i_req];
            if (svr_model.run(brand_id, user_svr) == 0) {
                double svr = svr_model.svr();
                double mult = svr_model.bid_multiplier();
            } else {
                std::cout << "oooops " << svr_model.message() << std::endl;
            }
        }
    }

    timer.stop();

    auto N = request_data.size();
    auto n = brand_ids.size();
    std::cout << "processing " << N << " requests:" << std::endl;
    timeit(N, timer);
    std::cout << std::endl << "considering " << n << " brands per request, "
        << "hence " << N * n << " model calls:" << std::endl;
    timeit(N * n, timer);
}


int main(int argc, char const * const * argv) {
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "    test_svr path" << std::endl;
        return 0;
    }
    auto modelpath = std::string(argv[1]);
    while (modelpath.back() == '/') {
        modelpath.pop_back();
    }

    auto feature_engine = saturn::FeatureEngine();
    auto svr_model = saturn::SvrModel(feature_engine, modelpath);

    auto col_info = read_column_list(modelpath + "/data_test/column_list.txt", feature_engine);
    auto request_data = read_request_data(modelpath + "/data_test/raw.txt", col_info);
    auto brand_ids = read_brand_list(modelpath + "/data_test/brand_ids.txt");

    std::vector<std::vector<double>> user_brand_svr;
    for (size_t i = 0; i < brand_ids.size(); i++) {
        user_brand_svr.push_back(read_user_brand_svr(modelpath + "/data_test/user_extlba/" + brand_ids[i] + ".txt"));
    }

    run(feature_engine, svr_model, col_info, request_data, brand_ids, user_brand_svr);

    return 0;
}