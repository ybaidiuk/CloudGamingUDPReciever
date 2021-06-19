#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

extern "C" {
#include <stdlib.h>
#include <sys/types.h>
}

typedef struct RTHeader {
	double		  time;
	unsigned long packetnum;
	unsigned char fragments;
	unsigned char fragnum;
} RTHeader_t;


class UDPReceive {

public:
	int sock;
	struct sockaddr_in addr;
	char* recbuffer;
	bool leftover;

	UDPReceive();
	~UDPReceive() { delete recbuffer; };
	void init(int port);
	int receive(char* buffer, double* ptime);
	int receive(char* buffer, const char* tag, double* ptime);
	void closeSock();


};