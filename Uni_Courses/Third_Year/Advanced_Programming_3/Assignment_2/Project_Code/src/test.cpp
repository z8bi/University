#include "../include/sklearn_cpp/sklearn_cpp.hpp"

#include <iostream>

int main() {

    sklearn_cpp::Dataset d(
        2,      // k 
        5,      // b 
        10,      // num_points
        0,      // min x
        100,    // max x
        3,      // seed -> same seed means same dataset, so we can test different lin reg methods on the same dataset for consistency
        0.2,    // noise_multiplier - larger meanst larger error
        1       // sigma - used in adding error, larger sigma means larger error
    );

    d.print();

    sklearn_cpp::Dataset concrete = sklearn_cpp::CSVReader::read_CSV("data/concrete.csv", true);

    sklearn_cpp::linear_model::LinearRegression lr(0.01, 10000);
    lr.fit(concrete);

    sklearn_cpp::Dataset concrete_incomplete = sklearn_cpp::CSVReader::read_CSV("data/concrete.csv", true);
    sklearn_cpp::Dataset concrete_predictions = lr.predict(concrete_incomplete);

    concrete_predictions.print();
    
    //make sure the console doesn't immediatelly close :D
    char bs;
    std::cin >> bs;

    return 0;
}
