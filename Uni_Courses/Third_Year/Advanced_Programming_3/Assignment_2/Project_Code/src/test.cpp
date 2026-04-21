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

    TEST 1: CONCRETE DATASET TESTING
    
    sklearn_cpp::Dataset concrete = sklearn_cpp::CSVReader::read_CSV("data/concrete.csv", true);

    sklearn_cpp::linear_model::LinearRegression lr(1e-7, 50000);   
    
    auto start_concrete = std::chrono::steady_clock::now(); //track start time

    lr.fit(concrete); //fit the model

    //track end time and console output it
    auto end_concrete = std::chrono::steady_clock::now(); 
    auto elapsed_ms_concrete = std::chrono::duration_cast<std::chrono::milliseconds>(end_concrete - start_concrete);
    std::cout << "Elapsed time for concrete model training with chosen parameters: \n\t" 
    << elapsed_ms_concrete.count()/1000.0 << " in seconds\n\t"
    << elapsed_ms_concrete.count()/60000.0 << " in minutes\n";

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

    std::cout << "Currently fitting model..." << std::endl;

    auto start_boston = std::chrono::steady_clock::now(); //track start time

    model.fit(boston); //fit the model to the data

    //track end time and console output it
    auto end_boston = std::chrono::steady_clock::now(); 
    auto elapsed_ms_boston = std::chrono::duration_cast<std::chrono::milliseconds>(end_boston - start_boston);
    std::cout << "Elapsed time for Boston model training with chosen parameters: \n\t" 
    << elapsed_ms_boston.count()/1000.0 << " in seconds\n\t"
    << elapsed_ms_boston.count()/60000.0 << " in minutes\n";

    double r2 = model.r2_score(boston); //calculate the R2 score on the same dataset
    sklearn_cpp::Dataset predictions = model.predict(boston); //use the predict function on the same dataset and print the predictions
    predictions.print(); //print dataset

    std::cout << "R2 Score: " << r2 << std::endl; //print R2 score

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
    std::cout << "Currently fitting model..." << std::endl;

    auto start_ecg = std::chrono::steady_clock::now(); //track start time

    logreg.fit(ecg); //fit the model to the data

    //track end time and console output it
    auto end_ecg = std::chrono::steady_clock::now(); 
    auto elapsed_ms_ecg = std::chrono::duration_cast<std::chrono::milliseconds>(end_ecg - start_ecg);
    std::cout << "Elapsed time for model training with chosen parameters: \n\t" 
    << elapsed_ms_ecg.count()/1000.0 << " in seconds\n\t"
    << elapsed_ms_ecg.count()/60000.0 << " in minutes\n";

    double accuracy = logreg.accuracy_score(ecg); //calculate the accuracy score on the same dataset
    std::cout << "Accuracy: " << accuracy << std::endl; //print accuracy

    //make sure the console doesn't immediatelly close :D
    char bs;
    std::cin >> bs;

    return 0;
}
