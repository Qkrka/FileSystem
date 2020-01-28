#ifndef BITVECT_H
#define BITVECT_H

#include <stdio.h>
#include <list>
#include "part.h"
#include "synch.h"

const unsigned long BitClusterSize = 2048 * 8;
typedef unsigned char byte;
using namespace std;

class KernPart;
class KernFile;

class BitVector {
public:
    BitVector(ClusterNo size, KernPart* p);
    ClusterNo getFirstEmpty();
    void free(ClusterNo);
    void format();
    void writeToDisk();
    ~BitVector();

    ClusterNo getRootPosition();

private:
    Sem mutex;
    void findFree();

    //void setFree(ClusterNo cl) {
    //    bitVect[cl / BitClusterSize][(cl % BitClusterSize) / 8] |= (1 << (7 - cl % 8));
    //}

    //void setNotFree(ClusterNo cl) {
    //    bitVect[cl / BitClusterSize][(cl % BitClusterSize) / 8] ^= (1 << (7 - cl % 8));

    //    // bitVect[freeCl / BitClusterSize][(freeCl % BitClusterSize) / 8] ^= (1 << (7 - freeCl %
    //    // 8)); cout << std::bitset<8> x(bitVect[freeCl / BitClusterSize][(freeCl % BitClusterSize)
    //    // / 8]);
    //}
    ClusterNo size;
    ClusterNo clusterNum;

    char** bitVect;

    KernPart* part;

    list<ClusterNo>* freeClusList;
    ClusterNo freeCl;

    // friend KernelFile;
    // friend PartitionKernel;
};
#endif