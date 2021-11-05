//
// Created by Israel Lenchner on 11/2/21.
//

#include "ConditionalAccessesChecker.h"
//saves for each task read/write access a map between the MemRegion to a set of ProgramStateRef in which it got accesses.
REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ProgramStateRefWrapper)
REGISTER_MAP_FACTORY_WITH_PROGRAMSTATE(AccessesMap, const MemRegion*, StateSet)
REGISTER_MAP_WITH_PROGRAMSTATE(WriteMap,StringWrapper, AccessesMap)
REGISTER_SET_WITH_PROGRAMSTATE(CurTaskName, StringWrapper)


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

    //init write map
    if(stackFrame->inTopFrame() && funcName == auxFunc){
        auto &AccessesMapF = State->getStateManager().get_context<AccessesMap>();
        State = State->set<WriteMap>(StringWrapper(taskA),AccessesMapF.getEmptyMap());
        State = State->set<WriteMap>(StringWrapper(taskB),AccessesMapF.getEmptyMap());
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

    if(IsLoad){
//           State = State->add<CurrWriteMap>(memRegion);
    }else{
//        const StateSet* write = (CurTaskName==taskA) ? State->get<AWriteMap>(memRegion) : State->get<BWriteMap>(memRegion);
        const AccessesMap *accessesMap = State->get<WriteMap>(CurTaskName) ;
        const StateSet* write = accessesMap->lookup(memRegion) ;
        StateSet newSet = (write) ? stateSetF.add(*write, ProgramStateRefWrapper(State)) :
                          stateSetF.add(stateSetF.getEmptySet(), ProgramStateRefWrapper(State));
        AccessesMap newAccessesMap = accessesMapF.remove(*accessesMap, memRegion);
        newAccessesMap = accessesMapF.add(newAccessesMap, memRegion, newSet);
        State = State->set<WriteMap>(CurTaskName,newAccessesMap) ;
        C.addTransition(State);
    }

}

bool checkSat(ProgramStateRef State1, ProgramStateRef State2){

    ConstraintSMTType C1 = State1->get<ConstraintSMT>();
    ConstraintSMTType C2 = State2->get<ConstraintSMT>();

//    auto C1 = State1->get<ConstraintSMT>();
//    auto C2 = State1->get<ConstraintSMT>();
    llvm::SMTSolverRef Solver = llvm::CreateZ3Solver();
    auto I1 = C1.begin(), IE1 = C1.end();
    auto I2 = C2.begin(), IE2 = C2.end();
    if(I1 == IE1 ){
        llvm::errs()<<"checkSat E1 empty\n";
    }
    if(I2 == IE2){
        llvm::errs()<<"checkSat E2 empty\n";
    }
    // if one of the states don't have constrains and the other is satisfiable then the union is satisfiable
    if(I1 == IE1 || I2 == IE2){
        llvm::errs()<<"checkSat empty\n";
        return true;
    }

    llvm::SMTExprRef Constraint = I1++->second;
    while (I1 != IE1) {
        Constraint = Solver->mkAnd(Constraint, I1++->second);
    }
    while (I2 != IE2) {
        Constraint = Solver->mkAnd(Constraint, I2++->second);
    }
    Solver->addConstraint(Constraint);

    Optional<bool> res = Solver->check();
    llvm::errs()<<"res.getValue()" << res.getValue()  <<"\n";
    if (!res.hasValue())
        llvm::errs()<<"!res.hasValue()\n";
    else
        llvm::errs()<<"res.hasValue()\n";

    return true;

}

void checkPathsViability(const StateSet *set1,const StateSet *set2){
    bool pathsViable = false;
    for(auto it1 = set1->begin(); it1!=set1->end(); ++it1){
        for(auto it2 = set2->begin(); it2!=set2->end(); ++it2){
            ProgramStateRef State1 = it1->get();
            ProgramStateRef State2 = it2->get();
            //TODO: use a safe type conversion
            SMTConstraintManager *CM = (SMTConstraintManager*) &State1->getConstraintManager();

            checkSat(State1,State2);

            State1->getConstraintManager().printJson(llvm::errs(),State1, "\n", 4, false);
            State2->getConstraintManager().printJson(llvm::errs(),State2, "\n", 4, false);

            ConstraintSMTType C1 = State1->get<ConstraintSMT>();
            ConstraintSMTType C2 = State2->get<ConstraintSMT>();

//    auto C1 = State1->get<ConstraintSMT>();
//    auto C2 = State1->get<ConstraintSMT>();
            llvm::SMTSolverRef Solver = llvm::CreateZ3Solver();
            auto I1 = C1.begin(), IE1 = C1.end();
            auto I2 = C2.begin(), IE2 = C2.end();
            if(I1 == IE1 ){
                llvm::errs()<<"checkPathsViability E1 empty\n";
            }
            if(I2 == IE2){
                llvm::errs()<<"checkPathsViability E2 empty\n";
            }
            // if one of the states don't have constrains and the other is satisfiable then the union is satisfiable
            if(I1 == IE1 || I2 == IE2){
                llvm::errs()<<"checkPathsViability empty\n";
            }

        }
    }
}
//void checkPath(ExplodedGraph &G, AWriteMapTy writeMap, string Task2Check){
//    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
//        ProgramStateRef State = (*i)->getState();
//        CurTaskNameTy CurTaskNameSet = State->get<CurTaskName>();
//        if(CurTaskNameSet.isEmpty()) {
//            llvm::errs() << "checkEndAnalysis: CurTaskNameSet is empty\n";
//            continue;
//        }
//        string CurTaskName = CurTaskNameSet.begin()->get();
//        if(CurTaskName !=Task2Check)
//            continue;
//        AWriteMapTy writeMap2 = State->get<AWriteMap>();
//        for( auto it = writeMap.begin(); it != writeMap.end(); ++it){
//            const StateSet *set2 = writeMap2.lookup(it->first);
//            if(!set2)
//                continue;
//            llvm::errs()<<"found colliding access :"<< it->first->getDescriptiveName().c_str() <<"\n";
//            checkPathsViability(&it->second, set2);
//
//        }
//    }
//}
using MemRegionSet = std::set<const MemRegion*>;
MemRegionSet findMutualAccesses(const AccessesMap *AMA, const AccessesMap *AMB){
    MemRegionSet mutualSet = MemRegionSet();
    for(auto it = AMA->begin(); it != AMA->end(); ++it){
        if(AMB->contains(it->first))
            mutualSet.insert(it->first);
    }

    for(const MemRegion* memRegion : mutualSet){
        llvm::errs()<<"colliding access on: "<< memRegion->getDescriptiveName().c_str() <<"\n";
    }
    return mutualSet;
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

    //TODO: filter all end nodes to look only at the relevant ones.
    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
        ProgramStateRef State = (*i)->getState();
        WriteMapTy writeMap = State->get<WriteMap>();
        MemRegionSet mutualSet = findMutualAccesses(writeMap.lookup(taskA), writeMap.lookup(taskB));

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