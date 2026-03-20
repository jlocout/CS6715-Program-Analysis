#ifndef _OTF_PTA_H_
#define _OTF_PTA_H_
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "andersen.h"

using namespace std;

typedef enum PTA_TYPE
{
    ANDERSEN = 0,
    STEENSGAARD = 1,
    LANDI = 2
}PTA_TYPE;

// on-the-fly pointer analysis
class OTFPTA
{
public:
    OTFPTA (ICFG& icfg, bool dumpGraph=true)
    {
        wICFG = &icfg;

        wPAG  = new PAG (wICFG);
        assert (wPAG != NULL);
        wPAG->build ();

        this->dumpGraph = dumpGraph;
    }

    ~OTFPTA () 
    {
        delete wPAG;
        delete pta;
    }

    /*
    * ============================================================
    * OTFPTA::solve() — Pseudocode Guidance (On-the-Fly Refinement)
    * ============================================================
    *
    * Goal:
    *   Perform an iterative “on-the-fly” pointer analysis loop:
    *     - Run pointer analysis to compute/update points-to sets.
    *     - Extract newly discovered indirect-call targets (function pointers).
    *     - Refine the ICFG and PAG using these new targets.
    *     - Repeat until no new targets are found (fixpoint).
    *
    * Why on-the-fly?
    *   Indirect call edges are initially unknown. Once we discover that a function
    *   pointer may point to concrete functions, we can add those call edges into
    *   the interprocedural graph, which may in turn create new pointer constraints,
    *   enabling discovery of even more targets.
    *
    * ------------------------------------------------------------
    * Step 0: Initialization
    *   - initNodes is an (optional) seed list for incremental PTA runs.
    *     It usually contains PAG nodes affected by the last refinement.
    *   - pta = initPTA(type) creates the selected PTA engine (e.g., Andersen).
    *
    * ------------------------------------------------------------
    * Step 1: Iterate until no new indirect-call targets appear
    *   while (true):
    *
    *     (1) Run pointer analysis
    *         pta->runAnalysis(initNodes)
    *
    *         Meaning:
    *           - If initNodes is empty, run a normal full analysis.
    *           - If initNodes is non-empty, re-solve starting from impacted nodes
    *             (incremental update), depending on your PTA implementation.
    *
    *     (2) Collect newly discovered function-pointer targets
    *         fPTS = pta->getFuncPts()
    *
    *         fPTS maps:
    *           fpVal  -> { candidate callees }
    *
    *         If fPTS is empty:
    *           break  // reached fixpoint: no new indirect targets
    *
    *     (3) For each function pointer that gained targets:
    *         for (fpVal, pCallees) in fPTS:
    *
    *         (3.1) Refine the ICFG
    *               wICFG->refine(fpVal, pCallees)
    *
    *               Meaning:
    *                 - For each indirect callsite associated with fpVal,
    *                   add call edges to each function in pCallees:
    *                     callsite -> callee entry
    *                     callee exits -> returnsite
    *
    *         (3.2) Refine the PAG for next iteration
    *               initNodes = wPAG->refine(fpVal, pCallees)
    *
    *               Meaning:
    *                 - Add any new PAG nodes/edges introduced due to new call edges
    *                   (e.g., argument passing, returns, globals, etc. depending on model).
    *                 - Return a list of affected PAG nodes so the next PTA run can
    *                   start from these changes efficiently.
    *
    * ------------------------------------------------------------
    * Output of this solve loop:
    *   - A refined (whole-program) ICFG that includes resolved indirect calls.
    *   - A refined PAG containing constraints exposed by those new call edges.
    *   - Pointer analysis results consistent with the refined graphs.
    *
    * Common mistakes to avoid:
    *   - Forgetting to break when fPTS is empty (infinite loop).
    *   - Not updating initNodes from wPAG->refine(), causing wasted full re-runs.
    *   - Refining ICFG but not PAG (or vice versa), leading to inconsistent state.
    *   - Not de-duplicating already-added callees/edges inside refine() (may explode).
    */
    void solve (PTA_TYPE type = ANDERSEN)
    {
        // your code

        // dump graphs
        if (dumpGraph)
        {
            dumpCG ();
            dumpICFG ();
            dumpPAG ();
        }  
    }

    const PTS& getPTS(llvm::Value *val) const
    {
        assert (pta != NULL && wPAG != NULL);
        PAGNode* pNode = wPAG->getValueNode (val);
        return pta->getPTS (pNode);
    }

private:
    ICFG *wICFG;
    PAG  *wPAG;
    PTA  *pta;
    bool dumpGraph;

private:
    inline PTA* initPTA (PTA_TYPE type)
    {
        switch (type)
        {
            case ANDERSEN:
                pta = new Andersen (wPAG);
                assert (pta != NULL);
                break;
            default:
                cout<<"Unsupported PTA type: "<<type<<"\n";
                exit (1);
        }
        
        return pta;
    }

    inline void dumpCG ()
    {
        CGVisual vis ("w-cg", wICFG->getCG());
        vis.writeGraph();
    }

    inline void dumpICFG ()
    {
        ICFGVisual vis ("w-icfg", wICFG);
        vis.writeGraph();
    }

    inline void dumpPAG ()
    {
        PAGVis vis("w-pag", wPAG);
        vis.writeGraph();
    }
};

#endif 
