/*
  This file is part of the clazy static checker.

  Copyright (C) 2021 Author <your@email>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "new-check-test.h"
#include "Utils.h"
#include "HierarchyUtils.h"
#include "QtUtils.h"
#include "TypeUtils.h"

#include <clang/AST/AST.h>


#include <clang/StaticAnalyzer/Frontend/CheckerRegistry.h>
#include <clang/StaticAnalyzer/Core/BugReporter/BugType.h>
#include <clang/StaticAnalyzer/Core/Checker.h>
#include <clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h>
#include <clang/StaticAnalyzer/Frontend/CheckerRegistry.h>

using namespace clang;
using namespace ento;


//NewCheckTest::NewCheckTest(const std::string &name, ClazyContext *context)
//    : CheckBase(name, context)
//{
//    llvm::errs()<<"hello world!2"<<"\n";
//}
//
//void NewCheckTest::VisitDecl(clang::Decl *decl)
//{
//}
//
//void NewCheckTest::VisitStmt(clang::Stmt *stmt)
//{
////    llvm::errs() << stmt->getLocStart().printToString(m_ci.getSourceManager()) << "\n";
//    llvm::errs() << stmt->getBeginLoc().printToString(sm())<<"\n";
//}


namespace {
    class StansaloneTest : public Checker<check::PreStmt < CallExpr>> {
    mutable std::unique_ptr<BugType> BT;

    public:
    void checkPreStmt(const CallExpr *CE, CheckerContext &C) const;
};
} // end anonymous namespace

void StansaloneTest::checkPreStmt(const CallExpr *CE,
                                  CheckerContext &C) const {
    llvm::errs()<<"checkPreStmt"<<"\n";
    const Expr *Callee = CE->getCallee();
    const FunctionDecl *FD = C.getSVal(Callee).getAsFunctionDecl();

    if (!FD)
        return;

    // Get the name of the callee.
    IdentifierInfo *II = FD->getIdentifier();
    if (!II) // if no identifier, not a simple C function
        return;

    if (II->isStr("main")) {
        ExplodedNode *N = C.generateErrorNode();
        if (!N)
            return;

        if (!BT)
            BT.reset(new BugType(this, "call to main", "example analyzer plugin"));

        std::unique_ptr<BugReport> report =
                llvm::make_unique<BugReport>(*BT, BT->getName(), N);
        report->addRange(Callee->getSourceRange());
        C.emitReport(std::move(report));
    }
}

//void hello() {
//    std::cout << "Hello, World!" << std::endl;
//}

//extern "C" const char clang_analyzerAPIVersionString[] =
//        CLANG_ANALYZER_API_VERSION_STRING;
//
//
//extern "C"
//void clang_registerCheckers (CheckerRegistry &registry) {
//    registry.addChecker <StansaloneTest >("alpha.core.StansaloneTest",
//                                          "Test to see if I can make the checker in a standalone version","");
//}