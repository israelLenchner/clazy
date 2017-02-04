set(CLAZY_LIB_SRC
  ${CMAKE_CURRENT_LIST_DIR}/AccessSpecifierManager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checkbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checkmanager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/SuppressionManager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ContextUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/FixItUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/HierarchyUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/LoopUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/MacroUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PreProcessorVisitor.cpp
  ${CMAKE_CURRENT_LIST_DIR}/QtUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/StringUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/TemplateUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/TypeUtils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Utils.cpp
)

set(CLAZY_SRCS
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/connect-non-signal.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/container-anti-pattern.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/lambda-in-connect.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qdatetimeutc.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qenums.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qfileinfo-exists.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qgetenv.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qmap-with-pointer-key.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qstringarg.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qstring-insensitive-allocation.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qstringref.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qt-macros.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/qvariant-template-instantiation.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/mutable-container-key.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/temporaryiterator.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/unused-non-trivial-variable.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/writingtotemporary.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level0/wrong-qglobalstatic.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/autounexpectedqstringbuilder.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/child-event-qobject-cast.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/detachingtemporary.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/foreach.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/inefficient-qlist-soft.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/ctor-missing-parent-argument.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/missing-qobject-macro.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/nonpodstatic.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/qdeleteall.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/qstring-left.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/qlatin1string-non-ascii.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/range-loop.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/returning-data-from-temporary.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/ruleoftwosoft.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/post-event.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level1/incorrect-emit.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/base-class-event.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/container-inside-loop.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/function-args-by-ref.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/function-args-by-value.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/globalconstcharpointer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/implicitcasts.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/missing-type-info.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/qstring-allocations.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/reservecandidates.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/ruleofthree.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/virtualcallsfromctor.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/returning-void-expression.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level2/copyable-polymorphic.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level3/assertwithsideeffects.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level3/detachingmember.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/level3/dynamic_cast.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/hiddenlevel/inefficientqlist.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/hiddenlevel/isempty-vs-count.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/hiddenlevel/tr-non-literal.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/hiddenlevel/qt4-qstring-from-array.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/detachingbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/inefficientqlistbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/requiredresults.cpp
  ${CMAKE_CURRENT_LIST_DIR}/checks/ruleofbase.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Clazy.cpp
)
