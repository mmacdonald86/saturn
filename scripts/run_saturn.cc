#include "saturn/saturn.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace saturn;

const std::string USAGE = 
        "Usage:\n"
        "  run_saturn model_file_path run brand_id adgroup_id psvr\n"
        "  run_saturn model_file_path has_model adgroup_id";


int has_model(saturn::SvrModel * svr_model, int argc, char const * const * argv)
{
    if (argc < 4) {
        return 1;
    }
    std::string adgroup_id = std::string(argv[3]);
    auto z = svr_model->has_model(adgroup_id);
    if (z) {
        std::cout << "yes" << std::endl;
    } else {
        std::cout << "no" << std::endl;
    }
    return 0;
}


int run(saturn::SvrModel * svr_model, int argc, char const * const * argv)
{
    if (argc < 6) {
        return 1;
    }

    std::string brand_id = std::string(argv[3]);
    std::string adgroup_id = std::string(argv[4]);
    double user_svr = std::stod(argv[5]);

    std::cout << "brand ID:        " << brand_id << std::endl;
    std::cout << "adgroup ID:      " << adgroup_id << std::endl;
    std::cout << "predicted SVR:   " << user_svr << std::endl;
    std::cout << std::endl;

    if (svr_model->run(brand_id, adgroup_id, user_svr) == 0) {
        std::cout << "model output" << std::endl;
        std::cout << "svr:             " << svr_model->svr() << std::endl;
        std::cout << "multiplier:      " << svr_model->bid_multiplier() << std::endl;
    } else {
        std::cout << "ERROR!" << std::endl;
        std::cout << svr_model->message() << std::endl;
    }

    return 0;
}


int main(int argc, char const * const * argv)
{
    int exit_code = 0;

    if (argc < 3) {
        exit_code = 1;

    } else {
        std::string modelpath = std::string(argv[1]);
        while (modelpath.back() == '/') {
            modelpath.pop_back();
        }
        std::cout << "model file path: " << modelpath << std::endl;

        std::string command = std::string(argv[2]);

        auto feature_engine = saturn::FeatureEngine();
        auto svr_model = new saturn::SvrModel(feature_engine, modelpath);

        if (command == "run") {
            exit_code = run(svr_model, argc, argv);
        } else if (command == "has_model") {
            exit_code = has_model(svr_model, argc, argv);
        } else {
            exit_code = 1;
        }

        delete svr_model;
    }

    if (exit_code != 0) {
        std::cout << USAGE << std::endl;
    }
    return exit_code;
}