#pragma once
#include <iostream>
#include "Windows.h"
using namespace std;

#define signal(x) ReleaseSemaphore(x, 1, NULL)
#define wait(x) WaitForSingleObject(x, INFINITE)
#define signalAndWait(x, y) SignalObjectAndWait(x, y, INFINITE, false);
typedef HANDLE Sem;

class DummySem
{
public:
    DummySem(Sem* s)
    {
        dumSem = s;
        wait(*dumSem);
    }
    ~DummySem()
    {
        signal(*dumSem);
    }
private:
    Sem* dumSem;
};