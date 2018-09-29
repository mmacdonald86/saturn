#include "saturn/saturn.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace saturn;


int main(int argc, char const * const * argv)
{
    std::string modelpath;
    std::string brand_id, adgroup_id;
    double user_svr;

    if (argc < 5) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  run_saturn model_file_path brand_id adgroup_id psvr" << std::endl;
        return 1;
    } else {
        modelpath = std::string(argv[1]);
        while (modelpath.back() == '/') {
            modelpath.pop_back();
        }
        brand_id = std::string(argv[2]);
        adgroup_id = std::string(argv[3]);
        user_svr = std::stod(argv[4]);
    }

    std::cout << "model file path: " << modelpath << std::endl;
    std::cout << "brand ID:        " << brand_id << std::endl;
    std::cout << "adgroup ID:      " << adgroup_id << std::endl;
    std::cout << "predicted SVR:   " << user_svr << std::endl;
    std::cout << std::endl;

    auto feature_engine = saturn::FeatureEngine();
    auto svr_model = new saturn::SvrModel(feature_engine, modelpath);

    if (svr_model->run(brand_id, adgroup_id, user_svr) == 0) {
        std::cout << "model output" << std::endl;
        std::cout << "svr:             " << svr_model->svr() << std::endl;
        std::cout << "multiplier:      " << svr_model->bid_multiplier() << std::endl;
    } else {
        std::cout << "ERROR!" << std::endl;
        std::cout << svr_model->message() << std::endl;
    }

    delete svr_model;

    return 0;
}