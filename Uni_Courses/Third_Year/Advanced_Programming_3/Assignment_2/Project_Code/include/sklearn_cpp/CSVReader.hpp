#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <stdexcept> //runtime errors

#include "DatasetClass.hpp"

namespace sklearn_cpp {
namespace CSVReader {

    /*
    ==This is an imported function== which we adapted for our Dataset class

    This is only a helper function and not a class so it has to be inline to work properly
    Can be called using the following line:

    sklearn_cpp::CSVReader::read_CSV("path", true/false);

    Parses a CSV file based on the filepath 
    Added a has_header to ignore the first line 
    WORKS ON COMMA SEPARATED CSV!

    */
    inline Dataset read_CSV(const std::string& filename,
                                            bool has_header = true) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        Dataset data;
        std::string line;

        //added functionality to exctract headers
        std::vector<std::string> headers;

        if (has_header && std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss, cell, ',')) {
                headers.push_back(cell);
            }
        }
        data.headers = headers;

        /*
        For every line in the CSV:
        1. Skip if empty or invalid
        2. Extract everything but the last column as the X
        3. Extract the last column as the Y
        4. Into data.X push back the vector of columns except the last one, data.Y is the last column
        */
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            std::stringstream ss(line);
            std::string cell;
            std::vector<double> row_values;

            while (std::getline(ss, cell, ',')) {
                row_values.push_back(std::stod(cell));
            }

            if (row_values.size() < 2) {
                throw std::runtime_error("Each row must contain at least 1 feature and 1 target.");
            }

            // Extract everything but the last column as the X
            std::vector<double> features(row_values.begin(), row_values.end() - 1);

            //The last column is the Y
            double target = row_values.back();
            
            //Into X push back the vector of columns except the last one, Y is the last column
            data.X.push_back(features);
            data.y.push_back(target);
        }

        //returns Dataset object
        return data;
    }

} //CSVReader
} //sklearn_cpp