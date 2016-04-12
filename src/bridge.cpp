/*----------------------------------------------------------------*/
#include "ip.h"
#include "head.h"
#include "util.h"
#include "ByteIO.h"

#define MAXHOSTS 32
/*----------------------------------------------------------------*/

char mLanName[100];
int mNumPorts;

typedef struct macSocket {
	MacAddr mac;
	int socket;
	int ttl;
	int port;
} MACSKT;

pthread_t timer_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int is_run = 1;
vector<MACSKT> learningTable;

void pushToLearning(MacAddr mac, int socket, int port) {
	pthread_mutex_lock(&mutex);
	vector<MACSKT>::iterator it = learningTable.begin();
	int old = 0;
	while (it != learningTable.end()) {
		MACSKT entry = *it;
		if (compareMac(entry.mac, mac) == 0) {
			//update timer
			entry.ttl = timeout;
			old = 1;
			break;
		}
		it++;
	}
	if (!old) {
		//insert
		MACSKT entry;
		memcpy(entry.mac, mac, 6);
		entry.socket = socket;
		entry.ttl = timeout;
		entry.port = port;
		learningTable.push_back(entry);
	}
	pthread_mutex_unlock(&mutex);
}

int getSocketFromLearning(MacAddr mac) {
	pthread_mutex_lock(&mutex);
	int i, res = -1;
	for (i = 0; i < (int) learningTable.size(); i++) {
		if (compareMac(learningTable[i].mac, mac) == 0) {
			res = i;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
	return res;

}

void *SL_timer_thread(void *arg) {
	while (is_run) {
		//check every second
		sleep(interval);

		pthread_mutex_lock(&mutex);
		vector<MACSKT>::iterator it = learningTable.begin();
		while (it != learningTable.end()) {
			MACSKT entry = *it;
			entry.ttl -= interval;

			//remove it
			if (entry.ttl < 1) {
				cout << "One entry in SL table timed out:" << endl;
				cout << "MAC address: ";
				for (int i = 0; i < 6; i++)
					printf("%02x", entry.mac[i]);
				cout << "\tPort: " << entry.port << " Sockfd: " << entry.socket
						<< "\tTTL: " << entry.ttl << endl;
				it = learningTable.erase(it);
				continue;
			}

			it++;
		}
		pthread_mutex_unlock(&mutex);
	}
	pthread_exit (NULL);
}

void clean() {
	is_run = 0;

	pthread_mutex_lock(&mutex);
	learningTable.clear();
	pthread_mutex_unlock(&mutex);
}

void show() {
	cout << "**********************************" << endl;
	cout << "\t\tSelf-Learning Table" << endl;
	cout << "\t\tMAC address/port mappings" << endl;
	cout << "**********************************" << endl;
	for (int i = 0; i < (int) learningTable.size(); i++) {
		MACSKT entry = learningTable[i];
		cout << "MAC address: ";
		for (int j = 0; j < 6; j++)
			printf("%02x", entry.mac[j]);
		cout << "\tPort: " << entry.port << " Sockfd: " << entry.socket
				<< "\tTTL: " << entry.ttl << endl;
	}
	cout << "**********************************" << endl << endl;
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
		printf("usage: bridge lan-name max-port\n");
		return 0;
	}

	strcpy(mLanName, argv[1]);

	mNumPorts = atoi(argv[2]);
	int client_socket[mNumPorts];
	for (i = 0; i < mNumPorts; i++) {
		client_socket[i] = 0;
	}

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

	printf("Bridge created on %s:%d\n", inet_ntoa(serv_addr.sin_addr),
			bridgePortNo);

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

	//timer thread
	pthread_create(&timer_thread, NULL, SL_timer_thread, (void*) NULL);

	// TODO Just copied the listen, read, send methods
	// TODO Need to modify the codes for project specific requirements
	// set stdin to nonblocking mode
	int in_fd = fileno(stdin);
	int flag = fcntl(in_fd, F_GETFL, 0);
	fcntl(in_fd, F_SETFL, flag | O_NONBLOCK);

	string line;
	char buf[BUFSIZ];
	while (1) {
		// clear the socket set
		FD_ZERO(&readfds);

		// add master socket to set
		FD_SET(bridgeSockfd, &readfds);
		max_sd = bridgeSockfd;
		FD_SET(in_fd, &readfds);
		max_sd = max(in_fd, max_sd);

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

		if (FD_ISSET(in_fd, &readfds)) {
			//input from user
			getline(cin, line);
			/* close the bridge */
			if (strncmp(buf, "quit", 4) == 0) {
				//close all connections
				for (i = 0; i < mNumPorts; i++) {
					if (client_socket[i] > 0)
						close(client_socket[i]);
				}
				clean();
				break;
			}
			strcpy(buf, line.c_str());
			if (strncmp(buf, "show sl", 7) == 0)
				show();
		}

		// If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(bridgeSockfd, &readfds)) {
			if ((newConnectionSockfd = accept(bridgeSockfd,
					(struct sockaddr *) &cli_addr, (socklen_t*) &clilen)) < 0) {
				perror("accept");
				exit (EXIT_FAILURE);
			}
			// new client information
			clCounter++;

			if (clCounter <= mNumPorts) {
				// send successful connection greeting message
				sprintf(welcome_msg, "accept");
				if (send(newConnectionSockfd, welcome_msg, strlen(welcome_msg),
						0) != 6) {
					perror("send");
				}

				// add new socket to array of sockets
				for (i = 0; i < mNumPorts; i++) {
					// if position is empty
					if (client_socket[i] == 0) {
						client_socket[i] = newConnectionSockfd;
						break;
					}
				}
				printf("accept a new host on sockfd %d, port %d!\n",
						newConnectionSockfd, i);
			} else {
				// send reject connection message
				sprintf(welcome_msg, "reject");
				if (send(newConnectionSockfd, welcome_msg, strlen(welcome_msg),
						0) != 6) {
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
				if ((msglen = read(sd, buffer, 1024)) <= 0) {
					// Somebody disconnected , get his details and print
					//getpeername(sd, (struct sockaddr*) &cli_addr, (socklen_t*) &clilen);
					cout << "host on port " << i << " disconnected" << endl;

					// Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;
					clCounter--;
				}

				// Forward the message that came in
				else {
					ByteIO frame((byte *) buffer, msglen);
					MacAddr srcAddr, dstAddr;
					int type = frame.ReadUInt16(); //type 0: arp, 1: ip
					int pkt_size = frame.ReadUInt16(); //ip packet size
					char *pkt = new char[pkt_size];
					frame.ReadArray(srcAddr, 6);
					frame.ReadArray(dstAddr, 6);
					frame.ReadArray(pkt, pkt_size);

					cout << "received 20 bytes Ethernet Header!" << endl;
					if (type == 1) {
						cout << "Received " << msglen << " bytes ARP frame"
								<< endl;
					} else if (type == 0) {
						cout << "Received " << msglen << " bytes IP frame"
								<< endl;
					}

					//extract IP
					char msg[BUFSIZ];
					ByteIO ipPacket((byte *) pkt, pkt_size);
					int data_len = ipPacket.ReadUInt16();
					ipPacket.ReadUInt32();  //srcIP
					ipPacket.ReadUInt32(); //dstIP
					ipPacket.ReadArray(msg, data_len);
					msg[data_len] = 0;
					//cout << srcIP << ", " << dstIP << ", " << msg << endl;
					msg[data_len] = 0;
					delete[] pkt;

					// TODO push information to the learning table
					// INFO frame will contain the mac address of its source
					// INFO lookup for interface name for that mac address
					// INFO and add it to the link_socket table

					// CALL pushToLearning(MacAddr macAddress, int socket i.e. sd here)
					pushToLearning(srcAddr, sd, i);

					// TODO forward message according to larningg table
					// INFO first look for the learning table if we already
					// INFO know which socket to forward
					// INFO if socket informaiton is not in learning table
					// INFO broadcast to all socket except the socket sd

					// CALL getSocketFromLearning(MacAddr mac)

					int toSocket = getSocketFromLearning(dstAddr);

					// TODO If find in learning table, send to specific socket
					// TODO else send it to all available socket except sd

					if (toSocket != -1) {
						cout << "will send to port " << toSocket << " only"
								<< endl;
						send(learningTable[toSocket].socket, buffer, msglen, 0);
						cout << "sockfd " << learningTable[toSocket].socket
								<< ": sent " << msglen << " bytes!" << endl;
					} else {
						cout << "send to all ports" << endl;
						int j;
						for (j = 0; j < mNumPorts; j++) {
							if (sd != client_socket[j]) {
								send(client_socket[j], buffer, msglen, 0);
								cout << "sockfd " << client_socket[j]
										<< ": sent " << msglen << " bytes!"
										<< endl;
							}
						}
					}
				}
			}
		}
	}

	close(bridgeSockfd);
	return 0;
}
/*----------------------------------------------------------------*/
