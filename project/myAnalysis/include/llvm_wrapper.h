#ifndef _LLVM_WRAPPER_H_
#define _LLVM_WRAPPER_H_
#include <string>
#include <set>
#include <memory>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace std;

namespace llvm 
{
    class LLVMContext;
    class Module;
    class Function;
    class GlobalVariable;
    class MemoryBuffer;
}

class LLVM 
{
public:
    typedef set<llvm::Value*> ValueSet;
    typedef set<llvm::GlobalVariable*>::iterator gv_iterator;
    typedef set<llvm::Function*>::iterator func_iterator;

    typedef map<string, set<llvm::Function*>> VTSet;
    typedef VTSet::iterator vt_iterator;

public:
    LLVM() {}
    LLVM(const string &bcFile)
    {
        Context = make_unique<llvm::LLVMContext>();

        auto bufferOrError = llvm::MemoryBuffer::getFile(bcFile);
        if (!bufferOrError) 
        {
            llvm::errs() << "Error reading bitcode file: " << bcFile << "\n";
            exit(0);
        }
        unique_ptr<llvm::MemoryBuffer> buffer = move(bufferOrError.get());

        auto moduleOrError = llvm::parseBitcodeFile(buffer->getMemBufferRef(), *Context);
        if (!moduleOrError) 
        {
            llvm::errs() << "Error parsing bitcode file: " << bcFile << "\n";
            exit(0);
        }
        ModulePtr = move(moduleOrError.get());

        loadFunctions();
        loadGlobals();

        buildClassHierarchy();
        buildVTables();
    }

    ~LLVM() {}

    // Iterators over the global variables.
    gv_iterator gv_begin() { return globals.begin(); }
    gv_iterator gv_end()   { return globals.end(); }

    // Iterators over the functions.
    func_iterator func_begin() { return functions.begin(); }
    func_iterator func_end()   { return functions.end(); }

    llvm::Module* getModule() const { return ModulePtr.get(); }

    inline string getValueLabel(const llvm::Value* V)
    {
        if (auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(V)) 
        {
            return GV->getName().str();
        }

        if (auto *F = llvm::dyn_cast<llvm::Function>(V))
        {
            return F->getName().str();
        }

        std::string label;
        llvm::raw_string_ostream rso(label);
        V->print(rso);

        size_t pos = label.find(", align ");
        if (pos != string::npos)
        {
            label.erase(pos);
        }

        pos = label.find(", !dbg ");
        if (pos != string::npos)
        {
            label.erase(pos);
        }
        
        const std::string token = "noalias ";
        pos = label.find(token);
        if (pos != string::npos) {
            label.erase(pos, token.size());
        }
        return label;
    }

    inline bool isAddressOf(llvm::Instruction *I)
    {
        // 1) p = alloca
        if (llvm::isa<llvm::AllocaInst>(I))
            return true;

        // 2) p = getelementptr @Global
        if (auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(I))
        {
            llvm::Value *base = gep->getOperand(0);
            if (llvm::isa<llvm::GlobalVariable>(base))
            {
                return true;
            }
        }

        // 3) p = inttoptr x
        if (llvm::isa<llvm::IntToPtrInst>(I))
        {
            return true;
        }

        // Otherwise, not treated as address-of
        return false;
    }


    inline std::pair<llvm::Value*, llvm::Value*> getOperandsAddressOf(llvm::Instruction *I)
    {
        if (auto *allocaInst = llvm::dyn_cast<llvm::AllocaInst>(I)) 
        {
            return std::make_pair(I, I);
        }

        return std::make_pair(nullptr, nullptr);
    }


    inline bool isAssignment(llvm::Instruction *I)
    {
        if (auto *castInst = llvm::dyn_cast<llvm::CastInst>(I)) 
        {
            if (castInst->getSrcTy()->isPointerTy() && castInst->getDestTy()->isPointerTy()) 
            {
                return true;
            }
        }

        if (llvm::dyn_cast<llvm::GetElementPtrInst>(I)) 
        {
            return true;
        }

        return false;
    }

    inline std::pair<llvm::Value*, llvm::Value*> getOperandsAssignment(llvm::Instruction *I)
    {
        if (auto *castInst = llvm::dyn_cast<llvm::BitCastInst>(I)) 
        {
            llvm::Value *srcVal = castInst->getOperand(0);
            llvm::Value *dstVal = castInst;
            return std::make_pair(srcVal, dstVal);
        }

        if (auto *gep = llvm::dyn_cast<llvm::GetElementPtrInst>(I))
        {
            llvm::Value *basePtr = gep->getPointerOperand();
            llvm::Value *dstVal  = gep;
            return std::make_pair(basePtr, dstVal);
        }

        return std::make_pair(nullptr, nullptr);
    }


