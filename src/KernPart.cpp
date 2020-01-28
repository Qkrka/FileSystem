#include "KernPart.h"
#include "KernFile.h"
#include "file.h"

#include <iostream>
using namespace std;

FileControl::FileControl(char mode, char *n, Entry *ent, ClusterNo pos, BytesCnt off)
{
    fname = new char[strlen(n) + 1];
    strncpy(fname, n, strlen(n) + 1);
    readSem = CreateSemaphore(0, 0, 100, 0);
    writeSem = CreateSemaphore(0, 0, 1, 0);
    entry = new Entry();
    strncpy(entry->name, ent->name, 8);
    strncpy(entry->ext, ent->ext, 3);
    entry->size = ent->size;
    entry->indexCluster = ent->indexCluster;
    entry->reserved = ent->reserved;
    lvl2Clus = pos;
    entryIndex = off;
    writers = 0;
    readers = 0;
    writersWaiting = 0;
    readersWaiting = 0;

    if (mode == 'r')
    {
        readers++;
    }
    if (mode == 'w' || mode == 'a')
    {
        writers++;
    }
}

void KernPart::close(KernFile *kernfile)
{
    DummySem d(&mutex);

    unsigned long sz = kernfile->getFileSize();
    if (sz != kernfile->fc->getEntry()->size)
    {
        kernfile->fc->getEntry()->size = sz;
        char temp[ClusterSize];
        readCluster(kernfile->fc->lvl2Clus, temp);
        ((Entry *)(temp))[kernfile->fc->entryIndex].size = sz;
        writeCluster(kernfile->fc->lvl2Clus, temp);
    }
    string name = kernfile->fc->fname;
    map<string, FileControl *>::iterator it = mapOpen.find(name);
    if (it != mapOpen.end() && it->second)
    {
         /*if (it->second->readers == 1 && it->second->writers == 0 && it->second->writersWaiting == 0) {
             delete (it->second);
             mapopen.erase(it);
             if (mapopen.empty() && toDestroy) signal(deletesem);
             return;
         }
         else if (it->second->writers == 1 && it->second->readers == 0 &&
                  it->second->writersWaiting == 0 && it->second->readersWaiting == 0)
         {
             delete (it->second);
             mapopen.erase(it);
             if (mapopen.empty() && toDestroy)
                 signal(deletesem);
             return;
         }*/

        if (it->second->readers > 0)
        {
            it->second->readers--;
            if (it->second->readers == 0)
                if (it->second->writersWaiting > 0)
                {
                    it->second->writersWaiting--;
                    signal(it->second->writeSem);
                }
                else
                {
                    delete (it->second);
                    mapOpen.erase(it);
                    if (mapOpen.empty() && toDestroy)
                        signal(deleteSem);
                }
            return;
        }
        else
        {
            it->second->writers--;
            if (it->second->writersWaiting > 0)
            {
                it->second->writersWaiting--;
                signal(it->second->writeSem);
            }
            else
            {
                if (it->second->readersWaiting > 0)
                {
                    while (it->second->readersWaiting > 0)
                    {
                        it->second->readersWaiting--;
                        signal(it->second->readSem);
                    }
                }
                else
                {
                    delete (it->second);
                    mapOpen.erase(it);
                    if (mapOpen.empty() && toDestroy)
                        signal(deleteSem);
                }
            }
        }
    }
}

KernPart::KernPart(Partition *part)
{
    this->part = part;
    mutex = CreateSemaphore(0, 1, 1, 0);
    deleteSem = CreateSemaphore(0, 0, 1, 0);
    bitVectSem=CreateSemaphore(0,1,1,0);
    bitVect = new BitVector(part->getNumOfClusters(), this);
    part->readCluster(bitVect->getRootPosition(), rootCache);
    dirtyRoot=false;
    toDestroy = false;
}

char KernPart::format()
{
    DummySem d(&mutex);
    bool old = toDestroy;
    toDestroy = true;
    wait(bitVectSem);
    bitVect->format();
    signal(bitVectSem);
    for (int i = 0; i < ClusterSize; i++)
        rootCache[i] = 0;
    dirtyRoot=true;
    toDestroy = old;
    return 1;
}

bool KernPart::mark()
{
    DummySem d(&mutex);
    if (toDestroy)
        return false;
    toDestroy = true;
    if (mapOpen.empty())
        return true;
    signalAndWait(mutex, deleteSem);
    wait(mutex);
    return true;
}

// int KernPart::readCluster(ClusterNo n, char *buffer) { return part->readCluster(n, buffer); }

// ClusterNo KernPart::getNumOfClusters() const { return part->getNumOfClusters(); }

