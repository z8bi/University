#pragma once

#include "../DatasetClass.hpp"
#include <cmath>
#include <algorithm> //used for the MODE deciding function (online documentation)
#include <stdexcept> //runtime errors

/*

=================================
========LOGISTIC REGRESSION=======
=================================

Two "backend" calculation algorithms are implemented into this class, as per the assignment:
Binary classification 
Multiclass classification

=================================
========VECTOR STRUCTURES========
=================================

weights -> [feature] -> in binary we use one weight per feature
b -> one scalar bias for binary classification
multiclass_weights -> [class][feature] -> in multiclass we use one full weight vector per class
multiclass_biases -> [class] -> in multiclass its K values
outputs[sample][class] -> predictions: each sample has a vector of probabilities for each class

====================================
========IMPLEMENTED FEATURES=========
====================================

The fit() function decides which one to use based on the number of unique y values and sets an internal enum
The fit function is the main training function which takes in a Dataset object and trains the model based on the X and y values. 
The gradient descent method is implemented as the main training algoritm.

Prediction API is based on the sklearn API, so we have separated the predict functions into three which may be called separately 
-> like the real library, but the predict() is the main user interface:
1. decision_function() -> returns the raw y prediction values before the sigmoid
2. predict_proba() -> returns the predicted probabilities for the positive class, one probability for each sample
3. predict() -> returns the final class predictions based on the probabilities -> MAIN USER INTERFACE
4. Dataset predict() -> The predict function is overloaded to work with a Dataset object which we generate from the CSV files, which is going to be the main way to use our library

*/

namespace sklearn_cpp {
namespace linear_model {

class LogisticRegression {

    private:
        //These are the learnable parameters -> weights has to be a vector to enable using multi-feature datasets
        std::vector<double> weights;
        double b = 0.0;

        //these are the learnabale parameters for multiclass classification -> one full weight vector and one bias per class
        std::vector<std::vector<double>> multiclass_weights;
        std::vector<double> multiclass_biases;

        //store the original class values so predictions can map back correctly even if the classes are not 0,1,2,...
        std::vector<int> multiclass_labels;

        //These are the default learning parameters
        double learning_rate_ = 0.0001;
        size_t n_iterations_ = 2000;
        double lambda = 0.01; //regularization parameter -> used later in fit

        //tiny epsilon used in the loss calculation so log(0) never happens
        double eps_ = 1e-15;

        //stores the final loss after fitting
        double current_loss = 0.0;

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

        /*
        Helper to extract abd store unique class labels for multiclass classification.
        We sort first so the mapping is stable and predictable
        */
        std::vector<int> extract_multiclass_labels(const sklearn_cpp::Dataset& data) const {
            std::vector<int> labels{};

            for (double y_value : data.y) {
                labels.push_back(static_cast<int>(y_value));
            }

            std::sort(labels.begin(), labels.end());
            labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
            return labels;
       }

       /*
       Helper which maps an original label value ro the internal class index 0,1,2,...,k-1.
       This is useful because the maths for softmax is much easier with compact class indices.
       */
       size_t label_to_index(int label) const {
            for (size_t i{0}; i < multiclass_labels.size(); i++) {
                if (multiclass_labels[i] == label) {
                    return i;
                }
            }
            throw std::runtime_error("Unknown class label encountered.");
       }

