#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


 // Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

extern "C" {
#include <stdlib.h>
#include <sys/types.h>
}



class UDPReceive {
	
public:
	int sock;
	struct sockaddr_in addr;
	char *recbuffer;
	bool leftover;
	
	UDPReceive();
	~UDPReceive(){ delete recbuffer; };
	void init( int port );
	int receive( char *buffer, int len, double *ptime  );
	int receive( char *buffer, int len, const char *tag, double *ptime  );
	void closeSock();
};