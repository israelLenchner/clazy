//
// Created by israel lenchner on 22/06/2021.
//

#ifndef CLAZY_LISTALLMEMLOCCHECKER_H
#define CLAZY_LISTALLMEMLOCCHECKER_H


#include "clang/StaticAnalyzer/Checkers/BuiltinCheckerRegistration.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Frontend/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/Analysis/CFG.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

class StringWrapper;



using namespace clang;
using namespace ento;
using std::string;
using std::endl;
using dupSetTy = llvm::ImmutableSet<StringWrapper>;
using dupMepTy = llvm::ImmutableMap<StringWrapper,int>;


typedef std::set<const int> regionSet;
typedef const MemRegion* accessElement;


class StringWrapper {
    const std::string Str;
public:
    StringWrapper(const std::string &S = "") : Str(S) {}
    const std::string &get() const { return Str; } void Profile(llvm::FoldingSetNodeID &ID) const {
        ID.AddString(Str); }
    bool operator==(const StringWrapper &RHS) const { return Str == RHS.Str; }
    bool operator<(const StringWrapper &RHS) const { return Str < RHS.Str; }
};


namespace {
//    typedef Checker<check::Location,check::EndAnalysis,check::EndFunction, check::ASTDecl<FunctionDecl>> CheckerBaseI;
    typedef Checker<check::Location,check::EndAnalysis,check::EndFunction, check::BeginFunction, check::PreCall> CheckerBaseI;
    bool IsGlobalVAriable(const MemRegion* memRegion) {
        const MemSpaceRegion *MS = memRegion->getMemorySpace();
        return isa<GlobalsSpaceRegion>(MS);
    }
    class ListAllMemLocChecker : public CheckerBaseI {

        mutable std::unique_ptr<BugType> BT;
        dupSetTy::Factory F;
        dupSetTy duplicablesSet;
        dupMepTy::Factory mapFactory;
        dupMepTy duplicabsleMap;
    public:
        void checkLocation(SVal Loc, bool IsLoad, const Stmt *S, CheckerContext & C) const;
        void checkEndAnalysis(ExplodedGraph &G, BugReporter &BR, ExprEngine &Eng) const;
        void checkEndFunction(const ReturnStmt *RS, CheckerContext &Ctx) const;
        void checkBeginFunction(CheckerContext &Ctx) const;
        void checkPreCall(const CallEvent &Call, CheckerContext &C) const;
        ListAllMemLocChecker();
    };
} // end anonymous namespace






#endif //CLAZY_LISTALLMEMLOCCHECKER_H