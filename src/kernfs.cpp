#include "kernfs.h"
#include <stdio.h>
#include <iostream>
#include "File.h"
#include "KernPart.h"
#include "synch.h"



KernFS::KernFS() : kernpart(nullptr) {
    mountedsem = CreateSemaphore(0, 1, 1, NULL);  // samo jedan montira
    mutex = CreateSemaphore(0, 1, 1, 0);
}

KernFS::~KernFS() {
    unmount();
}

char KernFS::mount(Partition* partition) {
    wait(mutex);
    if (partition == nullptr) return 0;
    signalAndWait(mutex, mountedsem);
    wait(mutex);
    kernpart = new KernPart(partition);
    signal(mutex);
    return 0;
}

char KernFS::unmount() {
    DummySem d(&mutex);
    if (kernpart == nullptr || !kernpart->mark()) return 0;
    delete kernpart;
    //kernpart = nullptr;
    signal(mountedsem);
    return 0;
}

char KernFS::format() {
    DummySem d(&mutex);
    if (kernpart == nullptr) return 0;
    return kernpart->format();
}

FileCnt KernFS::readRootDir() {
    DummySem d(&mutex);
    if (kernpart == nullptr) return 0;
    return kernpart->readRootDir();
}

char KernFS::doesExist(char* fname) {
    DummySem d(&mutex);
    if (kernpart == nullptr) return 0;
    return kernpart->doesExistBool(fname);
}

File* KernFS::open(char* fname, char mode) {
    DummySem d(&mutex);
    if (kernpart == nullptr) return nullptr;
    KernFile* kFile= kernpart->openFile(fname, mode);
    if(kFile)
    {
        File* myFile=new File();
        myFile->myImpl=kFile;
        return myFile;
    }
    return nullptr;
}
char KernFS::deleteFile(char* fname) {
    DummySem d(&mutex);
    if (kernpart == nullptr) return 0;
    return kernpart->deleteFile(fname);
}
