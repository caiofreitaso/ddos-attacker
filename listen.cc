#include "ddos.h"

int main(int argc, char const *argv[])
{
	attack::node mynode;
	mynode.nodes.push_back(std::string("127.0.0.1"));
	attack::initialize(mynode);
	return 0;
}