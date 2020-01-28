#include "KernFile.h"

KernFile::KernFile(FileControl *f, KernPart *p, char m)
{
    part=p;
    mode=m;
    fc=f;
    size=fc->entry->size;
    cacheRootCluster = fc->getEntry()->indexCluster;
    part->readCluster(cacheRootCluster, rCache);
    if (mode == 'a')
        seek(size);
    else
        seek(0);
    if (mode == 'w')
        truncate();
}

KernFile::~KernFile()
{
    if (dirtyRoot)
        part->writeCluster(cacheRootCluster, rCache);
    if (dirtyHelp)
        part->writeCluster(cacheHelpCluster, hCache);
    if (dirtyData)
        part->writeCluster(cacheDataCluster, dCache);

    part->close(this);
}
BytesCnt KernFile::filePos() { return cursor; }

char KernFile::eof()
{
    if (cursor > size)
        return 1;
    if (cursor == size)
        return 2;
    return 0;
}

char KernFile::seek(BytesCnt b)
{
    /*if (b > size)
        b = size;*/
    cursor = b;
    cursorClus = cursor / ClusterSize;
    cursorOff = cursor % ClusterSize;
    return 1;
}

BytesCnt KernFile::getFileSize() { return size; }

bool KernFile::readByte(char *ch)
{

    if (cursor >= size)
        return false;
    if (cursorClus == dataRelative)
    {
        *ch = dCache[cursorOff];
        seek(cursor + 1);
        return true;
    }
    ClusterNo ind1 = cursorClus / clusIndNum;
    ClusterNo t = ((ClusterNo *)rCache)[ind1];
    if (cacheHelpCluster != t)
    {
        if (dirtyHelp)
        {
            part->writeCluster(cacheHelpCluster, hCache);
        }
        part->readCluster(t, hCache);
        cacheHelpCluster = t;
        dirtyHelp = false;
    }
    ClusterNo lvl2ind = cursorClus % clusIndNum;
    t = ((ClusterNo *)hCache)[lvl2ind];
    if (dirtyData)
    {
        part->writeCluster(cacheDataCluster, dCache);
    }
    part->readCluster(t, dCache);
    cacheDataCluster = t;
    dirtyData = false;
    *ch = dCache[cursorOff];
    dataRelative = cursorClus;
    seek(cursor + 1);
    return true;
}

BytesCnt KernFile::read(BytesCnt b, char *buffer)
{
    BytesCnt i;
    for (i = 0; i < b && readByte(buffer + i); i++)
        ;
    return i;
}

bool KernFile::writeByte(char *ch)
{
    if (cursorClus == dataRelative)
    {
        dCache[cursorOff] = *ch;
        dirtyData = true;
        if (cursor == size)
            size++;
        seek(cursor + 1);
        return true;
    }

    ClusterNo ind1 = cursorClus / clusIndNum;
    ClusterNo t = ((ClusterNo *)rCache)[ind1];

    if ((size == cursor) && (size % (ClusterSize * clusIndNum) == 0))
    {
        t = part->getEmpty();
        ((ClusterNo *)rCache)[ind1] = t;
        dirtyRoot = true;
    }
    if (cacheHelpCluster != t)
    {
        if (dirtyHelp)
        {
            part->writeCluster(cacheHelpCluster, hCache);
        }
        cacheHelpCluster = t;
        part->readCluster(cacheHelpCluster, hCache);
        dirtyHelp = false;
    }
    ClusterNo lvl2ind = cursorClus % clusIndNum;
    t = ((ClusterNo *)hCache)[lvl2ind];
    if (cursor == size && size % ClusterSize == 0)
    {
        t = part->getEmpty();
        ((ClusterNo *)hCache)[lvl2ind] = t;
        dirtyHelp = true;
    }
    if (cacheDataCluster != t)
    {
        if (dirtyData)
        {
            part->writeCluster(cacheDataCluster, dCache);
        }
        cacheDataCluster = t;
        part->readCluster(cacheDataCluster, dCache);
        dirtyData = false;
    }
    part->readCluster(cacheDataCluster, dCache);
    dCache[cursorOff] = *ch;
    dirtyData = true;
    dataRelative = cursorClus;
    if (size == cursor) size++;
    seek(cursor + 1);
    return true;
}

char KernFile::write(BytesCnt b, char *buffer)
{
    BytesCnt i=0;
    for (i = 0; i < b; i++)
    {
        writeByte(buffer+i);
    }
    return (i == b);
}

char KernFile::truncate()
{
    seek(0);
    ClusterNo sizeClus = size / ClusterSize;
    if(size%ClusterSize!=0)sizeClus++;

    if(dirtyHelp)part->writeCluster(cacheHelpCluster, hCache);
    dirtyHelp = false;
    for (ClusterNo cursorClus=0; cursorClus < sizeClus; cursorClus++)
    {
        if (cursorClus % clusIndNum == 0)
        {
            ClusterNo rootDelNumlvl2 = cursorClus / clusIndNum;
            part->readCluster(((ClusterNo *)rCache)[rootDelNumlvl2], hCache);
        }
        part->free(((ClusterNo *)hCache)[cursorClus % clusIndNum]);
    }
    sizeClus = sizeClus / ClusterSize;
    if (sizeClus % ClusterSize != 0)sizeClus++;
    for (ClusterNo cursorClus=0; cursorClus < sizeClus; cursorClus++){
        part->free(((ClusterNo *)rCache)[cursorClus]);
    }
    size=0;
    seek(0);
    return 1;
}
