#ifndef _PAG_H_
#define _PAG_H_

#include <algorithm>
#include <iostream>
#include <vector>
#include <queue>
#include "generic_graph.h"
#include "graph_visual.h"
#include "llvm_wrapper.h"
#include "icfg.h"

using namespace std;

typedef enum  
{
    CST_NONE=0,
    CST_ADDR_OF,
    CST_COPY, 
    CST_LOAD, 
    CST_STORE,
    CST_PTS
} CstType;

typedef enum  
{
    PNT_NONE=0,
    PNT_GLOBAL,
    PNT_FUNCTION, 
    PNT_GENERAL
} PagNodeType;

// Forward declarations
class PAGNode;

// PAGEdge: Inherit from GenericEdge with NodeTy = PAGNode.
class PAGEdge : public GenericEdge<PAGNode> 
{
public:
    CstType cstType;

    PAGEdge (PAGNode* s, PAGNode* d, CstType ct=CST_NONE)
      : GenericEdge<PAGNode>(s, d) 
    {
        cstType = ct;
    }  
};


// PAGNode: Inherit from GenericNode with EdgeTy = PAGEdge.
// We add a Label field for storing node-specific information.
class PAGNode : public GenericNode<PAGEdge> 
{
public:
    string nodeLabel;
    llvm::Value *value;
    PagNodeType nodeType;

    PAGNode (unsigned id)
      : GenericNode<PAGEdge> (id)
    { 
        nodeLabel = "";
        value     = NULL;
        nodeType  = PNT_NONE;
    }
    
    PAGNode(unsigned id, llvm::Value *val, const string &label, PagNodeType ntype)
      : GenericNode<PAGEdge>(id)
    {
        nodeLabel = label;
        value     = val;
        nodeType  = ntype;
    }
};


// PAG: Our graph type; inherit from GenericGraph using PAGNode and PAGEdge.
class PAG : public GenericGraph<PAGNode, PAGEdge> 
{
public:
    PAG (ICFG *icfg) 
    {
        assert (icfg != NULL);
        wicfg = icfg;
        llvmParser = wicfg->getLLVMParser ();
    } 
    ~PAG () {}

public:
    void build()
    {
        // Pre-populate globals.
        addGlobalNode ();

        for (auto itr = wicfg->cfg_begin (); itr != wicfg->cfg_end (); itr++)
        {
            // Pre-populate function arguments.
            llvm::Function *F = itr->first;
            addFuncArgNode (F);

            CFG *cfg = itr->second;
            for (auto itn = cfg->begin (); itn != cfg->end (); itn++)
            {
                CFGNode *node = itn->second;
                llvm::Instruction *inst = node->getInstruction();
                if (inst == NULL)
                {
                    continue;
                }

                if (auto *callInst = llvm::dyn_cast<llvm::CallBase>(inst))
                {
                    handleInterEdge (callInst);
                }
                else
                {
                    handleIntraEdge (inst);
                }
            }
        }
    }

    vector<PAGNode*> refine (llvm::Value *fpVal,
                             const std::unordered_set<llvm::Function*> &callees)
    {
        vector<PAGNode*> imptNodes;
        set<llvm::CallBase*> callSites = CG::getCallsites (fpVal);
        for (llvm::CallBase *callInst : callSites)
        {
            for (auto *callee : callees)
            {
                handleInterEdge (callInst, callee);
            }

            for (auto argIt = callInst->arg_begin(); argIt != callInst->arg_end(); ++argIt)
            {
                llvm::Value *actualVal = *argIt;
                PAGNode *actualNode = getValueNode(actualVal);
                if (actualNode == NULL)
                {
                    continue;
                }
                imptNodes.push_back (actualNode);
            }
            
        }

        return imptNodes;
    }

    inline PAGNode* getValueNode (llvm::Value* val)
    {
        auto it = valueToNode.find(val);
        if (it != valueToNode.end())
        {
            return it->second;
        }

        return NULL;
    }

    LLVM* getLLVMParser () 
    { 
        return llvmParser; 
    }

private:
    inline unsigned getNextNodeId () 
    {
        return getNodeNum() + 1;
    }

    inline PAGNode* addValueNode(llvm::Value* val, PagNodeType nodeType = PNT_GENERAL)
    {
        PAGNode* existingNode = getValueNode(val);
        if (existingNode != nullptr)
        {
            return existingNode;
        }

        unsigned nodeId = getNextNodeId();
        std::string label = getValueLabel(val);
        if (label.find(".dbg.") != std::string::npos)
        {
            return nullptr;
        }

        PAGNode* valNode = new PAGNode(nodeId, val, label, nodeType);
        assert(valNode != nullptr);
        valueToNode[val] = valNode;

        addNode(nodeId, valNode);
        return valNode;
    }

