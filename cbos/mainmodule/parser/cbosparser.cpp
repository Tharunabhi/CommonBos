#include "cbosparser.h"
#include <iostream>
using json = nlohmann::json;

std::string cbosparser::cbosparse(const char* buffer) {
    std::string data(buffer);

    // Find custom STX '<' and ETX '>'
    size_t start = data.find('<');
    size_t end = data.find('>');

    if (start == std::string::npos || end == std::string::npos || start >= end) {
        return generateResponse("Unknown", 0, false);
    }

    std::string jsonString = data.substr(start + 1, end - start - 1);

    try {
        json parsed = json::parse(jsonString);

        std::string apiName = parsed["Api_Name"];
        int version = parsed["Api_Version"];  

        return generateResponse(apiName, version, true);

    } catch (const std::exception& e) {
        std::cerr << "JSON Parse error: " << e.what() << std::endl;
        return generateResponse("Unknown", 0, false);
    }
}

std::string cbosparser::generateResponse(const std::string& apiName, const int& version, bool isValid) {
    json response;
    response["Api_Name"] = apiName;
    response["Api_Version"] = version;  
    response["Is_Valid"] = isValid;
    response["Data"] = "none";
    return response.dump();  
}
