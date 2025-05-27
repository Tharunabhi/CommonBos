#include "tcpIp.h"  
#include "../parser/cbosparser.h"
#include <cstdio>   
#include <cstdlib>  
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <unistd.h>     
#include <cstring>      
#include <iostream>    
#include <thread>       

using namespace std;
#define MAX_CLIENTS 50  // Maximum number of simultaneous client connections

namespace Tcp {

    // Constructor for the tcpIp class — default and does nothing here
    tcpIp::tcpIp() = default;

    // Destructor for the tcpIp class — default and does nothing here
    tcpIp::~tcpIp() = default;

    // Function to handle communication with a connected client
    
    void handle_client(int client_socket) {
        char buffer[100000];
        int bytes_received;
    
        cbosparser cbosparser;  // Create object
    
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            buffer[bytes_received] = '\0';
    
            // Send buffer to parser
            std::string fccData = cbosparser.cbosparse(buffer);
    
            std::cout << "Returned from parser: " << fccData << std::endl;
    
            //  Send response back to the client
            ssize_t bytes_sent = send(client_socket, fccData.c_str(), fccData.length(), 0);
    
            if (bytes_sent == -1) {
                std::cerr << "Failed to send response.\n";
            } else {
                std::cout << "Sent response \n";
            }
        }
    
        //close(client_socket);
    }
    // Function to initialize the server socket
    void tcpIp::serverInitialize() {
        // Create a TCP socket (IPv4)
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        int optval = 1;

        // Set socket option to reuse address (avoids "address already in use" error)
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            std::cerr << "Error setting socket option\n";
            close(server_socket);
        }

        // Check if socket creation failed
        if (server_socket == -1) {
            std::cerr << "Error creating socket" << std::endl;
        }

        // Configure the server address structure
        server_address.sin_family = AF_INET;            // IPv4
        server_address.sin_addr.s_addr = INADDR_ANY;    // Accept connections from any IP address
        server_address.sin_port = htons(4444);          // Port number 4444 (host to network short)

        // Bind the socket to the specified IP and port
        if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
            std::cerr << "Error binding socket" << std::endl;
        }

        // Start listening for incoming client connections
        if (listen(server_socket, MAX_CLIENTS) == -1) {
            std::cerr << "Error listening for connections" << std::endl;
        }

        std::cout << "[*] Listening on port 4444..." << std::endl;  // Server is ready
    }

    // Function to create the server and handle client connections

    void tcpIp::createServer() {
        serverInitialize();  // Initialize server socket

        vector<std::thread> client_threads;  // Container to keep track of client threads

        // Infinite loop to keep accepting new client connections
        
        while (true) {
            
            // Accept a new client connection
            
            client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_address_size);

            // Check for errors in accepting the client
            
            if (client_socket == -1) {
                std::cerr << "Error accepting connection" << std::endl;

                // Close current server socket and try to reinitialize
                
                close(server_socket);
                serverInitialize();
            }

            // Display client's IP address and port
            
            std::cout << "[*] Accepted connection from " 
                      << inet_ntoa(client_address.sin_addr) << ":"
                      << ntohs(client_address.sin_port) << std::endl;

            // Create a new thread to handle the client connection

            client_threads.emplace_back(handle_client, client_socket);

            // Detach the thread to allow it to run independently

            client_threads.back().detach();
        }

        // Close the server socket (this will never be reached due to infinite loop)
        close(server_socket);
    }
}