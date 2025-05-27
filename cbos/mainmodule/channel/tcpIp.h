#ifndef FCCADDITIONAL_TCPIP_H
#define FCCADDITIONAL_TCPIP_H

#include <netinet/in.h>
#include <string>
#include <vector>

#define PORT 4444

using namespace std;
namespace Tcp{
    class tcpIp{
    private:


    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size = sizeof(client_address);
    fd_set readfds;
    vector<int> client_sockets;

    public:

    tcpIp();

    virtual ~tcpIp();

        void createServer();

        void serverInitialize();

    };
}

#endif 