#include "tcpip.h"
#include "../parser/cbosparser.h"  // Add your module that handles JSON/AMQP
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>  // Include the string header to use std::string


TCPServer::TCPServer(int port) : port_(port), running_(false), nextClientId_(0) {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        throw std::runtime_error("Failed to create server socket");
    }

    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

TCPServer::~TCPServer() {
    stop();
    close(serverSocket_);
}

void TCPServer::start() {
    if (running_) return;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
    }

    if (listen(serverSocket_, MAX_CLIENTS) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }

    running_ = true;
    serverThread_ = std::thread(&TCPServer::runServer, this);
}

void TCPServer::stop() {
    if (!running_) return;

    running_ = false;
    shutdown(serverSocket_, SHUT_RDWR);
    if (serverThread_.joinable()) serverThread_.join();

    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (auto& pair : clientThreads_) {
        shutdown(pair.first, SHUT_RDWR);
        if (pair.second.joinable()) pair.second.join();
        close(pair.first);
    }
    clientThreads_.clear();
}

void TCPServer::runServer() {
    std::cout << "Server running on port " << port_ << std::endl;

    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            if (running_) std::cerr << "Accept failed." << std::endl;
            continue;
        }

        int clientId = nextClientId_++;
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clientThreads_[clientSocket] = std::thread(&TCPServer::handleClient, this, clientSocket, clientId);
    }

    std::cout << "Server stopped." << std::endl;
}

void TCPServer::handleClient(int clientSocket, int clientId) {
    char buffer[BUFFER_SIZE];
    cbosparser handler;  // your parser & AMQP handler

    while (running_) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {
            std::cout << "Client " << clientId << " disconnected or error occurred." << std::endl;
            break;
        }

        // Send the raw data directly to the cbosparser for processing

        std::string response = handler.cbosparse(buffer);
        send(clientSocket, response.c_str(), response.size(), 0);  // Send response back  
        // Process or forward fccStatusData as required
        std::cout << "Processed data from client " << clientId << std::endl;
    }

    std::lock_guard<std::mutex> lock(clientsMutex_);
    close(clientSocket);
    clientThreads_.erase(clientSocket);
}
