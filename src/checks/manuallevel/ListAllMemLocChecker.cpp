//
// Created by israel lenchner on 06/06/2021.
//

#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/Analysis/CFG.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
using namespace clang;
using namespace ento;
using std::string;
using std::endl;
namespace {
    bool IsGlobalVAriable(const MemRegion* memRegion) {
        const MemSpaceRegion *MS = memRegion->getMemorySpace();
        return isa<GlobalsSpaceRegion>(MS);
    }
    class ListAllMemLocChecker : public Checker<check::Location,check::EndAnalysis,check::EndFunction> {
        mutable std::unique_ptr<BugType> BT;

    public:
        void checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                           CheckerContext & C) const;
        void checkEndAnalysis(ExplodedGraph &G,
                              BugReporter &BR,
                              ExprEngine &Eng) const;
        void checkEndFunction(const ReturnStmt *RS, CheckerContext &Ctx) const;
    };
} // end anonymous namespace

typedef std::set<const int> regionSet;
typedef const MemRegion* accessElement;


REGISTER_SET_WITH_PROGRAMSTATE(accessList,accessElement)
REGISTER_SET_WITH_PROGRAMSTATE(loadAccessList,accessElement)
REGISTER_SET_WITH_PROGRAMSTATE(storeAccessList,accessElement)
REGISTER_MAP_WITH_PROGRAMSTATE(accessMap, accessElement, bool)

/// Called when the analyzer core reaches the end of a
/// function being analyzed regardless of whether it is analyzed at the top
/// level or is inlined.
///
/// check::EndFunction

void ListAllMemLocChecker::checkEndFunction(const ReturnStmt *RS, CheckerContext &Ctx) const {


    ProgramStateRef State = Ctx.getState();


    //const auto *LCtx = Ctx.getLocationContext();
    //AnalysisDeclContext * declarationComtectm= LCtx->getAnalysisDeclContext();

    const accessListTy &Syms = State->get<accessList>();
    std::string msg;
    llvm::raw_string_ostream os(msg);
    std::ofstream fs("./cache/memoryLocations",std::ofstream::out | std::ofstream::app);

    for (auto I = Syms.begin(), E = Syms.end(); I != E; ++I) {
        accessElement elem=*I;
//        os<<"   ,"<<elem->getDescriptiveName();
        State = State->remove<accessList>(elem);
        //os<<std::get<0>(elem)<<"    ,   "<<std::get<1>(elem)<<"         ";
    }

    if (const FunctionDecl *EnclosingFunctionDecl =
            dyn_cast<FunctionDecl>(Ctx.getStackFrame()->getDecl())){
        fs<<(EnclosingFunctionDecl->getName()).str()<<"$$$$";
    }else{
        fs<<"$$$$"<<"unknown"<<"$$$$";
    }

    const loadAccessListTy &loadSymsList = State->get<loadAccessList>();

    for (loadAccessListTy::iterator I = loadSymsList.begin(), E = loadSymsList.end(); I != E; ++I) {
        accessElement elem=*I;

        fs<<elem->getDescriptiveName().c_str();
        loadAccessListTy::iterator T=I;
        if(T++ != E)
            fs<<";";
        State = State->remove<loadAccessList>(elem);

    }
    const storeAccessListTy &storeSymsList = State->get<storeAccessList>();

    fs<<"$$$$";
    for (storeAccessListTy::iterator I = storeSymsList.begin(), E = storeSymsList.end(); I != E; ++I) {
        accessElement elem=*I;
        fs<<elem->getDescriptiveName().c_str();
        loadAccessListTy::iterator T=I;
        if(T++ != E)
            fs<<";";
        State = State->remove<storeAccessList>(elem);
    }
    fs<<endl;
    Ctx.addTransition(State);
//    we dont need to report a bug becaose we print the data to file/
//    ExplodedNode *N = Ctx.generateNonFatalErrorNode();
//    if (!N)
//        return;
//
//    if (!BT)
//        BT.reset(new BugType(this, os.str(), "bla"));
//
//    std::unique_ptr<BugReport> report =
//    llvm::make_unique<BugReport>(*BT,os.str(), N);
//    //llvm::make_unique<BugReport>(*BT, BT->getName(), N);
//    //report->addRange(S->getSourceRange());
//    Ctx.emitReport(std::move(report));
    fs.close();

}

void ListAllMemLocChecker::checkEndAnalysis(ExplodedGraph &G,
                                            BugReporter &BR,
                                            ExprEngine &Eng) const{


    // ((Eng.getCheckerManager()).getChecker<ListAllMemLocChecker>())->c
    //    ProgramStateRef State = Ctx.getState();
    //    llvm::ImmutableMap<std::string, regionSet> myMap(State->get<accessList>());
    //myMap.foreach(Callback &C)


}
/// Called on a load from and a store to a location.
///
/// The method will be called each time a location (pointer) value is
/// accessed.
/// \param Loc    The value of the location (pointer).
/// \param IsLoad The flag specifying if the location is a store or a load.
/// \param S      The load is performed while processing the statement.
///
/// check::Location
void ListAllMemLocChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                                         CheckerContext & C) const{
    std::string msg;
    llvm::raw_string_ostream os(msg);
//    Loc.dumpToStream(os);
    //Loc.dump();
    const MemRegion* memRegion = Loc.getAsRegion();
//    os<< "  memRegion->getDescriptiveName: " << memRegion->getDescriptiveName();
//    os<<"       memRegion->getString():"<<memRegion->getString();
    ProgramStateRef State = C.getState();
    State = State->add<accessList>(memRegion);
    State = State->set<accessMap>(memRegion,IsLoad);
    if(IsGlobalVAriable(memRegion)){
        if(IsLoad){
            State = State->add<loadAccessList>(memRegion);
        }else{
            State = State->add<storeAccessList>(memRegion);
        }
    }

    C.addTransition(State);

}

extern "C" const char clang_analyzerAPIVersionString[] =
        CLANG_ANALYZER_API_VERSION_STRING;


extern "C"
void clang_registerCheckers (CheckerRegistry &registry) {
    registry.addChecker <ListAllMemLocChecker >("alpha.core.ListAllMemLoc",
                                          "save all memory accesses locations to a file","");
}
