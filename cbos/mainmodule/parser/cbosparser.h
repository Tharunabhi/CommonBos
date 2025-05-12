#ifndef CBOSPARSER_H
#define CBOSPARSER_H

#include <string>
#include "json.h"

class cbosparser {
public:
    std::string cbosparse(const char* buffer);

private:
    std::string generateResponse(const std::string& apiName, const int& version, bool isValid);
};

#endif // CBOSPARSER_H
