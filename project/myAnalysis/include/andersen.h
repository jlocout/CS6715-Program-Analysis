#ifndef _ANDERSEN_H_
#define _ANDERSEN_H_
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "pta.h"

using namespace std;

class Andersen: public PTA
{
public:
    Andersen (PAG *graph): PTA(graph), initialized (false) {} 
    ~Andersen () {}

    void runAnalysis(vector<PAGNode*>& imptNodes)
    {
        newFuncPTSs.clear ();
        for (auto it = imptNodes.begin (); it != imptNodes.end (); it++)
        {
            worklist.push (*it);
        }

        initializePTS();

        solveConstraints();

        refinePAG ();
    }

private:
    bool initialized;

private:
    // 1) Initialize sets from ADDR_OF edges
    void initializePTS()
    {
        if (initialized == true)
        {
            return;
        }

        for (auto it = pag->begin (); it != pag->end (); it++) 
        {
            PAGNode *node = it->second;
            ptsMap[node] = PTS ();
        }

        for (auto it = pag->begin (); it != pag->end (); it++) 
        {
            PAGNode *node = it->second;

            for (auto itEdge = node->inEdgeBegin(); itEdge != node->inEdgeEnd(); itEdge++)
            {
                PAGEdge* edge = *itEdge;
                if (edge->cstType != CST_ADDR_OF) 
                {
                    continue;
                }

                PAGNode *src = edge->getSrcNode();
                bool changed = ptsMap[node].insert(src);
                if (changed) 
                {
                    worklist.push(node);
                }

                //cout<<"@@initializePTS: \n";
                //showPTS (src);
                //showPTS (node);
                //cout<<"\n";
            }
        }

        initialized = true;   
    }

    // 2) Propagate constraints until fixpoint
    void solveConstraints()
    {
        while (!worklist.empty()) 
        {
            PAGNode *node = worklist.front();
            worklist.pop();

            // For each outgoing edge from 'n', apply the relevant rule
            for (auto it = node->outEdgeBegin(); it != node->outEdgeEnd (); it++)
            {
                PAGEdge* outEdge = *it;
                switch (outEdge->cstType) 
                {
                    case CST_COPY:
                        handleCopyEdge(outEdge);
                        break;
                    case CST_STORE:
                        handleStoreEdge(outEdge);
                        break;
                    case CST_LOAD:
                        handleLoadEdge(outEdge);
                        break;
                    case CST_ADDR_OF:
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // 2a) Copy: PTS(dst) ⊇ PTS(src)
    void handleCopyEdge(PAGEdge *edge)
    {
        PAGNode *src = edge->getSrcNode();
        PAGNode *dst = edge->getDstNode();

        if (isPointerValue (src) == false)
        {
            return;
        }

        bool changed = ptsMap[dst].unionWith(ptsMap[src]);
        if (changed) 
        {
            checkAndSetFuncPtr (dst);
            worklist.push(dst);
        }
    }

    // 2b) Store: (*src) = dst
    //   For each object o in PTS(src), do PTS(o) ⊇ PTS(dst)
    void handleStoreEdge(PAGEdge *edge)
    {
        PAGNode *valNode = edge->getSrcNode();
        PAGNode *ptrNode = edge->getDstNode();

        if (isPointerValue (valNode) == false)
        {
            return;
        }

        auto nPts = ptsMap[ptrNode].getSet();
        if (nPts.empty())
        {
            worklist.push(valNode);
            return;
        }

        for (auto *obj : nPts) 
        {
            // If 'obj' can itself hold a points-to set, unify them
            bool changed = ptsMap[obj].unionWith(ptsMap[valNode]);
            if (changed) 
            {
                checkAndSetFuncPtr (obj);
                worklist.push(obj);
            }
        }
    }

    // 2c) Load: dst = (*src)
    //   For each object o in PTS(src), do PTS(dst) ⊇ PTS(o)
    void handleLoadEdge(PAGEdge *edge)
    {
        PAGNode *ptrNode = edge->getSrcNode();
        PAGNode *dstNode = edge->getDstNode();
        if (isPointerValue (dstNode) == false)
        {
            return;
        }

        // For each object that 'ptrNode' points to, union that object’s PTS into 'dstNode'
        for (auto *obj : ptsMap[ptrNode].getSet()) 
        {
            bool changed = ptsMap[dstNode].unionWith(ptsMap[obj]);
            if (changed) 
            {
                checkAndSetFuncPtr (dstNode);
                worklist.push(dstNode);
            }
        }
    }
};


#endif 
