#include <stdio.h>
#include <stdlib.h>
#include <compute.h>


int main(int argc, char ** argv) 
{
    unsigned input = 1;
    for (int i = 0; i < 4; i++)
    {
        unsigned val = compute (input);
        printf ("[%d]val = %u\r\n", i, val);
    }
    return 0;
}




