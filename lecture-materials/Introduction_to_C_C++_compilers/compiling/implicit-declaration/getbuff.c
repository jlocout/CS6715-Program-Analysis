#include <stdlib.h>
#include <assert.h>

static char *buffer = NULL;

unsigned long getBuffer ()
{
    buffer = (char*)malloc (1024 * 1024);
    assert (buffer != NULL);
    printf ("%p\n", buffer);
    return (unsigned long)buffer;
}


