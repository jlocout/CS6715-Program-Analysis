#include <graph.h>
 

int main() 
{
    CallGraph *cg = new CallGraph ("CallGraph");
    cg->showName ();

    delete cg;
}

