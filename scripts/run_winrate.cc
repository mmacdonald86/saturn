#include "saturn/saturn.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace saturn;

const std::string USAGE = 
        "Usage:\n"
        "  run_winrate model_file_path run features\n";


int run(saturn::WrModel * wr_model, int argc, char const * const * argv)
{
    if (argc < 6) {
        return 1;
    }
    std::vector<std::string> input;
    input.push_back(std::string(argv[3]));
    input.push_back(std::string(argv[4]));
    input.push_back(std::string(argv[5]));

    std::cout << "Input Features:   " << std::endl;
    for (auto i = input.begin(); i != input.end(); ++i){
        std::cout << *i << ' ';
    }
    std::cout << std::endl;

    if (wr_model->get_prob(input) == 0) {
        std::cout << "model output" << std::endl;
        std::cout << "prob:             " << wr_model->prob() << std::endl;
    } else {
        std::cout << "ERROR!" << std::endl;
        std::cout << wr_model->message() << std::endl;
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
        auto wr_model = new saturn::WrModel(feature_engine, modelpath);

        if (command == "run") {
            exit_code = run(wr_model, argc, argv);
        } else {
            exit_code = 1;
        }

        delete wr_model;
    }

    if (exit_code != 0) {
        std::cout << USAGE << std::endl;
    }
    return exit_code;
}