    inline void addGlobalNode ()
    {
        // Add global pointer nodes (global variables)
        for (auto it = llvmParser->gv_begin (); it != llvmParser->gv_end (); it++)
        {
            llvm::GlobalVariable *gv = *it;
            PAGNode *gvNode = addValueNode (gv, PNT_GLOBAL);
            if (!gvNode)
            {
                continue;
            }

            llvm::Type *elemTy = gv->getType()->getPointerElementType();
            if (elemTy->isPointerTy() && gv->hasInitializer())
            {
                // add an ADDR_OF edge to reflect "gv = &someone".
                llvm::Constant *init = gv->getInitializer();
                handleGlobalInitializer(gvNode, init);
            }
            
            // always consider global var as a memory object
            PAGEdge *addrEdge = new PAGEdge(gvNode, gvNode, CST_ADDR_OF);
            addEdge(addrEdge);
        }

        /// Add function nodes as global pointers
        for (auto it = llvmParser->func_begin(); it != llvmParser->func_end(); ++it)
        {
            llvm::Function *func = *it;
            PAGNode *fnode = addValueNode (func, PNT_FUNCTION);
            if (fnode == NULL)
            {
                continue;
            }
                    
            // let function point to itself
            PAGEdge *addrEdge = new PAGEdge(fnode, fnode, CST_ADDR_OF);
            addEdge(addrEdge);  
        }
    }

    inline void handleGlobalInitializer(PAGNode *gvNode, llvm::Constant *init)
    {
        // If init is a function
        if (auto *func = llvm::dyn_cast<llvm::Function>(init))
        {
            PAGNode *funcNode = addValueNode(func, PNT_FUNCTION);
            if (funcNode)
            {
                // "func -> gvNode" means "PTS(gvNode) includes func"
                PAGEdge *addrEdge = new PAGEdge(funcNode, gvNode, CST_ADDR_OF);
                addEdge(addrEdge);
            }
        }

        // If init is another global
        else if (auto *anotherGV = llvm::dyn_cast<llvm::GlobalVariable>(init))
        {
            PAGNode *objNode = addValueNode(anotherGV, PNT_GLOBAL);
            if (objNode)
            {
                PAGEdge *addrEdge = new PAGEdge(objNode, gvNode, CST_ADDR_OF);
                addEdge(addrEdge);
            }
        }
        // If init is a ConstantExpr (bitcast, GEP, etc.)
        else if (auto *ce = llvm::dyn_cast<llvm::ConstantExpr>(init))
        {
            handleConstantExpr(gvNode, ce);
        }
        else
        {
        }
    }

    inline void handleConstantExpr(PAGNode *gvNode, llvm::ConstantExpr *ce)
    {
        unsigned opcode = ce->getOpcode();
        if (opcode == llvm::Instruction::BitCast ||
            opcode == llvm::Instruction::IntToPtr ||
            opcode == llvm::Instruction::PtrToInt)
        {
            // Usually the base is ce->getOperand(0)
            llvm::Value *baseVal = ce->getOperand(0)->stripPointerCasts();
            if (auto *baseNode = addValueNode(baseVal, PNT_GENERAL))
            {
                PAGEdge *addrEdge = new PAGEdge(baseNode, gvNode, CST_ADDR_OF);
                addEdge(addrEdge);
            }
        }
        else if (opcode == llvm::Instruction::GetElementPtr)
        {
            // For field-insensitive, unify with the GEP base
            llvm::Value *gepBase = ce->getOperand(0)->stripPointerCasts();
            if (auto *baseNode = addValueNode(gepBase, PNT_GENERAL))
            {
                PAGEdge *addrEdge = new PAGEdge(baseNode, gvNode, CST_ADDR_OF);
                addEdge(addrEdge);
            }
        }
        else
        {

        }
    }

    inline void addFuncArgNode (llvm::Function *F)
    {
        for (llvm::Argument &arg : F->args()) 
        {
             if (arg.getType()->isPointerTy())
            {
                addValueNode(&arg);
            }
        }
    }

    inline string getValueLabel(const llvm::Value* V) 
    {
        return llvmParser->getValueLabel (V);
    }

