#include "saturn/saturn.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <utility>  // as_const

using size_t = std::size_t;


struct Arg {
    std::string brand_id;
    double user_brand_svr;
}


std::vector<std::pair<std::string, std::string>> get_column_list(std::string const & rootpath)
{
    std::vector<std::pair<std::string, std::string>> columns;
    std::string col_name, col_type;
    auto infile = std::ifstream(rootpath + "/column_list.txt");
    while (infile >> col_name >> col_type) {
        columns.emplace_back(col_name, col_type);
    }
    infile.close();
    return std::move(columns);
}

// DemoFeatureBox& ingest_line(string line_, string delimiter = "\t")
// {
//     _line = std::move(line_);
//     auto line = string_view(_line);

//     size_t pos;
//     const size_t ll = delimiter.length();

//     for (size_t idx = 0; idx < _columns.size(); idx++) {
//         pos = line.find(delimiter);
//         auto substr = line.substr(0, pos);
//         if (_columns[idx].second == "int") {
//             _record[idx] = std::stoi(string(substr));
//         } else if (_columns[idx].second == "float") {
//             _record[idx] = std::stod(string(substr));
//         } else {
//             _record[idx] = substr;
//         }
//         line.remove_prefix(pos + ll);
//     }

//     return *this;
// }



int main(int argc, char const * const * argv) {
    // `rootpath` is absolute path to the root of the model directory.
    // This directory contains, in addition to what `Rover` assumes,
    // test data, model predictions on test data, model ids, and column list.
    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "    test_svr path" << std::endl;
        return 0;
    }

    auto feature_engine = saturn::FeatureEngine();

    auto modelpath = std::string(argv[1]);
    auto svr_model = saturn::SvrModel(feature_engine, modelpath);

    while (modelpath.back() == '/') {
        modelpath.pop_back();
    }
    
    auto columns = get_column_list(modelpath);

    std::vector<Arg> run_args;    
    std::ifstream argfile = std::ifstream(modelpath + "/run_args.txt");

    // TODO:
    // read in common request level data
    std::ifstream datafile(modelpath + "/test_data.txt");
    std::string line;
    while (std::getline(datafile, line)) {
    }

    // TODO:
    // read in additional arguments

    // TODO:
    // run model
}
