#include <vector>
#include <random>
#include <iostream>
#include <cmath>

//allows to ommit the ever so annoying std::, no other libraries used here so no conflict due to the namespace, just saves time
using namespace std; 

//Little helper to calculate mean

template<typename T> //We can actualy use the templates here to make sure our helper works for int and double, etc.

double calculate_mean(vector<T> &vect) {
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
    double b_value; //intercept
    size_t size;

    vector<double> x_values; 
    vector<double> y_values;

    //futureproofing - to use in lin reg calculations - 
    double omega_value{0};
    double x_mean{0}; 
    double y_mean{0};
    vector<double> y_lin_reg_values; //this way the dataset contains both the original data and lin_reg data

    //Simple contrustor so we can easily create new datasets
    Dataset(double k , double b, size_t num_points, int min, int max, int seed, double noise_multiplier, double sigma) {

        k_value = k;
        b_value = b;
        
        mt19937 rng(seed); //initialize random using our seed
        std::uniform_int_distribution<int> range(min, max); //clamp random values to a range
        normal_distribution<double> noise_gauss(0.0, sigma);; //clamp random values to a range

        for (size_t i{0}; i < num_points; i++) {
            double current_x = range(rng);
            x_values.push_back(current_x); //cast to integer the creation of mt
            y_values.push_back(k*current_x + b + noise_multiplier*noise_gauss(rng)); // calculate y + add noise
        }
        
        size = num_points;

    };

    //default contructor (empty)
    Dataset() {
        k_value = 0;
        b_value = 0;
        size = 0;
        x_values = {};
        y_values = {};
        omega_value = 0;
        x_mean = 0;
        y_mean = 0;

    };

    //Basic print function to confirm the dataset + linear regression data (only if it exists) - const as a safeguard only in case print somehow manipulates the data (that would be a very bad print)
    void print_datasets() const {
        cout << "\nOriginal Values:\n";
        for (size_t i{0}; i < x_values.size(); i++) {
            cout << "[ " << x_values[i] << ", " << y_values[i] << " ]\n";
        }
        cout << "\n";

        //if linear regresison was calculated, print too
        if(y_lin_reg_values.size() == x_values.size()) { 
            cout << "\nLinear Regression Values:\n";
            for (size_t i{0}; i < x_values.size(); i++) {
                cout << "[ " << x_values[i] << ", " << y_lin_reg_values[i] << " ]\n";
            }
            cout << "\n";
        }
    };

    //Basic print function to confirm internal values - once again const to be safe
    void print_internals() const {
        cout << "k: " << k_value;
        cout << "\nb: " << b_value;
        cout << "\nomega: " << omega_value;
        cout << "\nx_mean: " << x_mean;
        cout << "\ny_mean: " << y_mean << "\n";
    };
};

//==========================================================
//=================DOUBLE DATASET STRUCT====================
//==========================================================

struct DoubleDataset {
    vector<vector<double>> x_values;
    vector<double> y_values;
    vector<double> weights;
    double b_value;
};

//this function could also be expanded to accept a vector of Datasets meaning the multiple linear regression would accept a variable amount
//this assignment however only requires two meaning this is the easier, but less futureproof method

DoubleDataset multiple_linear_regression(DoubleDataset data) {

}

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

    //keep iterating until a sufficiently small derivative is found (absolute value because negative derivative also possible)

    //make sure we don't burn CPU time lol
    size_t iter = 0;
    const size_t max_iters = 100000; 
    
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


int main() {

    //our basic example dataset
    Dataset d(2, 5, 7, 0, 100, 3, 0.2, 1);

    cout << "\n=========================================\n";
    cout << "Printing values for default dataset: \n";
    cout << "=========================================\n";

    d.print_datasets(); 
    d.print_internals(); 


    //normal linear regression model
    Dataset d_lin_reg = normal_equation_lin_reg(d);

    cout << "\n==================================================\n";
    cout << "Printing values for normal linear regression dataset: \n";
    cout << "====================================================\n";

    d_lin_reg.print_datasets();
    d_lin_reg.print_internals(); 


    //gradient based linear regression model
    Dataset d_gradient_reg = gradient_method_linear_regression(d, 0.0001, 0.01);

    cout << "\n==========================================================\n";
    cout << "Printing values for gradient based linear regression dataset: \n";
    cout << "============================================================\n";

    d_gradient_reg.print_datasets();
    d_gradient_reg.print_internals();

    //make sure the console doesn't immediatelly close :D
    char bs;
    cin >> bs;

    return 0;
}

  