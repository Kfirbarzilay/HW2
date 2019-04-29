//
// Created by compm on 26/04/19.
//
#include "CacheInterface.h"


/**************************************************************************************
 *                                   CacheController                                  *
 **************************************************************************************/

void CacheController::accessCache(unsigned address, bool isWrite)
{
    // Update stats when accessing cache. One access to L1 cache is definite.
    this->L1Accessed++;

    // Look up address in cache Layer 1
    bool hitL1 = this->L1.gotHit(address);

    // If not found in L1 proceed to L2 cahce
    if(!hitL1)
    {
        this->L2Accessed++;
        bool hitL2 = this->L2.gotHit(address);

        // If not found in L2 proceed to victim cache if exists
        if(!hitL2)
        {
            this->L2Misses++;
            if (victimCacheExists)
            {
                this->victimCacheAccessed++;
                // search Victim Cache for the address.
                bool hitVictimCache = false;
                auto it = this->victimCache.LRU.begin();
                for (; it != this->victimCache.LRU.end(); ++it)
                    if (*it == address)
                        hitVictimCache = true;

                // Not found in victim cache and memory is accessed
                if(!hitVictimCache)
                    this->memAccessed++;
            }
            else
            {
                this->memAccessed++;
            }
            this->insertBlockOnMiss(address, isWrite);
        }
        else // Hit in L2 cache.
        {
            this->L2.updateLruOnHit(address);
        }
    }
    else // Hit in L1 cache.
    {
        this->L1.updateLruOnHit(address);
    }
}

void CacheController::insertBlockOnMiss(unsigned address, bool isWrite)
{
    // Form the address concatenated with the dirty bit.
    unsigned address_dirty = (address << 1u) | isWrite;

    // insert to caches.
    L1.insertBlockToCacheOnMiss(address_dirty);
    L2.insertBlockToCacheOnMiss(address_dirty);
}


/**************************************************************************************
 *                                   Cache                                            *
 **************************************************************************************/

unsigned Cache::parseSet(unsigned address)
{
    // Create a mask by taking only the address set bits.
    // Example for a setSize == 4:
    //       first shift 0b1 to the left setSize times getting 0b10000.
    //       then subtract 1 and get the required mask 0b1111.
    unsigned mask = 1;
    mask = ( mask << this->setSize ) - 1;

    address >>= (this->offsetSize) + 1; // Get rid of offset bits and the dirty bit

    return mask & address;
}

unsigned Cache::parseTag(unsigned address)
{
    return address >> (this->setSize + this->offsetSize + 1); // Removing dirty bit and set and offset bits.
}

bool Cache::gotHit(unsigned address)
{
    bool found;
    // Get the set and tag.
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);

    // Search the tag in the set's ways.
    found = this->cacheLines[set].gotHit(tag);
    return found;
}

void Cache::insertBlockToCacheOnMiss(unsigned addrees_dirty)
{
    // get the set and adress and insert block to set in cache.
    unsigned set = this->parseSet(addrees_dirty);
    unsigned tag = this->parseTag(addrees_dirty);

    this->cacheLines[set].insertBlockOnMiss(tag);
}
void Cache::updateLruOnHit(unsigned addrees_dirty)
{
    unsigned set = this->parseSet(addrees_dirty);
    unsigned tag = this->parseTag(addrees_dirty);

    CacheLine& line = cacheLines[set];

    // update LRU: erase from current location and move to head of LRU.
    auto it = line.map[tag];
    line.LRU.erase(it);
    line.LRU.push_front(tag);
    line.map[tag] = line.LRU.begin();
}


/**************************************************************************************
 *                                   CacheSet                                         *
 **************************************************************************************/
bool CacheSet::gotHit(unsigned tag)
{
    bool found = true;

    // not present in cache
    if (map.find(tag) == map.end())
    {
        found = false;
    }
    return found;
}

void CacheSet::insertBlockOnMiss(unsigned tag)
{
    if (this->isCacheSetFull())
    {
        //delete least recently used block.
        unsigned last = LRU.back();
        LRU.pop_back();
        map.erase(last); // Remove iterator reference to way in map.
        // TODO: if this is L1 cache then
    }
    // Insert tag to cache set and update reference
    LRU.push_front(tag);
    map[tag] = LRU.begin();
}

bool CacheSet::isCacheSetFull()
{
    return this->map.size() == this->maximumCapacity;
}

// TODO: 1. Make sure dirty bit is not lost in insert or in removal
//       2. Write back functionality: updating lower layers when a block gets kicked out.
//       3. Write on allocate and write with no allocate.
//       4. Victim cache functionality.