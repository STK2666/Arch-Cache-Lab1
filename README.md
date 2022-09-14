# 计算机体系结构实践





## 实践一

### 需求分析

<img src="./asset/Cache Proj1.png" style="zoom:40%;" />

在我们了解Cache的基本原理后，我们对实验要求进行进一步分析。

- 需要我们完成一个r/w流程的模拟
  - main.c过程实现
  - main.c完成arg的分析，从而进行Cache的初始化
  - main.c中实现一个模拟器sim，读取操作文件，并进行文件分析f
  - f分析的目的是将输入文件变为CPU对Cache的一条条指令inst，文件分析可以通过fstream读取，每行通过strtol()分解r/w和地址，或者直接根据位置索引读取
  - sim不断根据f给出的inst模拟CPU对Cache进行的读写
  - sim过程结束后main.c通过对Cache的访问读取相应数据，通过计算完成相应性能评价的计算，最后通过控制台进行输出。

- 需要我们完成一些参数的配置，即根据我们给出的参数初始化相应的Cache
  - Cache通过对象实现
  - Cache模型是统一的，方便L1、L2、Victim的不同实例化。
  - Lab1可以通过实例化L1实现Cache模型，Lab2通过将实例化的Cache按照L1->victim->L2->Memory顺序逐级排布，并且完成相应的策略配置即可实现Cache模型
  - 给出构造函数进行对象的初始化可以完成相应参数的配置
  - 需要我们实现配置策略和写策略，通过函数指针完成配置，重点是写好接口
  - 应当存储最后需要使用的信息，读、写计数，读、写缺失计数
  - Cache中的块需要采用什么样的数据结构。Cache中的块是应该直接创建为空，还是当我们需要的时候再新建？
    - 直接创建为空列表，好处是一次分配内存，免去动态分配可能造成的疏漏。
- 需要我们将内存的模型进行抽象，采取何种数据结构？
  - Cache中的块除了相应的内部信息（计数器、age）外，存储块的起始地址。当CPU对地址进行访问时，Cache查找表，该地址是否在现有的块中（地址右移块的大小，可以保证落在同一块中的hash-key相同），完成命中与否的模拟，替换块的时候，可以直接删除块再新建块，或者直接修改原有块。
  - 直接创建空列表。索引对应块的存储“位置”，（最慢的方法就是遍历列表。），利用hash可以快速查找相应位置是否在Cache中，并且可以得到索引，比遍历快，但是查找空缺位置或者替换时好像还需要遍历，起码保证了Cache在同一内存反复使用时的高效这一基本作用，miss时代价较高，冷miss一定需要遍历（如何改进？）。

```c++
struct block{
    uint32 key; // u32-type makes sure block index always can be contained 
    /*
    some other counters or inner attributes.
    */
}
```

- 编写Cache类过程中，发现其难点在于，如何设计替换策略，相对应的数据结构如何统一。

```C++
bool read(addr){
	index = Cache.indexize(addr);
    if(lookup(hashmap, index)){
        readcount++;
    }
    else{
        readcount++;
        readmisscount++;
        replaceStrategy(index);
    }
}

bool write(addr){
    index = Cache.indexize(addr);
    writeStrategy(addr)
}
```

- 编写第二组读写策略时发现写策略中应该内部调用替换策略，这样的话就应该修改计数器的调用位置，现有设计耦合度太高。

```C++
bool read(addr,cache){
	bool Miss = isMiss(addr);
    bool WriteBack = replaceStrategy(addr, cache);//replace中有命中后相应操作：命中块count++还是其他的块count++
    if(Miss)	cache.readMiss++;
    if(WriteBack)	cache.writeBack++;
    cache.readCount++;
}

bool write(addr,cache){
	bool Miss = isMiss(addr);
    bool Dirty = writeStrategy(addr, cache);
    if(Miss)	cache.writeMiss++;
    else	replaceStrategy(addr, cache);
    if(WriteBack)	cache.writeBack++;
    cache.writeCount++;
}
```

- Hit：replaceStrategy(), mark dirty
- Not hit：
  - Replace: replaceStrategy(), mark dirty
  - Not replace: writeback; 

|      | replaced counter | maintain counters | miss | writeBack |
| ---- | ---------------- | ----------------- | ---- | --------- |
| LRU  | biggest          | set 0, else ++    |      |           |
| LFU  | smallest         | ++                |      |           |

读：是否命中，然后replace（其中包括命中后的操作，替换后的初始化

写：是否命中，根据写策略决定是否替换，然后replace

#### 替换策略之间的相同点

编码过程中发现简单的根据命令行参数，通过函数指针想要区分各种策略，会导致替换策略和写策略当中有许多耦合部分，所以需要进一步分解功能，这就需要我们对整个读写过程进行划分，提取出不同替换策略或不同写策略之间耦合的流程，将不同策略间本质区别的部分提取出来

##### 读操作

1. 首先判断该地址是否命中，这里我们将该操作定义为Class Cache的 isHit() 方法，方法中应该将是否命中的结果返回。
2. 判断读写命中后：
   - hit: 两种策略都不需要替换，而是“维护”数据，即根据替换策略，对相应的“计数器”进行对应的维护操作。这里我们将该操作定义为Class Cache的 maintainCounter() 方法，初始化时我们根据命令行参数选择相应策略的函数指针，该函数中应当将读取的地址的索引，是否命中的结果作为参数传递进去。
   - miss: 两种策略都需要替换，根据 replace() 方法进行替换，该函数需要相应地址的索引，相应的组索引，进行替换，需要记录替换的过程是否有写回。之后再通过maintainCounter() 方法进行初始化替换块的counter，或者维护另外块的counter
3. 完成后，我们需要维护三个变量，读次数，读缺失次数，写回次数
   - 读次数：一定++
   - 读缺失：根据 isHit() 方法的结果
   - 写回次数：根据 replace() 的结果

##### 写操作

1. 同读操作1.
2. 判断读写命中后：先“写维护”，后“维护计数器”
   - hit: 除了根据替换策略完成不同 maintainCounter() 方法，此外还需根据不同的写策略，对相应Block进行“写维护”，这里我们将该操作定义为Class Cache的 writeMaintain() 方法，即：
     - WBWA: 相应Block标记Dirty
     - WTNA: 相应Block写回内存，即WriteBack
   - miss: 也需要在进行 maintainCounter() 之外进行“写维护”：
     - WBWA: 调用对应替换策略的 replace() 函数，记录是否WriteBack，相应Block标记Dirty
     - WTNA: 直接写回内存，WriteBack

3. 完成后，我们需要维护三个变量，写次数，写缺失次数，写回次数
   - 写次数：一定++
   - 写缺失：根据 isHit() 方法的结果
   - 写回次数：
     - WBWA: 默认false，根据 replace() 的结果
     - WTNA: 一定++

### 数据流图

### 系统设计

###  测试结果
