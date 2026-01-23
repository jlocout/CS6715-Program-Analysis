#include <stdio.h>

int sum (int x)
{
    int sum = 0;

    if (x > 0)
    {
        x = x * 2;
    }   
    else
    {
        x = x - 1;
    }

    for (int i = 0; i < x; i++)
        sum += x;
    return sum;
}

int main(int argc, char* argv[]) 
{
    int a = argc;
    return sum(a);
}

