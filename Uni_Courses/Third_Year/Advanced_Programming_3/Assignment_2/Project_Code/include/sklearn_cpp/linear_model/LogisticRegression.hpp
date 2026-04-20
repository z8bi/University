#pragma once

#include "../DatasetClass.hpp"
#include <cmath>
#include <stdexcept> //runtime errors

/*

Guide:

Fit:
The fit function is the main training function which takes in a Dataset object and trains the model based on the X and y values. 
The gradient descent method is implemented as the main training algoritm.

Prediction API is based on the sklearn API, so we have separated the predict functions into three which may be called separately 
-> like the real library, but the predict() is the main user interface:
1. decision_function() -> returns the raw y prediction values before the sigmoid
2. predict_proba() -> returns the predicted probabilities for the positive class (in binary classification
3. predict() -> returns the final class predictions based on the probabilities -> MAIN USER INTERFACE
4. Dataset predict() -> The predict function is overloaded to work with a Dataset object which we generate from the CSV files, which is going to be the main way to use our library

*/

//Little helper to calculate mean
template<typename T> //We can actualy use the templates here to ensure our helper works for int and double, etc.

inline double calculate_mean(const std::vector<T> &vect) {
    double mean{0};
    for(size_t i{0}; i < vect.size(); i++) {
        mean += vect[i];
    } 
    return mean / vect.size();
}

namespace sklearn_cpp {
namespace linear_model {

class LogisticRegression {

    private:
        //These are the learnable parameters -> weights has to be a vector to enable using multi-feature datasets
        std::vector<double> weights;
        double b = 0.0;

        //These are the default learning parameters
        double learning_rate_ = 0.0001;
        size_t n_iterations_ = 10000;
        double lambda = 0.01; //regularization parameter -> used later in fit

        //enum for deciding whether to implement the binary or multiclass version -> decided based on the fit function
        enum class MODE {
            NOT_FIT,
            BINARY,
            MULTICLASS
        };

        //default to NOT_FIT, will be changed in fit function
        MODE current_mode = MODE::NOT_FIT; 

        /*
        Private helper which counts the number of unique y_values and decides whether to use binary or multiclass:
        Pass by refference to limit copying, internally copy the Y_values only
        ==used online resources to research how to count unique values in a vector===
        */
        MODE decide_mode(const sklearn_cpp::Dataset& data) {
            std::vector<double> y_copy = data.y;
            std::sort(y_copy.begin(), y_copy.end());
            int uniqueCount = std::unique(y_copy.begin(), y_copy.end()) - y_copy.begin();
            if (uniqueCount == 2) {
                return MODE::BINARY;
            } else if (uniqueCount > 2) {
                return MODE::MULTICLASS;
            }
            else {
                throw std::runtime_error("Invalid number of unique classes in y values.");
            }
        }

    public:
        //Default constructor for safety -> we have default parameter values anyway
        LogisticRegression() = default;

        //Actual constructor which specifies learning parameters
        LogisticRegression(double learning_rate, size_t n_iterations, double lambda)
            : learning_rate_(learning_rate), n_iterations_(n_iterations), lambda(lambda)
        {
            
        }
    

        /*
        ===================================
        ========FIT IMPLEMENTATION=========
        ===================================
        */

        //This is the main fit functions which takes in the main Dataset object and trains the model
        void fit(const sklearn_cpp::Dataset& data) {

            //Main empty data checks
            if (data.X.empty()) {
                throw std::runtime_error("Dataset is empty.");
            }
            if (data.X.size() != data.y.size()) {
                throw std::runtime_error("X and y size mismatch.");
            }

            //Decide the mode
            this->current_mode = decide_mode(data);

            if(this->current_mode == MODE::BINARY) {
                //these are used in the in the gradient descent calculations and loops -> have to cast to double in order to use in calculations
                const size_t m = data.X.size();
                const size_t num_features = data.X[0].size();

                //Uses assign to set the size of the weight vector to match the X values and sets to zero
                weights.assign(num_features, 0.0); 

                //This is the main training loop -> runs for n_iterations
                for(size_t iter{0}; iter < n_iterations_; iter++) {

                    //Create the gradient vectors and set to zero for new iteration
                    std::vector<double> dL_dOmega(num_features, 0.0); 
                    double dL_db = 0.0;

                    //1. calculate the y probabilities for the current weights and b -> uses the predict_proba function to get the probabilities based on the sigmoid of the decision function
                    std::vector<double> y_predictions = predict_proba(data.X);

                    //2. Loss function calculation
                    double loss = 0.0;
                    
                    //this is the sigma part for each data point
                    for (size_t i = 0; i < m; i++) {
                        double y_hat = y_predictions[i];

                        //loss equation
                        loss += data.y[i] * std::log(y_hat)
                             + (1.0 - data.y[i]) * std::log(1.0 - y_hat);
                    }

                    //divide by - 1/ m to get the final loss value (no regularization yet)
                    loss = -(1.0 / (double)(m)) * loss;

                    //regularization calculation
                    double reg = 0.0;

                    //range based for loop implementing the sigma
                    for (double w : weights) {
                        reg += w * w; //weight squared
                    }
                    reg += b * b;

                    /*
                    Final regularized loss -> regularization added at the end
                    */
                    
                    loss += lambda * reg;

                    /*
                    3. Gradient of loss function calculations
                    */
                    for(size_t i{0}; i < m; i++) {
                        //Calculate the error for the current iteration
                        double error = y_predictions[i] - data.y[i];

                        //Calculate dL_dOmega for each weight
                        for(size_t j{0}; j < data.X[i].size(); j++) {
                            dL_dOmega[j] += (error * data.X[i][j]) + (2 * lambda * weights[j]); //add regularization term
                        }
                        dL_db += error + (2 * lambda * b); //add regularization term
                    }

                    /*
                    Calculate new weights and b based on the gradients and learning rate
                    */
                    for(size_t j{0}; j < weights.size(); j++) {
                        weights[j] -= (1 / (double)m) * learning_rate_ * dL_dOmega[j];
                    }
                    b -= (1 / (double)m) * learning_rate_ * dL_db; 
                }
            }

            //Multiclass implementation
            else {
                throw std::runtime_error("Multiclass classification not implemented yet.");
            }
        }


