#include <stdio.h>
#include <stdlib.h>

unsigned compute (unsigned val)
{
    unsigned sVal = 1;
    printf ("@compute: &val = %p \r\n", &val);   
    sVal = sVal + val;
    return sVal;
}