    /*
    * ============================================================
    * PAG::handleIntraEdge() — Pseudocode Guidance
    * ============================================================
    *
    * Goal:
    *   Inspect ONE LLVM instruction and, if it represents a pointer-related
    *   operation, add the corresponding constraint edge(s) into the PAG.
    *
    *   We handle four core pointer constraints:
    *     (1) Address-of:  p = &a
    *     (2) Copy:        q = p
    *     (3) Store:       *p = q
    *     (4) Load:        q = *p
    *
    * ------------------------------------------------------------
    * Step 0: Classify the instruction
    *   Use llvmParser helpers to decide which constraint this instruction is:
    *     - isAddressOf(inst)  / getOperandsAssignment
    *     - isAssignment(inst) / getOperandsAssignment
    *     - isStore(inst)      / getOperandsStore
    *     - isLoad(inst)       / getOperandsLoad
    *
    *   If none match, ignore the instruction (not relevant to PAG).
    *
    * ------------------------------------------------------------
    * Step 1: Address-of constraint (p = &a)
    *   If inst is an address-of:
    *     (A) Extract operands:
    *         (pointerVal, memLocVal) = getOperandsAddressOf(inst)
    *           - pointerVal : the pointer variable p
    *           - memLocVal  : the memory object a (the “location”)
    *
    *     (B) Create/get PAG nodes:
    *         ptrNode    = addValueNode(pointerVal)
    *         memLocNode = addValueNode(memLocVal)
    *
    *     (C) Add constraint edge:
    *         memLoc  ->  ptr     with type CST_ADDR_OF
    *
    *   Meaning:
    *     The pointer p can point to the abstract object a.
    *
    * ------------------------------------------------------------
    * Step 2: Copy / assignment constraint (q = p)
    *   If inst is an assignment:
    *     (A) Extract operands:
    *         (srcVal, dstVal) = getOperandsAssignment(inst)
    *           - srcVal : p
    *           - dstVal : q
    *
    *     (B) Create/get PAG nodes:
    *         srcNode = addValueNode(srcVal)
    *         dstNode = addValueNode(dstVal)
    *
    *     (C) Add constraint edge:
    *         src  ->  dst    with type CST_COPY
    *
    *   Meaning:
    *     pts(q) ⊇ pts(p)
    *
    * ------------------------------------------------------------
    * Step 3: Store constraint (*p = q)
    *   If inst is a store:
    *     (A) Extract operands:
    *         (ptrVal, srcVal) = getOperandsStore(inst)
    *           - ptrVal : p (the pointer being dereferenced)
    *           - srcVal : q (the value being stored)
    *
    *     (B) Create/get PAG nodes:
    *         ptrNode = addValueNode(ptrVal)
    *         srcNode = addValueNode(srcVal)
    *
    *     (C) Add constraint edge:
    *         src  ->  ptr    with type CST_STORE
    *
    *   Meaning (in Andersen solving):
    *     For each o in pts(p), pts(o) ⊇ pts(q)
    *
    * ------------------------------------------------------------
    * Step 4: Load constraint (q = *p)
    *   If inst is a load:
    *     (A) Extract operands:
    *         (ptrVal, dstVal) = getOperandsLoad(inst)
    *           - ptrVal : p (the pointer being dereferenced)
    *           - dstVal : q (the loaded value)
    *
    *     (B) Create/get PAG nodes:
    *         ptrNode = addValueNode(ptrVal)
    *         dstNode = addValueNode(dstVal)
    *
    *     (C) Add constraint edge:
    *         ptr  ->  dst    with type CST_LOAD
    *
    *   Meaning (in Andersen solving):
    *     For each o in pts(p), pts(q) ⊇ pts(o)
    *
    * ------------------------------------------------------------
    * Output of this function:
    *   - Adds at most ONE constraint edge per instruction (in this simplified model).
    *   - Ensures all involved LLVM Values have PAG nodes (via addValueNode()).
    *
    * Common mistakes to avoid:
    *   - Swapping operand order for COPY (must be src -> dst).
    *   - Confusing STORE vs LOAD directions:
    *       STORE uses (src -> ptr)  [because it writes through ptr]
    *       LOAD  uses (ptr -> dst)  [because it reads through ptr]
    *   - Forgetting to create nodes for both operands before creating the edge.
    *   - Not stripping casts for pointer operands if your parser requires it
    *     (your llvmParser helpers usually handle this).
    */
    inline void handleIntraEdge (llvm::Instruction* inst)
    {
        
        return;
    }

