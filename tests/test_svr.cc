/*
The benchmark program takes a directory that contains

```
- model_config.json
- model_object.data
- data_test/
|     |- column_list.txt
|     |- adgroup_ids.txt
|     |- raw.txt
|     |- user_extlba/
|     |      |- adgroup_id_1.txt
|     |      |- adgroup_id_2.txt
|     |      |- ...
```

`raw.txt` is a tab-separated csv file containing feature values *except* user-level SVR prediction.
Usually this file should have at least 10 thousand rows.
**This file is ignored (its presence is optional), because the model uses a single feature,
which is 'user-level SVR prediction'.**

`column_list.txt` contains name/type pairs of features in `raw.txt`, in corresponding order.
One feature per line. **This file is ignored (its presence is optional).**

`adgroup_ids.txt` contains adgroup IDs trained for the model. One adgroup per line.
The file looks like this:

```
aaa
bbb
ccc
```

where 'aaa', 'bbb', 'ccc' are adgroup ID's.

`user_extlab` contains user-level SVR predictions corresponding to each row in `raw.txt`
(which is for a particular user) for the adgroup ID that is used as the file name.
This directory contains files `aaa.txt`, `bbb.txt`, `ccc.txt`, corresponding to the content of the file `adgroup_ids.txt`.
*/


#include "saturn/saturn.h"
#include "saturn/utils.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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


size_t find_column_idx(std::string const & name, std::string const & type)
{
    if ("str" == type) {
        auto const & fields = FeatureEngine::STRING_FIELDS;
        auto it = std::find(fields.cbegin(), fields.cend(), name);
        return static_cast<size_t>(std::distance(fields.cbegin(), it));
    }
    if ("int" == type) {
        auto const & fields = FeatureEngine::INT_FIELDS;
        auto it = std::find(fields.cbegin(), fields.cend(), name);
        return static_cast<size_t>(std::distance(fields.cbegin(), it));
    }
    if ("float" == type) {
        auto const & fields = FeatureEngine::FLOAT_FIELDS;
        auto it = std::find(fields.cbegin(), fields.cend(), name);
        return static_cast<size_t>(std::distance(fields.cbegin(), it));
    }
    throw std::runtime_error(std::string("unrecognized type '" + type + "'"));
}


std::vector<ColumnInfo> read_column_list(std::string const & filename)
{
    std::vector<ColumnInfo> columns;
    auto infile = std::ifstream(filename);
    std::string col_name, col_type;
    while (infile >> col_name >> col_type) {
        columns.emplace_back(col_type[0], find_column_idx(col_name, col_type));
    }
    infile.close();
    return columns;
}


std::vector<std::string> read_adgroup_list(std::string filename)
{
    std::vector<std::string> values;
    auto infile = std::ifstream(filename);
    std::string adgroup_id;
    while (infile >> adgroup_id) {
        values.push_back(adgroup_id);
    }
    infile.close();
    return values;
}


std::vector<double> read_user_adgroup_svr(std::string filename)
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
    SvrModel * svr_model,
    std::vector<ColumnInfo> const & col_info,
    std::vector<std::vector<ColumnValue>> const & request_data,
    std::vector<std::string> const & adgroup_ids,
    std::vector<std::vector<double>> user_adgroup_svr
)
{
    auto n_req = user_adgroup_svr[0].size();
    auto timer = Timer();
    timer.start();

    for (size_t i_req = 0; i_req < n_req; i_req++) {
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

        // Run SVR model for each adgroup.
        for (size_t i_adgroup = 0; i_adgroup < adgroup_ids.size(); i_adgroup++) {
            std::string adgroup_id = adgroup_ids[i_adgroup];
            if (!svr_model->has_model(adgroup_id)) {
                throw std::runtime_error(std::string("adgroup ") + adgroup_id + " is not recognized by model!");
            }
            // std::cout << "adgroup " << adgroup_id << std::endl;
            std::string brand_id = adgroup_id;  // actual brand_id plays no role in this test
            double user_svr = user_adgroup_svr[i_adgroup][i_req];
            if (svr_model->run(brand_id, adgroup_id, user_svr) == 0) {
                double svr = svr_model->svr();
                double mult = svr_model->bid_multiplier();
                // std::cout << "  svr " << svr << "; multiplier " << mult << std::endl;
            } else {
                std::cout << "oooops " << svr_model->message() << std::endl;
            }
        }
    }

    timer.stop();

    auto N = n_req;
    auto n = adgroup_ids.size();
    std::cout << "processing " << N << " requests:" << std::endl;
    timeit(N, timer);
    std::cout << std::endl << "considering " << n << " brands per request, "
              << "hence " << N * n << " model calls:" << std::endl;
    timeit(N * n, timer);
}


int main(int argc, char const * const * argv)
{
    std::string modelpath;

    if (argc < 2) {
        modelpath = std::string(std::getenv("DATADIR")) + "/svr";
    } else {
        modelpath = std::string(argv[1]);
        while (modelpath.back() == '/') {
            modelpath.pop_back();
        }
    }

    auto feature_engine = saturn::FeatureEngine();
    auto svr_model = new saturn::SvrModel(feature_engine, modelpath);

    std::vector<ColumnInfo> col_info; // = read_column_list(modelpath + "/data_test/column_list.txt");
    std::vector<std::vector<ColumnValue>> request_data; // = read_request_data(modelpath + "/data_test/raw.txt", col_info);
    std::vector<std::string> adgroup_ids = read_adgroup_list(modelpath + "/data_test/adgroup_ids.txt");

    std::vector<std::vector<double>> user_adgroup_svr;
    for (size_t i = 0; i < adgroup_ids.size(); i++) {
        user_adgroup_svr.push_back(read_user_adgroup_svr(modelpath + "/data_test/user_extlba/" + adgroup_ids[i] + ".txt"));
    }
    auto n = user_adgroup_svr[0].size();
    for (size_t i = 1; i < adgroup_ids.size(); i++) {
        if (user_adgroup_svr[i].size() != n) {
            throw std::runtime_error("user-level SVR files contain different number of records");
        }
    }

    run(feature_engine, svr_model, col_info, request_data, adgroup_ids, user_adgroup_svr);

    delete svr_model;

    return 0;
}