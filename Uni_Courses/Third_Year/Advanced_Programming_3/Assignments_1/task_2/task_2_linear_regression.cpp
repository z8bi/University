#include <vector>
#include <random>
#include <iostream>
#include <cmath>

//Little helper to calculate mean
template<typename T> //We can actualy use the templates here to ensure our helper works for int and double, etc.

double calculate_mean(const std::vector<T> &vect) {
    double mean{0};
    for(size_t i{0}; i < vect.size(); i++) {
        mean += vect[i];
    } 
    return mean / vect.size();
}

//==========================================================
//=================MAIN DATASET STRUCT======================
//==========================================================

struct Dataset {
    double k_value; //gradient
    double prev_b_value; //intercept
    size_t size;

    std::vector<double> x_values; 
    std::vector<double> y_values;

    //futureproofing - to use in lin reg calculations - 
    double omega_value{0};
    double b_value{0};
    double x_mean{0}; 
    double y_mean{0};
    std::vector<double> y_lin_reg_values; //this way the dataset contains both the original data and lin_reg data

    //Simple contrustor so we can easily create new datasets
    Dataset(double k , double b, size_t num_points, int min, int max, int seed, double noise_multiplier, double sigma) {

        std::mt19937 rng(seed); //initialize random using our seed
        std::uniform_int_distribution<int> range(min, max); //clamp random values to a range
        std::normal_distribution<double> noise_gauss(0.0, sigma);; //clamp random values to a range

        for (size_t i{0}; i < num_points; i++) {
            double current_x = range(rng);
            x_values.push_back(current_x); //cast to integer the creation of mt
            y_values.push_back(k*current_x + b + noise_multiplier*noise_gauss(rng)); // calculate y + add noise
        }
        
        size = num_points;
        k_value = k;
        prev_b_value = b;

    };

    //default contructor (empty) - to be safe, since we declared a custom constructor the default one is overwritten
    Dataset() = default;

    //Basic print function to confirm the dataset + linear regression data (only if it exists) - const as a safeguard only in case print somehow manipulates the data (that would be a very bad print)
    void print_datasets() const {
        std::cout << "\nOriginal Values:\n";
        for (size_t i{0}; i < x_values.size(); i++) {
            std::cout << "[ " << x_values[i] << ", " << y_values[i] << " ]\n";
        }
        std::cout << "\n";

        //if linear regresison was calculated, print too
        if(y_lin_reg_values.size() == x_values.size()) { 
            std::cout << "\nLinear Regression Values:\n";
            for (size_t i{0}; i < x_values.size(); i++) {
                std::cout << "[ " << x_values[i] << ", " << y_lin_reg_values[i] << " ]\n";
            }
            std::cout << "\n";
        }
    };

    //Basic print function to confirm internal values - once again const to be safe
    void print_internals() const {
        std::cout << "\ndataset size: " << size;
        std::cout << "\nk: " << k_value;
        std::cout << "\nprev_b: " << prev_b_value;
        std::cout << "\nomega: " << omega_value;
        std::cout << "\nb: " << b_value;
        std::cout << "\nx_mean: " << x_mean;
        std::cout << "\ny_mean: " << y_mean << "\n";
    };

    void print_errors() const {
        
        //calculate percentage errors
        double omega_error = (omega_value - k_value) / k_value * 100;
        double b_error = (b_value - prev_b_value) / prev_b_value * 100;
        //print percentage errors
        std::cout << "\nPercentage errors are the following:\n";
        std::cout << "\nomega error: " << omega_error << " %";
        std::cout << "\nb error: " << b_error << " %\n";
    };
};


//==========================================================
//===============LINEAR REGRESSION FUNCTIONS================
//==========================================================


