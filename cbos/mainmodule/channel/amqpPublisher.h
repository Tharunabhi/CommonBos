#ifndef AMQP_PUBLISHER_H
#define AMQP_PUBLISHER_H

#include <string>
#include <amqp.h>
#include <amqp_tcp_socket.h>

namespace Amqp {

class amqpPublisher {
public:
    amqpPublisher();
    ~amqpPublisher();

    void createAmqpChannel(const char* hostname, int port, const char* exchangeName, const char* routingKey);
    bool sendDataToFcc(const void* data, const std::string& serialized);

private:
    amqp_connection_state_t conn = nullptr;
    amqp_socket_t* socket = nullptr;

    std::string exchange;
    std::string routingkey;

    amqp_bytes_t queue_name = amqp_empty_bytes;  // Holds the generated queue name
};

}  // namespace Amqp

#endif // AMQP_PUBLISHER_H
