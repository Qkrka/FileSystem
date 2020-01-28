#pragma once
#include "KernPart.h"
#include "fs.h"
#include "part.h"

typedef unsigned long ClusterNo;
const ClusterNo clusIndNum = 512ul;
const ClusterNo nullClusterNo = -1;  // init values
class File;

class KernFile {
    friend class KernPart;

    char mode;
    KernPart* part;
    BytesCnt size;

    BytesCnt cursor = 0;
    BytesCnt cursorClus = 0;
    BytesCnt cursorOff = 0;

    ClusterNo dataRelative = nullClusterNo;

    ClusterNo cacheDataCluster = nullClusterNo;
    ClusterNo cacheHelpCluster = nullClusterNo;
    ClusterNo cacheRootCluster = nullClusterNo;

    char rCache[ClusterSize];
    char hCache[ClusterSize]; 
    char dCache[ClusterSize];

    bool dirtyRoot = false;
    bool dirtyHelp = false;
    bool dirtyData = false;

    bool readByte(char* where);
    bool writeByte(char* ch);

    // void setcurr(BytesCnt n) {
    //     curr = n;
    //     currClus = curr / ClusterSize;
    //     currOffs = curr % ClusterSize;
    // }

public:
    FileControl* fc;
    KernFile(FileControl* fc, KernPart* p, char m);

    ~KernFile();

    char write(BytesCnt, char* buffer);
    BytesCnt read(BytesCnt, char* buffer);
    char seek(BytesCnt);
    BytesCnt filePos();
    char eof();
    BytesCnt getFileSize();
    char truncate();
};