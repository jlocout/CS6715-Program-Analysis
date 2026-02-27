#ifndef _PTS_H_
#define _PTS_H_
#include <unordered_set>
#include <vector>
#include "llvm_wrapper.h"
#include "pag.h"

class PTS
{
public:
    inline bool insert(PAGNode *node)
    {
        auto result = ptsSet.insert(node);
        return result.second;
    }

    inline bool unionWith(const PTS &other)
    {
        bool changed = false;
        for (auto *n : other.ptsSet)
        {
            auto result = ptsSet.insert(n);
            if (result.second)
                changed = true;
        }
        return changed;
    }

    inline bool contains(PAGNode *node) const
    {
        return (ptsSet.find(node) != ptsSet.end());
    }

    inline const std::unordered_set<PAGNode*>& getSet() const
    {
        return ptsSet;
    }

    inline bool isEmpty() const
    {
        return ptsSet.empty ();
    }

    inline std::unordered_set<PAGNode*>::iterator begin ()
    {
        return ptsSet.begin ();
    }

    inline std::unordered_set<PAGNode*>::iterator end ()
    {
        return ptsSet.end ();
    }

private:
    std::unordered_set<PAGNode*> ptsSet;
};

class PTA
{
public:
    PTA (PAG *graph): pag(graph) {}
    virtual ~PTA () {}

    virtual void runAnalysis(vector<PAGNode*>& initNodes)
    {
        return;
    }

    const PTS& getPTS(PAGNode *node) const
    {
        static PTS emptySet;
        auto it = ptsMap.find(node);
        if (it != ptsMap.end())
        {
            return it->second;
        }
            
        return emptySet;
    }

    void printAllPTS() const
    {
        for (const auto &entry : ptsMap)
        {
            PAGNode *node = entry.first;
            const PTS &pts = entry.second;

            showPTS (node, &pts);
        }
    }

    unordered_map<llvm::Value*, unordered_set<llvm::Function*>> getFuncPts ()
    {
        return newFuncPTSs;
    }

protected:
    PAG *pag;
    unordered_map<PAGNode*, PTS> ptsMap;
    queue<PAGNode*> worklist;

    unordered_map<llvm::Value*, unordered_set<llvm::Function*>> solvedFuncPTSs;
    unordered_map<llvm::Value*, unordered_set<llvm::Function*>> newFuncPTSs;


protected:
    inline void refinePAG ()
    {
        for (const auto &entry : ptsMap)
        {
            PAGNode *node = entry.first;
            const PTS &pts = entry.second;  

            for (PAGNode *obj : pts.getSet())
            {
                PAGEdge *ptsEdge = new PAGEdge(node, obj, CST_PTS);
                pag->addEdge(ptsEdge);
            }
        }
    }

    inline void checkAndSetFuncPtr (PAGNode *dstNode)
    {
        LLVM* llvmParser = pag->getLLVMParser();

        llvm::Value* fpVal = dstNode->value;
        if (fpVal == NULL)
        {
            return;
        }

        set<llvm::CallBase*> callSites = CG::getCallsites (fpVal);
        if (callSites.empty()) 
        {
            return;
        }

        PTS fPTS = getPTS(dstNode);
        for (PAGNode* pagNode : fPTS.getSet())
        {
            llvm::Value *val = pagNode->value;
            if (val == NULL)
            {
                continue;
            }

            if (llvm::Function* func = llvm::dyn_cast<llvm::Function>(val))
            {
                auto &solvedSet = solvedFuncPTSs[fpVal];
                if (solvedSet.find(func) == solvedSet.end())
                {
                    newFuncPTSs[fpVal].insert(func);
                    solvedSet.insert(func);
                }
            }
            else
            {
                llvm::FunctionType* fType = llvmParser->getFuncTypeFromValue(fpVal);
                if (fType)
                {
                    // try virtual function resolution
                    vector<llvm::Function*> pbCallees = llvmParser->resolveVirtualCall(fpVal, fType);
                    for (llvm::Function* func : pbCallees)
                    {
                        auto &solvedSet = solvedFuncPTSs[fpVal];
                        if (solvedSet.find(func) == solvedSet.end())
                        {
                            newFuncPTSs[fpVal].insert(func);
                            solvedSet.insert(func);
                        }
                    }
                }
                else
                {
                    ;
                }
            }
        }
    }

    inline bool isPointerValue(PAGNode *node)
    {
        llvm::Value *val = node->value;
        if (val == NULL)
        {
            return false;
        }
        return val->getType()->isPointerTy();
    }

    inline void showPTS (PAGNode *node, const PTS *pts=NULL) const
    {
        if (pts == NULL)
        {
            auto it = ptsMap.find(node);
            if (it == ptsMap.end())
            {
                return;
            }

            pts = &(it->second);
        }
        
        if (llvm::Function* func = llvm::dyn_cast<llvm::Function>(node->value))
        {
            llvm::errs() <<"@@Node["<<node->getId()<<"]"<< func->getName ()<< " -> PTS: \n{\n";
        }
        else
        {
            llvm::errs() <<"@@Node["<<node->getId()<<"]"<< *(node->value)<< " -> PTS: \n{\n";
        }    

        for (PAGNode *obj : pts->getSet())
        {
            if (llvm::Function* func = llvm::dyn_cast<llvm::Function>(obj->value))
            {
                llvm::errs()<<"\t"<<func->getName ()<<",\n";
            }
            else
            {
                llvm::errs()<<"\t"<<*(obj->value)<<",\n";
            }    
        }
        llvm::errs()<< "}\n\n";
    }

};

#endif 
