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

    void showName () {
        cout<<"my name is" + gName<<endl;
    }
private:
    string gName;
};

#endif
