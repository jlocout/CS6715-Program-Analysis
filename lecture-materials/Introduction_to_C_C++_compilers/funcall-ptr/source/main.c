#include <stdio.h>
#include <stdlib.h>
#include <compute.h>


int main(int argc, char ** argv) 
{
    unsigned input = 1;
    printf ("@main: &input = %p \r\n", &input);   
    for (int i = 0; i < 4; i++)
    {  
        unsigned val = compute (&input);
    }
    return 0;
}




