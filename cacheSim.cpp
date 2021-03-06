#include "CacheInterface.h"
#include <assert.h>

using namespace std;

void printMap(Cache L) {
    for (size_t i = 0; i < L.cacheLines.size(); ++i)
    {
        cout << "cache set # " << i << "   ";
        for (auto const it : L.cacheLines[i].LRU)
        {
//            cout << "{" <<  pair.first << ", " << *(pair.second) << ", " << ((*(pair.second) & 1) ? "dirty" : "") << "}   ";
//            cout << "{" <<  pair << "}" ;
            cout << "{" << it << ", " << ((it & 1) ? "dirty" : "") << "}    ";
        }
        cout << "   LRU block is: " << L.cacheLines[i].lruBlock();
        cout << "   MRU block is: " << L.cacheLines[i].LRU.front() << endl;
    }
}

void printVicMap(VictimCache vic) {
    cout << "victim cache:" << endl;
    for (unsigned const i : vic.fifoQueue)
    {
        cout << "{" << i << ", " << ((i & 1)  ? "dirty" : "") << "}  ";
    }
    cout << endl;
}

void checkRep(CacheController controller)
{
    Cache& L1 = controller.L1;
    Cache& L2 = controller.L2;
    VictimCache& vic = controller.victimCache;

    unsigned L1SetSize = L1.cacheLines.size();
    unsigned L2SetSize = L2.cacheLines.size();

    for (size_t i = 0; i < L1SetSize; ++i)
    {
        if (L1.cacheLines[i].map.size() > 0) {
            for (auto it1 = L1.cacheLines[i].map.begin(); it1 != L1.cacheLines[i].map.end(); ++it1) {
                unsigned count = 0;
                for (auto it2 = L1.cacheLines[i].map.begin(); it2 != L1.cacheLines[i].map.end(); ++it2) {
                    if (*(it1->second) == *(it2->second)) {
                        count++;
                    }
                }
                if (count > 1)
                {
                    cout << "{" << it1->first <<  ", " <<  *(it1->second) << "}" << endl;
//                    cout << "{" << it2->first <<  ", " <<  *(it2->second) << "}" << endl;
                    assert (count == 1);
                }
            }
        }
    }

    for (size_t i = 0; i < L2SetSize; ++i)
    {
        if (L2.cacheLines[i].map.size() > 0) {
            for (auto it1 = L2.cacheLines[i].map.begin(); it1 != L2.cacheLines[i].map.end(); ++it1) {
                unsigned count = 0;
                for (auto it2 = L2.cacheLines[i].map.begin(); it2 != L2.cacheLines[i].map.end(); ++it2) {
                    if (*(it1->second) == *(it2->second)) {
                        count++;
                    }
                }
                if (count > 1)
                {
                    cout << "{" << it1->first <<  ", " <<  *(it1->second) << "}" << endl;
//                    cout << "{" << it2->first <<  ", " <<  *(it2->second) << "}" << endl;
                    assert (count == 1);
                }
            }
        }
    }

    for (unsigned value : vic.fifoQueue)
    {
        bool found = false;
        if (L1.gotHit(value))
        {
            cout << "not suppose to be in L1" << endl;
            assert(false);
        }
        if (L2.gotHit(value))
        {
            cout << "not suppose to be in L2" << endl;
            assert(false);
        }
    }
}


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
//	Cache L1(BSize, L1Size,L1Assoc);
    Cache* L2 = new Cache(BSize, L2Size, L2Assoc);
//	Cache L2(BSize, L2Size, L2Assoc);

    // Constructing a victim cache. Used only if VicCache == 1.
    VictimCache* victimCache = new VictimCache(BSize);

    // Construct cache controller
    CacheController controller(*L1, *L2, *victimCache, VicCache, MemCyc, L1Cyc, L2Cyc, WrAlloc);

    // TODO delete this
    unsigned count = 1;
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

//		// DEBUG - remove this line
        cout << count++ << " operation: " << operation;

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        // DEBUG - remove this line
        cout << ", address (hex)" << cutAddress;

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), nullptr, 16);

        // DEBUG - remove this line
        cout << " (dec) " << num << endl;

        if (operation == 'w')
        {
            controller.accessCacheOnWrite(num);
        }
        else
        {
            controller.accessCacheOnRead(num);
        }
        cout << "L1 cache" << endl;
        printMap(controller.L1);
        cout << endl << "L2 cache" << endl;
        printMap(controller.L2);
        printVicMap(controller.victimCache);
        cout << endl;
        checkRep(controller);

    }
    controller.calculateStats();
    printf("L1miss=%.03f ", controller.L1MissRate);
    printf("L2miss=%.03f ", controller.L2MissRate);
    printf("AccTimeAvg=%.03f\n", controller.avgAccTime);
    delete L1;
    delete L2;
    delete victimCache;
    return 0;
}