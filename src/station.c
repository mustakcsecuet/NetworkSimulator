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
	/*for (int i = 0; i < hostcnt; i++) {
	 printf("name: %s; IP: %ld\n", host[i].name, host[i].addr);
	 }*/

	fclose(fp);
}

Host getHost(char *hostName) {
	for (int i = 0; i < hostcnt; i++) {
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
	/*for (int i = 0; i < intr_cnt; i++) {
	 printf("name: %s; IP: %ld; Mask: %ld; Mac: %s; Lan %s\n",
	 iface_list[i].ifacename, iface_list[i].ipaddr, iface_list[i].mask,
	 iface_list[i].macaddr, iface_list[i].lanname);
	 }*/

	fclose(fp);
}

Iface getInterface(char *hostName) {
	for (int i = 0; i < hostcnt; i++) {
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
	/*for (int i = 0; i < intr_cnt; i++) {
	 printf("Dest: %ld; Next: %ld; Mask: %ld; Name %s\n",
	 rt_table[i].destsubnet, rt_table[i].nexthop, rt_table[i].mask,
	 rt_table[i].ifacename);
	 }*/

	fclose(fp);

}

Rtable getRouting(char *hostName) {
	for (int i = 0; i < hostcnt; i++) {
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

