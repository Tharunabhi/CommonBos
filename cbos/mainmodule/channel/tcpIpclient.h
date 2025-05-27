//
// Created by bctftsm on 6/3/24.
//

#ifndef HPCLFCCBOSCONNECTOR_TCPIPCLIENT_H
#define HPCLFCCBOSCONNECTOR_TCPIPCLIENT_H


#include <netinet/in.h>
#include <string>

#define PORT 4444

class tcpIpClient {
private:
    char * SERVER_IP;
    int clientSocketForSend;
    struct sockaddr_in serverAddrForSend;
public:
    void sendData(std::string requestData);
    int createServerForSend(char * ip);
    void closeServer();
};


#endif //HPCLFCCBOSCONNECTOR_TCPIPCLIENT_H
