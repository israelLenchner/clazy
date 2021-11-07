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

using pairTy = std::pair<std::string, std::string>;
class PairWrapper {
    const pairTy _pair;
public:
    PairWrapper(const pairTy& newPair) : _pair(newPair) {}
    const pairTy &get() const { return _pair; }
    void Profile(llvm::FoldingSetNodeID &ID) const {
        ID.AddString(_pair.first);
        ID.AddString(_pair.second);
    }
    bool operator==(const PairWrapper &RHS) const { return _pair == RHS._pair; }
    bool operator<(const PairWrapper &RHS) const { return _pair < RHS._pair; }
};

enum RaceKind {
    WriteWrite,
    ReadWrite,
    RaceKind_NumKinds
};
string RaceKindToStr(RaceKind race){
    switch (race) {
        case WriteWrite: return "Write-Write Collision";
        case ReadWrite: return "Read-Write Collision";
        default: return "Unknown";
    }
}
enum AccessKind {
    Write,
    Read,
    AccessKind_NumKinds
};
string AccessKindToStrMy(AccessKind access){
    switch (access) {
        case Write: return "Write";
        case Read: return "Read";
        default: return "Unknown";
    }
}
class BugTypeWrapper{
    std::array<std::unique_ptr<BugType>, RaceKind_NumKinds> BT;
public:
    BugTypeWrapper(const std::array<std::unique_ptr<BugType>, RaceKind_NumKinds>& _BT = std::array<std::unique_ptr<BugType>, RaceKind_NumKinds>()) {
        BT[WriteWrite].reset(_BT[WriteWrite].get());
        BT[ReadWrite].reset(_BT[ReadWrite].get());
    }
    const std::array<std::unique_ptr<BugType>, RaceKind_NumKinds>& get(){return BT;}
    void Profile(llvm::FoldingSetNodeID &ID) const {
        ID.AddPointer(BT[WriteWrite].get());
        ID.AddPointer(BT[ReadWrite].get());
    }
    bool operator==(const BugTypeWrapper &RHS) const { return BT == RHS.BT; }
    bool operator<(const BugTypeWrapper &RHS) const { return BT < RHS.BT; }
};

//saves for each task read/write access a map between the MemRegion to a set of ProgramStateRef in which it got accesses.
//REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ProgramStateRefWrapper)
REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ExplodedNode *)
REGISTER_MAP_FACTORY_WITH_PROGRAMSTATE(AccessesMap, const MemRegion*, StateSet)
REGISTER_MAP_WITH_PROGRAMSTATE(WriteMap,StringWrapper, AccessesMap)
REGISTER_MAP_WITH_PROGRAMSTATE(ReadMap,StringWrapper, AccessesMap)
REGISTER_SET_WITH_PROGRAMSTATE(CurTaskName, StringWrapper)


REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(Tasks2Check, PairWrapper)
REGISTER_MAP_FACTORY_WITH_PROGRAMSTATE(BugTypes, PairWrapper,  int)
#define MAX_RACES 40

namespace {
    typedef Checker<check::Location,check::EndAnalysis, check::BeginFunction> ConditionalCheckerBaseI;
    class ConditionalAccessesChecker : public ConditionalCheckerBaseI {



        mutable std::unique_ptr<BugType> BT[MAX_RACES][RaceKind_NumKinds];
        BugTypes::Factory BTFactory;
        BugTypes bugTypesmap;
        Tasks2Check::Factory tasks2CheckF;
        Tasks2Check needsCheck;
        std::string auxFunc = "conditional_aux";

    public:
        void checkLocation(SVal Loc, bool IsLoad, const Stmt *S, CheckerContext & C) const;
        void checkBeginFunction(CheckerContext &Ctx) const;
        void checkEndAnalysis(ExplodedGraph &G, BugReporter &BR, ExprEngine &Eng) const;
        ConditionalAccessesChecker();

    private:
        void reportCollidingAccesses(const AccessesMap *AMA, string nameA, const AccessesMap *AMB, string nameB,
                                     BugReporter &BR, AccessKind AAccessType,
                                     AccessKind BAccessType, int raceNum) const;
        void reportCollidingAccesses(const AccessesMap *readA, const AccessesMap *writeA, string nameA,
                                     const AccessesMap *readB, const AccessesMap *writeB, string nameB,
                                     BugReporter &BR, int raceNum) const;
        bool isTaskNeeds2BChecked(string taskName) const;
        std::list<std::string> getPairs(string taskName) const;
    };
} // end  namespace







#endif //CLAZY_CONDITIONALACCESSESCHECKER_H
