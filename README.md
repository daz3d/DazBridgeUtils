# Daz Bridge Library

## Table of Contents
1. About the Daz Bridge Library
2. How to Install
3. How to Build
4. How to QA Test
5. How to Modify and Develop
6. How to Use with Daz Scripts
7. Directory Structure


## 1. About the Daz Bridge Library
Daz Bridge Library is a multipurpose C++ library and Script framework containing classes for:
1. Writing C++ and DazScript Plugins to export Daz assets to external software packages,
2. Performing common conversion-related operations such as Normal Map Generation, Texture Baking and Mesh Subdivision,
3. Automating Quality Assurance tests including automated unit-tests and test-cases, and file format validation.

This repository can be added to existing or new projects as as a git submodule.  The library itself can be linked as a static or shared library or built into a stand-alone scriptable plugin.


## 2. How to Install
The stand-alone, scriptable plugin can be copied to the Daz Studio plugins folder (example: "\Daz 3D\Applications\64-bit\DAZ 3D\DAZStudio4\plugins").  Daz Studio can then be started, and the plugin accessed via scripts written in the IDE, or pressing F3 and adding the Bridge->"Daz Scriptable Bridge" Action to your main menu or toolbar. Script API documentation and examples can be found in the Scripting section below.


## 3. How to Build
Setup and configuration of the build system is done via CMake to generate project files for Windows or Mac.  The CMake configuration requires:
-	Modern CMake (tested with 3.27.2 on Win and 3.27.0-rc4 on Mac)
-	Daz Studio 4.5+ SDK (from DIM)
-	Fbx SDK 2020.1 (win) / Fbx SDK 2015.1 (mac)
-	OpenSubdiv 3.4.4

(Please note that you MUST use the Qt 4.8.1 build libraries that are built-into the Daz Studio SDK.  Using an external Qt library will result in build errors and program instability.)

Download or clone the Daz Bridge Library github repository to your local machine.  The build setup process is designed to be run with CMake gui in an interactive session.  After setting up the source code folder and an output folder, the user can click Configure.  CMake will stop during the configurtaion process to prompt the user for the following paths:

-	DAZ_SDK_DIR – the root folder to the Daz Studio 4.5+ SDK.  This MUST be the version purchased from the Daz Store and installed via the DIM.  Any other versions will NOT work with this source code project and result in build errors and failure. example: C:/Users/Public/Documents/My DAZ 3D Library/DAZStudio4.5+ SDK
-	DAZ_STUDIO_EXE_DIR – the folder containing the Daz Studio executable file.  example: C:/Program Files/DAZ 3D/DAZStudio4
-	FBX_SDK_DIR – the root folder containing the “include” and “lib” subfolders.  example: C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1
-	OPENSUBDIV_DIR – root folder containing the “opensubdiv”, “examples”, “cmake” folders.  It assumes the output folder was set to a subfolder named “build” and that the osdCPU.lib or libosdCPU.a static library files were built at: <root>/build/lib/Release/osdCPU.lib or <root>/build/lib/Release/libosdCPU.a.  A pre-built library for Mac and Windows can be found at https://github.com/danielbui78/OpenSubdiv/releases that contains the correct location for include and prebuilt Release static library  binaries.  If you are not using this precompiled version, then you must ensure the correct location for the OPENSUBDIV_INCLUDE folder path and OPENSUBDIV_LIB filepath.

Once these paths are correctly entered into the CMake gui, the Configure button can be clicked and the configuration process should resume to completion.  The project files can then be generated and the project may be opened.  Please note that a custom version of Qt 4.8 build tools and libraries are included in the DAZ_SDK_DIR.  If another version of Qt is installed in your system and visible to CMake, it will likely cause errors with finding the correct version of Qt supplied in the DAZ_SDK_DIR and cause build errors and failure.

The resulting project files should have “DzBridge Shared” and “DzBridge Static” as project targets.  The DLL/DYLIB binary file produced by DzBridge Shared should be a working Daz Studio plugin.  Since the Daz Bridge Library is designed to be subclassed, the main C++ class for the “DzBridge Shared” project is DzBridgeAction_Scriptable (.cpp/.h).