//Create linear regression using normal equation
Dataset normal_equation_lin_reg(Dataset data) {
    //use the helper to calculate mean
    data.x_mean = calculate_mean(data.x_values);
    data.y_mean = calculate_mean(data.y_values);
 
    //compute W numerator value
    double omega_numerator{0};
    double omega_denominator{0};
    for(size_t i{0}; i < data.size; i++) {
        omega_numerator += ( (data.x_values[i] - data.x_mean) * (data.y_values[i] - data.y_mean) ); //this is the numerator
    } 

    //compute W denominator value
    for(size_t i{0}; i < data.size; i++) {
        double dx = (data.x_values[i] - data.x_mean);
        omega_denominator +=  dx*dx; //square the sigma
    } 

    //final omega lin reg value
    data.omega_value = omega_numerator / omega_denominator;

    //compute new b value
    data.b_value = data.y_mean - data.omega_value*data.x_mean;

    //compute new Y values based on the model and return in the dataset
    data.y_lin_reg_values.resize(data.x_values.size()); //first resize the vector
    for(size_t i{0}; i < data.size; i++) {
        data.y_lin_reg_values[i] = (data.omega_value * data.x_values[i] + data.b_value); 
    } 

    return data;
}

Dataset gradient_method_linear_regression(Dataset data, double learning_rate, double gradient_components_limit) {

    //initialize all required variables
    double L_domega{1};
    double L_db{1};

    //make sure the lin reg y values are empty, in case user calls it twice on a dataset which already has them calculated
    data.y_lin_reg_values.clear();

    //make sure we don't burn CPU time lol
    size_t iter = 0;
    const size_t max_iters = 100000; 

    //keep iterating until a sufficiently small derivative is found (absolute value because negative derivative also possible)
    while( ( std::abs(L_domega) > gradient_components_limit || std::abs(L_db) > gradient_components_limit ) && iter < max_iters) {

        //ensure they are clean for the next iteration
        L_db = 0.0;
        L_domega = 0.0;

        //compute the sigma in the derivative of L with respect to b
        for(size_t i{0}; i < data.size; i++) {
            L_db += data.omega_value * data.x_values[i] + data.b_value - data.y_values[i];
        }   

        //compute the sigma in the derivative of L with respect to omega
        for(size_t i{0}; i < data.size; i++) {
            L_domega += ( data.omega_value * data.x_values[i] + data.b_value - data.y_values[i] ) * data.x_values[i];
        } 

        //update the derivatives
        L_domega = (1.0 / data.size) * L_domega;
        L_db = (1.0 / data.size) * L_db;

        //update the omega and b
        data.omega_value = data.omega_value - learning_rate*L_domega;
        data.b_value = data.b_value - learning_rate*L_db;

        //keep track of iteration num
        iter++;
    }

    //recompute y values based on the newly found omega and b:
    data.y_lin_reg_values.resize(data.x_values.size()); //first resize the vector
    for(size_t i{0}; i < data.size; i++) {
        data.y_lin_reg_values[i] = ( data.x_values[i] * data.omega_value + data.b_value );
    } 

    //use the helper to calculate mean (in this function for the print only essentially)
    data.x_mean = calculate_mean(data.x_values);
    data.y_mean = calculate_mean(data.y_values);

    return data;
}



//==========================================================
//=================DOUBLE DATASET STRUCT====================
//==========================================================



struct DoubleDataset {

    //these are the inputs
    std::vector<std::vector<double>> x_values{{}, {}};
    std::vector<double> y_values{};
    double k1_value{0}; 
    double k2_value{0}; 
    double prev_b_value{0}; 
    size_t dataset_size{0}; 

    //these are the outputs after lin reg
    std::vector<double> omega_values{0,0};
    double b_value{0};
    std::vector<double> lin_reg_y_values{};
    