    inline bool isStore(llvm::Instruction *I)
    {
        return llvm::isa<llvm::StoreInst>(I);
    }

    inline std::pair<llvm::Value*, llvm::Value*> getOperandsStore(llvm::Instruction *I)
    {
        if (auto *storeInst = llvm::dyn_cast<llvm::StoreInst>(I)) 
        {
            llvm::Value *srcVal     = storeInst->getValueOperand();
            llvm::Value *pointerVal = storeInst->getPointerOperand();
            return std::make_pair(pointerVal, srcVal);
        }

        return std::make_pair(nullptr, nullptr);
    }


    inline bool isLoad(llvm::Instruction *I)
    {
        return llvm::isa<llvm::LoadInst>(I);
    }

    inline std::pair<llvm::Value*, llvm::Value*> getOperandsLoad(llvm::Instruction *I)
    {
        if (auto *loadInst = llvm::dyn_cast<llvm::LoadInst>(I)) 
        {
            llvm::Value *pointerVal = loadInst->getPointerOperand();
            llvm::Value *dstVal     = loadInst; 
            return std::make_pair(pointerVal, dstVal);
        }

        return std::make_pair(nullptr, nullptr);
    }

    inline bool isMemAlloc(llvm::Function *callee)
    {
        if (!callee)
        {
            return false;
        }

        // Common C allocation functions
        const std::set<std::string> cAllocFuncs = 
        {
            "malloc",
            "calloc",
            "realloc",
            "aligned_alloc",
            "valloc",
            "memalign",
            "posix_memalign"
        };

        // Common C++ allocation functions: operator new, operator new[]
        const std::set<std::string> cppAllocFuncs = 
        {
            "_Znw",
            "_Zna",
            "_Znwj",
            "_Znaj",
            "operator new",
            "operator new[]"
        };

        std::string calleeName = callee->getName().str();
        if (cAllocFuncs.count(calleeName) || cppAllocFuncs.count(calleeName))
        {
            return true;
        }

        return false;
    }

    inline void getDefUse(llvm::Instruction *I, ValueSet &def, ValueSet &use) 
    {
        if (llvm::StoreInst *SI = llvm::dyn_cast<llvm::StoreInst>(I)) 
        {
            // *def = use
            llvm::Value* useOp = SI->getValueOperand();
            llvm::Value* defOp = SI->getPointerOperand();

            def.insert(defOp);
            use.insert(useOp);
            use.insert(defOp);         
        }
        else if (llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(I))
        {
            // def = call (...)
            if (!CI->getType()->isVoidTy())
            {
                def.insert(CI);
            }        

            llvm::Value* calledVal = CI->getCalledOperand()->stripPointerCasts();
            use.insert(calledVal);
            for (unsigned i = 0; i < CI->arg_size(); ++i)
            {
                llvm::Value *operand = CI->getArgOperand(i);
                use.insert(operand);         
            }
        }
        else if (llvm::PHINode *PN = llvm::dyn_cast<llvm::PHINode>(I))
        {
            // PHI nodes: def = [...]
            def.insert(PN);
            for (unsigned i = 0; i < PN->getNumIncomingValues(); ++i) 
            {
                llvm::Value *incoming = PN->getIncomingValue(i);
                use.insert(incoming);   
            }
        }
        else if (llvm::BranchInst *BI = llvm::dyn_cast<llvm::BranchInst>(I))
        {
            // Branch instruction (conditional): condition is used
            if (BI->isConditional()) 
            {
                llvm::Value* cond = BI->getCondition();
                use.insert(cond);      
            }
        }
        else if (llvm::ReturnInst *RI = llvm::dyn_cast<llvm::ReturnInst>(I))
        {
            // Return instruction: returned value is used
            if (llvm::Value* retVal = RI->getReturnValue())
            {
                use.insert(retVal);
            }          
        }
        else if (auto *allocaInst = llvm::dyn_cast<llvm::AllocaInst>(I))
        {
            def.insert(I);
        }
        else 
        {
            // For non-store instructions
            if (!I->getType()->isVoidTy())
            {
                def.insert(I);
            }

            for (unsigned i = 0; i < I->getNumOperands(); ++i)
            {
                llvm::Value *operand = I->getOperand(i);
                use.insert(operand);              
            }
        }
    }


    bool isFuncPtrValue(llvm::Value *V)
    {
        return V && isFuncPtrTy(V->getType());
    }

