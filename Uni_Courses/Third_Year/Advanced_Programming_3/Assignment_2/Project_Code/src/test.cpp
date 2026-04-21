#include "../include/sklearn_cpp/sklearn_cpp.hpp"

#include <iostream>

#include <chrono>

/*

We will use the std::chrono to track elapsed time between start and stop for fitting
Useful to see how long it actually takes to fit the model based on the chosen learning parameters

*/

int main() {

    /*
    ============================
    LINEAR REGRESSION TESTING
    ============================
    */

    //TEST 1: CONCRETE DATASET TESTING
    //sklearn_cpp::Dataset concrete = sklearn_cpp::CSVReader::read_CSV("data/concrete.csv", true);

    //sklearn_cpp::linear_model::LinearRegression lr(1e-7, 50000);    
    //lr.fit(concrete);

    //sklearn_cpp::Dataset concrete_incomplete = sklearn_cpp::CSVReader::read_CSV("data/concrete_incomplete.csv", true);
    //sklearn_cpp::Dataset concrete_predictions = lr.predict(concrete_incomplete);
    //double r2 = lr.r2_score(concrete_incomplete);

    //concrete_predictions.print();
    //std::cout << "R2 Score: " << r2 << std::endl;

    //TEST 2: BOSTON HOME PRICE DATASET TESTING
    //sklearn_cpp::Dataset boston = sklearn_cpp::CSVReader::read_CSV("data/Boston.csv", true);

    //sklearn_cpp::linear_model::LinearRegression model(4e-7, 1000000);
    //model.fit(boston);
    //double r2 = model.r2_score(boston);
    //sklearn_cpp::Dataset predictions = model.predict(boston);
    //predictions.print();
    //std::cout << "R2 Score: " << r2 << std::endl;

    /*
    ============================
    LOGISTIC REGRESSION TESTING - BINARY CLASSIFICATION
    ============================
    */

    //TEST 1: ECG DATASET TESTING
    //sklearn_cpp::Dataset ecg = sklearn_cpp::CSVReader::read_CSV("data/ecg.csv", true);
    //sklearn_cpp::linear_model::LogisticRegression logreg(4e-7, 2000, 0.01);
    //logreg.fit(ecg);
    //double accuracy = logreg.accuracy_score(ecg);
    //std::cout << "Accuracy: " << accuracy << std::endl;

    /*
    ============================
    LOGISTIC REGRESSION TESTING - MULTICLASS CLASSIFICATION
    ============================
    */

    //TEST 2: MNIST DATASET TESTING
    //Train on the training file, then evaluate accuracy on the separate test file
    sklearn_cpp::Dataset mnist_train = sklearn_cpp::CSVReader::read_CSV("data/mnist_train.csv", true);
    sklearn_cpp::Dataset mnist_test  = sklearn_cpp::CSVReader::read_CSV("data/mnist_test.csv", true);

    sklearn_cpp::linear_model::LogisticRegression mnist_logreg(1e-5,   //iteration speed
                                                               100,    //number of iterations
                                                               0.0001  //lambda
                                                            );

    //Fit only on the training dataset
    mnist_logreg.fit(mnist_train);

    //Check the model on unseen training data and unseen test data separately
    double mnist_train_accuracy = mnist_logreg.accuracy_score(mnist_train);
    double mnist_test_accuracy  = mnist_logreg.accuracy_score(mnist_test);

    std::cout << "MNIST Train Accuracy: " << mnist_train_accuracy << std::endl;
    std::cout << "MNIST Test Accuracy: "  << mnist_test_accuracy  << std::endl;

    //make sure the console doesn't immediatelly close :D
    char bs;
    std::cin >> bs;

    return 0;
}