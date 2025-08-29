
#include <QtCore>

#include "dzapp.h"
#include "dzfloatproperty.h"

#include "DzBridgeAction.h"
#include "BridgeTools.h"

//void overrideExportOptions(DzFileIOSettings &ExportOptions)
//{
//
//	//// TODO: REMOVE OVERRIDE WHEN WORKING
//	//// DEBUG: Override
//	ExportOptions.setBoolValue("doSelected", false);
//	ExportOptions.setBoolValue("doVisible", true);
//	ExportOptions.setBoolValue("doFigures", true);
//	ExportOptions.setBoolValue("doProps", false);
//	ExportOptions.setBoolValue("doLights", false);
//	ExportOptions.setBoolValue("doCameras", false);
//	ExportOptions.setBoolValue("doAnims", false);
//	ExportOptions.setBoolValue("doMorphs", true);
//	ExportOptions.setBoolValue("doFps", true);
////	ExportOptions.setStringValue("rules", m_sMorphSelectionRule);
//	ExportOptions.setStringValue("format", "FBX 2014 -- Binary");
//	ExportOptions.setIntValue("RunSilent", true);
//	ExportOptions.setBoolValue("doEmbed", false);
//	ExportOptions.setBoolValue("doCopyTextures", false);
//	ExportOptions.setBoolValue("doDiffuseOpacity", false);
//	ExportOptions.setBoolValue("doMergeClothing", true);
//	ExportOptions.setBoolValue("doStaticClothing", false);
//	ExportOptions.setBoolValue("degradedSkinning", false);
//	ExportOptions.setBoolValue("degradedScaling", false);
//	ExportOptions.setBoolValue("doSubD", false);
//	//ExportOptions.setBoolValue("doCollapseUVTiles", false);
//	ExportOptions.setBoolValue("doLocks", false);
//	ExportOptions.setBoolValue("doLimits", false);
//	ExportOptions.setBoolValue("doBaseFigurePoseOnly", false);
//	ExportOptions.setBoolValue("doHelperScriptScripts", false);
//	ExportOptions.setBoolValue("doMentalRayMaterials", false);
//	//// DEBUG: Override
//
//}

using namespace DzBridgeNameSpace;

// DB 2024-11-080: Used for Work around to correct Character Morphs using non - standard Object ERC Offset
bool DzBridgeTools::CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset)
{
	if (pNode == NULL) return false;

	auto pPropX = pNode->getXPosControl();
	auto pPropY = pNode->getYPosControl();
	auto pPropZ = pNode->getZPosControl();

	if (pPropX == NULL || pPropY == NULL || pPropZ == NULL) return false;

	double baseX = pPropX->getValue();
	double baseY = pPropY->getValue();
	double baseZ = pPropZ->getValue();

	double rawX = pPropX->getRawValue();
	double rawY = pPropY->getRawValue();
	double rawZ = pPropZ->getRawValue();

	vOffset.m_x = rawX - baseX;
	vOffset.m_y = rawY - baseY;
	vOffset.m_z = rawZ - baseZ;

	return true;
}

bool DzBridgeTools::IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList) {


	QString sExtension = fi.suffix().toLower();
	if (sExtension.isEmpty() || sExtension == "") {
		return false;
	}

	foreach(QString sKnownExtension, aExtensionsList) {
		if (sExtension == sKnownExtension) {
			return true;
		}
	}

	return false;
}

// aExtensionsList - QStringList of extensions to DELETE
// sSubFolderPath - QString of full path to subfolder to clean contents and RMDIR
bool DzBridgeTools::SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete)
{
	if (IsDangerousPath(sSubFolderPath)) {
		return false;
	}

	QDir qdirScripts(sSubFolderPath);
	if (qdirScripts.exists()) {
		// remove all known intermediate file types within it (nonrecursively)
		foreach(QFileInfo fi, qdirScripts.entryInfoList())
		{
			if (fi.isFile() && DzBridgeTools::IsFileTypeInList(fi, aExtensionsToDelete)) {
				qdirScripts.remove(fi.filePath());
			}
		}
	}
	bool bResult = qdirScripts.rmdir(qdirScripts.absolutePath());

	return bResult;
}

QString DzBridgeTools::CleanTrailingSeparator(QString sFolderPath)
{
	QString sResult = sFolderPath;
	sResult = sResult.replace("\\", "/").toLower();

	while (sResult.endsWith("/") && sResult.length() > 2)
	{
		sResult.chop(1);
	}

	if (sResult == "/") {
		return "";
	}

	return sResult;
}

bool DzBridgeTools::IsDangerousPath(const QString& sPath)
{
	if (sPath.isEmpty()) {
		return true;
	}

	QDir dir(sPath);
	// Normalize path
	QString sNormalizedPath = dir.canonicalPath(); // Removes symbolic links, redundant "." and ".."
	sNormalizedPath = sNormalizedPath.replace("\\", "/").toLower(); // Normalize directory separators

	// Check common dangerous paths
	if (sNormalizedPath.isEmpty() ||
		sNormalizedPath == "/" ||
		sNormalizedPath == "~" ||
		sNormalizedPath == QDir::homePath() ||
		sNormalizedPath == QDir::rootPath() ||
		sNormalizedPath == QDir::tempPath()) // system temporary directory
	{
		return true;
	}

	// Optionally add platform-specific checks
#ifdef Q_OS_WIN
	if (sNormalizedPath == "c:/" || sNormalizedPath == "d:/") {
		return true;
	}
	QStringList sSplit = sNormalizedPath.split(":");
	if (sSplit.length() == 2) {
		QString cleanedString = DzBridgeTools::CleanTrailingSeparator(sSplit[1]);
		if (cleanedString == "/windows") {
			return true;
		}
	}
#endif

	return false;
}
