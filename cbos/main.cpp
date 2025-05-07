
#include "mainmodule/channel/tcpip.h"
#include "mainmodule/parser/cbosparser.h"
#include <iostream>


int main() {
    const int PORT = 4444;

    TCPServer server(PORT);

    try {
        server.start();
        std::cout << "Press Enter to stop the server..." << std::endl;
        std::cin.get();
        server.stop();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
