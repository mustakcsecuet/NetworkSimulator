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
#include "ether.h"

#define MAXHOSTS 32
/*----------------------------------------------------------------*/

char mLanName[100];
int mNumPorts;

int learningCounter;

typedef struct macSocket {
	MacAddr mac;
	int socket;
} MACSKT;

MACSKT learningTable[MAXHOSTS];

void pushToLearning(MacAddr mac, int socket) {
	int i;
	for (i = 0; i < learningCounter; i++) {
		if (strcmp(learningTable[i].mac, mac) == 0)
			return;
	}

	strcpy(learningTable[learningCounter].mac, mac);
	learningTable[learningCounter].socket = socket;
	learningCounter++;
}

int getSocketFromLearning(char *mac) {
	int i;
	for (i = 0; i < learningCounter; i++) {
		if (strcmp(learningTable[i].mac, mac) == 0)
			return i;
	}
	return -1;
}

/* bridge : recvs pkts and relays them */
/* usage: bridge lan-name max-port */
int main(int argc, char *argv[]) {

	int bridgeSockfd, newConnectionSockfd;

	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;

	char bridgeIP[100];
	int bridgePortNo;

	fd_set readfds;
	int max_sd, sd, clCounter;

	int activity, msglen;
	int i;
	char welcome_msg[200];
	char buffer[1024];

	if (argc != 4) {
		printf(
				"Please provide with specified format: ./bridge lan_name num_ports\n");
		return 0;
	}

	strcpy(mLanName, argv[1]);

	mNumPorts = atoi(argv[2]);
	int client_socket[mNumPorts];
	for (i = 0; i < mNumPorts; i++) {
		client_socket[i] = 0;
	}

	/*printf("Test: lan name -> %s\n", mLanName);
	 printf("Test: num ports -> %d\n", mNumPorts);*/

	bridgeSockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (bridgeSockfd < 0)
		printf("ERROR opening sockechart!");

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
	sprintf(temp, "%d", bridgePortNo);
	symlink(temp, linkPortName);

	clilen = sizeof(cli_addr);
	clCounter = 0;

	// TODO Just copied the listen, read, send methods
	// TODO Need to modify the codes for project specific requirements
	while (1) {
		// clear the socket set
		FD_ZERO(&readfds);

		// add master socket to set
		FD_SET(bridgeSockfd, &readfds);
		max_sd = bridgeSockfd;

		// add child sockets to set
		for (i = 0; i < mNumPorts; i++) {
			// socket descriptor
			sd = client_socket[i];

			// if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			// highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		// wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("select error");
		}

		// If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(bridgeSockfd, &readfds)) {
			if ((newConnectionSockfd = accept(bridgeSockfd,
					(struct sockaddr *) &cli_addr, (socklen_t*) &clilen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			// new client information
			printf("bridge: connect from at \'%d\'\n",
					ntohs(cli_addr.sin_port));
			clCounter++;

			if (clCounter <= mNumPorts) {
				// send successful connection greeting message
				sprintf(welcome_msg, "success");
				if (send(newConnectionSockfd, welcome_msg, strlen(welcome_msg),
						0) != strlen(welcome_msg)) {
					printf("send");
				}

				// add new socket to array of sockets
				for (i = 0; i < mNumPorts; i++) {
					// if position is empty
					if (client_socket[i] == 0) {
						client_socket[i] = newConnectionSockfd;
						break;
					}
				}
			} else {
				// send reject connection message
				sprintf(welcome_msg, "reject");
				if (send(newConnectionSockfd, welcome_msg, strlen(welcome_msg),
						0) != strlen(welcome_msg)) {
					perror("send");
				}
				close(newConnectionSockfd);
			}
		}

		memset(buffer, '\0', 1024);

		// else its some IO operation on some other socket :)
		for (i = 0; i < mNumPorts; i++) {
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds)) {
				// Check if it was for closing , and also read the incoming message
				if ((msglen = read(sd, buffer, 1024)) == 0) {
					// Somebody disconnected , get his details and print
					getpeername(sd, (struct sockaddr*) &cli_addr,
							(socklen_t*) &clilen);
					printf("bridge: disconnect at %d\'\n",
							ntohs(cli_addr.sin_port));

					// Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;
					clCounter--;
				}

				// Forward the message that came in
				else {
					//set the string terminating NULL byte on the end of the data read
					printf("Message Received\n");
					buffer[msglen] = '\0';
					printf("%s\n", buffer);

					// TODO push information to the learning table
					// INFO frame will contain the mac address of its source
					// INFO lookup for interface name for that mac address
					// INFO and add it to the link_socket table

					// CALL pushToLearning(MacAddr macAddress, int socket i.e. sd here)

					// TODO forward message according to larningg table
					// INFO first look for the learning table if we already
					// INFO know which socket to forward
					// INFO if socket informaiton is not in learning table
					// INFO broadcast to all socket except the socket sd

					// CALL getSocketFromLearning(MacAddr mac)
					/*
					 * int toSocket = getSocketFromLearning(destMac);
					 */
					// TODO If find in learning table, send to specific socket
					// TODO else send it to all available socket except sd
					/*
					 * if(toSocket != -1){
					 * 	send(learningTable[toSocket].socket, buffer, strlen(buffer), 0);
					 * } else {
					 *  int j;
					 * 	for(j=0;j<mNumPorts;j++){
					 * 	    if(sd != client_socket[j])
					 * 		  send(client_socket[j], buffer, strlen(buffer), 0);
					 * 	}
					 * }
					 */

					//send(where_to_send, buffer, strlen(buffer), 0);
				}
			}
		}
	}

	/* listen to the socket.
	 * two cases:
	 * 1. connection open/close request from stations/routers
	 * 2. regular data packets
	 */

	close(bridgeSockfd);
	return 0;
}
/*----------------------------------------------------------------*/
