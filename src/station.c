/*-------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include "ip.h"
/*----------------------------------------------------------------*/

char ifsFile[100];
char rouFile[100];
char hostFile[100];

void readFromHosts() {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;
	int counter = 0;
	hostcnt = 0;

	fp = fopen(hostFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 2 != 0) {
				strcpy(host[hostcnt].name, ch);
			} else {
				host[hostcnt].addr = inet_addr(ch);
			}
			ch = strtok(NULL, "\n");
			if (counter % 2 == 0)
				hostcnt++;
		}
	}

	// usage: to read sequentially from hostname and hostIP
	/*for (i = 0; i < hostcnt; i++) {
	 printf("name: %s; IP: %ld\n", host[i].name, host[i].addr);
	 }*/

	fclose(fp);
}

Host getHost(char *hostName) {
	int i;
	for (i = 0; i < hostcnt; i++) {
		if (strcmp(host[i].name, hostName) == 0)
			return host[i];
	}

	Host host;
	return host;
}

void readFromInterface() {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;
	int counter = 0;
	intr_cnt = 0;

	fp = fopen(ifsFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 5 == 1)
				strcpy(iface_list[intr_cnt].ifacename, ch);
			else if (counter % 5 == 2)
				iface_list[intr_cnt].ipaddr = inet_addr(ch);
			else if (counter % 5 == 3)
				iface_list[intr_cnt].mask = inet_addr(ch);
			else if (counter % 5 == 4)
				strcpy(iface_list[intr_cnt].macaddr, ch);
			else if (counter % 5 == 0)
				strcpy(iface_list[intr_cnt].lanname, ch);

			if (counter % 5 == 0) {
				ch = strtok(NULL, "\n");
				intr_cnt++;
			} else {
				ch = strtok(NULL, "\t");
			}
		}
	}

	// usage: to read sequentially from hostname and hostIP
	/*for (i = 0; i < intr_cnt; i++) {
	 printf("name: %s; IP: %ld; Mask: %ld; Mac: %s; Lan %s\n",
	 iface_list[i].ifacename, iface_list[i].ipaddr, iface_list[i].mask,
	 iface_list[i].macaddr, iface_list[i].lanname);
	 }*/

	fclose(fp);
}

Iface getInterface(char *hostName) {
	int i;
	for (i = 0; i < hostcnt; i++) {
		if (strcmp(iface_list[i].ifacename, hostName) == 0)
			return iface_list[i];
	}

	Iface iface;
	return iface;
}

void readFromRouting() {

	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;
	int counter = 0;
	rt_cnt = 0;

	fp = fopen(rouFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 4 == 1)
				rt_table[rt_cnt].destsubnet = inet_addr(ch);
			else if (counter % 4 == 2)
				rt_table[rt_cnt].nexthop = inet_addr(ch);
			else if (counter % 4 == 3)
				rt_table[rt_cnt].mask = inet_addr(ch);
			else if (counter % 4 == 0)
				strcpy(rt_table[rt_cnt].ifacename, ch);

			if (counter % 4 == 0) {
				ch = strtok(NULL, "\n");
				rt_cnt++;
			} else {
				ch = strtok(NULL, "\t");
			}
		}
	}

	// usage: to read sequentially from hostname and hostIP
	/*for (i = 0; i < intr_cnt; i++) {
	 printf("Dest: %ld; Next: %ld; Mask: %ld; Name %s\n",
	 rt_table[i].destsubnet, rt_table[i].nexthop, rt_table[i].mask,
	 rt_table[i].ifacename);
	 }*/

	fclose(fp);

}

Rtable getRouting(char *hostName) {
	int i;
	for (i = 0; i < hostcnt; i++) {
		if (strcmp(rt_table[i].ifacename, hostName) == 0)
			return rt_table[i];
	}

	Rtable rtable;
	return rtable;
}

/*----------------------------------------------------------------*/
/* station : gets hooked to all the lans in its ifaces file, sends/recvs pkts */
/* usage: station <-no -route> interface routingtable hostname */
int main(int argc, char *argv[]) {

	int portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	pid_t pid;
	int sts;

	char r_buffer[1024];
	char w_buffer[1024];

	/* initialization of hosts, interface, and routing tables */
	if (argc != 6) {
		printf(
				"Please provide with specified format: ./station -no interface routing_table hostname\n");
		return 0;
	}

	strcpy(ifsFile, argv[2]);
	strcpy(rouFile, argv[3]);
	strcpy(hostFile, argv[4]);

	readFromHosts();
	readFromInterface();
	readFromRouting();

	/* hook to the lans that the station should connected to
	 * note that a station may need to be connected to multilple lans
	 */

	if (strcmp(argv[1], "-no") == 0) {
		// TODO code for station
		int i;
		for (i = 0; i < intr_cnt; i++) {
			char *name = iface_list[i].lanname;

			char ip[100] = ".";
			strcat(ip, name);
			ip[strlen(ip) - 1] = '\0';
			strcat(ip, ".addr");

			char port[100] = ".";
			strcat(port, name);
			port[strlen(port) - 1] = '\0';
			strcat(port, ".port");

			char ipAddr[1024];
			size_t len;
			if ((len = readlink(ip, ipAddr, sizeof(ipAddr) - 1)) != -1)
				ipAddr[len] = '\0';

			char portNo[1024];
			if ((len = readlink(port, portNo, sizeof(portNo) - 1)) != -1)
				portNo[len] = '\0';

			portno = htons(atoi(portNo));

			// create socket
			int servSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (servSocket < 0)
				printf("ERROR opening socket");

			// server configuration
			bzero((char *) &serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			inet_aton(ipAddr, (struct in_addr *) &serv_addr.sin_addr.s_addr);
			serv_addr.sin_port = portno;

			// connect to server
			if (connect(servSocket, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0)
				printf("ERROR connecting");

			printf("%s %d\n", ipAddr, portno);

			pid = fork();

			// new process to read from server
			if (pid == 0) {
				while (1) {
					bzero(r_buffer, 1024);
					n = read(servSocket, r_buffer, 1024);
					if (n < 0) {
						printf("ERROR reading from socket");
					} else if (n == 0) {
						printf("Disconnected from bridge\n");
						kill(getpid(), SIGINT);
					}

					if (strcmp("success", r_buffer) == 0) {
						strcpy(link_socket[i].ifacename, name);
						link_socket[i].sockfd = servSocket;
					} else {
						printf(">> %s", r_buffer);
					}
				}
			} else if (pid > 0) {
				waitpid(pid, &sts, 0);
			}
		}

		// parent process will be responsible for write
		while (1) {
			bzero(w_buffer, 1024);
			fgets(w_buffer, 1023, stdin);

			if (strcmp(w_buffer, "e\n") == 0) {
				printf("Leaving the station!!!\n");
				kill(pid, SIGKILL);
				return 0;
			}

			char *to = strtok(w_buffer, " ");

			Host toHost = getHost(to);
			if (strlen(toHost.name) == 0) {
				printf("Host information unidentifiable!!!\n");
			} else {
				IPAddr toHostIP = toHost.addr;
				// TODO packet encapsulation required

				// TODO should we send the packet to both bridge that a host connects or one of them?
				/*n = write(which_bridge_to_use, w_buffer, strlen(w_buffer));
				 if (n < 0)
				 error("ERROR writing to socket");*/
			}
		}

	} else if (strcmp(argv[1], "-route") == 0) {
		// TODO code for router
	}

	return 0;
}
