#pragma once
#include <list>
#include <map>
#include <string>
#include "BitVect.h"
#include "FS.h"
#include "KernFS.h"
#include "part.h"
#include <string>
#include "synch.h"

typedef unsigned long EntryNum;
const unsigned long ClusterEntryNo = 2048 / 32;
const unsigned long ClusterDataNo = 2048;
const unsigned long ClusterRootNo = 512;

class FileControl {
    friend class KernPart;
    friend class KernFile;
    unsigned long writers;
    unsigned long readers;
    unsigned long writersWaiting;
    unsigned long readersWaiting;
    Sem writeSem;
    Sem readSem;
    char* fname;

    Entry* entry;
    ClusterNo lvl2Clus;
    ClusterNo entryIndex;
    FileControl(char mode, char*, Entry*, ClusterNo, BytesCnt);

    Entry* getEntry() { return entry; }
};

class KernPart {
private:
    friend class KernFile;
    Partition* part;

public:
    Sem mutex;
    Sem deleteSem;
    Sem bitVectSem;

    bool toDestroy = false;
    map<string, FileControl*> mapOpen;
    BitVector* bitVect;
    char rootCache[ClusterSize];
    bool dirtyRoot;

    void writeCluster(ClusterNo c, char* b){
        part->writeCluster(c,b);
    };
    void readCluster(ClusterNo c, char* b){
        part->readCluster(c,b);
    };
    KernPart(Partition* p);
    ~KernPart();
    bool doesExistBool(char*);
    FileControl* doesExistFC(char*, char);
    bool nameCmp(Entry*, char*);

    ClusterNo getEmpty();
    void free(ClusterNo clno);
    void close(KernFile* kernfile);

    FileControl* makeNewFile(char* fname, char mode);
    void addEntry(char*, char*, ClusterNo, ClusterNo);
    KernFile* openFile(char*, char);
    ClusterNo claimCluster(char*, char*, ClusterNo, ClusterNo);

    bool deleteFile(char*);
    bool freeEntry(Entry*);
    char readRootDir();
    ClusterNo getNewEmpty();
    char format();
    bool mark();
};
