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
vector<PENDING_QUEUE> pkt_que;

int isMyIP(IPAddr ip) {
	int i;
	for (i = 0; i < intr_cnt; i++) {
		if (iface_list[i].ipaddr == ip)
			return 1;
	}
	return 0;
}

int isMyMac(MacAddr mac) {
	int i;
	for (i = 0; i < intr_cnt; i++) {
		if (compareMac(iface_list[i].macaddr, mac) == 0)
			return 1;
	}
	return 0;
}

int getPendingPacket(IPAddr checkIP) {
	int i;
	for (i = 0; i < (int) iface_links.size(); i++) {
		if (pkt_que[i].next_hop_ipaddr == checkIP)
			return i;
	}

	return -1;
}

int doWeKnowMac(IPAddr destIP) {
	int i;
	for (i = 0; i < arp_cache_counter; i++) {
		if (arpCacheList[i].ipaddr == destIP)
			return i;
	}
	return -1;
}

void storeInArpCache(IPAddr ip, MacAddr mac) {
	int i;
	for (i = 0; i < arp_cache_counter; i++) {
		if (arpCacheList[i].ipaddr == ip)
			return;
	}

	printf("Store in ARP Cache:\n");
	printIP((char*) "IP", ip);
	printMac((char*) "Mac", mac);

	arpCacheList[arp_cache_counter].ipaddr = ip;
	memcpy(arpCacheList[arp_cache_counter].macaddr, mac, 6);
	arp_cache_counter++;
}

int isSameNetwork(IPAddr destSubnet, IPAddr mask, IPAddr checkIP) {
	IPAddr possible = checkIP & mask;
	if (possible == destSubnet)
		return 1;

	return 0;
}

int getSocket(char *lanName) {
	int i;
	for (i = 0; i < (int) iface_links.size(); i++) {
		if (strcmp(iface_links[i].ifacename, lanName) == 0)
			return i;
	}

	return -1;
}

void readFromHosts() {
	cout << "read hosts..\n\n";
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
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		cout << line;
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
	fclose(fp);
	cout << endl;
}

int getHost(char *hostName) {
	int i;
	for (i = 0; i < hostcnt; i++) {
		if (strcmp(host[i].name, hostName) == 0)
			return i;
	}
	return -1;
}

int getHostByIP(IPAddr ip) {
	int i;
	for (i = 0; i < hostcnt; i++) {
		if (host[i].addr == ip)
			return i;
	}
	return -1;
}

void readFromInterface() {
	cout << "reading ifaces..\n\n";
	ifstream ifs(ifsFile);
	string line;
	intr_cnt = 0;
	while (std::getline(ifs, line, '\n')) {
		if (line.length() < 1)
			continue;
		cout << line << endl;
		vector < string > res = split(line, '\t');
		strcpy(iface_list[intr_cnt].ifacename, res[0].c_str());
		iface_list[intr_cnt].ipaddr = inet_addr(res[1].c_str());
		iface_list[intr_cnt].mask = inet_addr(res[2].c_str());

		vector < string > splits = split(res[3], ':');
		for (int i = 0; i < (int) splits.size(); i++) {
			iface_list[intr_cnt].macaddr[i] = strtol(splits[i].c_str(), NULL,
					16);
		}
		strcpy(iface_list[intr_cnt].lanname, res[4].c_str());

		intr_cnt++;
	}
	ifs.close();
	cout << endl;
}

int getIfaceByName(char *ifname) {
	int i;
	for (i = 0; i < intr_cnt; i++) {
		if (strcmp(iface_list[i].ifacename, ifname) == 0)
			return i;
	}
	return -1;
}

int getInterface(IPAddr destIP) {
	int i;
	for (i = 0; i < intr_cnt; i++) {
		if (isSameNetwork(destIP, iface_list[i].mask, iface_list[i].ipaddr)
				== 1)
			return i;
	}
	return -1;
}

