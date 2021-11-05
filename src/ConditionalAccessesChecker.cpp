//
// Created by Israel Lenchner on 11/2/21.
//

#include "ConditionalAccessesChecker.h"


void ConditionalAccessesChecker::checkBeginFunction(CheckerContext &Ctx) const {

//    llvm::errs()<<"checkBeginFunction\n";
    const StackFrameContext *stackFrame = Ctx.getStackFrame();
    const FunctionDecl *Func = dyn_cast<FunctionDecl>(stackFrame->getDecl());
    if (!Func)
        return;
    string funcName = Func->getNameAsString();

    // this is only for duplicable task which don't have a direct call (true for all tasks)
    // Since we use a workaround, the Duplicable tasks are not in top Frame
//    if(!stackFrame->inTopFrame())
//        return;

    ProgramStateRef State = Ctx.getState();
    string Name = ( (!stackFrame->inTopFrame()) && (funcName == taskA || funcName == taskB) ) ? funcName: "";
    auto &F = State->getStateManager().get_context<CurTaskName>();
    State = State->set<CurTaskName>(F.add(F.getEmptySet(),StringWrapper(Name)));

    //init read/write map
    if(stackFrame->inTopFrame() && funcName == auxFunc){
        auto &AccessesMapF = State->getStateManager().get_context<AccessesMap>();
        State = State->set<WriteMap>(StringWrapper(taskA),AccessesMapF.getEmptyMap());
        State = State->set<WriteMap>(StringWrapper(taskB),AccessesMapF.getEmptyMap());
        State = State->set<ReadMap>(StringWrapper(taskA),AccessesMapF.getEmptyMap());
        State = State->set<ReadMap>(StringWrapper(taskB),AccessesMapF.getEmptyMap());
    }
    Ctx.addTransition(State);


}


void ConditionalAccessesChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S, CheckerContext &C) const {

    const MemRegion* memRegion = Loc.getAsRegion();
    if(!IsGlobalVAriable(memRegion))
        return;
    ProgramStateRef State = C.getState();
    CurTaskNameTy CurTaskNameSet = State->get<CurTaskName>();
    if(CurTaskNameSet.isEmpty()) {
        llvm::errs() << "CurTaskNameSet is empty\n";
        return;
    }
    string CurTaskName = CurTaskNameSet.begin()->get();
    if(CurTaskName =="")
        return;

    StateSet::Factory &stateSetF = State->getStateManager().get_context<StateSet>();
    AccessesMap::Factory &accessesMapF = State->getStateManager().get_context<AccessesMap>();

    const AccessesMap *accessesMap = (IsLoad) ? State->get<ReadMap>(CurTaskName):
            State->get<WriteMap>(CurTaskName) ;
    const StateSet* write = accessesMap->lookup(memRegion) ;
    // Later, in checkEndAnalysis, we'd throw a report against it if a Race is found.
    ExplodedNode *errorNode = C.generateNonFatalErrorNode();
    StateSet newSet = (write) ? stateSetF.add(*write, errorNode) :
                      stateSetF.add(stateSetF.getEmptySet(), errorNode);
    AccessesMap newAccessesMap = accessesMapF.remove(*accessesMap, memRegion);
    newAccessesMap = accessesMapF.add(newAccessesMap, memRegion, newSet);
    State = (IsLoad) ?State->set<ReadMap>(CurTaskName,newAccessesMap) :
            State->set<WriteMap>(CurTaskName,newAccessesMap) ;
    C.addTransition(State);

}



using MemRegionSet = std::set<const MemRegion*>;

MemRegionSet findMutualAccesses(const AccessesMap *AMA, const AccessesMap *AMB){

    MemRegionSet mutualSet = MemRegionSet();
    if(!AMA || !AMB)
        return mutualSet;

    for(auto it = AMA->begin(); it != AMA->end(); ++it){
        if(AMB->contains(it->first))
            mutualSet.insert(it->first);
    }

    for(const MemRegion* memRegion : mutualSet){
        llvm::errs()<<"colliding access on: "<< memRegion->getDescriptiveName().c_str() <<"\n";
    }
    return mutualSet;
}

