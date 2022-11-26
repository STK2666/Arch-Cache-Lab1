#pragma once
#include <tuple>
#include <list>
#include <utility>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

using uint = unsigned int;

struct Block
{
	bool valid;
	uint addr;
	uint tag;
	uint index;
	uint count;
	bool isDirty;
	Block(): valid(false), addr(0), tag(0), index(0), count(0), isDirty(false) {};
	Block(bool _valid, uint _addr, uint _tag, uint _index, uint _count, bool _isDirty): valid(_valid), addr(_addr), tag(_tag), index(_index), count(_count), isDirty(_isDirty) {};
};
typedef struct Block Block;



struct Set
{
	uint occupiedBlock;
	uint age;
	Block* head;
	Set(): occupiedBlock(0), age(0), head(nullptr) {};
	Set(uint _occupiedBlock, uint _age, Block* _head): occupiedBlock(_occupiedBlock), age(_age), head(_head) {};
};
typedef struct Set Set;

class Cache
{
private:
	uint c_size;
	uint c_blockSize;
	uint c_assoc;
	uint c_setsAmount;

	uint c_readCounter;
	uint c_writeCounter;
	uint c_readMissCounter;
	uint c_writeMissCounter;
	uint c_writeBackCounter;

	bool isHit(uint index);
		
	Block *blockListHead;	// 连续存储的Block起始地址
	Block* initBlockList();

	Cache *nextLevel; // 下一级存储
	

public:
	bool (*c_replaceStrategy)(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);	// 替换策略函数指针
	bool (*c_writeStrategy)(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);	// 写策略函数指针
	bool (*c_maintainCounter)(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache, bool isWrite);
	std::unordered_map<int, Block*> *hashMap;	// 主要作用只是在每次访存时快速判断是否命中
	Set *setList;	// 连续存储的set地址
	Set* initSetList();
	
	Cache();
	Cache(uint _size, uint _blockSize, uint _assoc, uint _setsAmount, 
		bool (*_replaceStrategy)(uint, uint, uint,Block*, bool, Cache*), bool (*_writeStrategy)(uint, uint, uint,Block*, bool, Cache*), bool maintainCounter(uint, uint, uint,Block*, bool, Cache*, bool), Cache* _nextLevel);
	~Cache();
	
	uint getSize(){return c_size;};
	uint getBlockSize(){return c_blockSize;};
	uint getAssoc(){return c_assoc;};
	uint getSetsAmount(){return c_setsAmount;};
	
	void countRead(){c_readCounter++;};
	void countWrite(){c_writeCounter++;};
	void countReadMiss(){c_readMissCounter++;};
	void countWriteMiss(){c_writeMissCounter++;};
	void countWriteBack(){c_writeBackCounter++;};

	bool isMiss(uint addr);
	bool readFromAddress(uint addr);
	bool writeFromAddress(uint addr);
	
	std::tuple<uint, uint, uint, uint, uint> getCounters();
};

bool LRU(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);
bool LFU(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);

bool LRU_Maintain(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache, bool isWrite);
bool LFU_Maintain(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache, bool isWrite);

bool WBWA(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);
bool WTNA(uint index, uint setIndex, uint blockIndex, Block *blockTemp, bool hit, Cache *cache);