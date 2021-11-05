//
// Created by Israel Lenchner on 11/2/21.
//

#ifndef CLAZY_CONDITIONALACCESSESCHECKER_H
#define CLAZY_CONDITIONALACCESSESCHECKER_H


#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SMTConstraintManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ExplodedGraph.h"
#include "clang/Analysis/CFG.h"

#include "ListAllMemLocChecker.h"

using namespace clang;
using namespace ento;
using std::string;
using std::endl;
class ProgramStateRefWrapper;


namespace {
    typedef Checker<check::Location,check::EndAnalysis, check::BeginFunction> ConditionalCheckerBaseI;
    class ConditionalAccessesChecker : public ConditionalCheckerBaseI {
        mutable std::unique_ptr<BugType> BT;
        std::string taskA = "task_A";
        std::string taskB ="task_B" ;
        std::string auxFunc = "conditional_aux";

    public:
        void checkLocation(SVal Loc, bool IsLoad, const Stmt *S, CheckerContext & C) const;
        void checkBeginFunction(CheckerContext &Ctx) const;
        void checkEndAnalysis(ExplodedGraph &G, BugReporter &BR, ExprEngine &Eng) const;
        ConditionalAccessesChecker(){}

    };
} // end  namespace



class ProgramStateRefWrapper {
    const ProgramStateRef ref;
public:
    ProgramStateRefWrapper(const ProgramStateRef &_ref ) : ref(_ref) {}
    const ProgramStateRef &get() const { return ref; }
    void Profile(llvm::FoldingSetNodeID &ID) const {
        ID.AddPointer(ref.get());
    }
    bool operator==(const ProgramStateRefWrapper &RHS) const { return ref.get() == RHS.ref.get(); }
    bool operator<(const ProgramStateRefWrapper &RHS) const { return ref.get() < RHS.ref.get();}
};
#endif //CLAZY_CONDITIONALACCESSESCHECKER_H