       /*
       Helper which calculates one full vector of softmax probabilities for one data point.
       This is kept private because the public predict_proba() function in this class was designed
       for the binary case and retruns one probability per data point.
       */
        std::vector<double> softmax_probabilities_one_sample(const std::vector<double>& x) const {
            if (multiclass_weights.empty()) {
                throw std::runtime_error("Multiclass parameters are not initialized.");
            }
            if (x.size() != multiclass_weights[0].size()) {
                throw std::runtime_error("Input feature size does not match learned weights.");
            }

            //First compute the raw class scores z_k = w_k.x + b_k
            std::vector<double> logits(multiclass_weights.size(), 0.0);
            for (size_t k{0}; k < multiclass_weights.size(); k++) {
                double score = 0.0;
                for (size_t j{0}; j < x.size(); j++) {
                    score += multiclass_weights[k][j] * x[j];
                }
                score += multiclass_biases[k];
                logits[k] = score;
            }

            //Numerical stability trick: subtract the largest logit before exponentiating
            double max_logit = *std::max_element(logits.begin(), logits.end());

            std::vector<double> exp_shifted(logits.size(), 0.0);
            double denominator = 0.0;
            for (size_t k{0}; k < logits.size(); k++) {
                exp_shifted[k] = std::exp(logits[k] - max_logit);
                denominator += exp_shifted[k];
            }

            std::vector<double> probabilities(logits.size(), 0.0);
            for (size_t k{0}; k < logits.size(); k++) {
                probabilities[k] = exp_shifted[k] / denominator;
            }

            return probabilities;
        }

        /*
        Helper whcih returns the full softmax probability matrix for all rows in X.
        Each row in the output corresponds to one smaple and contains one probability per class.
        */
       std::vector<std::vector<double>> multiclass_predict_proba_internal(const std::vector<std::vector<double>>& X) const {
            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            std::vector<std::vector<double>> all_probabilities{};

            for (size_t i{0}; i < X.size(); i++) {
                all_probabilities.push_back(softmax_probabilities_one_sample(X[i]));
            }

            return all_probabilities;
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

            //Decide the mode -> binary or multiclass, changes the internal backend of the functions
            this->current_mode = decide_mode(data);

            if(this->current_mode == MODE::BINARY) {
                //these are used in the in the gradient descent calculations and loops -> have to cast to double in order to use in calculations
                const size_t m = data.X.size();
                const size_t num_features = data.X[0].size();

                //Uses assign to set the size of the weight vector to match the X values and sets to zero
                weights.assign(num_features, 0.0); 
                b = 0.0;

                //clear multiclass parameters just in case this same object was previously used for multiclass fitting
                multiclass_weights.clear();
                multiclass_biases.clear();
                multiclass_labels.clear();

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

                        //bound the probability away from 0 and 1 so log(0) never happens
                        if (y_hat < eps_) {
                            y_hat = eps_;
                        }
                        if (y_hat > 1.0 - eps_) {
                           y_hat = 1.0 - eps_;
                        }

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
                            dL_dOmega[j] += error * data.X[i][j];
                        }
                        dL_db += error;
                    }

                    //average the gradient over the dataset size and then add regulization
                    for(size_t j{0}; j < weights.size(); j++) {
                        dL_dOmega[j] = (1.0 / (double)m) * dL_dOmega[j] + (2.0 * lambda * weights[j]);
                    }
                    dL_db = (1.0 / (double)m) * dL_db + (2.0 * lambda * b);

                    /*
                    Calculate new weights and b based on the gradients and learning rate
                    */
                    for(size_t j{0}; j < weights.size(); j++) {
                        weights[j] -= learning_rate_ * dL_dOmega[j];
                    } 
                    b -= learning_rate_ * dL_db; 
                }

                /*
                //calculate the loss one last time after training loop finished and store it in the member variable
                */
                double loss = 0.0;
                std::vector<double> y_predictions = predict_proba(data.X);

                for (size_t i = 0; i < m; i++) {
                    double y_hat = y_predictions[i];

                    //bound the probability away from 0 and 1 so log(0) never happens
                    if (y_hat < eps_) {
                        y_hat = eps_;
                    }
                    if (y_hat > 1.0 - eps_) {
                        y_hat = 1.0 - eps_;
                    }

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
                this->current_loss = loss; //update the current loss value after training

            }

