#**********************************************************************
#	Copyright (C) 2002-2020 Daz 3D, Inc. All Rights Reserved.
#
#	This file is part of the DAZ Studio SDK.
#
#	This file may be used only in accordance with the DAZ Studio SDK
#	license provided with the DAZ Studio SDK.
#
#	The contents of this file may not be disclosed to third parties,
#	copied or duplicated in any form, in whole or in part, without the
#	prior written permission of Daz 3D, Inc, except as explicitly
#	allowed in the DAZ Studio SDK license.
#
#	See http://www.daz3d.com to contact DAZ 3D or for more
#	information about the DAZ Studio SDK.
#**********************************************************************

cmake_minimum_required(VERSION 3.4.0)

#if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
#  if (NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
#  endif()
#endif()
#set(CMAKE_CONFIGURATION_TYPES
#	"Release"
#	"MinSizeRel"
#	"RelWithDebInfo"
#	"Debug"
#)
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} "MinSizeRel;RelWithDebInfo;Debug;Release")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)


if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "" FORCE)
	if(NOT CMAKE_OSX_ARCHITECTURES)
  		message( FATAL_ERROR "Mac needs CMAKE_OSX_ARCHITECTURES, set to i386 or x86_64" )
		return()
	endif()
endif(APPLE)

project("DzBridgeProject")
set(FBX_SDK_DIR "" CACHE PATH "Path to FBX SDK" )
set(OPENSUBDIV_DIR "" CACHE PATH "Path to Opensubdiv folder" )
set(DAZ_STUDIO_EXE_DIR "" CACHE PATH "Path to DAZ Studio, needs to be installed to a writeable location" )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32)
	set(DZ_LIB_SUFFIX ".lib")
	set(DZ_BIN_SUFFIX ".dll")
	set(DZ_LIB_PREFIX "")
	set(UTIL_EXT ".exe")
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(DZ_PLATFORM x86)
		set(DZ_MIXED_PLATFORM Win32)
		set(DZ_OS_PLATFORM Win32)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(DZ_PLATFORM x64)
		set(DZ_MIXED_PLATFORM x64)
		set(DZ_OS_PLATFORM Win64)
	else()
		message(FATAL_ERROR "Unknown architecture")
	endif()