        /*
        ==========================================
        ======PREDICT VECTOR IMPLEMENTATION=======
        ==========================================
        */

        /*
        THESE FUNCTIONS ARE THE CORE ONES USED BY INTERNALS BUT ALSO CALLABLE DIRECTLY BY THE USER, BUT THE MAIN USER INTERFACE IS predict() WHICH CALLS THESE INTERNALS
        These functions all work with vector inputs and outputs, the main user interface is predict which is the only one overloaded to work with the Dataset class
        */

        /*
        This function returns the raw y prediction values before the sigmoid
        */
        //
        std::vector<double> decision_function(const std::vector<std::vector<double>>& X) const {

            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            //temporary y_value vector
            std::vector<double> y_predictions{};

            //Binary implementation
            if(current_mode == MODE::BINARY) {


                for(size_t i{0}; i < X.size(); i++) {

                    //We check the current X vector's validity
                    if(X[i].size() != weights.size()) {
                        throw std::runtime_error("Input feature size does not match learned weights.");
                    };

                    double current_y_prediction{};

                    //calculate the prediction based on ax + b 
                    for(size_t j{0}; j < X[i].size(); j++) {
                        current_y_prediction += weights[j]*X[i][j];
                    }
                    current_y_prediction += b;

                    //store in the predictions vector
                    y_predictions.push_back(current_y_prediction);

                }
            }

            //Multiclass implementation
            else {
                throw std::runtime_error("Multiclass classification not implemented yet.");
            }
            
            return y_predictions;

        }

        /*
        This function uses the sigmoid on the predicted y value
        */
        std::vector<double> predict_proba(const std::vector<std::vector<double>>& X) const {
            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            //First we get the raw y predition -> same with linear regression
            std::vector<double> decision_values = decision_function(X);
            std::vector<double> probabilities{};

            if(current_mode == MODE::BINARY) {

                //range based for loop to go through all decision values and apply sigmoid to get the probabilities
                for (auto val : decision_values) {
                    double probability = 1.0 / (1.0 + std::exp(-val)); //sigmoid function
                    probabilities.push_back(probability); //push back the sigmoid
                }

            }

            //Multiclass implementation
            else {
                throw std::runtime_error("Multiclass classification not implemented yet.");
            }

            return probabilities;
            
        }

        /**
        This function returns the final class predictions based on the probabilities -> MAIN USER INTERFACE
        */
        std::vector<int> predict(const std::vector<std::vector<double>>& X) const {
            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            //First we get the probabilities
            std::vector<double> probabilities = predict_proba(X);
            std::vector<int> class_predictions{};

            //Binary implementation
            if(current_mode == MODE::BINARY) {

                //range based for loop to go through all probabilities and apply threshold of 0.5 to get the class predictions
                for (auto prob : probabilities) {
                    double class_pred = (prob >= 0.5) ? 1 : 0; //threshold at 0.5
                    class_predictions.push_back(class_pred);
                }

            }

            //Multiclass implementation
            else {
                throw std::runtime_error("Multiclass classification not implemented yet.");
            }

            return class_predictions;
        }
        /*
        ==========================================
        ======PREDICT DATASET IMPLEMENTATION======
        ==========================================
        */

        /*
        This function returns the final class predictions based on the probabilities -> MAIN USER INTERFACE USING OUR DATASET CLASS, which we generate from the CSV files
        */
        sklearn_cpp::Dataset predict(sklearn_cpp::Dataset data) const {
            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            //First we get the probabilities
            std::vector<double> probabilities = predict_proba(data.X);
            
            //Then we clear the existing y values to prepare for predictions
            data.y.clear();

            //Binary implementation
            if(current_mode == MODE::BINARY) {

                //range based for loop to go through all probabilities and apply threshold of 0.5 to get the class predictions
                for (auto prob : probabilities) {
                    double class_pred = (prob >= 0.5) ? 1.0 : 0.0; //threshold at 0.5
                    data.y.push_back(class_pred);
                }

            }

            //Multiclass implementation
            else {
                throw std::runtime_error("Multiclass classification not implemented yet.");
            }

            return data;
        }

        /*
        ==========================================
        =======ACCURACY SCORE IMPLEMENTATION======
        ==========================================
        */
        double accuracy_score(const sklearn_cpp::Dataset& data) const {
            //once again the basic checks
            if (current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }
            if (data.X.empty()) {
                throw std::runtime_error("Dataset is empty.");
            }
            if (data.X.size() != data.y.size()) {
                throw std::runtime_error("X and y size mismatch.");
            }

            //use the model to predic the Y values based on the X values
            std::vector<int> y_pred = predict(data.X);

            //range based for loop which compares if the predicted class matches the original in the dataset
            size_t correct = 0;
            for (size_t i = 0; i < data.y.size(); ++i) {
                int y_true = static_cast<int>(data.y[i]);
                if (y_pred[i] == y_true) {
                    ++correct;
                }
            }

            //Return a percentage accuracy score -> correct predictions / total predictions 
            //Have to cast to double because correct is size_t
            return (double)correct / (double)data.y.size();
        }

};

} // namespace linear_model
} // namespace sklearn_cpp