## 4. How to QA Test
The Test folder contains a `QA Manual Test Cases.md` document with instructions for performaing manual tests.  The Test folder also contains subfolders for UnitTests, TestCases and Results. To run automated Test Cases, run Daz Studio and load the `Test/testcases/test_runner.dsa` script, configure the sIncludePath on line 4, then execute the script. Results will be written to report files stored in the `Test/Reports` subfolder.

To run UnitTests, you must first build special Debug versions of the DzBridge-Unity and DzBridge Static sub-projects with Visual Studio configured for C++ Code Generation: Enable C++ Exceptions: Yes with SEH Exceptions (/EHa). This enables the memory exception handling features which are used during null pointer argument tests of the UnitTests. Once the special Debug version of DazToUnity dll is built and installed, run Daz Studio and load the `Test/UnitTests/RunUnitTests.dsa` script. Configure the sIncludePath and sOutputPath on lines 4 and 5, then execute the script. Several UI dialog prompts will appear on screen as part of the UnitTests of their related functions. Just click OK or Cancel to advance through them. Results will be written to report files stored in the `Test/Reports` subfolder.

Finally, there is a "How To Use QA Test Scripts.md" file with instructions for performing a full automated test-suite of UnitTests and TestCases for Daz Bridge Library, DazToUnity and DazToUnreal Bridges.  See the section below for "How to Use with Daz Scripts" for information on writing your own automated QA scripts.

Special Note: The QA Report Files generated by the UnitTest and TestCase scripts have been designed and formatted so that the QA Reports will only change when there is a change in a test result.  This allows Github to conveniently track the history of test results with source-code changes, and allows developers and QA testers to notified by Github or their git client when there are any changes and the exact test that changed its result.


## 5. How to Modify and Develop
The "src" folder contains C++ classes for interactive GUI and scripted conversions.  The DLL plugin source files include a pluginmain.cpp which contains compiler macro references to Daz SDK plugin definitions to export the `DzBridgeAction` and `DzBridgeDialog` classes.  DzBridgeDialog acts as the Dialog Options interface manager, while DzBridgeAction handles the conversion operations.  The pluginmain.cpp, and the multiple files named `DzBridge***_Scriptable.cpp/.h` build a stand-alone plugin and also serve as an example of how to create a custom Bridge plugin for external software.  The "include" folder contains header files which can be added to external projects for static and shared linkage with the Daz Bridge Library.

**DZ_BRIDGE_NAMESPACE**: The DazToBlender Bridge is derived from base classes in the Daz Bridge Library that are within the DZ_BRIDGE_NAMESPACE (see bridge.h). Prior published versions of the official Daz Bridge plugins used custom namespaces to isolate shared class names from each plugin.  While this theoretically works to prevent namespace collisions for platforms that adhere to C++ namespaces, it may not hold true for some implementations of Qt and the Qt meta-object programming model, which is heavily used by Daz Studio and the Bridge plugins.  Notably, C++ namespaces may not be isolating code on the Mac OS implementation of Qt.  With these limitations in mind, I have decided to remove the recommendation to rename the DZ_BRIDGE_NAMESPACE in order to streamline and reduce deployment complexity for potential bridge plugin developers.

In order to link and share C++ classes between this plugin and the Daz Bridge Library, a custom `CPP_PLUGIN_DEFINITION()` macro is used instead of the standard DZ_PLUGIN_DEFINITION macro and usual .DEF file. NOTE: Use of the DZ_PLUGIN_DEFINITION macro and DEF file use will disable C++ class export in the Visual Studio compiler.


## 6. How to Use with Daz Scripts
The `DzBridge Script Documentation and Example.dsa` serves as both an example script as well as API documentation for how to use the scriptable plugin, starting with how to instantiate a Bridge object, configuring conversion settings and output folder, etc.  Additional practical scripting examples for loading assets and exporting them can be found in the script test-cases for DazToUnity and DazToUnreal Bridges.  The `QA Script Documentation and Examples.dsa` serves as documentation, API and example script for using the Daz Bridge Library to write automation tests, including output file format validation.


## 7. Directory Structure
The directory structure is as follows:
- `Extras` :    Supplemental scripts and support files to help the conversion process, especially for game-engines and other real-time appllications.
- `include` :   Header files that define the Bridge API for use by derived plugins.
- `src` :       Source files containing base classes and utility classes for use by derived plugins.
- `Test`:       Scripts and generated output (reports) used for Quality Assurance Testing.
