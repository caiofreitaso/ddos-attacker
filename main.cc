#include "ddos.h"

int main(int argc, char const *argv[])
{
	std::cout << "ntohs: " << ntohs(11265) << "\n";
	attack::propagate(std::string("127.0.0.1"),3,"192.168.0.5:100");
	return 0;
}