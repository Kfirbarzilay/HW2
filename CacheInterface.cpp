//
// Created by compm on 26/04/19.
//
#include "CacheInterface.h"



/**************************************************************************************
 *                                   CacheController                                  *
 **************************************************************************************/

void CacheController::accessCacheOnRead(unsigned address)
{
    // Update stats when accessing cache. One access to L1 cache is definite.
    this->L1Accessed++;
    bool hitL1 = this->L1.gotHit(address);

    // Miss
    if(!hitL1)
    {
        this->L2Accessed++;
        bool hitL2 = this->L2.gotHit(address);

        // Got read miss in L2
        if(!hitL2)
        {
            // If not found in L2 proceed to victim cache if exists
            if (victimCacheExists)
            {
                this->victimCacheAccessed++;
                // search Victim Cache for the address.
                bool hitVictimCache = false;
                bool isDirty = false;
                for (unsigned& value : this->victimCache.fifoQueue)
                {
                    unsigned mask = value & 0xFFFFFFFC;
                    if (mask == address) {
                        isDirty = 1 & value;
                        hitVictimCache = true;
                        this->victimCache.fifoQueue.remove(value);
                        break;
                    }
                }

                // Not found in victim cache and memory is accessed
                if(!hitVictimCache)
                    this->memAccessed++;

                // Load to L1 and L2.
                this->loadToL2AndL1(address, isDirty);

            }
            else // No victim cache
            {
                this->memAccessed++;
                this->loadToL2AndL1(address, false);
            }
        }
        else // Hit in L2 cache.
        {
            this->L2.updateLruOnHit(address, false);
            // TODO check if dirty
            bool isBlockDirty = this->L2.isDirty(address);
            if (isBlockDirty)
                this->L2.unmarkDirty(address);
            this->loadToL1(address, isBlockDirty);
        }
    }
    else // Hit in L1 cache.
    {
        this->L1.updateLruOnHit(address, false);
    }
}

void CacheController::accessCacheOnWrite(unsigned address)
{
// Update stats when accessing cache. One access to L1 cache is definite.
    this->L1Accessed++;

    // no write allocate
    if (!this->writeAllocate)
    {
        bool hitL1 = this->L1.gotHit(address);
        // Miss
        if(!hitL1)
        {
            this->L2Accessed++;
            bool hitL2 = this->L2.gotHit(address);

            // Got read miss in L2
            if(!hitL2)
            {
                // If not found in L2 proceed to victim cache if exists
                if (victimCacheExists)
                {
                    this->victimCacheAccessed++;
                    // search Victim Cache for the address.
                    bool hitVictimCache = false;
                    for (unsigned& value : this->victimCache.fifoQueue)
                    {
                        unsigned mask = value & 0xFFFFFFFC;
                        if (mask == address) {
                            value |= 1; // write to victim cache and finish.
                            hitVictimCache = true;
                            break;
                        }
                    }

                    // Not found in victim cache and memory is accessed
                    if(!hitVictimCache)
                        this->memAccessed++;

                }
                else // No victim cache
                {
                    this->memAccessed++;
                }
            }
            else // Hit in L2 cache.
            {
                this->L2.updateLruOnHit(address, true);
            }
        }
        else // Hit in L1 cache.
        {
            this->L1.updateLruOnHit(address, true);
        }
    }
    else // Write allocate
    {
        bool hitL1 = this->L1.gotHit(address);

        // Miss
        if(!hitL1)
        {
            this->L2Accessed++;
            bool hitL2 = this->L2.gotHit(address);

            // Got read miss in L2
            if(!hitL2)
            {
                // If not found in L2 proceed to victim cache if exists
                if (victimCacheExists)
                {
                    this->victimCacheAccessed++;
                    // search Victim Cache for the address.
                    bool hitVictimCache = false;
                    bool isDirty = false;
                    for (unsigned value : this->victimCache.fifoQueue)
                    {
                        unsigned mask = value & 0xFFFFFC;
                        if (mask == address) {
                            isDirty = 1 & value;
                            hitVictimCache = true;
                            this->victimCache.fifoQueue.remove(value);
                        }
                    }

                    // Not found in victim cache and memory is accessed
                    if(!hitVictimCache)
                        this->memAccessed++;

                    // Load to L1 and L2.
                    this->loadToL2AndL1(address, isDirty);

                }
                else // No victim cache. Access memory
                {
                    this->memAccessed++;
                    this->loadToL2AndL1(address, true);
                }
            }
            else // Hit in L2 cache.
            {
                this->L2.updateLruOnHit(address, false);
                // TODO check if dirty
                bool isBlockDirty = this->L2.isDirty(address);
                if (isBlockDirty)
                    this->L2.unmarkDirty(address);
                this->loadToL1(address, isBlockDirty);
            }
        }
        else // Hit in L1 cache.
        {
            this->L1.updateLruOnHit(address, true);
        }
    }
}

