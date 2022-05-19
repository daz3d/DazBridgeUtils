# Daz Bridge Library

# Table of Contents
1. About the Daz Bridge Library
2. How to Install
3. How to Build
4. How to QA Test
5. How to Modify and Develop
6. How to Use with Daz Scripts


# 1. About the Daz Bridge Library
Daz Bridge Library is a multipurpose C++ library and Script framework containing classes for:
1. Writing C++ and DazScript Plugins to export Daz assets to external software packages,
2. Performing common conversion-related operations such as Normal Map Generation, Texture Baking and Mesh Subdivision,
3. Automating Quality Assurance tests including automated unit-tests and test-cases, and file format validation.
This repository can be added to existing or new projects as as a git submodule.  The library itself can be linked as a static or shared library or built into a stand-alone scriptable plugin.

# 2. How to Install
The stand-alone, scriptable plugin can be copied to the Daz Studio plugins folder (example: "\Daz 3D\Applications\64-bit\DAZ 3D\DAZStudio4\plugins").  Daz Studio can then be started, and the plugin accessed via scripts written in the IDE, or pressing F3 and adding the Bridge->"Daz Scriptable Bridge" Action to your main menu or toolbar. Script API documentation and examples can be found in the Scripting section below.

# 3. How to Build
Requirements: Daz Studio 4.5+ SDK, Qt 4.8.1, Autodesk Fbx SDK, Pixar OpenSubdiv Library, CMake, C++ development environment

Download or clone the Daz Bridge Library github repository to your local machine. Use CMake to configure the project files. If using the CMake gui, you will be prompted for folder paths to dependencies: Daz SDK, Qt 4.8.1, Fbx SDK and OpenSubdiv during the Configure process.

# 4. How to QA Test
The Test folder contains a `QA Manual Test Cases.md` document with instructions for performaing manual tests.  The Test folder also contains subfolders for UnitTests, TestCases and Results. To run automated Test Cases, run Daz Studio and load the `Test/testcases/test_runner.dsa` script, configure the sIncludePath on line 4, then execute the script. Results will be written to report files stored in the `Test/Reports` subfolder.

To run UnitTests, you must first build special Debug versions of the DzBridge-Unity and DzBridge Static sub-projects with Visual Studio configured for C++ Code Generation: Enable C++ Exceptions: Yes with SEH Exceptions (/EHa). This enables the memory exception handling features which are used during null pointer argument tests of the UnitTests. Once the special Debug version of DazToUnity dll is built and installed, run Daz Studio and load the `Test/UnitTests/RunUnitTests.dsa` script. Configure the sIncludePath and sOutputPath on lines 4 and 5, then execute the script. Several UI dialog prompts will appear on screen as part of the UnitTests of their related functions. Just click OK or Cancel to advance through them. Results will be written to report files stored in the `Test/Reports` subfolder.

Finally, there is a "How To Use QA Test Scripts.md" file with instructions for performing a full automated test-suite of UnitTests and TestCases for Daz Bridge Library, DazToUnity and DazToUnreal Bridges.  See the section below for "How to Use with Daz Scripts" for information on writing your own automated QA scripts.

# 5. How to Modify and Develop
The "src" folder contains C++ classes for interactive GUI and scripted conversions.  The "pluginmain.cpp", multiple files named "DzBridge***_Scriptable.cpp/.h" build a stand-alone plugin and also serve as an example of how to create a custom Bridge plugin for external software.  The "include" folder contains header files which can be added to external projects for static and shared linkage with the Daz Bridge Library.

The Daz Bridge Library uses a default namespace named "DzBridgeNameSpace".  If you static-link with a Daz Studio plugin or make modifications to the existing Daz Bridge Library source-code, it is recommended that you change the namespace to a unique name.  This ensures that there are no C++ Namespace collisions when other plugins based on the Daz Bridge Library are also loaded in Daz Studio. In order to link and share C++ classes between this plugin and the Daz Bridge Library, a custom `CPP_PLUGIN_DEFINITION()` macro is used instead of the standard DZ_PLUGIN_DEFINITION macro and usual .DEF file. NOTE: Use of the DZ_PLUGIN_DEFINITION macro and DEF file use will disable C++ class export in the Visual Studio compiler.

# 6. How to Use with Daz Scripts
The "DzBridge Script Documentation and Example.dsa" serves as both an example script as well as API documentation for how to use the scriptable plugin, starting with how to instantiate a Bridge object, configuring conversion settings and output folder, etc.  Additional practical scripting examples for loading assets and exporting them can be found in the script test-cases for DazToUnity and DazToUnreal Bridges.  The "QA Script Documentation and Examples.dsa" serves as documentation, API and example script for using the Daz Bridge Library to write automation tests, including output file format validation.
