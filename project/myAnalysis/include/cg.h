
#ifndef _CG_H_
#define _CG_H_
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <unordered_set>
#include "generic_graph.h"
#include "graph_visual.h"

using namespace std;

class CGNode;  // Forward declaration
class CGEdge : public GenericEdge<CGNode> 
{
public:
    CGEdge(CGNode* s, CGNode* d)
        : GenericEdge<CGNode>(s, d) {}
    
    ~CGEdge() {}
};

class CGNode : public GenericNode<CGEdge> 
{  
public:
    typedef vector<llvm::CallBase*>::iterator cs_iterator;

    CGNode(unsigned Id, llvm::Function* func = NULL)
        : GenericNode<CGEdge>(Id) { llvmFunc = func; }

    ~CGNode() {}

    llvm::Function* getLLVMFunc ()
    {
        return llvmFunc;
    }

    void addCallsite (llvm::CallBase* callInst)
    {
        callsites.push_back (callInst);
    }

    cs_iterator begin () { return callsites.begin (); }
    cs_iterator end () { return callsites.end (); }

private:
    llvm::Function* llvmFunc;
    vector<llvm::CallBase*> callsites;
};

class CG : public GenericGraph<CGNode, CGEdge> 
{
public:
    CG(LLVM *llvmpas = NULL) { llvmParser = llvmpas; }
    ~CG() {}

public:
    void build ()
    {
        if (!llvmParser) return;

        for (auto it = llvmParser->func_begin(); it != llvmParser->func_end(); ++it) 
        {
            llvm::Function *F = *it;
            if (F->isDeclaration()) continue;

            CGNode *node = addCGNode(F);
            func2Nodes[F] = node;
        }

        set<CGNode*> visited;
        for (auto nodeItr = begin (); nodeItr != end (); nodeItr++)
        {
            CGNode *node = nodeItr->second;
            if (visited.find(node) != visited.end()) continue;

            queue<CGNode*> worklist;
            worklist.push (node);
            while (!worklist.empty()) 
            {
                node = worklist.front();
                worklist.pop();
                visited.insert(node);

                llvm::Function* callerFunc = node->getLLVMFunc ();
                for (llvm::BasicBlock &BB : *callerFunc) 
                {
                    for (llvm::Instruction &I : BB) 
                    {
                        if (llvm::isa<llvm::DbgInfoIntrinsic>(&I) ||
                            llvm::isa<llvm::DbgVariableIntrinsic>(&I)) 
                        {
                            continue;
                        }

                        llvm::CallBase* callInst = llvm::dyn_cast<llvm::CallBase>(&I);
                        if (callInst == NULL)
                        {
                            continue;
                        }

                        llvm::Function* calleeFunc = callInst->getCalledFunction();
                        if (calleeFunc != NULL)
                        {
                            CGNode* calleeNode = getCGNode (calleeFunc);
                            if (calleeNode == NULL)
                            {
                                continue;
                            }
                            addCGEdge (node, calleeNode);

                            if (visited.find(calleeNode) == visited.end()) 
                            {
                                worklist.push(calleeNode);
                            }
                        }
                        else
                        {
                            llvm::Value *fpVal = callInst->getCalledOperand();
                            fpVal = fpVal->stripPointerCasts();
                            value2IndirectCS[fpVal].insert(callInst);
                        }

                        node->addCallsite (callInst);  
                    }
                }
            }
        }     
    }

    inline CGNode* getCGNode (llvm::Function *llvmFunc)
    {
        auto itr = func2Nodes.find(llvmFunc);
        if (itr == func2Nodes.end())
        {
            return NULL;
        }
        return itr->second;
    }

    void refine(const set<llvm::CallBase*> &callSites,
                const unordered_set<llvm::Function*> &callees)
    {
        for (llvm::CallBase *callInst : callSites)
        {
            llvm::Function *caller = callInst->getFunction();
            if (!caller) 
            {
                continue;
            }
            CGNode* callerNode = getCGNode (caller);
            assert (callerNode != NULL);

            for (llvm::Function *callee : callees)
            {
                CGNode* calleeNode = getCGNode (callee);
                assert (calleeNode != NULL);

                addCGEdge (callerNode, calleeNode);
            }
        }
    }

    static set<llvm::CallBase*> getCallsites (llvm::Value* fVal)
    {
        auto it = value2IndirectCS.find(fVal);
        if (it == value2IndirectCS.end())
        {
            return {};
        }

        return it->second;
    }

private:
    inline unsigned getNextNodeId () 
    {
        return getNodeNum() + 1;
    }

    inline CGNode* addCGNode (llvm::Function *llvmFunc) 
    {
        CGNode *node = new CGNode(getNextNodeId(), llvmFunc);
        assert (node != NULL);
        addNode(node->getId(), node);

        return node;
    }

    inline CGEdge* addCGEdge (CGNode *src, CGNode *dst) 
    {
        CGEdge *edge = new CGEdge(src, dst);
        assert (edge != NULL);
        addEdge(edge); 

        return edge;
    }
private:
    LLVM *llvmParser;
    map<llvm::Function*, CGNode*> func2Nodes;
    static map<llvm::Value*, set<llvm::CallBase*>> value2IndirectCS;
};

class CGVisual : public GraphVis<CGNode, CGEdge, CG>
{
public:
    CGVisual(std::string graphName, CG* graph)
        : GraphVis<CGNode, CGEdge, CG>(graphName, graph) {}
    
    ~CGVisual() {}

    inline string getNodeAttributes(CGNode *node) 
    {
        string str = "shape=rectangle, color=black";   
        return str;
    }

    inline string getNodeLabel(CGNode *node) 
    {
        llvm::Function *F = node->getLLVMFunc ();

        string str = "";
        str = F->getName ().str();
        return str;
    }
};

#endif 
