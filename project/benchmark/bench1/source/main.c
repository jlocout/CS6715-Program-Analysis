#include <stdio.h>

int g = 7;

int main(int argc, char **argv) 
{
    if (argc < 1) 
    {
        return -1;
    }

    int x = 0;
    x = x + g;
    
    return x;
}
