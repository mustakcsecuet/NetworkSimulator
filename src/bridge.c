/*----------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
/*----------------------------------------------------------------*/

char mLanName[100];
int mNumPorts;

/* bridge : recvs pkts and relays them */
/* usage: bridge lan-name max-port */
int main(int argc, char *argv[]) {

	int bridgeSockfd, newConnectionSockfd;

	struct sockaddr_in serv_addr, cli_addr;
	struct hostent *name;

	char bridgeIP[100];
	int bridgePortNo;

	if (argc < 3) {
		printf(
				"Please provide with specified format: ./bridge lan_name num_ports");
	}

	strcpy(mLanName, argv[1]);

	mNumPorts = atoi(argv[2]);

	/*printf("Test: lan name -> %s\n", mLanName);
	 printf("Test: num ports -> %d\n", mNumPorts);*/

	bridgeSockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bridgeSockfd < 0)
		printf("ERROR opening socket!");

	bzero((char *) &serv_addr, sizeof(serv_addr));

	// configure server information
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serv_addr.sin_port = ntohs(0);

	// bind socket with server information
	if (bind(bridgeSockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			< 0)
		printf("ERROR on binding");

	// turn to passive
	listen(bridgeSockfd, 5);

	inet_ntop(AF_INET, &(serv_addr.sin_addr), bridgeIP, sizeof(serv_addr));

	socklen_t serv_len = sizeof(serv_addr);
	getsockname(bridgeSockfd, (struct sockaddr *) &serv_addr, &serv_len);
	bridgePortNo = ntohs(serv_addr.sin_port);

	/* create the symbolic links to its address and port number
	 * so that others (stations/routers) can connect to it
	 */

	char linkIPName[100] = ".";
	strcat(linkIPName, mLanName);
	strcat(linkIPName, ".addr");
	symlink(bridgeIP, linkIPName);

	char linkPortName[100] = ".";
	strcat(linkPortName, mLanName);
	strcat(linkPortName, ".port");
	char temp[100];
	sprintf(temp,"%d",bridgePortNo);
	symlink(temp, linkPortName);

	while (1) {

	}

	/* listen to the socket.
	 * two cases:
	 * 1. connection open/close request from stations/routers
	 * 2. regular data packets
	 */
}
/*----------------------------------------------------------------*/