elseif(APPLE)
	set(DZ_LIB_SUFFIX ".dylib")
	set(DZ_BIN_SUFFIX ".dylib")
	set(DZ_LIB_PREFIX "lib")
	set(UTIL_EXT "")
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(DZ_PLATFORM x86)
		set(DZ_MIXED_PLATFORM Mac32)
		set(DZ_OS_PLATFORM Mac32)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(DZ_PLATFORM x64)
		set(DZ_MIXED_PLATFORM Mac64)
		set(DZ_OS_PLATFORM Mac64)
	else()
		message(FATAL_ERROR "Unknown architecture")
	endif()
	set(CMAKE_MACOSX_RPATH TRUE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#	SET(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
else()
	message(FATAL_ERROR "Unknown architecture")
endif(WIN32)

if(NOT USE_DZBRIDGE_SUBMODULE)

option(INSTALL_SCRIPTABLE_PLUGIN "Install Scriptable Plugin to Daz Studio (INSTALL_SCRIPTABLE_PLUGIN)" OFF)

set(DAZ_SDK_DIR_DEFAULT "")
set(DAZ_SDK_CORE_RELATIVE_PATH "lib/${DZ_MIXED_PLATFORM}/${DZ_LIB_PREFIX}dzcore${DZ_LIB_SUFFIX}")
if(NOT DAZ_SDK_DIR)
	if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/DAZStudio4.5+ SDK/${DAZ_SDK_CORE_RELATIVE_PATH}")
		set( DAZ_SDK_DIR_DEFAULT "${CMAKE_CURRENT_LIST_DIR}/DAZStudio4.5+ SDK" )
	endif()
endif()

set(DAZ_SDK_DIR ${DAZ_SDK_DIR_DEFAULT} CACHE PATH "Path to root of the DAZ Studio SDK" )

if(NOT DAZ_SDK_DIR)
	message(FATAL_ERROR "Missing path to DAZ Studio SDK")
	return()
endif()

set(QT_BINARY_DIR_DEFAULT "" CACHE PATH "Path to directory with QT binaries")
if(NOT QT_BINARY_DIR_DEFAULT)
	if(EXISTS "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}/qmake${UTIL_EXT}")
		set( QT_BINARY_DIR_DEFAULT "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}" )
	endif()
endif()

if(NOT QT_BINARY_DIR_DEFAULT)
	message(FATAL_ERROR "Missing path QT binaries. Check QT_BINARY_DIR_DEFAULT path")
	return()
endif()

find_package(OpenGL REQUIRED)

#we only have release libraries for dzcore/qt so make sure even in debug they we use MD and undef debug
if(WIN32)
	add_compile_options( "/MD" "/U_DEBUG" )
endif()

# Set dzcore as import target
set(DZ_SDK_INCLUDE "${DAZ_SDK_DIR}/include" CACHE FILEPATH "path to daz sdk includes" )
set(DAZ_SDK_LIB "${DAZ_SDK_DIR}/${DAZ_SDK_CORE_RELATIVE_PATH}" CACHE FILEPATH "path to dzcore" )
if(NOT EXISTS ${DAZ_SDK_LIB})
	message(FATAL_ERROR "The library dzcore could not be located.  Check the path for DAZ_SDK_DIR.")
	return()
endif()

add_library(dzcore SHARED IMPORTED)
if(WIN32)
	set_property(TARGET dzcore APPEND PROPERTY IMPORTED_IMPLIB ${DAZ_SDK_LIB})
else()
	set_property(TARGET dzcore APPEND PROPERTY IMPORTED_LOCATION ${DAZ_SDK_LIB})
endif(WIN32)
set_property(TARGET dzcore APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${DZ_SDK_INCLUDE}" )

# Set dpc as import target
set(DAZ_SDK_DPC_EXE "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}/dpc${UTIL_EXT}" CACHE FILEPATH "path to sdk dpc" )
if(NOT EXISTS ${DAZ_SDK_DPC_EXE})
	message(FATAL_ERROR "The executable dpc could not be located.  Check the path for DAZ_SDK_DIR.")
	return()
endif()
add_executable(dpc IMPORTED)
set_property(TARGET dpc APPEND PROPERTY IMPORTED_LOCATION ${DAZ_SDK_DPC_EXE})

# Setup Qt from the DAZ SDK
if(WIN32)
	set(DAZ_SDK_QTCORE_LIBRARY "${DAZ_SDK_DIR}/lib/${DZ_MIXED_PLATFORM}/QtCore4.lib")
elseif(APPLE)
	set(DAZ_SDK_QTCORE_LIBRARY "${DAZ_SDK_DIR}/lib/${DZ_MIXED_PLATFORM}/QtCore.framework")
endif()

set(QT_BINARY_DIR_DEFAULT "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}")
set(QT_IMPORTS_DIR "${DAZ_SDK_DIR}/lib/${DZ_MIXED_PLATFORM}")

set(QT_QTCORE_LIBRARY_RELEASE ${DAZ_SDK_QTCORE_LIBRARY})
#set(QT_BINARY_DIR "${QT_BINARY_DIR_DEFAULT}")
#set(QT_QMAKE_EXECUTABLE "${QT_BINARY_DIR_DEFAULT}/qmake${UTIL_EXT}")
set(QT_QMAKE_EXECUTABLE "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}/qmake${UTIL_EXT}")
set(QT_BINARY_DIR "${DAZ_SDK_DIR}/bin/${DZ_MIXED_PLATFORM}")
set(QT_HEADERS_DIR "${DAZ_SDK_DIR}/include")
set(QT_QTCORE_INCLUDE_DIR "${DAZ_SDK_DIR}/include/QtCore")

# the qt find module needs this folder but our build does not so just fake it
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/DUMMY_MKSPECS/default" )
set(QT_MKSPECS_DIR "${CMAKE_CURRENT_BINARY_DIR}/DUMMY_MKSPECS")

find_package(Qt4 4.8.1 REQUIRED QtCore QtGui QtScript QtOpenGL QtNetwork QtSql QtXml)

set(DZSDK_QT_CORE_TARGET Qt4::QtCore)
set(DZSDK_QT_GUI_TARGET Qt4::QtGui)
set(DZSDK_QT_SCRIPT_TARGET Qt4::QtScript)
set(DZSDK_QT_OPENGL_TARGET Qt4::QtOpenGL)
set(DZSDK_QT_NETWORK_TARGET Qt4::QtNetwork)
set(DZSDK_QT_SQL_TARGET	Qt4::QtSql)
set(DZSDK_QT_XML_TARGET	Qt4::QtXml)

list(APPEND CMAKE_AUTOMOC_MOC_OPTIONS -i)


############################
# FBX SETTINGS
############################
IF(NOT WIN32)
	set(FBX_ARCH "x64")
	SET(CMAKE_CXX_FLAGS "-m64 ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "-m64 ${CMAKE_C_FLAGS}")
	SET(FBX_TMP_TARGET_LIBS ${FBX_TMP_TARGET_LIBS} dl pthread)
	SET(CMAKE_CXX_FLAGS "-D_NDEBUG -Os ${CMAKE_CXX_FLAGS}")
	SET(CMAKE_C_FLAGS "-D_NDEBUG -Os ${CMAKE_C_FLAGS}")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")

	IF(APPLE)
		set(FBX_LINKER_FLAGS "-lz -lxml2 -liconv")
		IF(NOT FBX_CLANG)
			SET(FBX_LINKER_FLAGS "-framework Carbon -framework SystemConfiguration ${FBX_LINKER_FLAGS}")
		ELSE(NOT FBX_CLANG)
			SET(FBX_LINKER_FLAGS "-framework CoreFoundation -framework SystemConfiguration ${FBX_LINKER_FLAGS}")
		ENDIF(NOT FBX_CLANG)
		SET(FBX_TMP_TARGET_LIBS ${FBX_TMP_TARGET_LIBS} iconv)
	ENDIF()

#	SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
#	SET(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")

ENDIF(NOT WIN32)

#set(FBX_SDK_DIR "" CACHE PATH "Path to FBX SDK" )
if(NOT FBX_SDK_DIR)
	message(FATAL_ERROR "Missing path to FBX SDK folder")
	return()
endif()

set(FBX_SDK_INCLUDE "${FBX_SDK_DIR}/include" CACHE PATH "Path to FBX SDK Includes" )
if(WIN32)
	set(FBX_PLATFORM "vs2017/x64/release")
	set(FBX_SDK_LIB "${FBX_SDK_DIR}/lib/${FBX_PLATFORM}/libfbxsdk-md.lib" CACHE FILEPATH "Path to FBX SDK static library (libfbx-md.lib)" )
	set(FBX_SDK_XMLLIB "${FBX_SDK_DIR}/lib/${FBX_PLATFORM}/libxml2-md.lib" CACHE FILEPATH "Path to FBX SDK XML library (libxml2-md.lib)" )
	set(FBX_IMPORT_LIBS
		${FBX_SDK_LIB}
		${FBX_SDK_XMLLIB}
		${FBX_LINKER_FLAGS})
elseif(APPLE)
#	set(FBX_PLATFORM "clang/libstdcpp/release")
    set(FBX_PLATFORM "clang/release")
	set(FBX_SDK_LIB "${FBX_SDK_DIR}/lib/${FBX_PLATFORM}/libfbxsdk.a" CACHE FILEPATH "Path to FBX SDK static library (libfbxsdk.a)" )
	set(FBX_IMPORT_LIBS
		${FBX_SDK_LIB}
		${FBX_LINKER_FLAGS})
endif()

############################
# Opensubdiv SETTINGS
############################
#set(OPENSUBDIV_DIR "" CACHE PATH "Path to Opensubdiv folder" )
if(NOT OPENSUBDIV_DIR)
	message(FATAL_ERROR "Missing path to Opensubdiv folder")
	return()
endif()
set(OPENSUBDIV_INCLUDE "${OPENSUBDIV_DIR}" CACHE PATH "Path to Opensubdiv include folder (usually same as root folder)" )
if(WIN32)
	set(OPENSUBDIV_LIB "${OPENSUBDIV_DIR}/build/lib/Release/osdCPU.lib" CACHE FILEPATH "Path to Opensubdiv CPU static library (osdCPU.lib)" )
elseif(APPLE)
	set(OPENSUBDIV_LIB "${OPENSUBDIV_DIR}/build/lib/Release/libosdCPU.a" CACHE FILEPATH "Path to Opensubdiv CPU static library (libosdCPU.a)" )
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# if building a plugin and you want the compiled result placed in the Daz Studio ./plugins directory
if(INSTALL_SCRIPTABLE_PLUGIN)
	if(DAZ_STUDIO_EXE_DIR)
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DAZ_STUDIO_EXE_DIR}/plugins)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DAZ_STUDIO_EXE_DIR}/plugins)
	endif()
endif()

endif(NOT USE_DZBRIDGE_SUBMODULE)

add_subdirectory("include")
add_subdirectory("Test/UnitTests")
add_subdirectory("src")