    bool isFuncPtrRefValue(llvm::Value *V)
    {
        return V && isFuncPtrRefTy(V->getType());
    }

    llvm::FunctionType* getFuncTypeFromValue(llvm::Value *V) 
    {
        llvm::Type *Ty = V->getType();
        if (!Ty->isPointerTy())
        {
            return nullptr;
        } 
        return llvm::dyn_cast<llvm::FunctionType>(Ty->getPointerElementType());
    }

    vector<llvm::Function*> resolveVirtualCall(llvm::Value *fpVal, llvm::FunctionType *fType)
    {
        vector<llvm::Function*> possibleCallees;

        string clsName = getClassNameByFPtr(fpVal);
        if (clsName.empty() || !fType)
        {
            return possibleCallees;
        }

        //llvm::errs() << "Resolving virtual call for class: " << clsName << ", function type: " << *fType << "\n";
        // BFS over class hierarchy: clsName + all derived classes
        set<string> visited;
        vector<string> worklist;
        worklist.push_back(clsName);
        visited.insert(clsName);

        while (!worklist.empty())
        {
            string cur = worklist.back();
            worklist.pop_back();

            // Look up vtable for this class
            auto vtIt = vtables.find(cur);
            if (vtIt != vtables.end())
            {
                const set<llvm::Function*> &vtable = vtIt->second;
                for (llvm::Function *f : vtable)
                {
                    //llvm::errs() << "  VTable entry: " << f->getName() <<" " << *(f->getFunctionType()) << "\n";
                    // keep your simple type check (exact match)
                    if (f && matchFuncTypeRelaxed(f, fType))
                    {
                        //llvm::errs() << "    Possible callee: " << f->getName() << "\n";
                        possibleCallees.push_back(f);
                    }
                }
            }

            // Expand to derived classes
            auto chIt = clsHierarchy.find(cur);   // map<base, set<derived>>
            if (chIt != clsHierarchy.end())
            {
                for (const string &child : chIt->second)
                {
                    if (visited.insert(child).second)
                    {
                        worklist.push_back(child);
                    }
                }
            }
        }

        return possibleCallees;
    }
    
private:
    void loadFunctions() 
    {
        for (llvm::Function &F : *ModulePtr) 
        {
            functions.insert(&F);
        }
    }

    void loadGlobals() 
    {
        for (llvm::GlobalVariable &GV : ModulePtr->globals()) 
        {
            globals.insert(&GV);
        }
    }

    bool isClsType(llvm::GlobalVariable *GV, std::string prefix = "_ZTV")
    {
        if (!GV->hasName() || !GV->getName().startswith(prefix) || !GV->hasInitializer())
        {
            return false;
        }

        return true;
    }

    string parseTypeNameFromZTVI(const llvm::StringRef &zvName, string prefix = "_ZTV")
    {
        if (!zvName.startswith(prefix)) return "";
        llvm::StringRef s = zvName.drop_front(prefix.size());

        size_t pos = 0;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') pos++;
        return s.drop_front(pos).str();
    }

    void buildClassHierarchy() 
    {
        // Helper: if a constant (after stripping casts) is a GlobalVariable, return it.
        auto stripToGlobal = [&](const llvm::Constant *C) -> llvm::GlobalVariable* {
            if (!C) return nullptr;
            const llvm::Constant *S = llvm::dyn_cast<llvm::Constant>(C->stripPointerCasts());
            if (!S) return nullptr;
            return llvm::dyn_cast<llvm::GlobalVariable>(const_cast<llvm::Constant*>(S));
        };

        for (llvm::GlobalVariable &GV : ModulePtr->globals()) 
        {
            std::string prefix = "_ZTI";
            if (!isClsType(&GV, prefix)) 
            {   
                continue;
            }

            // We only handle single inheritance RTTI objects:
            // __si_class_type_info has 3 fields: (vptr, name, base_typeinfo)
            const llvm::Constant *Init = GV.getInitializer();
            auto *CS = llvm::dyn_cast<llvm::ConstantStruct>(Init);
            if (!CS || CS->getNumOperands() != 3) 
            {
                llvm::errs() << "Skipping non-single-inheritance RTTI object: " 
                             << GV.getName() << " has " 
                             << CS->getNumOperands() << " operands\n";
                continue;
            }

            string derived = parseTypeNameFromZTVI(GV.getName(), prefix);
            if (derived.empty())
            {
                continue;
            } 

            const llvm::Constant *BaseOp = CS->getOperand(2);
            llvm::GlobalVariable *BaseZTI = stripToGlobal(BaseOp);
            if (!BaseZTI || !BaseZTI->hasName()) 
            {
                continue;
            }

            string base = parseTypeNameFromZTVI(BaseZTI->getName(), prefix);
            if (!base.empty())
            {
                clsHierarchy[base].insert(derived);
            }     
        }
    }