            //Multiclass implementation
            else {
                //these are used in the gradient descent calculation and loops
                const size_t m = data.X.size();
                const size_t num_features = data.X[0].size();

                //store the original labels and initialise one weight vector and one bias per class
                multiclass_labels = extract_multiclass_labels(data);
                const size_t num_classes = multiclass_labels.size();

                multiclass_weights.assign(num_classes, std::vector<double>(num_features, 0.0));
                multiclass_biases.assign(num_classes, 0.0);

                //Clear binary parameters just in case the same object was previously used for binary fitting
                weights.clear();
                b = 0.0;

                //This is the main trianing loop -> runs for n_iterations
                for(size_t iter{0}; iter < n_iterations_; iter++) {

                    //Create the gradient containers and set them to zero for the new iteration
                    std::vector<std::vector<double>> dL_dOmega(num_classes, std::vector<double>(num_features, 0.0));
                    std::vector<double> dL_db(num_classes, 0.0);

                    //1. calculate the full probability matrix for the current multiclass weights and biases
                    std::vector<std::vector<double>> y_probabilities = multiclass_predict_proba_internal(data.X);

                    //2. Loss function calculation
                    double loss = 0.0;

                    //this loop calculates the multiclass cross entropy loss
                    for (size_t i{0}; i < m; i++) {
                        int current_label = static_cast<int>(data.y[i]);
                        size_t true_class_index = label_to_index(current_label);

                        //only the true class contributes to the one-hot cross entropy expression
                        double y_hat_true = y_probabilities[i][true_class_index];

                        //bound the probability away from 0 so log(0) never happens
                        if (y_hat_true < eps_) {
                            y_hat_true = eps_;
                        }
                        if (y_hat_true > 1.0) {
                            y_hat_true = 1.0;
                        }

                        loss += -std::log(y_hat_true);
                    }

                    //average over all data points
                    loss = (1.0 / (double)m) * loss;

                    //regularization calculation for all class weights and biases
                    double reg = 0.0;
                    for (size_t k{0}; k < num_classes; k++) {
                        for (double w : multiclass_weights[k]) {
                            reg += w * w;
                        }
                        reg += multiclass_biases[k] * multiclass_biases[k];
                    }

                    /*
                    Final regularized loss -> regularization added at the end
                    */
                    loss += lambda * reg;

                    /*
                    3. Gradient of loss function calculations
                    For each class k, the error term is:
                    p_k - 1 if k is the correct class
                    p_k - 0 otherwise
                    */
                    for(size_t i{0}; i < m; i++) {
                        int current_label = static_cast<int>(data.y[i]);
                        size_t true_class_index = label_to_index(current_label);

                        for (size_t k{0}; k < num_classes; k++) {
                            double target = (k == true_class_index) ? 1.0 : 0.0;
                            double error = y_probabilities[i][k] - target;

                            for(size_t j{0}; j < num_features; j++) {
                                dL_dOmega[k][j] += error * data.X[i][j];
                            }
                            dL_db[k] += error;
                        }
                    }

                    //average the gradients over the dataset size and then add regularization
                    for(size_t k{0}; k < num_classes; k++) {
                        for(size_t j{0}; j < num_features; j++) {
                            dL_dOmega[k][j] = (1.0 / (double)m) * dL_dOmega[k][j] + (2.0 * lambda * multiclass_weights[k][j]);
                        }
                        dL_db[k] = (1.0 / (double)m) * dL_db[k] + (2.0 * lambda * multiclass_biases[k]);
                    }

                    /*
                    Calculate new weights and biases based on the gradients and learning rate
                    */
                    for(size_t k{0}; k < num_classes; k++) {
                        for(size_t j{0}; j < num_features; j++) {
                            multiclass_weights[k][j] -= learning_rate_ * dL_dOmega[k][j];
                        }
                        multiclass_biases[k] -= learning_rate_ * dL_db[k];
                    }
                }

                /*
                calculate the loss one last time after training loop finished and store it in the member variable
                */
                double loss = 0.0;
                std::vector<std::vector<double>> y_probabilities = multiclass_predict_proba_internal(data.X);

                for (size_t i{0}; i < m; i++) {
                    int current_label = static_cast<int>(data.y[i]);
                    size_t true_class_index = label_to_index(current_label);

                    double y_hat_true = y_probabilities[i][true_class_index];

                    //bound the probability away from 0 so log(0) never happens
                    if (y_hat_true < eps_) {
                        y_hat_true = eps_;
                    }
                    if (y_hat_true > 1.0) {
                        y_hat_true = 1.0;
                    }

                    loss += -std::log(y_hat_true);
                }

                loss = (1.0 / (double)m) * loss;

                double reg = 0.0;
                for (size_t k{0}; k < num_classes; k++) {
                    for (double w : multiclass_weights[k]) {
                        reg += w * w;
                    }
                    reg += multiclass_biases[k] * multiclass_biases[k];
                }

                loss += lambda * reg;
                this->current_loss = loss;
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


        This function returns the raw scores before the sigmoid
        nested vector because we need one row per multiclass

        the output is structures as [sample][class] -> aka each sample has a vector of probabilities correspoding to each class
        */

        std::vector<std::vector<double>> decision_function(const std::vector<std::vector<double>>& X) const {

            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            //Nested vector storing the predictins for each sampple, for each class, as per [sample][class]
            std::vector<std::vector<double>> y_prediction_rows{};

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

                    //store in the predictions first row of the vector
                    y_prediction_rows.push_back({current_y_prediction});

                }

                return y_prediction_rows;
            }

