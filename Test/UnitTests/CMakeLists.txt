set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(COMMON_LIB_INCLUDE_DIR ${COMMON_LIB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_LIB_INCLUDE_DIR ${COMMON_LIB_INCLUDE_DIR} PARENT_SCOPE)

include_directories(${COMMON_LIB_INCLUDE_DIR})

include_directories(${COMMON_LIB_INCLUDE_DIR})
set(QA_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeAction.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeAction.h
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeDialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeDialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeMorphSelectionDialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeMorphSelectionDialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeSubdivisionDialog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/UnitTest_DzBridgeSubdivisionDialog.h
	${CMAKE_CURRENT_SOURCE_DIR}/DzBridgeAction_Scriptable.h
	${CMAKE_CURRENT_SOURCE_DIR}/DzBridgeAction_Scriptable.cpp
)
set(QA_SRCS ${QA_SRCS} PARENT_SCOPE)