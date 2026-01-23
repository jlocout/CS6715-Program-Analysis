#include <stdio.h>
#include <stdlib.h>

unsigned Getpasswd (unsigned char Key);

void FuncA (char SEED [32], int Index)
{
    printf ("@@ start FuncA\r\n");
    sprintf (SEED, "test-%u", Index);
            
    FILE *F = fopen (SEED, "wb");
    unsigned value = (unsigned)random ()%18;
    printf ("value = %u \r\n", value);
    fwrite (&value, 1, sizeof (unsigned), F);
    fclose(F);
    printf ("@@ end FuncA\r\n");
}


unsigned FuncB (unsigned char Value)
{
    printf ("@@ start FuncB\r\n");
    unsigned Pwd = 0;
    /* y = x*x + 5x + 1*/
    unsigned FValue = Value *Value + 5*Value - 100;
    switch (FValue)
    {
        case 0:
        {
            Pwd = 0;
            break;
        }
        case 65535:
        {
            Pwd = 2;
            break;
        }
        case 999999:
        {
            Pwd = Getpasswd (4);
            break;
        }
        default:
        {
            Pwd = Getpasswd (4);
            printf ("@@ end FuncB\r\n");
            exit (0);
        }
    }
    printf ("@@ end FuncB\r\n");
    return Pwd;
}


int main(int argc, char ** argv) 
{
    printf ("@@ start main\r\n");
    if (argc == 1)
    {
        int Index = 0;
        char SEED [32];

        while (Index < 32)
        {
            FuncA (SEED, Index);
            Index++;
        }

        printf ("@@ end main\r\n");
        return 0;
    }
    
    unsigned Value = (unsigned)atoi(argv[1]);
    unsigned Pwd = 0;

    printf ("@@ Value = %u\r\n", Value);
    if (Value >= 4 && Value <= 16)
    {
        Pwd = Getpasswd (Value);
    }
    else
    {
        Pwd = FuncB (Value);        
    }
    
    printf ("@@ end main\r\n");
    return Pwd;
}