void CacheController::loadToL2AndL1(unsigned address, bool isWrite)
{
    // Check if cache set is full in L2. If so snoop in L1 for the evicted line.
    bool L2SetIsfull = this->L2.isSetFull(address);
    if (L2SetIsfull)
    {
        unsigned evictedBlock = L2.getLruBlock(address);
        bool foundBlockL1 = this->L1.gotHit(evictedBlock);
        if(foundBlockL1)
        {
            bool L1Dirty = this->L1.isDirty(evictedBlock);
            // If Dirty write to L2 and mark dirty
            if (L1Dirty)
            {
                // Write to L2
                this->L2.markAsDirty(evictedBlock);
            }
            // Invalidate evicted block in L1.
            this->L1.removeLruBlock(evictedBlock);
        }
        // If L2 evicted block is dirty write to victim cache \ memory. Not considering wb time.
        bool isL2EvictedBlockDirty = this->L2.isDirty(evictedBlock);
        if (this->victimCacheExists)
        {
            // Add to Victim cache. Not considering wb latency.
            this->victimCache.addBlock(evictedBlock, isL2EvictedBlockDirty);
        }
        // Invalidate L2 evicted block
        this->L2.removeLruBlock(evictedBlock);
    }

    // Update L2 with new block
    this->L2.insertBlockToCache(address, false);

    bool L1SetIsFull = this->L1.isSetFull(address);
    // L1 set is full
    if (L1SetIsFull)
    {
        // Evict fifoQueue block
        unsigned L1EvictedBlock = L1.getLruBlock(address);

        bool isL1BLockDirty = this->L1.isDirty(L1EvictedBlock);
        // Evicted block is dirty. Write to L2 and mark as dirty.
        if (isL1BLockDirty)
        {
            this->L2.markAsDirty(L1EvictedBlock); // TODO complete method
        }
        // Evict old block from L1 and load new block
        this->L1.removeLruBlock(L1EvictedBlock);
    }

    this->L1.insertBlockToCache(address, isWrite);
}

void CacheController::loadToL1(unsigned int address, bool isWrite)
{
    bool L1SetIsFull = this->L1.isSetFull(address);
    // L1 set is full
    if (L1SetIsFull)
    {
        // Evict fifoQueue block
        unsigned L1EvictedBlock = L1.getLruBlock(address);

        bool isL1BLockDirty = this->L1.isDirty(L1EvictedBlock);
        // Evicted block is dirty. Write to L2 and mark as dirty.
        if (isL1BLockDirty)
        {
//            TODO this->L2Accessed++; No consideration of wb.
            this->L2.markAsDirty(L1EvictedBlock); // TODO complete method
        }
        // Evict old block from L1 and load new block
        this->L1.removeLruBlock(L1EvictedBlock);
    }
    this->L1.insertBlockToCache(address, isWrite);
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

    address >>= this->offsetSize; // Get rid of offset bits and the dirty bit

    return mask & address;
}