void ConditionalAccessesChecker::reportCollidingAccesses(const AccessesMap *AMA, const AccessesMap *AMB,
                                                         BugReporter &BR, string bugType, string AAccessType,
                                                         string BAccessType) const{
    if(!AMA|| !AMB)
        return;
    llvm::errs()<<"reportCollidingAccesses: "<<"A " << AAccessType << "s "<< "B "<< BAccessType<< "s\n";
    MemRegionSet mutualSet = findMutualAccesses(AMA, AMB);
    llvm::errs()<<"mutualSet size: "<< mutualSet.size()<<"\n";
    for(const MemRegion* memRegion : mutualSet){
        const StateSet *SSA = AMA->lookup(memRegion);
        const StateSet *SSB = AMB->lookup(memRegion);
        // memRegion must be in both maps
        assert(SSA&&SSB);
//            reportCollidingAccesses(SSA, SSB, BR, (*i));

        if (!BT)
            BT.reset(new BugType(this, bugType, categories::MemoryError));
        string desc = string("A ") + AAccessType + "s " + memRegion->getDescriptiveName().c_str();
        for(auto it = SSA->begin(); it != SSA->end(); ++it)
        {
            auto report =
                    std::make_unique<PathSensitiveBugReport>(*BT, desc, *it);
            BR.emitReport(std::move(report));
        }
        desc = string("B ") + BAccessType + "s " + memRegion->getDescriptiveName().c_str();
        for(auto it = SSB->begin(); it != SSB->end(); ++it)
        {
            auto report =
                    std::make_unique<PathSensitiveBugReport>(*BT, desc, *it);
            BR.emitReport(std::move(report));
        }
    }
}

void ConditionalAccessesChecker::checkEndAnalysis(ExplodedGraph &G, BugReporter &BR, ExprEngine &Eng) const{
    //First check if this is the aux function
    string funcName = "";
    for( auto i = G.roots_begin(); i!=G.roots_end(); ++i){
        const StackFrameContext * stackFrame = (*i)->getStackFrame();
        if(!stackFrame->inTopFrame()){
            llvm::errs()<<"checkEndAnalysis: root is not top frame\n";
            return;
        }
        const FunctionDecl *Func = dyn_cast<FunctionDecl>(stackFrame->getDecl());
        if (!Func){
            llvm::errs()<<"checkEndAnalysis: root is not Function declaration\n";
            return;
        }
        funcName = Func->getNameAsString();
        break;
    }
    if(funcName!=auxFunc)
        return;
    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
        ProgramStateRef State = (*i)->getState();
        WriteMapTy writeMap = State->get<WriteMap>();
        const AccessesMap *writeAMA = writeMap.lookup(taskA);
        const AccessesMap *writeAMB = writeMap.lookup(taskB);
        WriteMapTy readMap = State->get<ReadMap>();
        const AccessesMap *readAMA = readMap.lookup(taskA);
        const AccessesMap *readAMB = readMap.lookup(taskB);
        if(!writeAMA || !writeAMB)
            continue;
        reportCollidingAccesses(writeAMA, writeAMB, BR, "Write-Write collision", "write", "write");
        reportCollidingAccesses(writeAMA, readAMB, BR, "Write-Read collision", "write", "read");
        reportCollidingAccesses(readAMA, writeAMB, BR, "Write-Read collision", "read", "write");
    }
}



extern "C" const char clang_analyzerAPIVersionString[] =
        CLANG_ANALYZER_API_VERSION_STRING;


extern "C"
void clang_registerCheckers (CheckerRegistry &registry) {

    registry.addChecker <ConditionalAccessesChecker>("alpha.core.ConditionalAccesses",
                                                     "check for conditional accesses between two Independent tasks","");
    my_checker_registration(registry);
}