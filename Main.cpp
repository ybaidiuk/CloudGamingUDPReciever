#include <winsock2.h>
#include <iostream>
#include "UDPReceive.h"

int main() {
        
        WSADATA wsaData;
        auto wVersionRequested = MAKEWORD(2, 2);
        auto err = WSAStartup(wVersionRequested, &wsaData);

        char buffer[500000];
        double time;
       
        UDPReceive receiver = UDPReceive();
        receiver.init(23042);
        receiver.receive(buffer, sizeof(buffer), &time);
       
        printf("Receiver");

        if (err != 0){
            printf("WSAStartup failed with error: %d\n", err);
        }
        return 0;
    
}