void readFromRouting() {
	cout << "reading rtables..\n\n";
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
		exit (EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, fp)) != -1) {
		cout << line;
		ch = strtok(line, "\t");
		while (ch != NULL) {
			counter++;
			if (counter % 4 == 1)
				rt_table[rt_cnt].destsubnet = inet_addr(ch);
			else if (counter % 4 == 2)
				rt_table[rt_cnt].nexthop = inet_addr(ch);
			else if (counter % 4 == 3)
				rt_table[rt_cnt].mask = inet_addr(ch);
			else if (counter % 4 == 0) {
				ch[strlen(ch) - 1] = 0;  //remove '\n'
				strcpy(rt_table[rt_cnt].ifacename, ch);
			}
			if (counter % 4 == 0) {
				ch = strtok(NULL, "\n");
				rt_cnt++;
			} else {
				ch = strtok(NULL, "\t");
			}
		}
	}
	fclose(fp);
	cout << endl;
}

int getRouting(IPAddr hostIP) {
	int i;
	for (i = 0; i < rt_cnt; i++) {
		if (isSameNetwork(rt_table[i].destsubnet, rt_table[i].mask, hostIP)
				== 1)
			return i;
	}
	return -1;
}

int connBridge(int pos, char *ip, int port) {
	int servSocket = -1;
	struct sockaddr_in serv_addr;

	char r_buffer[1024];

	int counterTime = 0;
	int n;

	servSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (servSocket < 0)
		printf("ERROR opening socket");

	// server configuration
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(ip, (struct in_addr *) &serv_addr.sin_addr.s_addr);
	serv_addr.sin_port = port;

	while (counterTime < 6) {
		// connect to server
		if (connect(servSocket, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0)
			perror("connect to serv");

		cout << "no responce.." << endl;
		sleep(2);
		// Add this portion to only check if connection succeed or rejected

		bzero(r_buffer, 1024);
		n = read(servSocket, r_buffer, 1024);
		if (n < 0) {
			printf("ERROR reading from socket");
		} else if (n == 0) {
			printf("Disconnected from bridge\n");
			break;
		}

		if (strcmp("accept", r_buffer) == 0) {
			ITF2LINK link;
			link.sockfd = servSocket;
			strcpy(link.ifacename, iface_list[pos].lanname);
			iface_links.push_back(link);
			break;
		} else {
			printf("get msg: %s!", r_buffer);
			break;
		}

		counterTime++;
	}
	return servSocket;
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
byte* msgToIPpkt(char *data, IPAddr srcIP, IPAddr dstIP) {
	short data_len = strlen(data);
	byte *packet = new byte[10 + data_len];
	ByteIO byteIO(packet, 10 + data_len);
	byteIO.WriteUInt16(data_len);
	byteIO.WriteUInt32(srcIP);
	byteIO.WriteUInt32(dstIP);
	byteIO.WriteArray(data, data_len);

	return packet;
}

void printIpPack(char *data, IPAddr srcIP, IPAddr dstIP) {
	printf("IP packet is ready to send:\n");
	cout << "*************************************" << endl;
	cout << "		IP Packet		" << endl;
	cout << "*************************************" << endl;
	cout << "dstip: " << int_to_ip(dstIP) << "\t" << "srcip: "
			<< int_to_ip(srcIP) << endl;
	cout << "data: " << data << endl;
	cout << "*************************************" << endl;
	cout << endl;
}

/*
 *  frame format:
 *  type[0-1]size[2-3]srcAddr[4-9]dstAddr[10-15]IPpacket...
 */
void sendInputMsg(char *data, Rtable rtabl, MacAddr srcAddr, MacAddr dstAddr,
		IPAddr srcIP, IPAddr dstIP, int type) {
	// get socket information
	int toIntf = getIfaceByName(rtabl.ifacename);

	int toSocket = getSocket(iface_list[toIntf].lanname);

	int sock = iface_links[toSocket].sockfd;
	if (sock < 0) {
		cout << rtabl.ifacename << " is not connected!" << endl;
		return;
	}
	// get socket information

	byte* pkt = msgToIPpkt(data, srcIP, dstIP);

	byte frame[BUFSIZ];
	ByteIO byteIO(frame, sizeof(frame));
	int pkt_size = 10 + strlen(data);

	//cout << "pkt_size: " << pkt_size << endl;
	byteIO.WriteUInt16(type);
	byteIO.WriteUInt16(pkt_size);
	byteIO.WriteArray(srcAddr, 6);
	byteIO.WriteArray(dstAddr, 6);
	byteIO.WriteArray(pkt, pkt_size);

	int sendSize = sizeof(frame) - byteIO.GetAvailable();

	//printInformation(srcIP, dstIP, srcAddr, dstAddr);

	int ret = send(sock, frame, sendSize, 0);
	if (ret < 0) {
		cerr << "Failed to send msg: " << sock << endl;
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
		string msg(data);

		size_t pos = msg.find(" ");
		string where = msg.substr(pos + 1, 1);
		string what = msg.substr(pos + 3);

		char hostname[100];
		char message[100];

		strcpy(hostname, where.c_str());
		strcpy(message, what.c_str());

		// get destination ip address
		int pi = getHost(hostname);
		if (pi < 0) {
			cout << "no such host" << endl;
			return;
		}
		Host dstHost = host[pi];
		IPAddr dstIP = dstHost.addr;
		cout << "IP of " << hostname << " = " << int_to_ip(dstIP) << endl;
		//printIP((char*) "Destination IP", dstIP);
		// get destination ip address

		// looking for next hop
		pi = getRouting(dstHost.addr);
		if (pi < 0) {
			cout << "no suitable rtable entry to forward" << endl;
			return;
		}
		Rtable dstRtabl = rt_table[pi];
		int nextHop;

		if (dstRtabl.nexthop == 0) {
			nextHop = dstHost.addr;
		} else {
			nextHop = dstRtabl.nexthop;
		}

		cout << "next hop interface = " << dstRtabl.ifacename << endl;
		printIP((char*) "Next Hop IP", nextHop);
		// looking for next hop

		// get source IP and MAC address
		int toIntf = getIfaceByName(dstRtabl.ifacename);

		//get source IP
		IPAddr srcIP;
		srcIP = iface_list[toIntf].ipaddr;

		printIpPack(data, srcIP, dstIP);

		// get source MAC
		MacAddr srcMac;
		memcpy(srcMac, iface_list[toIntf].macaddr, 6);
		// get source IP and MAC address

		// get destination mac address from arp cache
		int arpPointer = doWeKnowMac(nextHop);

		MacAddr dstMac;
		int type;

		if (arpPointer != -1) {
			memcpy(dstMac, arpCacheList[arpPointer].macaddr, 6);
			type = 0;
			sendInputMsg(message, dstRtabl, srcMac, dstMac, srcIP, dstIP, type);
		} else {
			cout << "MAC address not in ARP cache, put IP packet in queue" << endl;
			PENDING_QUEUE item;
			item.dst_ipaddr = dstHost.addr;
			item.next_hop_ipaddr = nextHop;
			strcpy(item.msg, data);
			pkt_que.push_back(item);

			setEmpty(dstMac);
			type = 1;
			char request[] = "request";
			sendInputMsg(request, dstRtabl, srcMac, dstMac, srcIP, nextHop,
					type);
		}
	} else {

	}
}

void reply(IPAddr srcIP, IPAddr dstIP, MacAddr dstMac) {
	char message[] = "reply";

	//get forward iface
	int pi = getRouting(dstIP);
	if (pi < 0) {
		cout << "no suitable rtable entry to forward" << endl;
		return;
	}
	Rtable dstRtabl = rt_table[pi];

	//get MAC address, not done, Arp
	int toIntf = getIfaceByName(dstRtabl.ifacename);

	MacAddr srcMac;
	memcpy(srcMac, iface_list[toIntf].macaddr, 6);

	sendInputMsg(message, dstRtabl, srcMac, dstMac, srcIP, dstIP, 2);
}

void procRevMsg(char *data, int size) {
	ByteIO frame((byte *) data, size);
	MacAddr srcAddr, dstAddr;
	int type = frame.ReadUInt16(); //0: arp, 1: ip
	int pkt_size = frame.ReadUInt16(); //ip packet size
	//cout << "type: " << type << ", pkt_size: " << pkt_size << endl;
	char *pkt = new char[pkt_size];
	frame.ReadArray(srcAddr, 6);
	frame.ReadArray(dstAddr, 6);
	frame.ReadArray(pkt, pkt_size);

	//extract IP
	char msg[BUFSIZ];
	ByteIO ipPacket((byte *) pkt, pkt_size);
	int data_len = ipPacket.ReadUInt16();
	IPAddr srcIP = ipPacket.ReadUInt32();
	IPAddr dstIP = ipPacket.ReadUInt32();
	ipPacket.ReadArray(msg, data_len);
	msg[data_len] = 0;
	//cout << srcIP << ", " << dstIP << endl;
	msg[data_len] = 0;
	delete[] pkt;

	printf("Message Received: \n");
	printInformation(srcIP, dstIP, srcAddr, dstAddr);

	if (isMyIP(dstIP) == 1) {
		if (type == 1) {
			storeInArpCache(srcIP, srcAddr);
			printf("ARP reply\n");
			reply(dstIP, srcIP, srcAddr);
		} else if (type == 2) {
			storeInArpCache(srcIP, srcAddr);
			// Retrieve original packet from queue
			int queID = getPendingPacket(srcIP);
			PENDING_QUEUE pending = pkt_que[queID];
			printf("Store the data %s\n", pending.msg);
			procInputMsg(pending.msg);
		} else if (type == 0) {
			storeInArpCache(srcIP, srcAddr);
			if (isMyMac(dstAddr) == 1) {
				cout << "Received: " << msg << endl;
			}
		}
	}
}

void procRouterRevMsg(char *data, int size) {

	ByteIO frame((byte *) data, size);
	MacAddr srcAddr, dstAddr;
	int type = frame.ReadUInt16(); //0: arp, 1: ip
	int pkt_size = frame.ReadUInt16(); //ip packet size
	//cout << "type: " << type << ", pkt_size: " << pkt_size << endl;
	char *pkt = new char[pkt_size];
	frame.ReadArray(srcAddr, 6);
	frame.ReadArray(dstAddr, 6);
	frame.ReadArray(pkt, pkt_size);

	//extract IP
	char msg[BUFSIZ];
	ByteIO ipPacket((byte *) pkt, pkt_size);
	int data_len = ipPacket.ReadUInt16();
	IPAddr srcIP = ipPacket.ReadUInt32();
	IPAddr dstIP = ipPacket.ReadUInt32();
	ipPacket.ReadArray(msg, data_len);
	msg[data_len] = 0;
	//cout << srcIP << ", " << dstIP << endl;
	msg[data_len] = 0;
	delete[] pkt;

	printf("Message Received: \n");
	printInformation(srcIP, dstIP, srcAddr, dstAddr);

	if (isMyIP(dstIP) == 1) {
		if (type == 1) {
			storeInArpCache(srcIP, srcAddr);
			printf("ARP reply\n");
			reply(dstIP, srcIP, srcAddr);
		} else if (type == 2) {
			storeInArpCache(srcIP, srcAddr);
			// Retrieve original packet from queue
			int queID = getPendingPacket(srcIP);
			PENDING_QUEUE pending = pkt_que[queID];
			printf("Store the data %s\n", pending.msg);
			procInputMsg(pending.msg);
		}
	} else {
		if (isMyMac(dstAddr) == 1 && type == 0) {
			int toWho = getHostByIP(dstIP);
			char temp[100] = "send ";
			strcat(temp, host[toWho].name);
			strcat(temp, " ");
			strcat(temp, msg);
			procInputMsg(temp);
		}
	}
}

void prompt(string name) {
	cout << name << ">";
}

void clean() {
	vector<ITF2LINK>::iterator itv = iface_links.begin();
	while (itv != iface_links.end()) {
		int tmp_fd = (*itv).sockfd;
		close(tmp_fd);
		itv++;
	}
	iface_links.clear();
}

void station() {
	int i, n;
	char buf[BUFSIZ];
	fd_set r_set, all_set;
	int max_fd = -1;
	FD_ZERO(&all_set);

	for (i = 0; i < intr_cnt; i++) {
		char *name = iface_list[i].lanname;

		char ip[100] = ".";
		strcat(ip, name);
		ip[strlen(ip)] = '\0';
		strcat(ip, ".addr");

		char port[100] = ".";
		strcat(port, name);
		port[strlen(port)] = '\0';
		strcat(port, ".port");

		char ipAddr[1024];
		size_t len;
		if ((int) (len = readlink(ip, ipAddr, sizeof(ipAddr) - 1)) != -1)
			ipAddr[len] = '\0';

		char portNo[1024];
		if ((int) (len = readlink(port, portNo, sizeof(portNo) - 1)) != -1)
			portNo[len] = '\0';
		int portno = htons(atoi(portNo));

		// connect to server
		cout << name << " " << ipAddr << " " << portNo << endl;
		cout << "iface " << i << " try to connect to bridge " << name
				<< "...\n";
		int sockfd = connBridge(i, ipAddr, portno);

		if (sockfd > 0) {
			cout << "connection accepted!" << endl;
			FD_SET(sockfd, &all_set);
			max_fd = max(max_fd, sockfd);
		} else
			cout << "connection rejected!" << endl;
		cout << endl;
	}

	cout << endl;

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
			prompt("STATION");
			//input from user
			getline(cin, line);
			strcpy(buf, line.c_str());

			/* quit the station */
			if (strncmp(buf, "quit", 4) == 0) {
				clean();
				break;
			}

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

					procRevMsg(buf, n);
				}
			}
			itv++;
		}
	}
}