// int KernPart::writeCluster(ClusterNo n, const char *buffer)
// {
//     return part->writeCluster(n, buffer);
// }

FileControl *KernPart::makeNewFile(char *fname, char mode)
{
    Entry *entryArray;
    ClusterNo *clusterArray;
    char lvl1Buffer[ClusterSize];
    char lvl2Buffer[ClusterSize];
    for (ClusterNo i = 0; i < ClusterRootNo; i++)
    {
        if (rootCache[i] != 0)
        {
            readCluster(rootCache[i], lvl1Buffer);
            clusterArray = (ClusterNo *)lvl1Buffer;
            for (ClusterNo j = 0; j < ClusterRootNo; j++){
                if (clusterArray[j] != 0)
                {
                    readCluster(clusterArray[j], lvl2Buffer);
                    entryArray = (Entry*)lvl2Buffer;
                    for (ClusterNo k = 0; k < ClusterEntryNo; k++)
                        if (freeEntry(entryArray + k))
                        {
                            addEntry(fname, lvl2Buffer, clusterArray[j], k);
                            return new FileControl(mode, fname, entryArray + k, clusterArray[j], k);
                        }
                }
                else
                {
                    ClusterNo clusterlvl2 = claimCluster(lvl1Buffer, lvl2Buffer, rootCache[i], j);
                    entryArray = (Entry *)lvl2Buffer;
                    addEntry(fname, lvl2Buffer, clusterlvl2, 0);
                    return new FileControl(mode, fname, entryArray, clusterlvl2, 0);
                }
            }
        }
        else
        {
            wait(bitVectSem);
            ClusterNo cl = bitVect->getRootPosition();
            signal(bitVectSem);
            ClusterNo clNo = claimCluster(rootCache, lvl1Buffer, cl, i);

            ClusterNo clusterlvl2 = claimCluster(lvl1Buffer, lvl2Buffer, clNo, 0);
            entryArray = (Entry *)lvl2Buffer;
            addEntry(fname, lvl2Buffer, clusterlvl2, 0);
            dirtyRoot=true;
            return new FileControl(mode, fname, entryArray, clusterlvl2, 0);
        }
    }
    return nullptr;
}

FileControl *KernPart::doesExistFC(char *fname, char mode)
{
    Entry *entryArray;
    ClusterNo *clusterArray;
    char lvl1Buffer[ClusterSize];
    char lvl2Buffer[ClusterSize];
    for (ClusterNo i = 0; i < ClusterRootNo; i++)
    {
        if (rootCache[i] != 0)
        {
            readCluster(rootCache[i], lvl1Buffer);
            clusterArray = (ClusterNo *)lvl1Buffer;
            for (ClusterNo j = 0; j < ClusterRootNo; j++)
                if (clusterArray[j] != 0)
                {
                    readCluster(clusterArray[j], lvl2Buffer);
                    entryArray = (Entry *)lvl2Buffer;
                    for (ClusterNo k = 0; k < ClusterEntryNo; k++)
                        if (nameCmp(entryArray + k, fname))
                            return new FileControl(mode, fname, entryArray + k, clusterArray[j], k);
                }
        }
    }
    return nullptr;
}

bool KernPart::doesExistBool(char *fname)
{
    FileControl* fc=doesExistFC(fname, 0);
    bool ret=false;
    if(fc){
        ret=true;
    }
    delete fc;
    return ret;
}

KernFile* KernPart::openFile(char *fname, char mode)
{
    DummySem d(&mutex);
    if (toDestroy) {
        return nullptr;
    }

    map<string, FileControl *>::iterator it = mapOpen.find(fname);
    if (it != mapOpen.end() && it->second)
    {
        if (mode == 'r')
        {
            if (it->second->writers == 1)
            {
                it->second->readersWaiting++;
                signalAndWait(mutex, it->second->readSem);
                wait(mutex);
            }
            it->second->readers++;
            return new KernFile(it->second, this, mode);
        }
        if (mode == 'w' || mode == 'a')
        {
            if (it->second->readers > 0 || it->second->writers == 1)
            {
                it->second->writersWaiting++;
                signalAndWait(mutex, it->second->writeSem);
                wait(mutex);
            }
            it->second->writers++;
            return new KernFile(it->second, this, mode);
        }
        return nullptr;
    }

    if (FileControl *myFileControl = doesExistFC(fname, mode))
    {
        mapOpen.insert(pair<char *, FileControl *>(fname, myFileControl));
        return new KernFile(myFileControl, this, mode);
    }

    if (mode == 'w')
    {
        if (FileControl *myFileControl = makeNewFile(fname, mode))
        {
            mapOpen.insert(pair<char *, FileControl *>(fname, myFileControl));
            return new KernFile(myFileControl, this, mode);
        }
    }
    return nullptr;
}

