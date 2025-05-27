#ifndef CBOSPARSER_H
#define CBOSPARSER_H

#include <string>
#include "../channel/amqpPublisher.h"

class cbosparser {
public:
    static std::string cbosparse(const char* buffer);
    static std::string generateResponse(const std::string& apiName, int version, bool isValid);
    static Amqp::amqpPublisher publisher;

private:
    
    static bool amqp_initialized;
};

#endif // CBOSPARSER_H
