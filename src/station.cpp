/*-------------------------------------------------------*/
#include "ip.h"
#include "head.h"
#include "util.h"
#include "ByteIO.h"

/*----------------------------------------------------------------*/

char ifsFile[100];
char rouFile[100];
char hostFile[100];

vector<ITF2LINK> iface_links;
vector<IP_PKT> ip_pkts;

int isSameNetwork(IPAddr destSubnet, IPAddr mask, IPAddr checkIP) {
	IPAddr possible = checkIP & mask;
	if (possible == destSubnet)
		return 1;

	return 0;
}

void readFromHosts() {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;
	int counter = 0;
	host_cnt = 0;

	fp = fopen(hostFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 2 != 0) {
				strcpy(host[host_cnt].name, ch);
			} else {
				host[host_cnt].addr = inet_addr(ch);
			}
			ch = strtok(NULL, "\n");
			if (counter % 2 == 0)
				host_cnt++;
		}
	}
	fclose(fp);
}

int getHost(char *hostName) {
	int i;
	for (i = 0; i < host_cnt; i++) {
		if (strcmp(host[i].name, hostName) == 0)
			return i;
	}
	return -1;
}

void readFromInterface() {
	ifstream ifs(ifsFile);
	string line;
	iface_cnt = 0;
	while (std::getline(ifs, line, '\n')) {
		if (line.length() < 1)
			continue;
		vector < string > res = split(line, '\t');
		strcpy(iface_list[iface_cnt].ifacename, res[0].c_str());
		iface_list[iface_cnt].ipaddr = inet_addr(res[1].c_str());
		iface_list[iface_cnt].mask = inet_addr(res[2].c_str());

		vector < string > splits = split(res[3], ':');
		for (int i = 0; i < (int) splits.size(); i++) {
			iface_list[iface_cnt].macaddr[i] = strtol(splits[i].c_str(), NULL,
					16);
		}
		strcpy(iface_list[iface_cnt].lanname, res[4].c_str());

		iface_cnt++;
	}
	ifs.close();
	/*
	 for (int i = 0; i < iface_cnt; i++) {
	 printf(
	 "name: %s; IP: %ld; Mask: %ld; Mac: %02x:%02x:%02x:%02x:%02x:%02x; Lan %s\n",
	 iface_list[i].ifacename, iface_list[i].ipaddr,
	 iface_list[i].mask, iface_list[i].macaddr[0],
	 iface_list[i].macaddr[1], iface_list[i].macaddr[2],
	 iface_list[i].macaddr[3], iface_list[i].macaddr[4],
	 iface_list[i].macaddr[5], iface_list[i].lanname);
	 }*/
}

int getInterface(IPAddr destIP) {
	int i;
	for (i = 0; i < iface_cnt; i++) {
		if (isSameNetwork(destIP, iface_list[i].mask, iface_list[i].ipaddr)
				== 1)
			return i;
	}
	return -1;
}

void readFromRouting() {
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	char *ch;
	int counter = 0;
	rtable_cnt = 0;

	fp = fopen(rouFile, "r");

	if (fp == NULL) {
		perror("Error while opening the host file.\n");
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 4 == 1)
				rt_table[rtable_cnt].destsubnet = inet_addr(ch);
			else if (counter % 4 == 2)
				rt_table[rtable_cnt].nexthop = inet_addr(ch);
			else if (counter % 4 == 3)
				rt_table[rtable_cnt].mask = inet_addr(ch);
			else if (counter % 4 == 0) {
				ch[strlen(ch) - 1] = 0;  //remove '\n'
				strcpy(rt_table[rtable_cnt].ifacename, ch);
			}
			if (counter % 4 == 0) {
				ch = strtok(NULL, "\n");
				rtable_cnt++;
			} else {
				ch = strtok(NULL, "\t");
			}
		}
	}
	fclose(fp);
	/*
	 for (int i = 0; i < rt_cnt; i++) {
	 printf("Dest: %ld; Next: %ld; Mask: %ld; Name %s\n",
	 rt_table[i].destsubnet, rt_table[i].nexthop, rt_table[i].mask,
	 rt_table[i].ifacename);
	 }*/
}

