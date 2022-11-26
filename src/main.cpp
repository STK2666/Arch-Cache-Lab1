#include "cache.h"

#define RS(i) (i?LFU:LRU)
#define RSM(i) (i?LFU_Maintain:LRU_Maintain)
#define WS(i) (i?WTNA:WBWA)


std::tuple<float, uint, float> getPerformance(std::tuple<uint, uint, uint, uint, uint> counters, uint writeStrategy, uint size, uint blockSize, uint assoc);
void printPerformance(std::tuple<uint, uint, uint, uint, uint> counters, std::tuple<float, uint, float> performance);

int main(int argc, char const *argv[])
{
    // init the params
    uint blocksize = (uint)atoi(argv[1]);
    uint size = (uint)atoi(argv[2]);
    uint assoc = (uint)atoi(argv[3]);
    uint setsAmount = size/(assoc*blocksize);
    uint replaceStrategy = (uint)atoi(argv[4]);
    uint writeStrategy = (uint)atoi(argv[5]);
    std::string traceFilePath = "./traces/";
    traceFilePath.append(argv[6]);
    traceFilePath.append(".txt");

    // print params on the console.
    std::cout << "  ===== Simulator configuration =====" << std::endl;
    std::cout << "  L1_BLOCKSIZE:" << std::setw(22) << blocksize << std::endl;
    std::cout << "  L1_SIZE:" << std::setw(27) << size << std::endl;
    std::cout << "  L1_ASSOC:" << std::setw(26) << assoc << std::endl;
    std::cout << "  L1_REPLACEMENT_POLICY:" << std::setw(13) << replaceStrategy << std::endl;
    std::cout << "  L1_WRITE_POLICY:" << std::setw(19) << writeStrategy << std::endl;
    std::cout << "  trace_file:" << std::setw(24) << traceFilePath << std::endl;
    std::cout << "  ===================================\n" << std::endl;
    
    // init cache
    Cache* L1 = new Cache(size, blocksize, assoc, setsAmount, RS(replaceStrategy), WS(writeStrategy), RSM(replaceStrategy), nullptr);
    if(L1 == nullptr)
    {
        std::cout << "Failed to init the cache!" << std::endl;
        return 1;
    }

    // read file
    std::ifstream traceFile;
    traceFile.open(traceFilePath, std::ifstream::in);
    if(!traceFile)
    {
        std::cout << "Failed to open the trace-file" << std::endl;
        return 2;
    }
    std::string lineTemp;
    std::string delimiter = " ";
    std::string inst;
    uint addr;
    size_t pos;
    while(getline(traceFile, lineTemp))
    {
        pos = lineTemp.find(delimiter);
        inst = lineTemp.substr(0, pos);
        addr = stoi(lineTemp.substr(pos+1, lineTemp.length()), 0, 16);
        // std::cout << inst << " " << addr << std::endl;
        if(!inst.compare("r"))
        {
            L1->readFromAddress(addr);
        }
        else if(!inst.compare("w"))
        {
            L1->writeFromAddress(addr);
        }
        else
        {
            std::cout << "Wrong instruction! Instructions should be 'w' or 'r'!" << std::endl;
        }
    }
    traceFile.close();

    std::cout << "===== L1 contents =====" << std::endl;
    Set *setList = &L1->setList[0];
    for(uint i = 0; i < setsAmount; i++)
    {
        std::cout << "set   " << std::setw(5) << std::dec << i <<":";
        for(uint j = 0; j < assoc; j++){
            std::cout << "   " << std::hex << setList[i].head[j].addr/setsAmount;
            if(setList[i].head[j].isDirty)  std::cout << " D";
            else    std::cout << "  ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl << std::dec;
    
    // caculate and print the raw and the performance on the console
    std::tuple<uint, uint, uint, uint, uint> counters = L1->getCounters();
    std::tuple<float, uint, float> perfomance = getPerformance(counters, writeStrategy, size, blocksize, assoc);
    printPerformance(counters, perfomance);

    delete L1;
    
    return 0;
}

std::tuple<float, uint, float> getPerformance(std::tuple<uint, uint, uint, uint, uint> counters, uint writeStrategy, uint size, uint blockSize, uint assoc)
{
    float missRate, averageAT;
    int traffic;
    missRate = ((float)std::get<1>(counters)+std::get<3>(counters))/((float)(std::get<0>(counters)+std::get<2>(counters)));
    traffic = writeStrategy ? (std::get<1>(counters)+std::get<2>(counters)) : (std::get<1>(counters)+std::get<3>(counters)+std::get<4>(counters));
    float HT = 0.25 + 2.5 * (((float)size)/(512*1024)) + 0.025 * (((float)blockSize)/16) + 0.025 * (float)assoc;
    float MissPenalty = 20 + 0.5 * (((float)blockSize)/16);
    averageAT = HT + (missRate*MissPenalty);
    return std::make_tuple(missRate, traffic, averageAT);
}

void printPerformance(std::tuple<uint, uint, uint, uint, uint> counters, std::tuple<float, uint, float> performance)
{
    std::cout << "  ====== Simulation results (raw) ======" << std::endl;
    std::cout << "  a. number of L1 reads:" << std::setw(16) << std::get<0>(counters) << std::endl;
    std::cout << "  b. number of L1 read misses:" << std::setw(10) << std::get<1>(counters) << std::endl;
    std::cout << "  c. number of L1 writes:" << std::setw(15) << std::get<2>(counters) << std::endl;
    std::cout << "  d. number of L1 write misses:" << std::setw(9) << std::get<3>(counters) << std::endl;
    std::cout << "  e. L1 miss rate:" << std::setw(22) << std::fixed << std::setprecision(4) << std::get<0>(performance) << std::endl;
    std::cout << "  f. number of writebacks from L1:" << std::setw(6) << std::get<4>(counters) << std::endl;
    std::cout << "  g. total memory traffic:" << std::setw(14) << std::get<1>(performance) << std::endl;
    std::cout << std::endl;

    std::cout << "  ==== Simulation results (performance) ====" << std::endl;
    std::cout << "  1. average access time:" << std::setw(15) << std::fixed << std::setprecision(4) << std::get<2>(performance) << " ns";
}