    //constructor which creates the two datsets
    DoubleDataset(double k1, double k2, double b, size_t num_points, int min, int max, int seed, double noise_multiplier, double sigma) {
        
        std::mt19937 rng(seed); //initialize random using our seed
        std::uniform_int_distribution<int> range(min, max); //clamp random values to a range
        std::normal_distribution<double> noise_gauss(0.0, sigma);; //clamp random values to a range

        for (size_t i{0}; i < num_points; i++) {
            double current_x1 = range(rng);
            double current_x2 = range(rng);
            x_values[0].push_back(current_x1); //cast to integer the creation of mt
            x_values[1].push_back(current_x2);
            y_values.push_back((k1*current_x1 + k2*current_x2) + b + noise_multiplier*noise_gauss(rng)); // calculate y + add noise
        }
        
        //assign the rest of the variables for later comparison
        dataset_size = num_points;
        k1_value = k1;
        k2_value = k2;
        prev_b_value = b;
    };


    //Constructor which uses two input datasets
    DoubleDataset(std::vector<double> x1, std::vector<double> x2, std::vector<double> y) {

        //assign all inner variables
        x_values[0] = x1;
        x_values[1] = x2;
        y_values = y;
        dataset_size = x1.size();
    };

    //default contructor (empty) - just in case, since default constructor isn't created when we specify a
    DoubleDataset() = default;

    //print functions again
    void print_internals() const {
        std::cout << "\ndataset size: " << dataset_size;
        std::cout << "\nk_1: " << k1_value;
        std::cout << "\nk_2: " << k2_value;
        std::cout << "\nprev_b: " << prev_b_value;
        std::cout << "\nomega_1: " << omega_values[0];
        std::cout << "\nomega_2: " << omega_values[1];
        std::cout << "\nb: " << b_value << "\n";
    };

    //prints datasets
    void print_datasets() const {
        std::cout << "\nOriginal Values:\n";
        for (size_t i{0}; i < x_values[0].size(); i++) {
            std::cout << "[ " << x_values[0][i] << ", "<< x_values[1][i] << ", "<< y_values[i] << " ]\n";
        }
        std::cout << "\n";

        // if linear regression was calculated, print too
        if (lin_reg_y_values.size() == x_values[0].size()) {
            std::cout << "\nLinear Regression Values:\n";
            for (size_t i{0}; i < x_values[0].size(); i++) {
                std::cout << "[ "<< x_values[0][i] << ", "<< x_values[1][i] << ", "<< lin_reg_y_values[i] << " ]\n";
            }
            std::cout << "\n";
        }
    }

    void print_errors() const {
        
        //calculate percentage errors
        double omega_1_error = (omega_values[0] - k1_value) / k1_value * 100;
        double omega_2_error = (omega_values[1] - k2_value) / k2_value * 100;
        double b_error = (b_value - prev_b_value) / prev_b_value * 100;
        //print percentage errors
        std::cout << "\nPercentage errors are the following:\n";
        std::cout << "\nomega_1 error: " << omega_1_error << " %";
        std::cout << "\nomega_2 error: " << omega_2_error << " %";
        std::cout << "\nb error: " << b_error << " %";
    };
};

//this function could also be expanded to accept a vector of Datasets meaning the multiple linear regression would accept a variable amount
//this assignment however only requires two meaning this is the easier, but less futureproof method

