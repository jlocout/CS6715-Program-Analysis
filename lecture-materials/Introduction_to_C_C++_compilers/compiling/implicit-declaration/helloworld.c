#include <stdio.h>

#define HW_MACRO "Hello, World!"

int main() 
{
    char* buffer = (char*)getBuffer();
    printf ("%p\n", buffer);
    strcpy (buffer, HW_MACRO);
    printf("%s\n", buffer);
    return 0;
}