    void buildVTables() 
    {
        for (auto &GV : ModulePtr->globals()) 
        {
            if (!isClsType(&GV)) 
            {
                continue;
            }

            std::string cls = parseTypeNameFromZTVI(GV.getName());
            if (cls.empty()) 
            {
                continue;
            }

            // vtable entries are usually inside a constant struct { [N x ...] } { [N x ...] [...] }
            llvm::Constant *Init = GV.getInitializer();
            llvm::ConstantArray *Arr = nullptr;
            if (auto *CS = llvm::dyn_cast<llvm::ConstantStruct>(Init))
            {
                Arr = llvm::dyn_cast<llvm::ConstantArray>(CS->getOperand(0));
            }       
            else
            {
                Arr = llvm::dyn_cast<llvm::ConstantArray>(Init);
            }
                
            if (!Arr) 
            {
                continue;
            }

            // collect function pointers in the initializer
            for (unsigned i = 0; i < Arr->getNumOperands(); i++) 
            {
                llvm::Constant *E = Arr->getOperand(i);
                llvm::Constant *S = llvm::dyn_cast<llvm::Constant>(E->stripPointerCasts());
                if (!S) 
                {
                    continue;
                } 

                if (auto *F = llvm::dyn_cast<llvm::Function>(S))
                {
                    vtables[cls].insert(F);
                    //llvm::errs() << "VTable for class " << cls << " includes function: " << F->getName() << "\n";
                }
            }
        }
    }

    std::string getClassNameByFPtr(llvm::Value *V) 
    {
        V = V->stripPointerCasts();

        auto *LI_fp = llvm::dyn_cast<llvm::LoadInst>(V);
        if (!LI_fp) return "";

        auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(LI_fp->getPointerOperand());
        if (!GEP) return "";

        auto *LI_vptr = llvm::dyn_cast<llvm::LoadInst>(GEP->getPointerOperand());
        if (!LI_vptr) return "";

        auto *BC = llvm::dyn_cast<llvm::BitCastInst>(LI_vptr->getPointerOperand());
        if (!BC) return "";

        llvm::Value *Obj = BC->getOperand(0);
        llvm::Type *ObjTy = Obj->getType();

        if (!ObjTy->isPointerTy()) return "";
        auto *ST = llvm::dyn_cast<llvm::StructType>(ObjTy->getPointerElementType());
        if (!ST || !ST->hasName()) return "";

        llvm::StringRef N = ST->getName();             // "class.Base"
        if (N.startswith("class."))  N = N.drop_front(6);
        if (N.startswith("struct.")) N = N.drop_front(7);
        return N.str();
    }

    bool matchFuncTypeRelaxed(llvm::Function *F, llvm::FunctionType *CallTy)
    {
        if (!F || !CallTy) return false;

        llvm::FunctionType *FT = F->getFunctionType();
        if (!FT) return false;

        if (FT->getReturnType() != CallTy->getReturnType()) return false;
        if (FT->isVarArg() || CallTy->isVarArg()) return false;
        if (FT->getNumParams() != CallTy->getNumParams()) return false;

        for (unsigned i = 0; i < FT->getNumParams(); i++)
        {
            llvm::Type *A = FT->getParamType(i);
            llvm::Type *B = CallTy->getParamType(i);

            if (i == 0)
            {
                // 'this' pointer: allow Derived* vs Base*
                if (!A->isPointerTy() || !B->isPointerTy()) return false;
                continue;
            }

            if (A != B) return false;
        }

        return true;
    }

    bool isFuncPtrTy(llvm::Type *Ty)
    {
        if (!Ty || !Ty->isPointerTy())
        {
            return false;
        } 
        return llvm::isa<llvm::FunctionType>(Ty->getPointerElementType());
    }

    bool isFuncPtrRefTy(llvm::Type *Ty)
    {
        if (!Ty || !Ty->isPointerTy())
        {
            return false;
        } 
        llvm::Type *Elem = Ty->getPointerElementType();
        return isFuncPtrTy(Elem);
    }
    
private:
    unique_ptr<llvm::LLVMContext> Context;
    unique_ptr<llvm::Module> ModulePtr;

    set<llvm::GlobalVariable*> globals;
    set<llvm::Function*> functions;

    map<string, set<string>> clsHierarchy; // base class name -> set of derived class names
    VTSet vtables;
};

#endif
