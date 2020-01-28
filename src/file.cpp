#include "file.h"

class KernFile;

File::File() {}

File::~File() { delete myImpl; }

char File::write(BytesCnt b, char* buffer) { return myImpl->write(b, buffer); }

BytesCnt File::read(BytesCnt b, char* buffer) { return myImpl->read(b, buffer); }

char File::seek(BytesCnt b) { return myImpl->seek(b); }

BytesCnt File::filePos() { return myImpl->filePos(); }

char File::eof() { return myImpl->eof(); }

BytesCnt File::getFileSize() { return myImpl->getFileSize(); }

char File::truncate() { return myImpl->truncate(); }