int getRouting(IPAddr hostIP) {
	int i;
	for (i = 0; i < rtable_cnt; i++) {
		if (isSameNetwork(rt_table[i].destsubnet, rt_table[i].mask, hostIP)
				== 1)
			return i;
	}
	return -1;
}

int connBridge(char *ip, int port) {
	int sockfd;
	int ioflags, n, error;
	fd_set rset, wset;
	socklen_t len;
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return (-1);
	ioflags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, ioflags | O_NONBLOCK);

	// server configuration
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(ip, (struct in_addr *) &serv_addr.sin_addr.s_addr);
	serv_addr.sin_port = port;

	if ((n = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
			< 0) {
		if (errno != EINPROGRESS) {
			close(sockfd);
			return (-1);
		}
	}
	if (n == 0)
		return (sockfd);

	// cannot establish the connection immediately
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;

	select(sockfd + 1, &rset, &wset, NULL, NULL);
	// should check signal interrupt
	error = 0;
	if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) {
		len = sizeof(error);
		if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			close(sockfd);
			return (-1);
		}
	}

	if (error) {
		close(sockfd);
		return (-1);
	}

	//restore properties
	fcntl(sockfd, F_SETFL, ioflags);
	return (sockfd);
}

int get_accept_msg(int sockfd) {
	int wait = 2;
	int times = 5;
	char buf[BUFSIZ];
	int n, ioflags;
	//ioflags = fcntl(sockfd, F_GETFL, 0);
	//fcntl(sockfd, F_SETFL, ioflags | O_NONBLOCK);

	n = read(sockfd, buf, BUFSIZ);
	if (n <= 0) {
		if (n == 0)
			cout << "connection closed" << endl;
		else
			perror("something wrong ");
		close(sockfd);
		return -1;
	}
	buf[n] = 0;
	if (strcmp(buf, "accept") == 0)
		return sockfd;
	else if (strcmp(buf, "reject") == 0) {
		close(sockfd);
		return -1;
	}
	return -1;
}

int getIfaceSock(char *ifacen) {
	vector<ITF2LINK>::iterator itv;
	for (itv = iface_links.begin(); itv != iface_links.end(); itv++) {
		ITF2LINK link = *itv;
		if (strcmp(link.ifacename, ifacen) == 0)
			return link.sockfd;
	}
	return -1;
}

/*
 * IP format:
 * Length[0-1]srcip[2-5]dstip[6-9]data...
 */
byte* msgToIPpkt(char *data, Host srcHost, Host dstHost) {
	short data_len = strlen(data);
	byte *packet = new byte[10 + data_len];
	ByteIO byteIO(packet, sizeof(packet));
	byteIO.WriteUInt16(data_len);
	byteIO.WriteUInt32(srcHost.addr);
	byteIO.WriteUInt32(dstHost.addr);
	byteIO.WriteArray(data, data_len);

	return packet;
}

/*
 *  frame format:
 *  type[0-1]size[2-3]srcAddr[4-9]dstAddr[10-15]IPpacket...
 */
void sendInputMsg(char *data, Rtable rtabl, MacAddr srcAddr, MacAddr dstAddr,
		Host srcHost, Host dstHost) {
	int sock = getIfaceSock(rtabl.ifacename);
	if (sock < 0) {
		cout << rtabl.ifacename << " is not connected!" << endl;
		return;
	}

	byte* pkt = msgToIPpkt(data, srcHost, dstHost);

	byte frame[BUFSIZ];
	ByteIO byteIO(frame, sizeof(frame));
	short type = 1; //IP frame
	byteIO.WriteUInt16(type);
	byteIO.WriteUInt16(sizeof(pkt));
	byteIO.WriteArray(srcAddr, 6);
	byteIO.WriteArray(dstAddr, 6);

	int sendSize = sizeof(frame) - byteIO.GetAvailable();
	int ret = send(sock, frame, sendSize, 0);
	if (ret < 0) {
		cerr << "Failed to send frame: " << sock << endl;
	}
	delete[] pkt;
}

