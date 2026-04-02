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

    void solve (PTA_TYPE type = ANDERSEN)
    {
        vector<PAGNode*> initNodes = {};
        pta = initPTA (type);

        while (true)
        {
            // 1) Run your pointer analysis to populate or update points-to sets.
            pta->runAnalysis(initNodes);

            // 2) Gather newly discovered function-pointer targets.
            unordered_map<llvm::Value*, unordered_set<llvm::Function*>> fPTS = pta->getFuncPts();
            if (fPTS.size () == 0)
            {
                break;
            }

            for (auto &pair : fPTS)
            {
                llvm::Value *fpVal = pair.first; 
                const auto &pCallees = pair.second;

                // 3) Refine the ICFG with the newly discovered function pointers.
                wICFG->refine (fpVal, pCallees);

                // 4) Refine the PAG for the next iteration.
                initNodes = wPAG->refine (fpVal, pCallees);
            }   
        }

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
