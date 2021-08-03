set(CLAZY_LIB_SRC
        ${CMAKE_CURRENT_LIST_DIR}/src/ListAllMemLocChecker.cpp
)

#set(CLAZY_CHECKS_SRCS
#  ${CMAKE_CURRENT_LIST_DIR}/src/checks/detachingbase.cpp
#  ${CMAKE_CURRENT_LIST_DIR}/src/checks/inefficientqlistbase.cpp
#  ${CMAKE_CURRENT_LIST_DIR}/src/checks/ruleofbase.cpp
#)
include(CheckSources.cmake)

set(CLAZY_SHARED_SRCS # sources shared between clazy-standalone and clazy plugin
  ${CLAZY_CHECKS_SRCS}
#  ${CMAKE_CURRENT_LIST_DIR}/src/ClazyContext.cpp
#  ${CMAKE_CURRENT_LIST_DIR}/src/Clazy.cpp
  ${CLAZY_LIB_SRC}
)

set(CLAZY_PLUGIN_SRCS # Sources for the plugin
  ${CLAZY_SHARED_SRCS}
)

if (MSVC)
  set(CLAZY_STANDALONE_SRCS
    ${CLAZY_SHARED_SRCS}
#    ${CMAKE_CURRENT_LIST_DIR}/src/ClazyStandaloneMain.cpp
  )
else()
  set(CLAZY_STANDALONE_SRCS
#    ${CMAKE_CURRENT_LIST_DIR}/src/ClazyStandaloneMain.cpp
  )
endif()
