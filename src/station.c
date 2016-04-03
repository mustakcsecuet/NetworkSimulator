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
/*----------------------------------------------------------------*/

char ifsFile[100];
char rouFile[100];
char hostFile[100];

char hostname[100][50];
char hostIP[100][50];
int hostCounter = 0;

void readFromHosts() {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;

	fp = fopen(hostFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			hostCounter++;
			if (hostCounter % 2 != 0)
				strcpy(hostname[((hostCounter - 1) / 2)], ch);
			else
				strcpy(hostIP[((hostCounter - 2) / 2)], ch);
			ch = strtok(NULL, "\n");
		}
	}

	// usage: to read sequentially from hostname and hostIP
	/*for(int i=0;i<hostCounter;i+=2){
	 printf("name: %s; IP: %s\n", hostname[i/2], hostIP[i/2]);
	 }*/

	fclose(fp);
}

char* getHostIP(char *host) {
	for (int i = 0; i < hostCounter; i += 2) {
		if (strcmp(hostname[i / 2], host) == 0)
			return hostIP[i / 2];
	}
	return NULL;
}

void readFromInterface() {

}

void readFromRouting() {

}

/*----------------------------------------------------------------*/
/* station : gets hooked to all the lans in its ifaces file, sends/recvs pkts */
/* usage: station <-no -route> interface routingtable hostname */
int main(int argc, char *argv[]) {

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

	/* hook to the lans that the station should connected to
	 * note that a station may need to be connected to multilple lans
	 */

	/* monitoring input from users and bridges
	 * 1. from user: analyze the user input and send to the destination if necessary
	 * 2. from bridge: check if it is for the station. Note two types of data
	 * in the ethernet frame: ARP packet and IP packet.
	 *
	 * for a router, it may need to forward the IP packet
	 */

	while (1) {

	}

	return 0;
}

