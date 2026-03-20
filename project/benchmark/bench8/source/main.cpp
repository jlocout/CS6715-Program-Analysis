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

void SetFptr(FuncPtr& fptr, int argc)
{
    if (argc < 1) 
    {
        fptr = B;
    }
    else
    {
        fptr = C;
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
        SetFptr(call2, argc);
        call2 ();
    }
}