            //Multiclass implementation
            else {
                throw std::runtime_error("decision_function() is only implemented for binary classification in this class.");
            }


        }

        /*
        This function uses the sigmoid on the predicted y value
        */
        std::vector<double> predict_proba(const std::vector<std::vector<double>>& X) const {
            if(current_mode == MODE::NOT_FIT) {
                throw std::runtime_error("Model has not been fitted yet.");
            }

            std::vector<double> probabilities{};

            if(current_mode == MODE::BINARY) {
                //First we get the raw y predition -> same with linear regression
                std::vector<std::vector<double>> decision_values = decision_function(X);

                //range based for loop to go through all decision values and apply sigmoid to get the probabilities
                for (const auto& val : decision_values) {
                    double probability = 1.0 / (1.0 + std::exp(-val[0])); //sigmoid function
                    probabilities.push_back(probability); 
                }

            }

            //Multiclass implementation
            else {
                throw std::runtime_error("predict_proba() returns one probability per data point and is only implemented for binary classification in this class.");
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

            std::vector<int> class_predictions{};

            //Binary implementation
            if(current_mode == MODE::BINARY) {
                //First we get the probabilities
                std::vector<double> probabilities = predict_proba(X);

                //range based for loop to go through all probabilities and apply threshold of 0.5 to get the class predictions
                for (auto prob : probabilities) {
                    int class_pred = (prob >= 0.5) ? 1 : 0; //threshold at 0.5
                    class_predictions.push_back(class_pred);
                }

                return class_predictions;
            }

            //Multiclass implementation
            else {
                //Get the full probability vector for each sample and choose the class with the largest probability
                std::vector<std::vector<double>> all_probabilities = multiclass_predict_proba_internal(X);

                for (size_t i{0}; i < all_probabilities.size(); i++) {
                    size_t best_class_index = 0;
                    double best_probability = all_probabilities[i][0];

                    for (size_t k{1}; k < all_probabilities[i].size(); k++) {
                        if (all_probabilities[i][k] > best_probability) {
                            best_probability = all_probabilities[i][k];
                            best_class_index = k;
                        }
                    }

                    //Map the internal class index back to the original class label
                    class_predictions.push_back(multiclass_labels[best_class_index]);
                }

                return class_predictions;
            }

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

            //Then we clear the existing y values to prepare for predictions
            data.y.clear();

            //Binary implementation
            if(current_mode == MODE::BINARY) {
                //First we get the probabilities
                std::vector<double> probabilities = predict_proba(data.X);

                //range based for loop to go through all probabilities and apply threshold of 0.5 to get the class predictions
                for (const auto& prob : probabilities) {
                    double class_pred = (prob >= 0.5) ? 1.0 : 0.0; //threshold at 0.5
                    data.y.push_back(class_pred);
                }

            }

            //Multiclass implementation
            else {
                std::vector<int> class_predictions = predict(data.X);

                for (int class_pred : class_predictions) {
                    data.y.push_back((double)class_pred);
                }
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
        double get_current_loss() const {
            return current_loss;
        }

};

} // namespace linear_model
} // namespace sklearn_cpp
