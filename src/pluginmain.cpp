#include "dzplugin.h"
#include "dzapp.h"

#include "common_version.h"
#include "DzBridgeDialog_Scriptable.h"
#include "DzBridgeMorphSelectionDialog_Scriptable.h"
#include "DzBridgeSubdivisionDialog_Scriptable.h"
#include "OpenFBXInterface.h"
#include "DzBridgeAction_Scriptable.h"

#include "dzbridge.h"

CPP_PLUGIN_DEFINITION("Daz Bridges Common Library")

DZ_PLUGIN_AUTHOR("Daz 3D, Inc");

DZ_PLUGIN_VERSION(COMMON_MAJOR, COMMON_MINOR, COMMON_REV, COMMON_BUILD);

#ifdef _DEBUG
DZ_PLUGIN_DESCRIPTION(QString(
"<b>Pre-Release Daz Bridge Library v%1.%2.%3.%4 </b><br>\
<a href = \"https://github.com/daz3d/DazBridgeUtils\">Github</a><br><br>"
).arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(COMMON_REV).arg(COMMON_BUILD));
#else
DZ_PLUGIN_DESCRIPTION(QString(
"This plugin provides the ability to access Daz Bridge functions from Daz Script. \
Documentation and source code are available on <a href = \"https://github.com/daz3d/DazBridgeUtils\">Github</a><br><br>"
));
#endif

NEW_PLUGIN_CUSTOM_CLASS_GUID(DzBridgeDialog, c0830510-cea8-419a-b17b-49b3353e3d07);
NEW_PLUGIN_CUSTOM_CLASS_GUID(DzBridgeMorphSelectionDialog, 321916ba-0bcc-45d9-8c7e-ebbe80dea51c);
NEW_PLUGIN_CUSTOM_CLASS_GUID(DzBridgeSubdivisionDialog, a2342e17-db3b-4032-a576-75b5843fa893);
DZ_PLUGIN_CLASS_GUID(OpenFBXInterface, 9aaaf080-28c1-4e0f-a3e9-a0205e91a154);
DZ_PLUGIN_CLASS_GUID(DzBridgeAction, 71fb7202-4b49-47ba-a82a-4780e3819776);

#ifdef UNITTEST_DZBRIDGE
#include "UnitTest_DzBridgeAction.h"
#include "UnitTest_DzBridgeDialog.h"
#include "UnitTest_DzBridgeMorphSelectionDialog.h"
#include "UnitTest_DzBridgeSubdivisionDialog.h"

DZ_PLUGIN_CLASS_GUID(UnitTest_DzBridgeAction, 1ae818ba-d745-4db7-afb9-b1cb5e7700db);
DZ_PLUGIN_CLASS_GUID(UnitTest_DzBridgeDialog, 15bdc1cf-fbe6-4085-b729-fcb5e428fe71);
DZ_PLUGIN_CLASS_GUID(UnitTest_DzBridgeMorphSelectionDialog, 8d4ba27a-bb2a-4d69-95da-c8dc1b095bcc);
DZ_PLUGIN_CLASS_GUID(UnitTest_DzBridgeSubdivisionDialog, fc3a8f28-fef2-44ed-ac99-25aadb91e3d5);
#endif
