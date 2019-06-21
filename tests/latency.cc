#include "saturn/common.h"
#include "saturn/saturn.h"
#include "saturn/utils.h"
#include "mars/mars.h"
#include "mars/numeric.h"
#include "mars/utils.h"
#include "mars/models/model.h"

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


void latency(int n_iter, double x_value, std::unique_ptr<mars::BinaryRandomForestClassifier> & model)
{
    double n_feature = model->n_predictors();
    std::vector<double> x_test(n_feature, x_value);
    double y_pred;
    auto timer = Timer();

    if (n_iter == 1) {
        timer.start();
        y_pred = model->predict_one(x_test);
        timer.stop();
    } else {
        timer.start();
        for (int i = 0; i < n_iter; i++) {
            y_pred = model->predict_one(x_test);
        }
        timer.stop();
    }
    std::cout << "  x_test: all " << x_value << "  \t  y_pred: " << y_pred << "\t"
        << n_iter << " prediction(s) \ttotal time: "
        << timer.milliseconds() << " milliseconds"
        << "\tper prediction: " << (double)timer.milliseconds() / n_iter << " milliseconds"
        << std::endl;
}


int main(int argc, char const * const * argv)
{
    if (argc < 2) {
        return 1;
    }
    std::string modelpath;

    modelpath = std::string(argv[1]);
    while (modelpath.back() == '/') {
        modelpath.pop_back();
    }

    mars::AvroReader areader(modelpath.c_str());
    auto const class_name = areader.get_scalar<std::string>("class_name");
    if ("BinaryRandomForestClassifier" != class_name) {
        throw SaturnError(mars::make_string(
        "expecting a `BinaryRandomForestClassifier` for `SvrModel`; get a `", class_name, "`"));
    }

    auto model = mars::BinaryRandomForestClassifier::from_avro(areader);

    auto predictor_count = areader.get_scalar<int>("predictor_count");
    auto tree_count = areader.get_scalar<int>("tree_count");
    std::cout << "  n_features: " << predictor_count << "; n_tree: " << tree_count << std::endl;

    latency(1, 0.0, model);
    latency(1, -1.0, model);
    latency(100, 0.0, model);
    latency(100, -1.0, model);

    return 0;
}