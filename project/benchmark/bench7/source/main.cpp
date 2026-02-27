#include <iostream>
using namespace std;

typedef void (*FuncPtr)();

FuncPtr call1 = NULL;
FuncPtr call2 = NULL;

void C ()
{
    printf("C\n");
}

void B ()
{
    printf("B\n");
    C ();
}

void A ()
{
    printf("A\n");
}

void SetFptr(int argc)
{
    if (argc < 1) 
    {
        call2 = B;
    }
    else
    {
        call2 = C;
    }
}
 
int main(int argc, char* argv[]) 
{
    call1 = A;
    if (argc < 1) 
    {
        call1 ();
    }
    else
    {
        SetFptr(argc);
        call2 ();
    }
}