//
// Created by Israel Lenchner on 11/2/21.
//

#include "ConditionalAccessesChecker.h"
ConditionalAccessesChecker::ConditionalAccessesChecker():bugTypesmap(BTFactory.getEmptyMap()),needsCheck(tasks2CheckF.getEmptySet()){
    std::ifstream MyReadFile("./cache/ConditionalAccessTasks2Check.txt");
    string Line;
    int race_num = 0;
    while (getline (MyReadFile, Line)) {
        if(race_num == MAX_RACES){
            llvm::errs()<<"Reached max races to check, ignoring the next races\n";
            break;
        }
        std::istringstream ss(Line);
        string nameA, nameB;
        ss >> nameA >> nameB;
        needsCheck = tasks2CheckF.add(needsCheck,PairWrapper(std::make_pair(nameA,nameB)));
        BT[race_num][WriteWrite].reset(new BugType(this,"Write-Write collision",
                                         "Race " + nameA + "-" + nameB));
        BT[race_num][ReadWrite].reset(new BugType(this,"Read-Write collision",
                                        "Race " + nameA + "-" + nameB));
        PairWrapper pair= PairWrapper(std::make_pair(nameA,nameB));
        bugTypesmap = BTFactory.add(bugTypesmap, pair ,race_num);
        llvm::errs()<<"Added pair to raceNum mapping. pair: (" << pair.get().first <<", "<< pair.get().second<<
        "). race_num: " <<race_num<< "\n";
        race_num++;
    }
    MyReadFile.close();
    llvm::errs()<<"tasks to check:\n";
    for(auto it = needsCheck.begin(); it!= needsCheck.end(); ++it){
        llvm::errs()<<it->get().first<< ", "<< it->get().second<<"\n";
    }
}

void ConditionalAccessesChecker::checkBeginFunction(CheckerContext &Ctx) const {

//    llvm::errs()<<"checkBeginFunction\n";
    const StackFrameContext *stackFrame = Ctx.getStackFrame();
    const FunctionDecl *Func = dyn_cast<FunctionDecl>(stackFrame->getDecl());
    if (!Func)
        return;
    string funcName = Func->getNameAsString();
    ProgramStateRef State = Ctx.getState();
    auto &F = State->getStateManager().get_context<CurTaskName>();
    //TODO: needs to implement a similar mechanism in the ListAllLoc as weel
    string Name = "";
    if(stackFrame->inTopFrame()){
        // if this is the top frame and this is an aux function then we want to analyze this path.
        // init read/write map
        if(funcName.find(auxFunc) != string::npos){
            Name = funcName;
//            auto &AccessesMapF = State->getStateManager().get_context<AccessesMap>();
//            State = State->set<WriteMap>(StringWrapper(taskA),AccessesMapF.getEmptyMap());
//            State = State->set<WriteMap>(StringWrapper(taskB),AccessesMapF.getEmptyMap());
//            State = State->set<ReadMap>(StringWrapper(taskA),AccessesMapF.getEmptyMap());
//            State = State->set<ReadMap>(StringWrapper(taskB),AccessesMapF.getEmptyMap());

        }
    }else{
        // the set is not empty since this is not top of frame
        string savedName = State->get<CurTaskName>().begin()->get();
        //Check if this is on a path that needs to be checked (starts at the aux func)
        if(savedName!=""){
            //starting a new Task
            if(isTaskNeeds2BChecked(funcName)){
                Name = funcName;
                auto &AccessesMapF = State->getStateManager().get_context<AccessesMap>();
                State = State->set<WriteMap>(StringWrapper(funcName),AccessesMapF.getEmptyMap());
                State = State->set<ReadMap>(StringWrapper(funcName),AccessesMapF.getEmptyMap());
            }

            // this is just a function call inside a task
            else
                Name = savedName;
        }
    }

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
    StateSet::Factory &stateSetF = State->getStateManager().get_context<StateSet>();
    AccessesMap::Factory &accessesMapF = State->getStateManager().get_context<AccessesMap>();

    const AccessesMap *accessesMap = (IsLoad) ? State->get<ReadMap>(CurTaskName):
            State->get<WriteMap>(CurTaskName) ;
    if(!accessesMap)
        llvm::errs()<<"accessesMap is Null\n";
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
    return mutualSet;
}

