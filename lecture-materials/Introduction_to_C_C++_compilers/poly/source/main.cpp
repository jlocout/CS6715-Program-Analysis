#include <graph.h>
 
string Graph::version = "v100.0";

int main() 
{
    Graph* g = new CallGraph ("CallGraph");
    g->showName ();

    delete g;
}

