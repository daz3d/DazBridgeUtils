#pragma once

//////////////////////////////////////////////
//
// Define Daz Bridge Namespace
// 
// NOTE: If you are want to static link the Daz Bridge Common library into
// a Daz Plugin, you **MUST** edit the DZ_BRIDGE_NAMESPACE macro so that it
// defines a unique namespace so that your version of the Common Library
// can co-exist with Common Library from other Daz Plugins. I recommend that
// you paste your Plugin's GUID onto the end of your namespace. See example
// below.
// 
//////////////////////////////////////////////

#define DZ_BRIDGE_NAMESPACE DzBridgeNameSpace
//#define DZ_BRIDGE_NAMESPACE DzBridgeStatic_71fb72024b4947baa82a4780e3819776
//#define VODSVERSION


/////////////////////////////////////////////
//
// Define C++ compatible DLL and Plugin Macros
// 
// NOTE: The order to export C++ classes in a Windows DLL, you should use
// the CPP_Export macro below. Additionally, you must **NOT** use .DEF file
// method of exporting DLL functions. The current Microsoft does not allow
// exporting of C++ class data when using DEF files. Alternative plugin
// macros are supplied below that do not rely on DEF files for export.
/////////////////////////////////////////////

#undef CPP_Export
#define CPP_Export Q_DECL_IMPORT
#ifdef DZ_BRIDGE_SHARED
	#undef CPP_Export
	#define CPP_Export Q_DECL_EXPORT
#elif DZ_BRIDGE_STATIC
	#undef CPP_Export
	#define CPP_Export
#endif

#ifdef __APPLE__
#define CPP_PLUGIN_DEFINITION DZ_PLUGIN_DEFINITION
#else
#define CPP_PLUGIN_DEFINITION( pluginName ) \
BOOL WINAPI DllMain( HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved )	\
{	\
	switch( fdwReason ) {		\
	case DLL_PROCESS_ATTACH:	\
		break;					\
	case DLL_THREAD_ATTACH:		\
		break;					\
	case DLL_THREAD_DETACH:		\
		break;					\
	case DLL_PROCESS_DETACH:	\
		break;					\
	}							\
	return TRUE;				\
} \
 \
static DzPlugin s_pluginDef( pluginName ); \
extern "C" __declspec(dllexport) DzVersion getSDKVersion() { return DZ_SDK_VERSION; } \
extern "C" __declspec(dllexport) DzPlugin *getPluginDefinition() { return &s_pluginDef; }
#endif

//////////////////////////////////////////////
// 
// Fixed DZ_PLUGIN_CUSTOM_CLASS macro
// 
// The original DazSDK macros are missing the proper factory functions to handle arguments
// for custom classes. The following alternative macros fix this.
// 
//////////////////////////////////////////////

#include <qwidget.h>
#include <qvariant.h>

static QWidget* getParentFromArgs(const QVariantList& args)
{
	if (args.count() < 1)
		return nullptr;

	QWidget* parent = nullptr;
	QVariant qvar = args[0];
	QObject* obj = qvar.value<QObject*>();
	if (obj)
		parent = qobject_cast<QWidget*>(obj);

	return parent;
}

#define NEW_PLUGIN_CUSTOM_CLASS_GUID( typeName, guid ) \
DZ_PLUGIN_CUSTOM_CLASS_GUID( typeName, guid ) \
 \
QObject* typeName ## Factory::createInstance(const QVariantList& args) const \
{ \
	QWidget* parent = getParentFromArgs(args); \
	return qobject_cast<QObject*>(new typeName(parent)); \
} \
QObject* typeName ## Factory::createInstance() const \
{ \
	return qobject_cast<QObject*>(new typeName(nullptr)); \
}

