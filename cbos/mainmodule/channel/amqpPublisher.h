#ifndef AMQPPUBLISHER_H
#define AMQPPUBLISHER_H

#include <string>
#include <amqp.h>

class amqpPublisher {
public:
    void createAmqpChannel(const char* hostname, int port, const char* exchange, const char* routing_key);
    void sendDataToFcc (const void* pSender, std::string str);

private:
    amqp_connection_state_t conn;
    std::string exchange;
    std::string routingkey;
};

#endif
