#include <iostream>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include "amqpPublisher.h"  // Assuming the class is declared here

void amqpPublisher::createAmqpChannel(const char *hostname, int port, const char *exchange, const char *routing_key) {
    this->exchange = exchange;
    this->routingkey = routing_key;

    // Create a new connection
    conn = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(conn);

    if (!socket) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        return;
    }

    // Open TCP socket connection to RabbitMQ server
    int status = amqp_socket_open(socket, hostname, port);
    if (status < 0) {
        std::cerr << "Failed to open TCP socket to RabbitMQ" << std::endl;
        return;
    } else {
        std::cout << "TCP Socket Created and Connected\n";
    }

    // Login to the RabbitMQ server
    amqp_rpc_reply_t login_reply = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    if (login_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Login failed" << std::endl;
        return;
    }

    // Open the AMQP channel
    amqp_channel_open(conn, 1);
    amqp_rpc_reply_t channel_reply = amqp_get_rpc_reply(conn);

    if (channel_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Failed to open AMQP channel" << std::endl;
        return;
    } else {
        std::cout << "AMQP channel opened successfully" << std::endl;
    }
}

void amqpPublisher::sendDataToFcc(const void* pSender, std::string str) {
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");

    amqp_bytes_t result;
    result.bytes = (void *)str.c_str();
    result.len = str.length();

    // Send message to RabbitMQ exchange
    amqp_basic_publish(conn,
                       1,
                       amqp_cstring_bytes(exchange.c_str()),
                       amqp_cstring_bytes(routingkey.c_str()),
                       0,
                       0,
                       &props,
                       result);

    std::cout << "[INFO] Message Sent to RabbitMQ: " << str << std::endl;
}