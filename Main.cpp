#include <winsock2.h>
#include <iostream>
#include "UDPReceive.h"

int main() {
        
        WSADATA wsaData;
        auto wVersionRequested = MAKEWORD(2, 2);
        auto err = WSAStartup(wVersionRequested, &wsaData);
        
        if (err != 0) {
            printf("WSAStartup failed with error: %d\n", err);
            return 1; 
        }

        char buffer[700000];
        double time;
       
        UDPReceive receiver = UDPReceive();
        receiver.init(5000);
        receiver.receive(buffer, sizeof(buffer), &time);
       
        printf("Receiver");


        return 0;
    
}