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
        // 每个组的第一个block间隔组相联度个块
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

Cache::Cache(uint _size, uint _blockSize, uint _assoc, uint _setsAmount, bool (*_replaceStrategy)(uint, uint, uint,Block*, bool, Cache*),
    bool (*_writeStrategy)(uint, uint, uint,Block*, bool, Cache*), bool(*_maintainCounter)(uint, uint, uint,Block*, bool, Cache*, bool), Cache* _nextLevel)
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

    c_maintainCounter = _maintainCounter;

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

bool Cache::isHit(uint index)
{
    if(hashMap->count(index))   return true;
    else    return false;
}

bool Cache::readFromAddress(uint addr)
{
    // std::cout << "r " << addr << std::endl;
    uint index = addr/c_blockSize;  // 块索引
    uint setIndex = index % c_setsAmount;   // 组索引

    uint blockIndex = 0;    //组内块索引
    Block* blockTemp = setList[setIndex].head;  //块内部索引对应的位置

    bool writeBack = false;
    bool hit = isHit(index);

    if(!hit)
    {
        countReadMiss();
        writeBack = (*c_replaceStrategy)(index, setIndex, blockIndex, blockTemp, hit, this);
    }
    
    // 维护计数器
    (*c_maintainCounter)(index, setIndex, blockIndex, blockTemp, hit, this, false);
    
    // 维护raw
    if(writeBack)   countWriteBack();
    countRead();

    return true;
}

bool Cache::writeFromAddress(uint addr)
{
    // std::cout << "w " << addr << std::endl;
    uint index = addr/c_blockSize;  // 块索引
    uint setIndex = index % c_setsAmount;   // 组索引

    uint blockIndex = 0;    //组内块索引
    Block* blockTemp = setList[setIndex].head;  //块内部索引对应的位置

    bool hit = isHit(index);

    bool writeBack = (*c_writeStrategy)(index, setIndex, blockIndex, blockTemp, hit, this);
        
    // 维护raw
    if(writeBack)   countWriteBack();
    if(!hit)    countWriteMiss();
    countWrite();

    return true;
}

bool LRU(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache){
    bool writeBack = false;
    uint assoc = cache->getAssoc();
    if(hit) std::cout << "Error" << std::endl;
    else
    {
        if(cache->setList[setIndex].occupiedBlock < assoc)  // set未满
        {
            blockIndex = cache->setList[setIndex].occupiedBlock;
            // blockIndex = cache->setList[setIndex].occupiedBlock++;
            // for(uint i= 0; i < blockIndex; i++)   blockTemp[i].count++;
        }
        else if(cache->setList[setIndex].occupiedBlock == assoc)
        {
            uint max = 0;
            //TODO:之前粗心把blockIndex写到上一行，于是变成局部变量了。
            blockIndex = 0;
            for(uint i = 0; i < assoc; i++)
            {
                if(blockTemp[i].count > max)
                {
                    max = blockTemp[i].count;
                    blockIndex = i;
                }
            }
            if(blockTemp[blockIndex].isDirty)   writeBack = true;
            // blockTemp[blockIndex].count = 0;
            // blockTemp[blockIndex].isDirty = false;
            cache->hashMap->erase(blockTemp[blockIndex].addr);
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
        }
        cache->hashMap->emplace(std::make_pair(index,blockTemp+blockIndex));
        blockTemp[blockIndex].addr = index;
    }

    return writeBack;
}

bool LFU(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache){
    bool writeBack = false;
    uint assoc = cache->getAssoc();
    if(hit) std::cout << "Error" << std::endl;
    else
    {
        if(cache->setList[setIndex].occupiedBlock < assoc)  // set未满
        {
            blockIndex = cache->setList[setIndex].occupiedBlock;
            // blockIndex = cache->setList[setIndex].occupiedBlock++;
            // for(uint i= 0; i < blockIndex; i++)   blockTemp[i].count++;
        }
        else if(cache->setList[setIndex].occupiedBlock == assoc)
        {
            uint min = -1;
            blockIndex = 0;
            for(uint i = 0; i < assoc; i++)
            {
                if(blockTemp[i].count < min)
                {
                    min = blockTemp[i].count;
                    blockIndex = i;
                }
            }
            if(blockTemp[blockIndex].isDirty)   writeBack = true;
            // blockTemp[blockIndex].count = 0;
            // blockTemp[blockIndex].isDirty = false;
            cache->hashMap->erase(blockTemp[blockIndex].addr);
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
        }
        cache->hashMap->emplace(std::make_pair(index,blockTemp+blockIndex));
        blockTemp[blockIndex].addr = index;
    }

    return writeBack;
}


bool LRU_Maintain(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache, bool isWrite)
{
    uint assoc = cache->getAssoc();
    if(hit)
    {
        Block* blockHit = cache->hashMap->at(index);
        for(uint i = 0; i < assoc; i++) blockTemp[i].count++;
        blockHit->count = 0;
    }
    else
    {
        if(cache->setList[setIndex].occupiedBlock < assoc)  // set未满
        {
            blockIndex = cache->setList[setIndex].occupiedBlock++;
            for(uint i= 0; i < blockIndex; i++)   blockTemp[i].count++;
            blockTemp[blockIndex].count = 0;
            //TODO:没初始化竟然错了（默认构造函数里明明就完成了预设的初始化），以后切记无论任何情况都要初始化
        }
        else if(cache->setList[setIndex].occupiedBlock == assoc)
        {
            Block* blockHit = cache->hashMap->at(index);
            for(uint i = 0; i < assoc; i++) blockTemp[i].count = (&blockTemp[i] == blockHit)? 0 : (blockTemp[i].count+1);
            blockHit->isDirty = isWrite;
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
        }
    }

    return true;
}

bool LFU_Maintain(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache, bool isWrite)
{
    uint assoc = cache->getAssoc();
    if(hit)
    {
        Block* blockHit = cache->hashMap->at(index);
        blockHit->count++;
    }
    else
    {
        if(cache->setList[setIndex].occupiedBlock == 0)
            cache->setList[setIndex].age = 0;
        if(cache->setList[setIndex].occupiedBlock < assoc)  // set未满
        {
            blockIndex = cache->setList[setIndex].occupiedBlock++;
            blockTemp[blockIndex].count = cache->setList[setIndex].age+1;
        }
        else if(cache->setList[setIndex].occupiedBlock == assoc)
        {
            Block* blockHit = cache->hashMap->at(index);
            cache->setList[setIndex].age = blockHit->count;
            blockHit->count++;
            blockHit->isDirty = isWrite;//TODO:少写一行代码你能怪谁呢？
        }
        else
        {
            std::cout << "Error! the set" << setIndex << "is overflow!" << std::endl;
        }
    }

    return true;
}

bool WBWA(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache)
{
    bool writeBack = false;
    if(!hit)
    {
        // 替换块
        writeBack = (*cache->c_replaceStrategy)(index, setIndex, blockIndex, blockTemp, hit, cache);
    }
    cache->hashMap->at(index)->isDirty = true;

    (*cache->c_maintainCounter)(index, setIndex, blockIndex, blockTemp, hit, cache, true);

    return writeBack;
}

bool WTNA(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache)
{

    if(hit) (*cache->c_maintainCounter)(index, setIndex, blockIndex, blockTemp, hit, cache, false);
    return false;
}