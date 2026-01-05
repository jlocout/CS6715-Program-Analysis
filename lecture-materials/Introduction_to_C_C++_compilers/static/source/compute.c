#include <stdio.h>
#include <stdlib.h>

unsigned compute (unsigned val)
{
    static unsigned sVal = 1;

    sVal = sVal + val;
    return sVal;
}





