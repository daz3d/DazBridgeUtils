# Daz Bridge Library Extras

The files here are supplemental scripts to assist with the conversion process, especially for game engines.  They can be integrated into the Daz Bridge export pipeline either through the PreProcessScene stage or the PostProcessFbx stage.  These files can be embedded as a zip file using the following CMake commands:

```
###########################
# PRE-BUILD: Create PLUGINDATA ZIP
############################
# Define your source and destination directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/PluginData")
set(ZIP_DEST_DIR "${CMAKE_SOURCE_DIR}/DazStudioPlugin/Resources")
set(ZIP_NAME "plugindata.zip")
# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${ZIP_DEST_DIR})
if(WIN32)
	# PowerShell command to remove the zip file if it exists
	set(PS_REMOVE_ZIP_COMMAND "if (Test-Path -Path ${ZIP_DEST_DIR}/${ZIP_NAME}) { Remove-Item -Path ${ZIP_DEST_DIR}/${ZIP_NAME} -ErrorAction SilentlyContinue }")
	# PowerShell command to zip the folder
	set(PS_ZIP_COMMAND "Compress-Archive -Path ${SOURCE_DIR}/* -DestinationPath ${ZIP_DEST_DIR}/${ZIP_NAME}")
	# Add a custom command that runs before anything else is built
	add_custom_command(OUTPUT ${ZIP_DEST_DIR}/${ZIP_NAME}
					COMMAND echo ${PS_REMOVE_ZIP_COMMAND}
					COMMAND powershell -Command ${PS_REMOVE_ZIP_COMMAND}
					COMMAND echo ${PS_ZIP_COMMAND}
					COMMAND powershell -Command ${PS_ZIP_COMMAND}
					COMMENT "Zipping up the PluginData folder...")
else()
	# Add a custom command that runs before anything else is built
	add_custom_command(OUTPUT ${ZIP_DEST_DIR}/${ZIP_NAME}
                    COMMAND ${CMAKE_COMMAND} -E rm -f ${ZIP_DEST_DIR}/${ZIP_NAME}
                    COMMAND cd ${SOURCE_DIR}
                    COMMAND zip -r ${ZIP_DEST_DIR}/${ZIP_NAME} .
					COMMENT "Zipping up the folder")
endif()
# Create a custom target that depends on the custom command
add_custom_target(plugindata ALL
				DEPENDS ${ZIP_DEST_DIR}/${ZIP_NAME})
# Make sure your main target depends on this custom target
add_dependencies(${DZ_PLUGIN_TGT_NAME} plugindata)
set_target_properties (plugindata
	PROPERTIES
	FOLDER ""
	PROJECT_LABEL "PluginData ZIP"
)
```