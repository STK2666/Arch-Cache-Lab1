#include "cache.h"

Block* Cache::initBlockList()
{
    Block* blockListHead = new Block[c_size/c_blockSize];
    return blockListHead;
}

Set* Cache::initSetList()
{
    Set* setList = new Set[c_setsAmount];
    Block* blockIter = blockListHead;
    for(int i = 0; i<c_setsAmount; i++)
    {
        setList[i].head = blockIter;
        blockIter += c_assoc;
    }
    return setList;
}

Cache::Cache()
{
    c_size = 0;
    c_blockSize = 0;
    c_assoc = 0;
    c_setsAmount = 0;

    c_readCounter = 0;
    c_writeCounter = 0;
    c_readMissCounter = 0;
    c_writeMissCounter = 0;
    c_writeBackCounter = 0;

    c_replaceStrategy = nullptr;
    c_writeStrategy = nullptr;

    hashMap = new std::unordered_map<int, Block*>;

    blockListHead = nullptr;

    setList = nullptr;

    nextLevel = nullptr;
}

Cache::Cache(uint _size, uint _blockSize, uint _assoc, uint _setsAmount, bool (*_replaceStrategy)(uint, Cache*), bool (*_writeStrategy)(uint, Cache*), Cache* _nextLevel)
{
    c_size = _size;
    c_blockSize = _blockSize;
    c_assoc = _assoc;
    c_setsAmount = _setsAmount;

    c_readCounter = 0;
    c_writeCounter = 0;
    c_readMissCounter = 0;
    c_writeMissCounter = 0;
    c_writeBackCounter = 0;

    c_replaceStrategy = _replaceStrategy;
    c_writeStrategy = _writeStrategy;

    hashMap = new std::unordered_map<int, Block*>;

    blockListHead = initBlockList();
    
    setList = initSetList();

    nextLevel = _nextLevel;
}

Cache::~Cache()
{
    delete hashMap;
    delete[] blockListHead;
    delete[] setList;
}

std::tuple<uint, uint, uint, uint, uint> Cache::getCounters()
{
    return std::make_tuple(c_readCounter, c_readMissCounter, c_writeCounter, c_writeMissCounter, c_writeBackCounter);
}


bool Cache::readFromAddress(uint addr)
{
    // std::cout << "r " << addr << std::endl;
    (*c_replaceStrategy)(addr, this);
    return true;
}

bool Cache::writeFromAddress(uint addr)
{
    // std::cout << "w " << addr << std::endl;
    (*c_writeStrategy)(addr, this);
    return true;
}


bool LRU(uint addr, Cache* cache)
{
    uint index = addr/cache->getBlockSize();
    uint setIndex = index % cache->getSetsAmount();

    cache->countRead();
    uint blockIndex = 0;
    Block* blockTemp = cache->setList[setIndex].head;
    if(!cache->hashMap->count(index))
    {
        if(cache->setList[setIndex].occupiedBlock < cache->getAssoc())
        {
            uint blockIndex = cache->setList[setIndex].occupiedBlock++;
            for(uint i= 0; i < blockIndex; i++)   blockTemp[i].count++;
        }
        else if(cache->setList[setIndex].occupiedBlock == cache->getAssoc())
        {
            uint max = 0, blockIndex = 0;
            for(uint i = 0; i < cache->getAssoc(); i++)
            {
                if(blockTemp[i].count > max)
                {
                    max = blockTemp[i].count++;
                    blockIndex = i;
                }
                else    blockTemp[i].count++;
            }
            if(blockTemp[blockIndex].isDirty)   cache->countWriteBack();
            blockTemp[blockIndex].count = 0;
            blockTemp[blockIndex].isDirty = false;
            cache->hashMap->erase(blockTemp[blockIndex].addr);
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
        }
        cache->hashMap->emplace(std::make_pair(index,blockTemp+blockIndex));
        cache->countReadMiss();
        blockTemp[blockIndex].addr = index;
    }
    else
    {
        Block* blockHit = cache->hashMap->at(index);
        uint offset = (uint)blockHit % cache->getAssoc();
        blockHit -= offset;
        for(uint i = 0; i < cache->getAssoc(); i++) blockHit[i].count = (i == offset)? 0 : (blockHit[i].count+1);
    }

    return true;
}

bool LFU(uint addr, Cache* cache)
{
    return true;
}


bool WBWA(uint addr, Cache* cache)
{
    uint index = addr/cache->getBlockSize();
    uint setIndex = index % cache->getSetsAmount();

    cache->countWrite();
    uint blockIndex = 0;
    Block* blockTemp = cache->setList[setIndex].head;

    if(!cache->hashMap->count(index))
    {
        if(cache->setList[setIndex].occupiedBlock < cache->getAssoc())
        {
            uint blockIndex = cache->setList[setIndex].occupiedBlock++;
            for(uint i= 0; i < blockIndex; i++)   blockTemp[i].count++;
        }
        else if(cache->setList[setIndex].occupiedBlock == cache->getAssoc())
        {
            uint max = 0, blockIndex = 0;
            for(uint i = 0; i < cache->getAssoc(); i++)
            {
                if(blockTemp[i].count > max)
                {
                    max = blockTemp[i].count++;
                    blockIndex = i;
                }
                else    blockTemp[i].count++;
            }
            blockTemp[blockIndex].count = 0;
            if(blockTemp[blockIndex].isDirty)   cache->countWriteBack();
            cache->hashMap->erase(blockTemp[blockIndex].addr);
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
            return false;
        }

        cache->hashMap->emplace(std::make_pair(index,blockTemp+blockIndex));
        cache->countWriteMiss();
        blockTemp[blockIndex].addr = index;
        blockTemp[blockIndex].isDirty = true;
    }
    else
    {
        Block* blockHit = cache->hashMap->at(index);
        blockHit->isDirty = true;
        uint offset = (uint)blockHit % cache->getAssoc();
        blockHit -= offset;
        for(uint i = 0; i < cache->getAssoc(); i++) blockHit[i].count = (i == offset)? 0 : (blockHit[i].count+1);
    }

    return true;
}

bool WTNA(uint addr, Cache* cache) 
{
    return true;
}