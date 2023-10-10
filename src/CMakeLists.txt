set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(COMMON_LIB_INCLUDE_DIR ${COMMON_LIB_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_LIB_INCLUDE_DIR ${COMMON_LIB_INCLUDE_DIR} PARENT_SCOPE)

include_directories(${COMMON_LIB_INCLUDE_DIR})

if(WIN32)
	set(OS_SOURCES "")
elseif(APPLE)
	set(OS_SOURCES "")
endif()

set(OFBXI_SOURCES
	OpenFBXInterface.cpp
	../include/OpenFBXInterface.h
)

set(OSDI_SOURCES
	OpenSubdivInterface.cpp
	../include/OpenSubdivInterface.h
)

set(MINI_LIBZIP_SOURCES
	miniz.h
	../include/zip.h
	zip.c
)

set(LIB_SRCS
	${LIB_HEADERS}
	DzBridgeAction.cpp
	DzBridgeMorphSelectionDialog.cpp
	DzBridgeSubdivisionDialog.cpp
	DzBridgeLodSettingsDialog.cpp
	DzBridgeDialog.cpp
	ImageTools.cpp
	${QA_SRCS}
)

add_library(dzbridge-static
	STATIC
	${LIB_SRCS}
	${OS_SOURCES}
	${OFBXI_SOURCES}
	${OSDI_SOURCES}
	${MLD_SOURCES}
	${MINI_LIBZIP_SOURCES}
)

target_include_directories(dzbridge-static
	PUBLIC
	${FBX_SDK_INCLUDE}
	${OPENSUBDIV_INCLUDE}
	${ALEMBIC_INCLUDE}
	${IMATH_INCLUDE}
)

target_link_libraries(dzbridge-static
	PRIVATE
	dzcore
	${DZSDK_QT_CORE_TARGET}
	${DZSDK_QT_GUI_TARGET}
	${DZSDK_QT_SCRIPT_TARGET}
	${DZSDK_QT_NETWORK_TARGET}
)

set_target_properties (dzbridge-static
	PROPERTIES
	FOLDER ""
	PROJECT_LABEL "DzBridge Static"
)

if (NOT USE_DZBRIDGE_SUBMODULE)

add_library(dzbridge-shared
	SHARED
	${LIB_SRCS}
	${OS_SOURCES}
	${OFBXI_SOURCES}
	${OSDI_SOURCES}
	${MLD_SOURCES}
	${MINI_LIBZIP_SOURCES}
	pluginmain.cpp
	DzBridgeAction_Scriptable.h
	DzBridgeAction_Scriptable.cpp
	DzBridgeMorphSelectionDialog_Scriptable.h
	DzBridgeMorphSelectionDialog_Scriptable.cpp
	DzBridgeSubdivisionDialog_Scriptable.h
	DzBridgeSubdivisionDialog_Scriptable.cpp
	DzBridgeDialog_Scriptable.h
	DzBridgeDialog_Scriptable.cpp
)

target_include_directories(dzbridge-shared
	PUBLIC
	${FBX_SDK_INCLUDE}
	${OPENSUBDIV_INCLUDE}
	${ALEMBIC_INCLUDE}
	${IMATH_INCLUDE}
)

target_link_libraries(dzbridge-shared
	PRIVATE
	dzcore
	${DZSDK_QT_CORE_TARGET}
	${DZSDK_QT_GUI_TARGET}
	${DZSDK_QT_SCRIPT_TARGET}
	${DZSDK_QT_NETWORK_TARGET}
	${FBX_IMPORT_LIBS}
	${OPENSUBDIV_LIB}
	${ALEMBIC_LIB}
	${IMATH_LIB}
)

set_target_properties (dzbridge-shared
	PROPERTIES
	FOLDER ""
	PROJECT_LABEL "DzBridge Shared"
)

target_compile_definitions(dzbridge-shared
	PUBLIC
	DZ_BRIDGE_SHARED
	$<$<CONFIG:DEBUG>:UNITTEST_DZBRIDGE>
)

endif(NOT USE_DZBRIDGE_SUBMODULE)

if (WIN32)
	target_compile_definitions(dzbridge-static
		PUBLIC
		DZ_BRIDGE_STATIC
		$<$<CONFIG:DEBUG>:UNITTEST_DZBRIDGE>
	)
elseif (APPLE)
	target_compile_definitions(dzbridge-static
		PUBLIC
		DZ_BRIDGE_STATIC
	)
endif()