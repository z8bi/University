#pragma once

#include <stdexcept> //runtime errors

#include "../DatasetClass.hpp"

namespace sklearn_cpp {
namespace linear_model {

//Little helper to calculate mean
template<typename T> //We can actualy use the templates here to ensure our helper works for int and double, etc.

inline double calculate_mean(const std::vector<T> &vect) {
    double mean{0};
    for(size_t i{0}; i < vect.size(); i++) {
        mean += vect[i];
    } 
    return mean / vect.size();
}

class LinearRegression {

private:
    //These are the learnable parameters -> weights has to be a vector to enable using multi-feature datasets
    std::vector<double> weights;
    double b = 0.0;

    //These are the default learning parameters
    double learning_rate_ = 0.001;
    size_t n_iterations_ = 1000;

public:
    //First default constructor because we will specify a different one too
    LinearRegression() = default;

    //Basic empty-body constructor changing the learning paramters
    LinearRegression(double learning_rate, size_t n_iterations)
        : learning_rate_(learning_rate), n_iterations_(n_iterations) 
    {

    }

    /*
    ===================================
    ========FIT IMPLEMENTATION=========
    ===================================
    */

    //These are the two main exposed functions which this library enables calling
    void fit(const sklearn_cpp::Dataset& data) {

    }


    /*
    ===================================
    ======PREDICT IMPLEMENTATION=======
    ===================================

    */

    /*
    This predict returns a Y-vector based on an input vector of X values for each 
    */
    
    std::vector<double> predict(const std::vector<std::vector<double>>& X) const {

        //First we check if model has be fitted
        if(weights.empty()) {
                throw std::runtime_error("Model has not been fitted yet.");
            };
        
        //temporary y_value vector
        std::vector<double> y_predictions{};

        for(size_t i{0}; i < X.size(); i++) {

            //We check the current X vector's validity
            if(X[i].size() != weights.size()) {
                throw std::runtime_error("Input feature size does not match learned weights.");
                continue;
            };

            double current_y_prediction{};

            for(size_t j{0}; j < X[i].size(); j++) {
                current_y_prediction += weights[j]*X[i][j];
            }
            current_y_prediction += b;

            y_predictions.push_back(current_y_prediction);

        }

        return y_predictions;
    }

    /*
    OVERLOADED predict which works with a Dataset object and overrides its y_values
    */

    sklearn_cpp::Dataset predict(sklearn_cpp::Dataset data) const {

        //First we check if model has be fitted
        if(weights.empty()) {
                throw std::runtime_error("Model has not been fitted yet.");
            };
        
        //Clear Dataset Y_vector just in case to prepare it for the predictions
        data.y.clear();

        for(size_t i{0}; i < data.X.size(); i++) {

            //We check the current X vector's validity
            if(data.X[i].size() != weights.size()) {
                throw std::runtime_error("Input feature size does not match learned weights.");
                continue;
            };

            double current_y_prediction{};

            for(size_t j{0}; j < data.X[i].size(); j++) {
                current_y_prediction += weights[j]*data.X[i][j];
            }
            current_y_prediction += b;

            data.y.push_back(current_y_prediction);

        }

        return data;
    }

    /*
    ===================================
    ===========GET HELPERS ============
    ===================================
    */
    const std::vector<double>& get_weights() const { 
        return weights; 
    }
    double get_b_value() const { 
        return b; 
    }

};

} // namespace linear_model
} // namespace sklearn_cpp