void ConditionalAccessesChecker::reportCollidingAccesses(const AccessesMap *AMA, string nameA, const AccessesMap *AMB, string nameB,
                                                         BugReporter &BR, AccessKind AAccessType,
                                                         AccessKind BAccessType, int raceNum) const{
    if(!AMA|| !AMB)
        return;
    MemRegionSet mutualSet = findMutualAccesses(AMA, AMB);
    RaceKind raceKind = (AAccessType == Write && BAccessType == Write) ? WriteWrite : ReadWrite;
    for(const MemRegion* memRegion : mutualSet){
        const StateSet *SSA = AMA->lookup(memRegion);
        const StateSet *SSB = AMB->lookup(memRegion);
        // memRegion must be in both maps
        assert(SSA&&SSB);
        string desc = nameA + " " + AccessKindToStrMy(AAccessType) + "s " + memRegion->getDescriptiveName().c_str() +" here.";
//        llvm::errs()<<"Race: found a "<< RaceKindToStr(raceKind)<<" collision between " << nameA << " and "<< nameB << "\n";

        if (!BT[raceNum][raceKind]){
            BT[raceNum][raceKind].reset(new BugType(this, RaceKindToStr(raceKind), "Race " + nameA + "-" + nameB));
        }
        for(auto it = SSA->begin(); it != SSA->end(); ++it)
        {
            auto report =
                    std::make_unique<PathSensitiveBugReport>(*BT[raceNum][raceKind], desc, *it);
            BR.emitReport(std::move(report));
        }
        desc = nameB + " " + AccessKindToStrMy(BAccessType) + "s " + memRegion->getDescriptiveName().c_str();
        for(auto it = SSB->begin(); it != SSB->end(); ++it)
        {
            auto report =
                    std::make_unique<PathSensitiveBugReport>(*BT[raceNum][raceKind], desc, *it);
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
    // this Analysis does not start in an aux function
    if(funcName.find(auxFunc) == string::npos)
        return;
    for( auto i = G.eop_begin(); i!=G.eop_end(); ++i){
        ProgramStateRef State = (*i)->getState();
        WriteMapTy writeMap = State->get<WriteMap>();
        string nameA="", nameB="";
        const AccessesMap *writeAMA = nullptr, *writeAMB = nullptr;
        bool first = true;
        for(auto it=writeMap.begin(); it!=writeMap.end(); ++it){
            nameA = (first)? it->first.get() : nameA;
            nameB= (!first)? it->first.get() : nameB;
            writeAMA = (first) ? writeMap.lookup(it->first.get()) : writeAMA;
            writeAMB = (!first) ? writeMap.lookup(it->first.get()) : writeAMB;
            first = false;
        }
        if(nameA=="" || nameB =="" || !writeAMA || !writeAMB)
            continue;
        WriteMapTy readMap = State->get<ReadMap>();
        const AccessesMap *readAMA = readMap.lookup(nameA);
        const AccessesMap *readAMB = readMap.lookup(nameB);
        if(!readAMA || !readAMB)
            continue;
        PairWrapper pair = PairWrapper(std::make_pair(nameA,nameB));
        PairWrapper pair2 = PairWrapper(std::make_pair(nameB,nameA));
        const int *raceNum = bugTypesmap.lookup(pair);
        if(!raceNum && !(raceNum = bugTypesmap.lookup(pair2))){
            llvm::errs()<<"Cannot find Pair to raceNum mapping for pair: ("<< pair.get().first <<", "<< pair.get().second<< "). skip\n";
            continue;
        }
        reportCollidingAccesses(readAMA,writeAMA, nameA, readAMB, writeAMB, nameB, BR, *raceNum);
    }
}

bool ConditionalAccessesChecker::isTaskNeeds2BChecked(string taskName) const{
    return !getPairs(taskName).empty();
}

std::list<std::string> ConditionalAccessesChecker::getPairs(string taskName) const{
    std::list<string> ret = std::list<string>();
    for(auto it = needsCheck.begin(); it!=needsCheck.end(); ++it){
        if(taskName == it->get().first){
            ret.push_back(it->get().second);
        } else if(taskName == it->get().second){
            ret.push_back(it->get().first);
        }
    }
    return ret;
}

void
ConditionalAccessesChecker::reportCollidingAccesses(const AccessesMap *readA, const AccessesMap *writeA, string nameA,
                                                    const AccessesMap *readB, const AccessesMap *writeB, string nameB,
                                                    BugReporter &BR,int raceNum) const {
//    llvm::errs()<<"reportCollidingAccesses: taskA: "<< nameA<< " taskB: "<< nameB<<"\n";
    reportCollidingAccesses(writeA,nameA, writeB, nameB,BR, Write, Write, raceNum);
    reportCollidingAccesses(writeA,nameA, readB, nameB, BR, Write, Read, raceNum);
    reportCollidingAccesses(readA, nameA, writeB, nameB, BR, Read, Write, raceNum);

}

extern "C" const char clang_analyzerAPIVersionString[] =
        CLANG_ANALYZER_API_VERSION_STRING;


extern "C"
void clang_registerCheckers (CheckerRegistry &registry) {

    registry.addChecker <ConditionalAccessesChecker>("alpha.core.ConditionalAccesses",
                                                     "check for conditional accesses between two Independent tasks","");
    my_checker_registration(registry);
}