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
    /*
    * ============================================================
    * Andersen::initializePTS() — Pseudocode Guidance (Phase 2)
    * ============================================================
    *
    * Goal:
    *   Initialize the points-to sets (PTS) for all PAG nodes, using only
    *   the ADDR_OF constraints (p = &a).
    *
    *   After this phase:
    *     - Every PAG node has an entry in ptsMap (possibly empty).
    *     - For each edge (a -> p) of type CST_ADDR_OF:
    *         a is inserted into pts(p)
    *     - Any node whose PTS changed is pushed into the worklist to
    *       start propagation in Phase 3.
    *
    * ------------------------------------------------------------
    * Step 0: Avoid repeated initialization
    *   If initialized == true:
    *     - Return immediately (do not re-create ptsMap / worklist content).
    *
    * ------------------------------------------------------------
    * Step 1: Create an empty PTS set for every PAG node
    *   For each node in PAG:
    *     ptsMap[node] = empty set
    *
    * Why?
    *   Later propagation assumes every node has a valid ptsMap entry.
    *
    * ------------------------------------------------------------
    * Step 2: Seed PTS using CST_ADDR_OF edges
    *   For each node 'n' in PAG:
    *     For each incoming edge e of n:
    *       - Only handle address-of constraints:
    *           if e.type != CST_ADDR_OF: continue
    *
    *       - Let src = e.getSrcNode()   // memory object 'a'
    *       - Insert src into ptsMap[n]  // pts(p) contains a
    *
    *       - If insertion changed ptsMap[n]:
    *           push n into worklist
    *
    * Intuition:
    *   Address-of edges directly create initial points-to facts.
    *   These facts enable later COPY/LOAD/STORE propagation.
    *
    * ------------------------------------------------------------
    * Output of this function:
    *   - ptsMap is allocated for all nodes.
    *   - Worklist contains exactly the nodes whose PTS became non-empty
    *     (or otherwise changed) due to ADDR_OF constraints.
    *   - initialized is set to true.
    *
    * Common mistakes to avoid:
    *   - Forgetting to create empty ptsMap entries first (can cause missing keys).
    *   - Seeding the wrong direction (must insert src into pts(node)).
    *   - Forgetting to push changed nodes into worklist (solver will not start).
    *   - Re-initializing multiple times (will wipe results and break OTF loop).
    */
    void initializePTS()
    {
        return;
    }

    /*
    * ============================================================
    * Andersen::solveConstraints() — Pseudocode Guidance (Phase 3)
    * ============================================================
    *
    * Goal:
    *   Propagate points-to information along PAG constraint edges until a fixpoint.
    *   This is the main Andersen worklist solver.
    *
    * Key idea:
    *   - The worklist contains nodes whose points-to sets (PTS) were updated.
    *   - When a node's PTS grows, its outgoing constraints may enable new facts
    *     for other nodes (or for objects reached through load/store).
    *   - We repeatedly apply constraint rules until no node changes (worklist empty).
    *
    * ------------------------------------------------------------
    * Step 0: Worklist-driven fixpoint iteration
    *   While worklist is not empty:
    *     (A) Pop one node 'n'
    *     (B) For each outgoing edge e from n:
    *         apply the propagation rule based on e.cstType
    *
    * ------------------------------------------------------------
    * Step 1: Handle each constraint type
    *   For each outgoing edge e = (src -> dst):
    *
    *   (1) COPY edge (src -> dst), CST_COPY
    *       - Meaning:  dst = src
    *       - Rule:     pts(dst) ⊇ pts(src)
    *       - Implementation: handleCopyEdge(e)
    *
    *   (2) STORE edge (src -> ptr), CST_STORE
    *       - Meaning:  *ptr = src
    *       - Rule:     for each o in pts(ptr): pts(o) ⊇ pts(src)
    *       - Implementation: handleStoreEdge(e)
    *
    *   (3) LOAD edge (ptr -> dst), CST_LOAD
    *       - Meaning:  dst = *ptr
    *       - Rule:     for each o in pts(ptr): pts(dst) ⊇ pts(o)
    *       - Implementation: handleLoadEdge(e)
    *
    *   (4) ADDR_OF edges are already handled in initializePTS()
    *       - Skip here.
    *
    * ------------------------------------------------------------
    * Output of this function:
    *   - When the loop terminates, the analysis reaches a fixpoint:
    *       no propagation rule can add new points-to facts.
    *
    * Common mistakes to avoid:
    *   - Not pushing affected nodes back into worklist inside handle*Edge()
    *     when their ptsMap changes (solver will stop too early).
    *   - Treating STORE/LOAD as simple copy edges (they require dereference via pts).
    *   - Forgetting that LOAD/STORE may update object nodes (not only pointer vars).
    */
    void solveConstraints()
    {
        return;
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
