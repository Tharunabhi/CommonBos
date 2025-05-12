#include "cbosparser.h"               
#include <iostream>                   
using json = nlohmann::json;          

// Method to parse incoming data and extract useful information from the buffer

std::string cbosparser::cbosparse(const char* buffer) {
    std::string data(buffer);         // Convert the incoming buffer (char*) to a std::string

    
    size_t start = data.find('<');    // Find the position of the first '<' character
    size_t end = data.find('>');      // Find the position of the first '>' character

    
    if (start == std::string::npos || end == std::string::npos || start >= end) {
        return generateResponse("Unknown", 0, false);  // Invalid or malformed data; return a response indicating this
    }

    // Extract the JSON string between the '<' and '>' markers
    std::string jsonString = data.substr(start + 1, end - start - 1);

    try {
        // Parse the extracted JSON string using the nlohmann::json library
        json parsed = json::parse(jsonString);

        // Extract the "Api_Name" and "Api_Version" fields from the parsed JSON
        std::string apiName = parsed["Api_Name"];       // Get the API name from the parsed JSON
        int version = parsed["Api_Version"];            // Get the API version from the parsed JSON

        // Return a valid response using the extracted data
        return generateResponse(apiName, version, true);

    } catch (const std::exception& e) {
        // If parsing the JSON fails, log the error and return an invalid response
        std::cerr << "JSON Parse error: " << e.what() << std::endl;  // Log the error message
        return generateResponse("Unknown", 0, false);  // Return an invalid response indicating parsing failure
    }
}

// Method to generate a response JSON based on API name, version, and validity

std::string cbosparser::generateResponse(const std::string& apiName, const int& version, bool isValid) {
    json response;  // Create a JSON object for the response

    // Set the response fields based on the inputs
    response["Api_Name"] = apiName;      // Set the API name in the response
    response["Api_Version"] = version;   // Set the API version in the response
    response["Is_Valid"] = isValid;      // Set the validity flag in the response
    response["Data"] = "none";           // Add a placeholder for data; "none" can be replaced with actual data if necessary

    // Convert the response JSON object to a string and return it
    return response.dump();  // Return the JSON response as a string
}