    inline void handleInterEdge (llvm::CallBase *callInst, llvm::Function *calleePtr=NULL)
    {
        llvm::Function *callee = callInst->getCalledFunction();
        if (callee == NULL)
        {
            if (calleePtr != NULL)
            {
                callee = calleePtr;
            }
            else
            {
                return;
            }
            
        }

        // Check if this is an allocation function
        if (llvmParser->isMemAlloc (callee))
        {
            handleAlloc(callInst);
            return;
        }

        if (callee->isDeclaration())
        {
            // external function, need modeling function, here just skip
            return;
        }

        // 1) copy: actual -> formal
        handlCallEdge (callInst, callee);

        // 2) copy: return -> call‐site result
        handlReturnEdge (callInst, callee);

        return;
    }

    inline void handleAlloc (llvm::CallBase *callInst)
    {
        // Create or retrieve a “fresh” heap object node
        static unsigned heapCount = 0;
        std::string heapLabel = "O_" + std::to_string(heapCount++);

        unsigned heapNodeId = getNextNodeId();
        PAGNode *heapObj = new PAGNode(heapNodeId, NULL, heapLabel, PNT_GENERAL);
        addNode(heapNodeId, heapObj);

        // The pointer receiving the allocation
        llvm::Value *callResult = callInst;
        PAGNode *ptrNode = addValueNode(callResult);

        // Edge: heapObj -> ptr
        PAGEdge *edge = new PAGEdge(heapObj, ptrNode, CST_ADDR_OF);
        addEdge(edge);

        return;
    }

    inline void handlCallEdge (llvm::CallBase *callInst, llvm::Function *callee)
    {
        unsigned numArgs = callInst->arg_size();
        unsigned idx = 0;
        for (auto argIt = callInst->arg_begin(); argIt != callInst->arg_end(); ++argIt, ++idx)
        {
            llvm::Value *actualVal = *argIt;
            if (idx >= callee->arg_size())
            {
                break;
            }

            llvm::Argument &formalArg = *(callee->arg_begin() + idx);
            llvm::Value   *formalVal  = &formalArg;

            PAGNode *actualNode = addValueNode(actualVal);
            PAGNode *formalNode = addValueNode(formalVal);
            PAGEdge *argEdge;
            if (llvmParser->isFuncPtrRefValue(formalVal))
            {
                argEdge = new PAGEdge(actualNode, formalNode, CST_ADDR_OF);
            }
            else
            {
                argEdge = new PAGEdge(actualNode, formalNode, CST_COPY);
            }
            addEdge(argEdge);
        }
    }

    inline void handlReturnEdge (llvm::CallBase *callInst, llvm::Function *callee)
    {
        if (callInst->use_empty()) 
        {
            return;
        }

        if (callee->getReturnType()->isVoidTy())
        {
            return;
        }

        PAGNode *callResultNode = addValueNode(callInst);
        std::vector<PAGNode*> retNodes = getReturnNodes (callee);
        for (auto *retNode : retNodes)
        {
            PAGEdge *retEdge = new PAGEdge(retNode, callResultNode, CST_COPY);
            addEdge(retEdge);
        }

        return;
    }

    inline std::vector<PAGNode*> getReturnNodes (llvm::Function *F)
    {
        std::vector<PAGNode*> retNodes;
        for (auto &BB : *F)
        {
            if (auto *retInst = llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator()))
            {
                llvm::Value *returnedVal = retInst->getReturnValue();
                if (returnedVal)
                {
                    PAGNode *retNode = addValueNode(returnedVal);
                    retNodes.push_back(retNode);
                }
            }
        }

        return retNodes;
    }

private:
    map<const llvm::Value*, PAGNode*> valueToNode;
    LLVM *llvmParser;
    ICFG *wicfg;
};



// PAGVis: A DOT generator for our PAG (constraint graph).
// Inherits from the template GraphVis using our PAG types.
class PAGVis: public GraphVis<PAGNode, PAGEdge, PAG>
{
public:
    PAGVis (string graphName, PAG *graph)
        : GraphVis<PAGNode, PAGEdge, PAG>(graphName, graph) {}

    ~PAGVis () {}

    inline string getNodeLabel(PAGNode *node) 
    {
        return node->nodeLabel;
    }

    inline string getNodeAttributes(PAGNode *node) 
    {
        vector<string> nodeColors = {"black", "grey", "purple", "black", "lightblue"};
        string str = "shape=rectangle, color=" + nodeColors[node->nodeType];   
        return str;
    }

    inline string getEdgeAttributes(PAGEdge *edge) 
    {
        vector<string> edgeColors = {"black", "green", "blue", "orange", "red", "grey"};
        string str = "color=" + edgeColors[edge->cstType];  
        return str;
    } 
};

#endif
