#include "cmd_parser.h"
#include "analysis.h"

static inline void addCmdOptions (CommandLineParser& cmdParser)
{
    // Add your new option here
    cmdParser.addOption('b', "", "Specify the path of LLVM bitcode file for analysis");
    cmdParser.addOption('f', "", "Specify the functionality: toy, cg, cfg, icfg, pag, pta, dfg, app");
    cmdParser.addOption('h', "", "Help info");
}

int main(int argc, char** argv) 
{
    CommandLineParser cmdParser(argc, argv);
    addCmdOptions (cmdParser);
    cmdParser.parse ();
    
    if (cmdParser.hasOption("h")) 
    {
        cmdParser.help ();
        return 0;
    }

    if (!cmdParser.hasOption("b"))
    {
        cmdParser.help ();
        return 0;
    }

    string bcFile = cmdParser.getOption ("b");
    LLVM llvmParser (bcFile);

    string funcType = cmdParser.getOption ("f");
    try
    {
        AnalyzerFactory af;
        Analyzer* any = af.getAnalyzer(funcType);
        any->runAnalysis(llvmParser, funcType);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