DoubleDataset multiple_linear_regression(DoubleDataset data, double learning_rate, double gradient_components_limit) {
    //initialize the derivatives, here omega has two doubles
    std::vector<double> L_domega = {1, 1}; 
    double L_db{1};

    //make sure the lin reg y values are empty, in case user calls it twice on a dataset which already has them calculated
    data.lin_reg_y_values.clear();

    //Again ensure we don't burn CPU time lol
    size_t iter = 0;
    const size_t max_iters = 100000;

    //keep going intil precision limit is met or maximum iterations reached
    while( ( std::abs(L_domega[0]) > gradient_components_limit || 
            std::abs(L_domega[1]) > gradient_components_limit || 
            std::abs(L_db) > gradient_components_limit ) 
            && iter < max_iters ) {
        
        //compute derivatives fresh
        L_domega = {0.0, 0.0}; 
        L_db = 0.0;
        
        //compute the all derivatives, outer loop iterates over the omega number (2 of them), inner computes it using the equation
        for(size_t i{0}; i < 2; i++) {
            for(size_t j{0}; j < data.dataset_size; j++) {
                L_domega[i] += ( 
                    ( data.omega_values[0] * data.x_values[0][j] + data.omega_values[1] * data.x_values[1][j]) + data.b_value // Y hat (predicted Y)
                    - data.y_values[j]) // - previous Y
                    * data.x_values[i][j]; // times current X
            } 
        } 

        //Compute b value
        for(size_t j{0}; j < data.dataset_size; j++) {
            L_db += ( data.omega_values[0] * data.x_values[0][j] + data.omega_values[1] * data.x_values[1][j]) + data.b_value // Y hat (predicted Y)
                    - data.y_values[j]; // - previous Y
        } 

        //Assign all three new variables - dataset_size is size_t, so we need to cast as double to ensure no errors
        data.omega_values[0] = data.omega_values[0] - learning_rate * (1 / double(data.dataset_size)) * L_domega[0]; 
        data.omega_values[1] = data.omega_values[1] - learning_rate * (1 / double(data.dataset_size)) * L_domega[1]; 
        data.b_value = data.b_value - learning_rate * (1 / double(data.dataset_size)) *L_db; 

        iter++;
    
    }

    //calculate the new Y based on the linear regression variables for comparison in the print functions
    for(size_t i{0}; i < data.dataset_size; i++) {
        data.lin_reg_y_values.push_back((data.omega_values[0]*data.x_values[0][i] + data.omega_values[1] * data.x_values[1][i]) + data.b_value); // calculate y + add noise
    }

    return data;
}





int main() {

    //our basic example dataset
    Dataset d(
        2,      // k 
        5,      // b 
        10,      // num_points
        0,      // min x
        100,    // max x
        3,      // seed
        0.2,    // noise_multiplier - larger meanst larger error
        1       // sigma - used in adding error, larger sigma means larger error
    );

    std::cout << "\n=========================================\n";
    std::cout << "Printing values for default dataset: \n";
    std::cout << "=========================================\n";

    d.print_datasets(); 
    d.print_internals(); 


    //normal linear regression model
    Dataset d_lin_reg = normal_equation_lin_reg(d);

    std::cout << "\n==================================================\n";
    std::cout << "Printing values for normal linear regression dataset: \n";
    std::cout << "====================================================\n";

    d_lin_reg.print_datasets();
    d_lin_reg.print_internals(); 
    d_lin_reg.print_errors(); 


    //gradient based linear regression model
    Dataset d_gradient_reg = gradient_method_linear_regression(
        d, //input dataset
        0.0001, //learning rate
        0.01 //stopping accuracy
    );

    std::cout << "\n==========================================================\n";
    std::cout << "Printing values for gradient based linear regression dataset: \n";
    std::cout << "============================================================\n";

    d_gradient_reg.print_datasets();
    d_gradient_reg.print_internals();
    d_gradient_reg.print_errors(); 



    //==========================================================
    //=================MULTIPLE LIN REG TEST====================
    //==========================================================


    std::cout << "\n=======================================================\n";
    std::cout << "Printing values for multiple linear regression dataset: \n";
    std::cout << "=========================================================\n";

    DoubleDataset dd(
        2.0,    // k1 
        -1.5,   // k2 
        4.0,    // b 
        10,   // num_points
        0,    // min x
        100,     // max x
        42,     // seed 
        0.2,    // noise_multiplier - larger means more noise
        1.0     // sigma - larger means more noise
    );

    dd = multiple_linear_regression(
        dd, //input double dataset
        0.0001, //learning rate
        0.01 //stopping accuracy
    );
    dd.print_datasets();
    dd.print_internals();
    dd.print_errors();

    //make sure the console doesn't immediatelly close :D
    char bs;
    std::cin >> bs;

    return 0;
}

  