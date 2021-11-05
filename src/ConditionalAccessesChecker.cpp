//
// Created by Israel Lenchner on 11/2/21.
//

#include "ConditionalAccessesChecker.h"
//saves for each task read/write access a map between the MemRegion to a set of ProgramStateRef in which it got accesses.
REGISTER_SET_FACTORY_WITH_PROGRAMSTATE(StateSet, ProgramStateRefWrapper)
REGISTER_MAP_WITH_PROGRAMSTATE(AWriteMap, const MemRegion*, StateSet)
REGISTER_MAP_WITH_PROGRAMSTATE(BWriteMap,const MemRegion*, StateSet)
REGISTER_MAP_WITH_PROGRAMSTATE(CurrWriteMap, const MemRegion*, StateSet)
REGISTER_SET_WITH_PROGRAMSTATE(CurTaskName, StringWrapper)


void ConditionalAccessesChecker::checkBeginFunction(CheckerContext &Ctx) const {

//    llvm::errs()<<"checkBeginFunction\n";

    const FunctionDecl *Func = dyn_cast<FunctionDecl>(Ctx.getStackFrame()->getDecl());
    if (!Func)
        return;
    const StackFrameContext *stackFrame = Ctx.getStackFrame();
    // this is only for duplicable task which don't have a direct call (true for all tasks)
    if(!stackFrame->inTopFrame())
        return;
//    llvm::errs()<<"checkBeginFunction: inTopFrame\n";
    string funcName = Func->getNameAsString();
//    llvm::errs()<<"funcName: "<< funcName << "\n";
    ProgramStateRef State = Ctx.getState();
    string Name = (funcName == taskA || funcName == taskB)? funcName: "";
//    llvm::errs()<<"funcName: "<< funcName << "\n";
    auto &F = State->getStateManager().get_context<CurTaskName>();
    State = State->set<CurTaskName>(F.add(F.getEmptySet(),StringWrapper(Name)));
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

    StateSet::Factory &F = State->getStateManager().get_context<StateSet>();

    if(IsLoad){
//           State = State->add<CurrWriteMap>(memRegion);
    }else{
//        const StateSet* write = (CurTaskName==taskA) ? State->get<AWriteMap>(memRegion) : State->get<BWriteMap>(memRegion);
        const StateSet* write = State->get<AWriteMap>(memRegion) ;
        StateSet newSet = (write) ? F.add(*write, ProgramStateRefWrapper(State)) :
                F.add(F.getEmptySet(), ProgramStateRefWrapper(State));
//        State = (CurTaskName==taskA) ? State->set<AWriteMap>(memRegion,newSet) : State->set<BWriteMap>(memRegion,newSet);
        State = State->set<AWriteMap>(memRegion,newSet) ;
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
void checkPath(ExplodedGraph &G, AWriteMapTy writeMap, string Task2Check){
    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
        ProgramStateRef State = (*i)->getState();
        CurTaskNameTy CurTaskNameSet = State->get<CurTaskName>();
        if(CurTaskNameSet.isEmpty()) {
            llvm::errs() << "checkEndAnalysis: CurTaskNameSet is empty\n";
            continue;
        }
        string CurTaskName = CurTaskNameSet.begin()->get();
        if(CurTaskName !=Task2Check)
            continue;
        AWriteMapTy writeMap2 = State->get<AWriteMap>();
        for( auto it = writeMap.begin(); it != writeMap.end(); ++it){
            const StateSet *set2 = writeMap2.lookup(it->first);
            if(!set2)
                continue;
            llvm::errs()<<"found colliding access :"<< it->first->getDescriptiveName().c_str() <<"\n";
            checkPathsViability(&it->second, set2);

        }
    }
}

void ConditionalAccessesChecker::checkEndAnalysis(ExplodedGraph &G, BugReporter &BR, ExprEngine &Eng) const{
    //TODO: filter all end nodes to look only at the relevant ones.
    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
        ProgramStateRef State = (*i)->getState();
        CurTaskNameTy CurTaskNameSet = State->get<CurTaskName>();
        if(CurTaskNameSet.isEmpty()) {
            llvm::errs() << "checkEndAnalysis: CurTaskNameSet is empty\n";
            continue;
        }
        string CurTaskName = CurTaskNameSet.begin()->get();
        if(CurTaskName =="")
            continue;
        AWriteMapTy writeMap = State->get<AWriteMap>();
        checkPath(G, writeMap, (CurTaskName==taskA)? taskB:taskA);

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