bool KernPart::deleteFile(char *fname)
{
    DummySem d(&mutex);
    FileControl* fc=nullptr;
    map<string, FileControl *>::iterator it = mapOpen.find(fname);
    if (it != mapOpen.end() && it->second)
    {
        return false;
    }

    fc=doesExistFC(fname, 'w');
    if(!fc) return false;

    KernFile * myFile=new KernFile(fc,this,'w');
    delete myFile;

    wait(bitVectSem);
    bitVect->free(fc->entry->indexCluster);
    signal(bitVectSem);

    char buffer[ClusterSize];
    readCluster(fc->lvl2Clus, buffer);
    for(ClusterNo i=fc->entryIndex*32; i<fc->entryIndex*32+8;i++)
        buffer[i]=0;
    writeCluster(fc->lvl2Clus, buffer);

    delete fc;
    return true;
}

bool KernPart::freeEntry(Entry *ent)
{
    for (int i = 0; i < 8; i++)
        if (ent->name[i] != 0)
            return false;
    return true;
}

void KernPart::addEntry(char *fname, char *buffer, ClusterNo pos, ClusterNo off)
{
    int k = 0;
    Entry *entry = (Entry *)buffer;
    bool flag = false;
    for (int i = 0; i < fileNameLen; i++)
    {
        if (i < strlen(fname) && fname[i] == '.')
        {
            flag = true;
            k = i + 1;
        }
        if (!flag)
            entry[off].name[i] = fname[i];
        else
            entry[off].name[i] = ' ';
    }
    for (unsigned i = 0; i < fileExtLen; i++)
        if (k >= strlen(fname))
            entry[off].ext[i] = ' ';
        else
            entry[off].ext[i] = fname[k++];

    wait(bitVectSem);
    entry[off].indexCluster = bitVect->getFirstEmpty();
    signal(bitVectSem);
    entry[off].size = 0;
    writeCluster(pos, buffer);
}

ClusterNo KernPart::claimCluster(char *bufferRoot, char *buffer, ClusterNo pos, ClusterNo off)
{
    wait(bitVectSem);
    ClusterNo k = bitVect->getFirstEmpty();
    signal(bitVectSem);

    ((ClusterNo *)bufferRoot)[off] = k;
    writeCluster(pos, bufferRoot);
    for (ClusterNo i = 0; i < ClusterSize; i++)
        buffer[i] = 0;
    return k;
}

char KernPart::readRootDir()
{
    DummySem d(&mutex);

    Entry *entryArray;
    ClusterNo *clusterArray;
    char lvl1Buffer[ClusterSize];
    char lvl2Buffer[ClusterSize];
    EntryNum cnt = 0;
    for (ClusterNo i = 0; i < ClusterRootNo; i++)
        if (rootCache[i] != 0)
        {
            readCluster(rootCache[i], lvl1Buffer);
            clusterArray = (ClusterNo *)lvl1Buffer;
            for (ClusterNo j = 0; j < ClusterRootNo; j++)
                if (clusterArray[j] != 0)
                {
                    readCluster(clusterArray[j], lvl2Buffer);
                    entryArray = (Entry *)lvl2Buffer;
                    for (ClusterNo k = 0; k < ClusterEntryNo; k++)
                        if (!freeEntry(entryArray + k))
                            cnt++;
                }
        }
    return (char)cnt;
}

ClusterNo KernPart::getEmpty() { 
    wait(bitVectSem);
    ClusterNo v=bitVect->getFirstEmpty(); 
    signal(bitVectSem);
    return v;
}

void KernPart::free(ClusterNo clno) { 
    wait(bitVectSem);
    bitVect->free(clno); 
    signal(bitVectSem);
}

KernPart::~KernPart()
{
    wait(bitVectSem);
    if(dirtyRoot) writeCluster(bitVect->getRootPosition(), rootCache);
    signal(bitVectSem);
    delete bitVect;
}

bool KernPart::nameCmp(Entry* ent, char* fname){
    int  k = 0;
    bool help = false;
    for (int i = 0; i < 8; i++)
    {
		if (i < strlen(fname) && fname[i] == '.')
		{
			help = true;
			k = i + 1;
		}
		if (!help && ent->name[i] != fname[i])
			return false;
		if (help && ent->name[i] != ' ')
			return false;
    }
    for (int i = 0; i < 3; i++)
    {
		if (k >= strlen(fname))
			return false;
		if (ent->ext[i] != fname[k++])
			return false;
    }
    return true;
}