void router() {

	int i, n;
	char buf[BUFSIZ];
	fd_set r_set, all_set;
	int max_fd = -1;
	FD_ZERO(&all_set);

	for (i = 0; i < intr_cnt; i++) {
		char *name = iface_list[i].lanname;

		char ip[100] = ".";
		strcat(ip, name);
		ip[strlen(ip)] = '\0';
		strcat(ip, ".addr");

		char port[100] = ".";
		strcat(port, name);
		port[strlen(port)] = '\0';
		strcat(port, ".port");

		char ipAddr[1024];
		size_t len;
		if ((int) (len = readlink(ip, ipAddr, sizeof(ipAddr) - 1)) != -1)
			ipAddr[len] = '\0';

		char portNo[1024];
		if ((int) (len = readlink(port, portNo, sizeof(portNo) - 1)) != -1)
			portNo[len] = '\0';
		int portno = htons(atoi(portNo));

		// connect to server
		cout << name << " " << ipAddr << " " << portNo << endl;
		cout << "iface " << i << " try to connect to bridge " << name
				<< "...\n";
		int sockfd = connBridge(i, ipAddr, portno);

		if (sockfd > 0) {
			cout << "connection accepted!" << endl;
			FD_SET(sockfd, &all_set);
			max_fd = max(max_fd, sockfd);
		} else
			cout << "connection rejected!" << endl;
	}

	cout << endl;

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
			prompt("ROUTER");
			getline(cin, line);
			/* quit the router */
			if (strncmp(buf, "quit", 4) == 0) {
				clean();
				break;
			}
			strcpy(buf, line.c_str());
			//procRouterInputMsg(buf);
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

					procRouterRevMsg(buf, n);
				}
			}
			itv++;
		}
	}

}

/*----------------------------------------------------------------*/
/* station : gets hooked to all the lans in its ifaces file, sends/recvs pkts */
/* usage: station <-no -route> interface routingtable hostname */
int main(int argc, char *argv[]) {
	arp_cache_counter = 0;

	/* initialization of hosts, interface, and routing tables */
	if (argc < 5) {
		printf(
				"Please provide with specified format: ./station -no interface routing_table hostname\n");
		return 0;
	}

	strcpy(ifsFile, argv[2]);
	strcpy(rouFile, argv[3]);
	strcpy(hostFile, argv[4]);

	cout << "initializing..\n\n";
	readFromInterface();
	readFromRouting();
	readFromHosts();

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
