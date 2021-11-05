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
class SourceRangeWrapper;

//saves for each task read/write access a map between the MemRegion to a set of ProgramStateRef in which it got accesses.
//REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ProgramStateRefWrapper)
REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ExplodedNode *)
REGISTER_MAP_FACTORY_WITH_PROGRAMSTATE(AccessesMap, const MemRegion*, StateSet)
REGISTER_MAP_WITH_PROGRAMSTATE(WriteMap,StringWrapper, AccessesMap)
REGISTER_MAP_WITH_PROGRAMSTATE(ReadMap,StringWrapper, AccessesMap)
REGISTER_SET_WITH_PROGRAMSTATE(CurTaskName, StringWrapper)


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

    private:
        void reportCollidingAccesses(const AccessesMap *AMA, const AccessesMap *AMB, BugReporter &BR, string bugType,
                                     string AAccessType, string BAccessType) const;
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


class SourceRangeWrapper{
    const SourceRange rangeref;

public:
    SourceRangeWrapper(const SourceRange _range): rangeref(_range){}
    const SourceRange& get() const{ return rangeref;}
    void Profile(llvm::FoldingSetNodeID &ID) const {
        ID.AddInteger(rangeref.getBegin().getRawEncoding());
        ID.AddInteger(rangeref.getEnd().getRawEncoding());
    }
    bool operator==(const SourceRangeWrapper &RHS) const { return rangeref == RHS.rangeref; }
    bool operator<(const SourceRangeWrapper &RHS) const {
        return (rangeref.getBegin()!=RHS.get().getBegin())
        ? rangeref.getBegin()<RHS.get().getBegin() : rangeref.getEnd()<RHS.get().getEnd();
    }


};
#endif //CLAZY_CONDITIONALACCESSESCHECKER_H
