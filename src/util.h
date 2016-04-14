/*
 * Course: CNT5505-01, DATA/COMPUTER COMMUN
 * Semester: Spring 2016
 * Names:
 * 		Mustakimur Rahman Khandaker (mrk15e@my.fsu.edu)
 *      Yongjiang Liang (yl14u@my.fsu.edu)
 *
 */

/*
 * util.h
 *
 *  Created on: April 1, 2016
 *      Author: Yongjiang Liang
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "head.h"
#include "ether.h"

using namespace std;

std::string trim(const std::string& str,
		const std::string& whitespace = " \t") {
	const unsigned strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const unsigned strEnd = str.find_last_not_of(whitespace);
	const unsigned strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

std::vector<std::string> &split(const std::string &s, char delim,
		std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		item = trim(item);
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector < std::string > elems;
	split(s, delim, elems);

	return elems;
}

map<string, string> loadConf(string file, char delim) {
	ifstream ifs(file.c_str());

	string line;
	map < string, string > conf;
	while (std::getline(ifs, line, '\n')) {
		line = trim(line);
		if (line.length() < 1)
			continue;
		vector < string > res = split(line, delim);
		if (res.size() < 2) {
			cout << "conf error: " << line << endl;
		}
		conf.insert(make_pair(res[0], res[1]));
	}
	ifs.close();
	return conf;
}

void bytes_to_string(byte *buf, int n, string &key) {
	for (int i = 0; i < n; i++)
		key.append(1, (char) buf[i]);
}

/* remove the whitespace at the beginning and ending of buffer */
char* remove_whitespace(char *buffer) {
	/* remove the whitespace from the ending */
	int len = strlen(buffer);
	int i = len - 1;
	while (i >= 0) {
		if (isspace(buffer[i]))
			buffer[i] = 0;
		else
			break;
		i--;
	}

	/* remove the whitespace from the beginning */
	while (*buffer && isspace(*buffer))
		buffer++;
	return buffer;
}

void printMac(char *msg, MacAddr mac) {
	int i;
	printf("%s :", msg);
	for (i = 0; i < 6; i++)
		printf("%02x", mac[i]);
	printf("\n");
}

int compareMac(MacAddr first, MacAddr second) {
	int i, c;
	c = 0;
	for (i = 0; i < 6; i++) {
		if ((first[i]&second[i]) == first[i])
			c++;
	}
	if (c == 6)
		return 0;
	return 1;
}

void setEmpty(MacAddr temp) {
	int i;

	for (i = 0; i < 6; i++) {
		temp[i] = 0;
	}
}

void setFF(MacAddr temp) {
	for (int i = 0; i < 6; i++) {
		temp[i] = 0xff;
	}
}

int isBroadcast(MacAddr temp) {
	int yes = 1;
	for (int i = 0; i < 6; i++) {
		if (temp[i] != 0xff) {
			yes = 0;
			break;
		}
	}
	return yes;
}

void printIP(char *msg, IPAddr ip) {
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;
	printf("%s : %s\n", msg, inet_ntoa(ip_addr));
}

/*
char *int_to_ip(IPAddr ip) {
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;
	return inet_ntoa(ip_addr);
}*/

void int_to_ip(IPAddr ip, char *res) {
	//ip = htonl(ip);
	inet_ntop(AF_INET, &ip, res, 16);
}

void printInformation(IPAddr srcIP, IPAddr dstIP, MacAddr srcMac,
		MacAddr dstMac) {
	printf("Information:\n");

	printIP((char*) "Source IP", srcIP);
	printIP((char*) "Destination IP", dstIP);
	printMac((char*) "Source Mac", srcMac);
	printMac((char*) "Dsetnation Mac", dstMac);
}

#endif /* UTIL_H_ */
