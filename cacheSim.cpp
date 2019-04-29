#include "CacheInterface.h"


using namespace std;


int main(int argc, char *argv[]) {

	if (argc < 21) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// File
	// Assuming it is the first argument
	char * fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned int 	MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0,L1Assoc = 0,L2Assoc = 0,
					L1Cyc = 0,L2Cyc = 0,WrAlloc = 0,VicCache = 0;

	for (int i = 2; i < 21; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else if (s == "--vic-cache") {
			VicCache = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// Constructing L1 and L2 caches:
	Cache* L1 = new Cache(BSize, L1Size,L1Assoc);
	Cache* L2 = new Cache(BSize, L2Size, L2Assoc);

	// Constructing a victim cache. Used only if VicCache == 1.
    VictimCache victimCache(BSize);

    // Construct cache controller
    CacheController controller(*L1, *L2, victimCache, VicCache);


	while (getline(file, line))
	{
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), nullptr, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		controller.accessCache(num,operation == 'w');
	}

//	printf("L1miss=%.03f ", L1MissRate);
//	printf("L2miss=%.03f ", L2MissRate);
//	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