/*
 * send <destination> <message> // send message to a destination host
 show arp             // show the ARP cache table information
 show pq              // show the pending_queue
 show host            // show the IP/name mapping table
 show iface           // show the interface information
 show rtable          // show the contents of routing table
 quit // close the station
 */
void procInputMsg(char *data) {
	data = remove_whitespace(data);
	if (strncmp(data, "send", 4) == 0) {
		char *p = data + 5;
		int i = 0;
		while (*(p + i) && !isspace(*(p + i)))
			i++;
		//host name
		char hostname[60];
		strncpy(hostname, p, i - 1);
		//real data
		p = p + i + 1;

		//get dst Host
		int pi = getHost(hostname);
		if (pi < 0) {
			cout << "no such host" << endl;
			return;
		}
		Host dstHost = host[pi];
		//get forward iface
		pi = getRouting(dstHost.addr);
		if (pi < 0) {
			cout << "no suitable rtable entry to forward" << endl;
			return;
		}
		Rtable dstRtabl = rt_table[pi];

		//get MAC address, not done, Arp

		//get src Host
		pi = getHost(dstRtabl.ifacename);
		Host srcHost = host[pi];

		MacAddr srcAddr, dstAddr;
		sendInputMsg(p, dstRtabl, srcAddr, dstAddr, srcHost, dstHost);
	} else {

	}
}

void station() {
	int i, n;
	char buf[BUFSIZ];
	fd_set r_set, all_set;
	int max_fd = -1;
	FD_ZERO(&all_set);

	for (i = 0; i < iface_cnt; i++) {
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
		int portno = htons(atoi(portNo));

		cout << iface_list[i].ifacename << " try to connecto to bridge " << name
				<< "..." << endl;
		// connect to server
		int sockfd = connBridge(ipAddr, portno);
		sockfd = get_accept_msg(sockfd);

		if (sockfd > 0) {
			cout << "connection accepted!" << endl;
			FD_SET(sockfd, &all_set);
			max_fd = max(max_fd, sockfd);

			ITF2LINK link;
			link.sockfd = sockfd;
			strcpy(link.ifacename, iface_list[i].ifacename);
			iface_links.push_back(link);
		} else
			cout << "connection rejected!" << endl;
	}

	// set stdin to nonblocking mode
	int in_fd = fileno(stdin);
	int flag = fcntl(in_fd, F_GETFL, 0);
	fcntl(in_fd, F_SETFL, flag | O_NONBLOCK);
	FD_SET(in_fd, &all_set);
	max_fd = max(in_fd, max_fd);

	string line;
	for (;;) {
		r_set = all_set;
		select(max_fd + 1, &r_set, NULL, NULL, NULL);

		if (FD_ISSET(in_fd, &r_set)) {
			//input from user
			getline(cin, line);
			strcpy(buf, line.c_str());
			procInputMsg(buf);
		}

		vector<ITF2LINK>::iterator itv = iface_links.begin();
		while (itv != iface_links.end()) {
			int tmp_fd = (*itv).sockfd;
			if (FD_ISSET(tmp_fd, &r_set)) {
				n = read(tmp_fd, buf, BUFSIZ);
				if (n <= 0) {
					//bridge exit
					close(tmp_fd);
					FD_CLR(tmp_fd, &all_set);

					cout << "iface: " << (*itv).ifacename << " disconnected!"
							<< endl;
					itv = iface_links.erase(itv);
					continue;
				} else {
					buf[n] = 0;

					//get msg

				}
			}
			itv++;
		}
	}
}

void router() {

}

/*----------------------------------------------------------------*/
/* station : gets hooked to all the lans in its ifaces file, sends/recvs pkts */
/* usage: station <-no -route> interface routingtable hostname */
int main(int argc, char *argv[]) {
	int portno, n;
	struct sockaddr_in serv_addr;

	pid_t pid;
	int sts;

	char r_buffer[1024];
	char w_buffer[1024];

	/* initialization of hosts, interface, and routing tables */
	if (argc < 5) {
		printf("Please provide with specified format: ./station -no interface routing_table hostname\n");
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
		// for station
		station();

	} else if (strcmp(argv[1], "-route") == 0) {
		// for router
		router();
	}

	return 0;
}
