#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/GraphWriter.h"
#include "analysis.h"

using namespace llvm;
map<llvm::Value*, set<llvm::CallBase*>> CG::value2IndirectCS;

void AnalyzerFactory::registerAnalyzers() 
{
    analyzers.emplace("toy", std::make_unique<AnalyzerToy>());

    // register your analyzer here
    analyzers.emplace("cg",   std::make_unique<CGAnalyzer>());
    analyzers.emplace("cfg",  std::make_unique<CFGAnalyzer>());
    analyzers.emplace("icfg", std::make_unique<ICFGAnalyzer>());
    analyzers.emplace("pag",  std::make_unique<PAGAnalyzer>());
    analyzers.emplace("pta",  std::make_unique<PTAAnalyzer>());

    return;
}

Analyzer* AnalyzerFactory::getAnalyzer(const std::string& type) 
{
    auto it = analyzers.find(type);
    if (it == analyzers.end()) 
    {
        throw std::runtime_error("Unknown analyzer type: " + type);
    }
    return it->second.get();
}

