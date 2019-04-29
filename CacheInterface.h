//
// Created by compm on 26/04/19.
//

#ifndef HW2_CACHEINTERFACE_H
#define HW2_CACHEINTERFACE_H


#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <math.h>
#include <list>
#include <queue>

#define EMPTY UINT32_MAX

using namespace std;

/**
 * Abstracts a line in cache associated with a specific set. Stores all tags of that set in all available ways and handles
 * an LRU queue to decide which block to remove from cache.
 */
typedef struct CacheSet
{
    CacheSet(unsigned n) // Initialized to EMPTY (== UINT32_MAX) as it's not a valid address.
    {
        maximumCapacity = n;
    }
    /**************************************************************************************
     *                                   Methods                                          *
     **************************************************************************************/

    /**
     * search for this cache set in all ways and returns true if found and false otherwise.
     * @param tag
     * @return returns true if found and false otherwise
     */
    bool gotHit(unsigned tag);

    /**
     * inserts a block to this cache set.
     * @param tag
     */
    void insertBlockOnMiss(unsigned tag);

    /**
     * checks if cache is full.
     */
     bool isCacheSetFull();
    /**************************************************************************************
     *                                   Members                                          *
     **************************************************************************************/

    struct Compare    // Function object to pass to map to find an element in the cache set.
    {
        bool operator()(const unsigned& p1, const unsigned& p2)const
        {
            return (p1 >> 1u) == (p2 >> 1u);
        }
    };
    unsigned maximumCapacity; // Max ways in set
    deque<unsigned> LRU; // Keeps track of least recently used blocks of all blocks associated with this set.
    unordered_map<unsigned, deque<unsigned>::iterator, hash<unsigned>, Compare> map; // Stores the block position in the ith way.
}CacheLine;



/**
 * Abstracts a cache with a given 2^log_BSize block size, 2^log_Lsize lines of cache divided into 2^log_Lassoc ways.
 * an LRU queue for each line is constructed to decide which block from a given to remove from cache.
 */
typedef struct Cache
{
    /**
     * Creating a cache with a number of lines (sets) equals cache size divided by block size and number of ways (LSize/(BSize * LAssoc))
     * and assigning each line (a set) with a vector in length of number of associated ways.
     * @param log_BSize
     * @param log_LSize
     * @param log_LAssoc
     */
    Cache(unsigned log_BSize, unsigned log_LSize, unsigned log_LAssoc):
            cacheLines(static_cast<unsigned >(pow(2,log_LSize - log_BSize - log_LAssoc)), static_cast<unsigned >(pow(2, log_LAssoc)))
    {
        offsetSize = log_BSize;
        setSize = log_LSize - log_BSize - log_LAssoc;
        tagSize = 32 - offsetSize - setSize;
        tagDirectorySize = static_cast<unsigned >( pow(2, log_LSize - (log_BSize + log_LAssoc) ) );
    }

    /**
     * Parses the set of a given address.
     * @param address
     * @return the set number.
     */
    unsigned parseSet(unsigned address);

    /**
     * Parses the tag of a given address.
     * @param address
     * @return the tag.
     */
    unsigned parseTag(unsigned address);

    /**
     * Looks up this cache for the given input address.
     * @param tag
     * @return true if found.
     */
    bool gotHit(unsigned address);

    /**
     * inserts the address with the dirty bit to this cache.
     * @param addrees_dirty
     */
    void insertBlockToCacheOnMiss(unsigned addrees_dirty);
    // Member variables.
    vector<CacheSet> cacheLines; // Holds the cache lines for all sets and their LRU queue.
    unsigned tagDirectorySize;
    unsigned tagSize; // Number of bits required for a tag. Used for searching the cache ways for a specific set.
    unsigned setSize; // Number of bits required for the set.
    unsigned offsetSize; // Number of bits required for the block size.
    void updateLruOnHit(unsigned address);
}Cache;

/**
 * A victim cache emulator that holds 4 LRU blocks of data removed from cache layers. Eviction is implemented in a
 * FIFO manner.
 */
typedef struct VictimCache
{
    VictimCache(unsigned Log_BSize): Log_BSize(Log_BSize)
    {
    }
    const unsigned maximumCapacitance = 4;
    list<unsigned> LRU; // LRU storing the complete address.
    unsigned Log_BSize;
}VictimCache;

typedef struct cacheController
{
    cacheController(Cache& L1, Cache& L2, VictimCache& victimCache, bool victimCacheExists)
            : L1(L1), L2(L2), victimCache(victimCache), victimCacheExists(victimCacheExists) {
    }
    // Methods
    void accessCache(unsigned address, bool isWrite);
    void insertBlockOnMiss(unsigned address, bool isWrite);
    // Members
    Cache L1, L2;
    VictimCache victimCache;
    bool victimCacheExists;
    float L1MissRate = 0;
    float L2MissRate = 0;
    float avgAccTime = 0;
    unsigned L1Accessed = 0;
    unsigned L2Accessed = 0;
    unsigned L2Misses = 0;
    unsigned victimCacheAccessed = 0;
    unsigned memAccessed = 0;
}CacheController;


#endif //HW2_CACHEINTERFACE_H
