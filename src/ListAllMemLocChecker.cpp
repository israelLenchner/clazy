//
// Created by Israel Lenchner on 06/06/2021.
//

#include "ListAllMemLocChecker.h"

using namespace clang;
using namespace ento;
using std::string;
using std::endl;
using dupSetTy = llvm::ImmutableSet<StringWrapper>;





//REGISTER_SET_WITH_PROGRAMSTATE(accessList,accessElement)
//REGISTER_MAP_WITH_PROGRAMSTATE(accessMap, accessElement, bool)


REGISTER_SET_WITH_PROGRAMSTATE(loadAccessList,accessElement)
REGISTER_SET_WITH_PROGRAMSTATE(storeAccessList,accessElement)
REGISTER_SET_WITH_PROGRAMSTATE(instanceIDState, StringWrapper)


ListAllMemLocChecker::ListAllMemLocChecker(): F(true),duplicablesSet(F.getEmptySet()), mapFactory(true), duplicabsleMap(mapFactory.getEmptyMap()){
    std::ifstream MyReadFile("./cache/duplicable_list.txt");
    string Line;
    while (getline (MyReadFile, Line)) {
        std::istringstream ss(Line);
        string dupName;
        int num;
        ss >> dupName >> num;
        duplicablesSet =F.add(duplicablesSet, StringWrapper(dupName));
        duplicabsleMap = mapFactory.add(duplicabsleMap, StringWrapper(dupName), num);
    }
    MyReadFile.close();

}

/// Pre-visit an abstract "call" event.
///
/// This is used for checkers that want to check arguments or attributed
/// behavior for functions and methods no matter how they are being invoked.
///
/// Note that this includes ALL cross-body invocations, so if you want to
/// limit your checks to, say, function calls, you should test for that at the
/// beginning of your callback function.
///
/// check::PreCall
void ListAllMemLocChecker::checkPreCall(const CallEvent &Call, CheckerContext &C) const {
    llvm::errs()<<"checkPreCall\n";
    const AnyFunctionCall *FC = dyn_cast<AnyFunctionCall>(&Call);
    // we need only functions call
    if(!FC)
        return;
    const FunctionDecl *FuncDecl = FC->getDecl();
    string funcName = FuncDecl->getNameAsString();
    // we need the set quota calls only
    if(funcName != "HAL_TASK_SET_QUOTA")
        return;
    llvm::errs()<<"call to HAL_TASK_SET_QUOTA\n";
    SVal quotaSVal = Call.getArgSVal(0);
    quotaSVal.dump();
    llvm::errs()<<"\n";

    Optional<nonloc::ConcreteInt> CI = quotaSVal.getAs<nonloc::ConcreteInt>();
    // the quota value for this call is known at compile time
    if(CI){
        // tge first getValue() is for the Optional to gibe the actual nonloc::ConcreteInt.
        // the second is for nonloc::ConcreteInt to give us the value (llvm::APSInt)
        uint64_t Int = CI.getValue().getValue().getLimitedValue();
        llvm::errs()<<"the concrete value of the quota is "<< std::to_string(Int)<<"\n";
    }

//    ConstraintManager &CM = C.getConstraintManager();
//    //CM.assumeInclusiveRange()
//    const llvm::APSInt* quotaVal = CM.getSymVal(C.getState(), quotaSVal.getAsSymExpr());
//    if(quotaVal){
//
//        llvm::errs()<<"CM.getSymVal(quotaSVal) returned "<< quotaVal->toString(10)<<"\n";
//    }else{
//        llvm::errs()<<"CM.getSymVal(quotaSVal) returned NULL\n";
//    }
    //C.getAnalysisManager().getConstraintManagerCreator()
}