unsigned Cache::parseTag(unsigned address)
{
    return address >> (this->setSize + this->offsetSize); // Removing set and offset bits.
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

void Cache::insertBlockToCache(unsigned address, bool isDirty) {
    // get the set and adress and insert block to set in cache.
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);
    unsigned address_dirty = address | isDirty;

    this->cacheLines[set].insertBlockToSet(address_dirty, tag);
}
void Cache::updateLruOnHit(unsigned address, bool isWrite) // TODO update
{
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);
    CacheSet& cacheSet = cacheLines[set];

    // update fifoQueue: erase from current location and move to head of fifoQueue.
    auto it = cacheSet.map[tag];
    unsigned value = *it | isWrite;
    cacheSet.map.erase(tag);
    cacheSet.LRU.erase(it);
    cacheSet.LRU.push_front(value);
    cacheSet.map[tag] = cacheSet.LRU.begin();
}

bool Cache::isSetFull(unsigned address) {
    unsigned set = this->parseSet(address);
    return this->cacheLines[set].isCacheSetFull();
}

bool Cache::isDirty(unsigned int address) {
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);
    return this->cacheLines[set].isDirty(tag); // TODO tag values don't match
}

unsigned Cache::getLruBlock(unsigned address) {
    unsigned set = this->parseSet(address);
    return this->cacheLines[set].lruBlock();
}

void Cache::removeLruBlock(unsigned address) {
    unsigned set = this->parseSet(address);
    unsigned last = this->cacheLines[set].LRU.back();
    unsigned lastTag = this->parseTag(last);
    this->cacheLines[set].removeLruBlock(lastTag);
}

void Cache::markAsDirty(unsigned address) {
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);
    this->cacheLines[set].markAsDirty(tag);
}
void Cache::unmarkDirty(unsigned address) {
    unsigned set = this->parseSet(address);
    unsigned tag = this->parseTag(address);
    this->cacheLines[set].unmarkDirty(tag);
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

void CacheSet::insertBlockToSet(unsigned address_dirty, unsigned tag)
{
    if (this->isCacheSetFull())
    {
        cout << "Debug, shouldn't enter here " << endl;
        //delete least recently used block.
        unsigned last = LRU.back();
        LRU.pop_back();
        map.erase(last); // Remove iterator reference to way in map.
        // TODO: if this is L1 cache then
    }
    // Insert tag to cache set and update reference
    LRU.push_front(address_dirty);
    map[tag] = LRU.begin();
}

bool CacheSet::isCacheSetFull()
{
    return this->LRU.size() == this->maximumCapacity;
}

bool CacheSet::isDirty(unsigned tag)
{
    auto it = this->map[tag];
    bool dirty = *it & 1u;
    return dirty;
}

unsigned CacheSet::lruBlock() {
    return this->LRU.back();
}

void CacheSet::removeLruBlock(unsigned lastTag)
{
    LRU.pop_back();
    map.erase(lastTag); // Remove iterator reference to way in map.
}

void CacheSet::markAsDirty(unsigned tag)
{
    auto it = this->map[tag];
    *it |= 1;
}

void CacheSet::unmarkDirty(unsigned tag) {
    auto it = this->map[tag];
    *it &= 0xFFFFFFFE;
}

/**************************************************************************************
 *                                   VictimCache                                      *
 **************************************************************************************/
void VictimCache::addBlock(unsigned address, bool isDirty) {
    if (this->isVictimCacheFull())
    {
        this->fifoQueue.pop_back();
    }
    unsigned value = address | isDirty;
    this->fifoQueue.push_front(value);
}

bool VictimCache::isVictimCacheFull() {
    return this->fifoQueue.size() == this->maximumCapacitance;
}

bool VictimCache::hitInVictim(unsigned address) {
    return false;
}


