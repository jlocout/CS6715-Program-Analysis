#ifndef __GRAPH_H__
#define __GRAPH_H__
#include <iostream>
#include <map>

using namespace std;

class Graph 
{
public:
    Graph (string name) {
        gName = name;
    }
    ~Graph () {}

    virtual void showName () {
        cout<<"@Graph: my name is " + gName<<endl;
    }
private:
    string gName;
    static string version;
};

class CallGraph : public Graph 
{
public:
    CallGraph(string name): Graph (name) {}
    ~CallGraph() {}

    void showName () {
        cout<<"@CallGraph:\n";
        Graph::showName ();
    }
};

#endif
