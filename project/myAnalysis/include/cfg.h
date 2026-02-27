
#ifndef _CFG_H_
#define _CFG_H_
#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include "generic_graph.h"
#include "graph_visual.h"
#include "llvm_wrapper.h"

using namespace std;

typedef enum EDGE_TYPE 
{
    EDGE_NONE = 0,
    EDGE_CFG  = 1<<0,
    EDGE_DFG  = 1<<1,
    EDGE_ICFG = 1<<2,
    EDGE_IDFG = 1<<3
}EDGE_TYPE;

class CFGNode;  // Forward declaration
class CFGEdge : public GenericEdge<CFGNode> 
{
public:
    CFGEdge(CFGNode* s, CFGNode* d, unsigned type = EDGE_CFG)
        : GenericEdge<CFGNode>(s, d) 
    {
        edgeType = type;
        values.clear ();
    }
    
    ~CFGEdge() {}

    unsigned edgeType;
    set<llvm::Value*> values;
};

class CFGNode : public GenericNode<CFGEdge> 
{  
public:
    CFGNode(unsigned Id, llvm::Instruction* inst = NULL)
        : GenericNode<CFGEdge>(Id) { llvmInst = inst; }

    ~CFGNode() {}

    inline llvm::Instruction* getInstruction () { return llvmInst; }
    inline llvm::BasicBlock* getBasicBlock () { return llvmInst->getParent(); }
    virtual inline bool isEntry () { return false; }
    virtual inline bool isExit () { return false; }
private:
    llvm::Instruction* llvmInst;
};

class CFGEntryNode : public CFGNode
{  
public:
    CFGEntryNode (unsigned Id, llvm::Function* func)
        : CFGNode (Id, NULL) { llvmFunc = func;}

    ~CFGEntryNode() {}

    inline bool isEntry () { return true; }
    inline llvm::Function* getFunction () { return llvmFunc; }
private:
    llvm::Function* llvmFunc;
};

class CFGExitNode : public CFGNode
{  
public:
    CFGExitNode (unsigned Id)
        : CFGNode (Id, NULL) {}

    ~CFGExitNode() {}
    inline bool isExit () { return true; }
};


class CFG : public GenericGraph<CFGNode, CFGEdge> 
{
public:
    CFG(unsigned initId = 1) { initNodeId = initId; }
    ~CFG() {}

    void build (llvm::Function &F)
    {
        if (F.isDeclaration()) return;

        // Create artificial entry/exit nodes
        entryNode = new CFGEntryNode (getNextNodeId(), &F); 
        addNode(entryNode->getId(), entryNode);
        exitNode  = new CFGExitNode (getNextNodeId()); 
        addNode(exitNode->getId(), exitNode);

        queue<CFGNode*> worklist;
        map<llvm::BasicBlock*, pair<CFGNode*, CFGNode*>> visited;

        llvm::BasicBlock *entryBB = &F.getEntryBlock();
        vector<CFGNode*> entrySubgraph = getBlockGraph(entryBB); 
        if (!entrySubgraph.empty()) 
        {
            addCFGEdge(entryNode, entrySubgraph.front());
            worklist.push(entrySubgraph.back());
        }
        visited[entryBB] = make_pair(entrySubgraph.front(), entrySubgraph.back());

        while (!worklist.empty()) 
        {
            CFGNode *bbNode = worklist.front();
            worklist.pop();
            llvm::BasicBlock *llvmBB = bbNode->getBasicBlock ();
            
            if (llvm::succ_empty(llvmBB)) 
            {
                addCFGEdge(bbNode, exitNode);
                continue;
            }

            for (llvm::BasicBlock *succBB : llvm::successors(llvmBB)) 
            {
                auto itr = visited.find(succBB);
                if (itr == visited.end()) 
                {
                    vector<CFGNode*> succSubgraph = getBlockGraph(succBB);
                    addCFGEdge(bbNode, succSubgraph.front());

                    worklist.push(succSubgraph.back());
                    visited[succBB] = make_pair(succSubgraph.front(), succSubgraph.back());
                }
                else
                {
                    auto & [subFirst, subLast] = itr->second;
                    addCFGEdge(bbNode, subFirst);
                }    
            }
        }
    }

    inline CFGNode* getCFGNode (llvm::Instruction* inst)
    {
        auto it = instToNode.find (inst);
        if (it == instToNode.end ())
        {
            return NULL;
        }

        return it->second;
    }

    inline CFGNode* getEntryNode() { return entryNode; }
    inline CFGNode* getExitNode() { return exitNode; }

private:
    map<const llvm::Instruction*, CFGNode*> instToNode;
    CFGNode* entryNode;
    CFGNode* exitNode;
    unsigned initNodeId;

private:
    inline unsigned getNextNodeId () 
    {
        return getNodeNum() + initNodeId;
    }

    inline CFGNode* addCFGNode (llvm::Instruction *llvmInst) 
    {
        CFGNode *node = new CFGNode(getNextNodeId(), llvmInst);
        assert (node != NULL);
        addNode(node->getId(), node);

        if (llvmInst != NULL)
        {
            instToNode [llvmInst] = node;
        }

        return node;
    }

    inline CFGEdge* addCFGEdge (CFGNode *src, CFGNode *dst) 
    {
        CFGEdge *edge = new CFGEdge(src, dst);
        assert (edge != NULL);
        addEdge(edge); 

        return edge;
    }

    inline vector<CFGNode*> getBlockGraph (llvm::BasicBlock* BB)
    {
        vector<CFGNode*> subgraph;
        CFGNode* prevInstNode = nullptr;
            
        for (llvm::Instruction &I : *BB) 
        {
            if (llvm::isa<llvm::DbgInfoIntrinsic>(&I) ||
                llvm::isa<llvm::DbgVariableIntrinsic>(&I)) 
            {
                continue;
            }

            CFGNode* instNode = addCFGNode (&I);
            subgraph.push_back (instNode);

            if (prevInstNode) 
            {
                addCFGEdge (prevInstNode, instNode);
            } 
            
            prevInstNode = instNode;   
        }

        return subgraph;
    }
};

class CFGVisual : public GraphVis<CFGNode, CFGEdge, CFG>
{
public:
    CFGVisual(std::string graphName, CFG* graph)
        : GraphVis<CFGNode, CFGEdge, CFG>(graphName, graph) {}
    
    ~CFGVisual() {}

    inline string getNodeLabel (CFGNode *node)
    {
        if (node->isEntry ())
        {
            CFGEntryNode* enNode = (CFGEntryNode*)node;
            llvm::Function* func = enNode->getFunction ();
            return LLVM().getValueLabel (func);
        }
        else if (node->isExit ())
        {
            return "exit";
        }
        else
        {
            llvm::Instruction* inst = node->getInstruction ();
            std::string instStr = LLVM().getValueLabel (inst);
            return escapeForDotLabel (instStr);
        }
    }

    inline string getNodeAttributes(CFGNode *node) 
    {
        string str = "shape=rectangle, color=black";   
        return str;
    }
};

#endif 


