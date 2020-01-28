
#include "bitvect.h"
#include <iostream>
#include "kernpart.h"

BitVector::BitVector(ClusterNo sz, KernPart* p) {
    part = p;
    size = sz;
    clusterNum = size / BitClusterSize;
    if (size % BitClusterSize > 0) clusterNum++;
    freeClusList = new list<ClusterNo>();
    bitVect = new char*[clusterNum];
    for (size_t i = 0; i < clusterNum; i++) {
        bitVect[i] = new char[ClusterSize];
        part->readCluster(i, (char*)bitVect[i]);
    }
    freeCl = clusterNum + 1;
    findFree();
}

void BitVector::findFree() {
    while (!(bitVect[freeCl / BitClusterSize][(freeCl % BitClusterSize) / 8] & ((1 << (7 - freeCl % 8))))) {
        freeCl = (freeCl + 1);
    }
    if (freeCl >= size) {
        cout << "nema mesta" << endl;
        exit(0);
    }
 
}

ClusterNo BitVector::getFirstEmpty() {

    if (!freeClusList->empty()) {
        ClusterNo tmp = freeClusList->front();
        freeClusList->pop_front();
        return tmp;

    }

    ClusterNo ret = freeCl;
    bitVect[freeCl / BitClusterSize][(freeCl % BitClusterSize) / 8] ^= (1 << (7 - freeCl % 8));
    findFree();
    return ret;
}

void BitVector::free(ClusterNo cl) {
    freeClusList->push_back(cl);
    bitVect[cl / BitClusterSize][(cl % BitClusterSize) / 8] |= (1 << (7 - cl % 8));
}

void BitVector::format() {

    for (size_t i = 0; i < clusterNum; i++)
        for (int j = 0; j < ClusterSize; j++) bitVect[i][j] = -1;

	for (int i = 0; i <= clusterNum; i++)
		bitVect[0][i / 8] ^= (1 << (7 - i % 8));

    freeCl = clusterNum + 1;
    freeClusList->clear();
}

void BitVector::writeToDisk() {
    for (size_t i = 0; i < clusterNum; i++) part->writeCluster(i, bitVect[i]);
}

ClusterNo BitVector::getRootPosition() { return clusterNum; }

BitVector::~BitVector() {
    for (size_t i = 0; i < clusterNum; i++) {
        part->writeCluster(i, bitVect[i]);
        delete bitVect[i];
    }
    delete freeClusList;
    delete bitVect;
}
