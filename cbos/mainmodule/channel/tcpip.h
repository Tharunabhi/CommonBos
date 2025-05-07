
#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 25

#include <thread>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <string>

class TCPServer {
public:
    explicit TCPServer(int port);
    ~TCPServer();

    void start();
    void stop();

    void setDataCallback(std::function<std::string(const std::string&)> callback);

private:
    void runServer();
    void handleClient(int clientSocket, int clientId);

    int port_;
    int serverSocket_;
    bool running_;
    std::thread serverThread_;
    std::mutex clientsMutex_;
    int nextClientId_;

    std::unordered_map<int, std::thread> clientThreads_;
    std::function<std::string(const std::string&)> dataCallback_;
};

#endif // TCPSERVER_HPP
