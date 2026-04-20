#include "../include/sklearn_cpp/sklearn_cpp.hpp"

#include <iostream>

int main() {

    /*
    ============================
    LINEAR REGRESSION TESTING
    ============================

    TEST 1: CONCRETE DATASET TESTING
    

    sklearn_cpp::Dataset concrete = sklearn_cpp::CSVReader::read_CSV("data/concrete.csv", true);

    sklearn_cpp::linear_model::LinearRegression lr(1e-7, 50000);    
    lr.fit(concrete);

    sklearn_cpp::Dataset concrete_incomplete = sklearn_cpp::CSVReader::read_CSV("data/concrete_incomplete.csv", true);
    sklearn_cpp::Dataset concrete_predictions = lr.predict(concrete_incomplete);
    double r2 = lr.r2_score(concrete_incomplete);

    concrete_predictions.print();
    std::cout << "R2 Score: " << r2 << std::endl;
    
    /*

    TEST 2: BOSTON HOME PRICE DATASET TESTING
    //DOWNLOADED FROM https://www.geeksforgeeks.org/machine-learning/dataset-for-linear-regression/
    
    

    sklearn_cpp::Dataset boston = sklearn_cpp::CSVReader::read_CSV("data/Boston.csv", true);

    sklearn_cpp::linear_model::LinearRegression model(4e-7, 1000000);
    model.fit(boston); //fit the model to the data
    double r2 = model.r2_score(boston); //calculate the R2 score on the same dataset
    sklearn_cpp::Dataset predictions = model.predict(boston); //use the predict function on the same dataset and print the predictions
    predictions.print(); //print dataset

    std::cout << "R2 Score: " << r2 << std::endl; //print R2 score

    */

    /*
    ============================
    LOGISTIC REGRESSION TESTING - BINARY CLASSIFICATION
    ============================

    TEST 1: ECG DATASET TESTING
    */

    sklearn_cpp::Dataset ecg = sklearn_cpp::CSVReader::read_CSV("data/ecg.csv", true);
    sklearn_cpp::linear_model::LogisticRegression logreg(4e-7, //iteration speed
                                                        10000, //number of iterations
                                                        0.01 //lambda
                                                    );
    logreg.fit(ecg); //fit the model to the data
    double accuracy = logreg.accuracy_score(ecg); //calculate the accuracy score on the same dataset
    std::cout << "Accuracy: " << accuracy << std::endl; //print accuracy

    //make sure the console doesn't immediatelly close :D
    char bs;
    std::cin >> bs;

    return 0;
}
