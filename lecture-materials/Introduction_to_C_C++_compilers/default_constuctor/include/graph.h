#ifndef __GRAPH_H__
#define __GRAPH_H__
#include <iostream>
#include <map>

using namespace std;

class Graph 
{
public:
    Graph () {
        gName = "";
        cout << "Default Graph constructor\n";
    }
    Graph (string name) {
        gName = name;
        cout << "Graph constructor\n";
    }
    ~Graph () {}

    void showName () {
        cout<<"my name is " + gName<<endl;
    }
private:
    string gName;
    static string version;
};

class CallGraph : public Graph 
{
public:
    CallGraph(string name) {
        cout << "CallGraph constructor\n";
    }
    ~CallGraph() {}
};

#endif
