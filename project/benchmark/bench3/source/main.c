#include <stdio.h>

void C ()
{
    printf("C\n");
}

void B ()
{
    printf("B\n");
    C ();
}

void (*func) () = B;
 
int main(int argc, char* argv[]) 
{
    if (argc < 1) 
    {
        func ();
    }
    else
    {
        C ();
    }
}

