//
// Created by bctftsm on 6/3/24.
//

#include "tcpIpclient.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>


int tcpIpClient::createServerForSend(char * ip) {

    SERVER_IP = ip;
    //     Create socket
    clientSocketForSend = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketForSend == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    // Set up server address structure
    serverAddrForSend.sin_family = AF_INET;
    serverAddrForSend.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &(serverAddrForSend.sin_addr));

    // Connect to server
    if (connect(clientSocketForSend, (struct sockaddr*)&serverAddrForSend, sizeof(serverAddrForSend)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        close(clientSocketForSend);
        return 1;
    }

    return 0;

}


void tcpIpClient::sendData(std::string requestData) {

    send(clientSocketForSend, requestData.c_str(), strlen(requestData.c_str()), 0);
    //std::cout << "Message sent to the server: " << requestData << std::endl;
}

void tcpIpClient::closeServer(){
    close(clientSocketForSend);
}