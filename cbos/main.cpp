#include <iostream>
#include <thread>
#include <chrono>
#include "mainmodule/channel/amqpPublisher.h"
#include "mainmodule/channel/tcpIp.h"

using namespace Tcp;

int main() {
    std::cout << "[INFO] Starting AMQP Publisher (Producer)..." << std::endl;

    std::thread amqpThread([] {
        try {
            Amqp::amqpPublisher amqpChannel;
            
            // Fix: use correct object name
            amqpChannel.createAmqpChannel("localhost", 5672, "BOSFCC", "BOSDATA");

            std::this_thread::sleep_for(std::chrono::seconds(1)); // slight delay to ensure setup done

            std::string message = "Hello World";
            if (amqpChannel.sendDataToFcc(nullptr, message)) {
                std::cout << "[INFO] AMQP message sent successfully." << message << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to send AMQP message." << std::endl;
            }
        } catch (const std::exception& ex) {
            std::cerr << "[ERROR] Exception in AMQP thread: " << ex.what() << std::endl;
        }
    });

    try {
        tcpIp tcpip;
        tcpip.createServer(); // your TCP server runs here
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] Exception in TCP server: " << ex.what() << std::endl;
    }

    if (amqpThread.joinable()) {
        amqpThread.join();
    }

    std::cout << "[INFO] AMQP Publisher is done!" << std::endl;
    return 0;
}
