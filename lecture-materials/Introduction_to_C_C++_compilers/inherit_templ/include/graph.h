#ifndef __GRAPH_H__
#define __GRAPH_H__
#include <iostream>
#include <string>
#include <vector>

using namespace std;

template <typename NodeT, typename EdgeT>
class Graph
{
public:
    Graph(const string& name) : gName(name) {}
    virtual ~Graph() {}

    virtual void showName() {
        cout << "@Graph: my name is " << gName << endl;
    }

    void addNode(const NodeT& node) {
        nodes.push_back(node);
    }

    void addEdge(const EdgeT& edge) {
        edges.push_back(edge);
    }
private:
    string gName;
    vector<NodeT> nodes;
    vector<EdgeT> edges;
};

class CallGraph: public Graph<int, int>
{
public:
    CallGraph (const string& name): Graph (name) {}
    ~CallGraph () {}
};








#endif
