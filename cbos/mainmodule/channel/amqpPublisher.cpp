#include "amqpPublisher.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace Amqp {

amqpPublisher::amqpPublisher() : conn(nullptr), socket(nullptr), queue_name(amqp_empty_bytes) {
    // Constructor: nothing special here
}

amqpPublisher::~amqpPublisher() {
    if (conn) {
        amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(conn);
        conn = nullptr;
    }

    if (queue_name.bytes) {
        amqp_bytes_free(queue_name);
        queue_name = amqp_empty_bytes;
    }
}

void amqpPublisher::createAmqpChannel(const char* hostname, int port, const char* exchangeName, const char* routingKey) {
    this->exchange = std::string(exchangeName);
    this->routingkey = std::string(routingKey);

    conn = amqp_new_connection();
    socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        throw std::runtime_error("Failed to create TCP socket");
    }

    int status = amqp_socket_open(socket, hostname, port);
    if (status) {
        throw std::runtime_error("Failed to open TCP socket");
    }

    amqp_rpc_reply_t login_reply = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
    if (login_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        throw std::runtime_error("AMQP login failed");
    }

    amqp_channel_open(conn, 1);
    amqp_rpc_reply_t channel_reply = amqp_get_rpc_reply(conn);
    if (channel_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        throw std::runtime_error("Failed to open AMQP channel");
    }

    // Declare a server-named, exclusive, auto-delete queue
    amqp_queue_declare_ok_t* r = amqp_queue_declare(conn, 1,
                                                   amqp_empty_bytes,  // empty = server-generated queue name
                                                   0,  // passive
                                                   1,  // durable
                                                   1,  // exclusive
                                                   1,  // auto_delete
                                                   amqp_empty_table);

    if (!r) {
        throw std::runtime_error("Failed to declare queue");
    }

    queue_name = amqp_bytes_malloc_dup(r->queue);
    if (queue_name.bytes == nullptr) {
        throw std::runtime_error("Failed to copy queue name");
    }

    // Bind queue to exchange with routing key
    amqp_queue_bind_ok_t* bind_ok = amqp_queue_bind(conn, 1,
                                                queue_name,
                                                amqp_cstring_bytes(exchangeName),
                                                amqp_cstring_bytes(routingKey),
                                                amqp_empty_table);
amqp_rpc_reply_t bind_reply = amqp_get_rpc_reply(conn);
if (bind_reply.reply_type != AMQP_RESPONSE_NORMAL) {
    throw std::runtime_error("Failed to bind queue");
}


    // Set QoS
    amqp_basic_qos(conn, 1, 0, 1, 0);
    amqp_rpc_reply_t qos_reply = amqp_get_rpc_reply(conn);
    if (qos_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        throw std::runtime_error("Failed to set QoS");
    }

    std::cout << "[INFO] AMQP channel created, queue bound: " << std::string((char*)queue_name.bytes, queue_name.len) << std::endl;
}

bool amqpPublisher::sendDataToFcc(const void* data, const std::string& serialized) {
    if (!conn) {
        std::cerr << "[ERROR] AMQP connection not initialized." << std::endl;
        return false;
    }

    amqp_bytes_t message_bytes;
    if (data) {
        message_bytes = amqp_bytes_malloc_dup(amqp_cstring_bytes(static_cast<const char*>(data)));
    } else {
        message_bytes = amqp_bytes_malloc_dup(amqp_cstring_bytes(serialized.c_str()));
    }

    if (!message_bytes.bytes) {
        std::cerr << "[ERROR] Failed to allocate message bytes" << std::endl;
        return false;
    }

    int ret = amqp_basic_publish(conn,
                                 1,                                    // channel id
                                 amqp_cstring_bytes(exchange.c_str()), // exchange
                                 amqp_cstring_bytes(routingkey.c_str()), // routing key
                                 0,  // mandatory
                                 0,  // immediate
                                 nullptr,  // properties
                                 message_bytes);

    amqp_bytes_free(message_bytes);

    if (ret < 0) {
        std::cerr << "[ERROR] Failed to publish message: " << amqp_error_string2(ret) << std::endl;
        return false;
    }

    return true;
}

}  // namespace Amqp
