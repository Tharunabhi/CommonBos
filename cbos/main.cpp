#include <iostream>      
#include <thread>        
#include <chrono>        
#include <unistd.h>      
#include "mainmodule/channel/tcpIp.h"          
#include "mainmodule/channel/amqpPublisher.h"  

using namespace Tcp;                   

int main() {
    std::cout << "[INFO] Starting AMQP Publisher (Producer)..." << std::endl;

    

    std::thread amqpthread([] {                                                 // Create a separate thread for AMQP (RabbitMQ) Publisher

        amqpPublisher amqpChannel;                                              // Create an instance of AMQP publisher

        amqpChannel.createAmqpChannel("localhost", 5672, "BOSFCC", "BOSDATA");  // Create AMQP channel to communicate with RabbitMQ server

        std::this_thread::sleep_for(std::chrono::seconds(5));                   // Sleep for 5 seconds to allow the AMQP connection to establish properly

        
        std::string message = "Hello, FCC from CBOS!";                          // Send a test message to RabbitMQ to verify the setup

        amqpChannel.sendDataToFcc(nullptr, message);                            // Send the message
    });

     
    tcpIp tcpip;                                                                // Create and start the TCP server to listen for client connections
    tcpip.createServer();

    std::cout << "[INFO] AMQP Publisher is done!" << std::endl;

    return 0; 
}
