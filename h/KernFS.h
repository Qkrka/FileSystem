// File: fs.h
#pragma once
#include "FS.h"
#include "synch.h"

class KernPart;

struct Entry {
    char name[fileNameLen];
    char ext[fileExtLen];
    char reserved;
    unsigned long indexCluster;
    unsigned long size;
    char extra[12] = {0};
};

class KernFS {
    KernPart* kernpart;
    Sem mountedsem;
    Sem mutex;
public:
    KernFS();
    ~KernFS();

    char mount(Partition* partition);  // montira particiju
    // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
    char unmount();  // demontira particiju
                     // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
    char format();   // formatira particiju;
                     // vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha
    FileCnt readRootDir();
    // vraca -1 u slucaju neuspeha ili broj fajlova u slucaju uspeha
    char doesExist(char* fname);  // argument je naziv fajla sa
                                  // apsolutnom putanjom

    File* open(char* fname, char mode);
    char deleteFile(char* fname);
};
