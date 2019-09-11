#include "saturn/saturn.h"

#include <cstdlib>
#include <iostream>
#include <string>

#include "saturn/utils.h"

#include <any>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace saturn;

const std::string USAGE =
        "Usage:\n"
        "  run_Ctr model_file_path run features\n";


int run(saturn::ctrModel * ctr_model, int argc, char const * const * argv)
{

    std::vector<std::string> input;
     
    for (int i = 2; i < argc; i++){ 
        input.push_back(std::string(argv[i]));
//   std::cout << "input" << argv[i]<< std::endl;
    }
  //   for (int i = 0; i< 23; i++){
  //   std::cout << input[i] << std::endl;
  //   }
 //   std::cout << "inside Run"<< std::endl;
    ctr_model->get_prob(input);
    return 0;
}


int main(int argc, char const * const * argv)
{
    auto timer = Timer();
    double prob = 0.0;
    if (argc < 18) { // What is count including features + args. 
        return 0;
    } else {
        std::string modelpath = std::string(argv[1]);
        while (modelpath.back() == '/') {
            modelpath.pop_back();
        }
        // std::string command = std::string(argv[2]); // Is a run needed? Can we just comment this out?

    auto feature_engine = saturn::FeatureEngine();
    auto ctr_model = new saturn::ctrModel(feature_engine, modelpath);
 //   std::cout << "MOdel Parsed from FEATYRE"<< std::endl;
    timer.start();
    prob = run(ctr_model, argc, argv);
    timer.stop();
    std::cout << "prob:" << ctr_model->prob() << std::endl;
    delete ctr_model;
    }
    std::cout << "Time:" << (double)timer.milliseconds() << "milliseconds"<< std::endl;
    std::cout << "Probability: " << prob << "prod"<< std::endl;
    return 1;
}