/// Called when the analyzer core starts analyzing a function,
/// regardless of whether it is analyzed at the top level or is inlined.
///
/// check::BeginFunction
/// this callback will be called for both analyzing the top level and for inlining analyze, I'm assuming that there is
/// no direct call for tasks, so we will analyze it only for the top level
void ListAllMemLocChecker::checkBeginFunction(CheckerContext &Ctx) const {

    const FunctionDecl *Func = dyn_cast<FunctionDecl>(Ctx.getStackFrame()->getDecl());
    if(!Func)
        return;
    string funcName = Func->getNameAsString();
    llvm::errs()<<"analyzing func: " << funcName << "\n";
    // we need to do thia analysis only for duplicable tasks.
    if(!duplicabsleMap.contains(StringWrapper(funcName))){
        return;
    }
    if(!duplicablesSet.contains(StringWrapper(funcName))){
        return;
    }
    const StackFrameContext *stackFrame = Ctx.getStackFrame();
    // this is only for duplicable task which dont have a direct call (true for all tasks)
    assert(stackFrame->inTopFrame());
    // duplicable tasks have only 1 parameter the Instance id
    assert(Func->getNumParams()==1);
    const ParmVarDecl *instanceID = Func->getParamDecl(0);
    llvm::errs()<<"dump instanceID ParmVarDecl: \n";
    instanceID->dump();
    const auto *LCtx = Ctx.getLocationContext();
    auto &State = Ctx.getState();
    auto &SVB = Ctx.getSValBuilder();
    auto Param = SVB.makeLoc(State->getRegion(Func->getParamDecl(0), LCtx));
    auto InstanceIDSval = State->getSVal(Param);
    InstanceIDSval.dump();
    llvm::errs()<<"\n";

    dupMepTy::data_type *maxInstances = duplicabsleMap.lookup(StringWrapper(funcName));
    llvm::errs()<<"\n dup: " <<funcName<<" num_instances: "<< std::to_string(*maxInstances)<<"\n";
    for(int i=0;i<*maxInstances;i++){
        llvm::APSInt number(std::to_string(i).c_str());
        Optional<DefinedOrUnknownSVal> sval = InstanceIDSval.castAs <DefinedOrUnknownSVal >();
        if(!sval)
            llvm::errs()<<" InstanceIDSval.castAs <DefinedOrUnknownSVal >() returned NULL\n";

        auto &SVB = State->getStateManager().getSValBuilder();
        DefinedOrUnknownSVal eq_cond = SVB.evalEQ(State, sval.getValue(), SVB.makeIntVal(number).castAs<DefinedOrUnknownSVal>());
        auto newState = State->assume(eq_cond, true);
//        auto newState = State->assumeInclusiveRange(sval.getValue(), number, number, true);
        newState = newState->add<instanceIDState>(StringWrapper(std::to_string(i).c_str()));
        Ctx.addTransition(newState);
    }

}

/// Called when the analyzer core reaches the end of a
/// function being analyzed regardless of whether it is analyzed at the top
/// level or is inlined.
///
/// check::EndFunction

void ListAllMemLocChecker::checkEndFunction(const ReturnStmt *RS, CheckerContext &Ctx) const {

    //TODO: needs to dump memory access only when we finish analyzing a task, not any function
    ProgramStateRef State = Ctx.getState();

    //const auto *LCtx = Ctx.getLocationContext();
    //AnalysisDeclContext * declarationComtectm= LCtx->getAnalysisDeclContext();

//    const accessListTy &Syms = State->get<accessList>();
    std::string msg;

    llvm::raw_string_ostream os(msg);

    const FunctionDecl *EnclosingFunctionDecl = dyn_cast<FunctionDecl>(Ctx.getStackFrame()->getDecl());
    if(!EnclosingFunctionDecl)
        return;
    std::ofstream fs;
    std::string funcName = (EnclosingFunctionDecl->getName()).str();
    if(duplicablesSet.contains(StringWrapper(funcName))){
        fs = std::ofstream ("./cache/DuplicableMemoryLocations", std::ofstream::out | std::ofstream::app);
        const instanceIDStateTy &instanceIDSet = State->get<instanceIDState>();
//        instanceIDSet.begin()
        fs<<funcName<<"$$$$"<<(*(instanceIDSet.begin())).get()<<"$$$$";
        State =  State->remove<instanceIDState>(*(instanceIDSet.begin()));
    }else{
        fs = std::ofstream("./cache/memoryLocations", std::ofstream::out | std::ofstream::app);
        fs<<funcName<<"$$$$";
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
    if(Loc.getAsSymExpr()){
        llvm::errs()<<"getAsSymExpr\n";
        Loc.getAsSymExpr()->dump();
    }

    const MemRegion* memRegion = Loc.getAsRegion();
    ProgramStateRef State = C.getState();
    if(IsGlobalVAriable(memRegion)){
        if(IsLoad){
            State = State->add<loadAccessList>(memRegion);
        }else{
            State = State->add<storeAccessList>(memRegion);
        }
    }

//    ExplodedNode *N = C.generateNonFatalErrorNode();
//    if (!N)
//        return;
//
//    if (!BT)
//        BT.reset(new BugType(this, "call to main", "example analyzer plugin"));
//
//    auto report =
//            std::make_unique<PathSensitiveBugReport>(*BT, BT->getDescription(), N);
////    report->addRange(Callee->getSourceRange());
//    C.emitReport(std::move(report));
//    llvm::errs()<<"emitting report\n";

    C.addTransition(State);

}



extern "C" void my_checker_registration(CheckerRegistry &registry){
    registry.addChecker <ListAllMemLocChecker >("alpha.core.ListAllMemLoc",
                                                "save all memory accesses locations to a file","");
}