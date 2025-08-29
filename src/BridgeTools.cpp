
#include <QtCore>

#include "dzapp.h"
#include "dzfloatproperty.h"

#include "DzBridgeAction.h"
#include "BridgeTools.h"

using namespace DzBridgeNameSpace;

// DB 2024-11-080: Used for Work around to correct Character Morphs using non - standard Object ERC Offset
bool BridgeTools::CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset)
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

bool BridgeTools::IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList) {


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
bool BridgeTools::SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete)
{
	if (IsDangerousPath(sSubFolderPath)) {
		return false;
	}

	QDir qdirScripts(sSubFolderPath);
	if (qdirScripts.exists()) {
		// remove all known intermediate file types within it (nonrecursively)
		foreach(QFileInfo fi, qdirScripts.entryInfoList())
		{
			if (fi.isFile() && BridgeTools::IsFileTypeInList(fi, aExtensionsToDelete)) {
				qdirScripts.remove(fi.filePath());
			}
		}
	}
	bool bResult = qdirScripts.rmdir(qdirScripts.absolutePath());

	return bResult;
}

QString BridgeTools::CleanTrailingSeparator(QString sFolderPath)
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

bool BridgeTools::IsDangerousPath(const QString& sPath)
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

// 40 Options for FbxExporter (as of 2025-08-29)
void BridgeTools::SetExportOptionsAllOff(DzFileIOSettings &ExportOptions)
{
//	ExportOptions.setStringValue("Format", "FBX 2012 -- Binary");
	ExportOptions.setStringValue("Format", "FBX 2012 -- Ascii");
	ExportOptions.setStringValue("Author", "");
	ExportOptions.setStringValue("Title", "");
	ExportOptions.setStringValue("Subject", "");
	ExportOptions.setStringValue("Keywords", "");
	ExportOptions.setStringValue("Revision", "");
	ExportOptions.setStringValue("Comment", "");
	ExportOptions.setBoolValue("IncludeSelectedOnly", false);
	ExportOptions.setBoolValue("IncludeVisibleOnly", false);
	ExportOptions.setBoolValue("IncludeFigures", false);
	ExportOptions.setBoolValue("IncludeProps", false);
	ExportOptions.setBoolValue("IncludeLights", false);
	ExportOptions.setBoolValue("IncludeCameras", false);
	ExportOptions.setBoolValue("MergeFollowers", false);
	ExportOptions.setBoolValue("StaticFollowers", false);
	ExportOptions.setBoolValue("AllowDegradedSkinning", false);
	ExportOptions.setBoolValue("AllowDegradedScaling", false);
	ExportOptions.setBoolValue("BasePoseOnly", false);
	ExportOptions.setBoolValue("IncludeRotationLimits", false);
	ExportOptions.setBoolValue("IncludeRotationLocks", false);
	ExportOptions.setBoolValue("IncludeAnimations", false);
//	ExportOptions.setStringValue("Take", "");
	ExportOptions.removeValue("Take");
	ExportOptions.setBoolValue("IncludeFPS", false);
	ExportOptions.setBoolValue("IncludeSubD", false);
	ExportOptions.setBoolValue("IncludeFaceGroupsAsPolygonSets", false);
	ExportOptions.setBoolValue("IncludeFaceGroupsAsPolygonGroups", false);
	ExportOptions.setBoolValue("IncludeMorphs", false);
	ExportOptions.setStringValue("MorphRules", "");
	ExportOptions.setBoolValue("CollapseUVTiles", false);
	ExportOptions.setBoolValue("EmbedTextures", false);
	ExportOptions.setBoolValue("CollectTextures", false);
	ExportOptions.setBoolValue("MergeDiffuseOpacity", false);
	ExportOptions.setBoolValue("IncludeNodeNamesLabels", false);
	ExportOptions.setBoolValue("IncludeNodePresentation", false);
	ExportOptions.setBoolValue("IncludeNodeSelectionMap", false);
	ExportOptions.setBoolValue("IncludeSceneIDs", false);
	ExportOptions.setBoolValue("IncludeFollowTargets", false);
	ExportOptions.setBoolValue("GenerateMayaHelperScript", false);
	ExportOptions.setBoolValue("MentalRayMaterials", false);
	ExportOptions.setIntValue("RunSilent", 0);
}

void BridgeTools::SetExportOptionsBridgeDefaults(DzFileIOSettings &ExportOptions)
{
	SetExportOptionsAllOff(ExportOptions);
	ExportOptions.setStringValue("Format", "FBX 2012 -- Binary");
	ExportOptions.setBoolValue("IncludeSelectedOnly", true);
//	ExportOptions.setBoolValue("IncludeVisibleOnly", false);
	ExportOptions.setBoolValue("IncludeFigures", true);
	ExportOptions.setBoolValue("IncludeProps", true);
	ExportOptions.setBoolValue("MergeFollowers", true);
//	ExportOptions.setBoolValue("StaticFollowers", false);
	ExportOptions.setBoolValue("AllowDegradedSkinning", true);
	ExportOptions.setBoolValue("AllowDegradedScaling", true);
//	ExportOptions.setBoolValue("IncludeRotationLimits", false);
//	ExportOptions.setBoolValue("IncludeRotationLocks", false);
//	ExportOptions.setBoolValue("IncludeAnimations", false);
//	ExportOptions.setStringValue("Take", "Animation");
	ExportOptions.setBoolValue("IncludeSubD", true);
//	ExportOptions.setBoolValue("IncludeMorphs", false);
//	ExportOptions.setStringValue("MorphRules", "");
	ExportOptions.setBoolValue("IncludeNodeNamesLabels", true);
	ExportOptions.setBoolValue("IncludeNodePresentation", true);
	ExportOptions.setBoolValue("IncludeSceneIDs", true);
	ExportOptions.setBoolValue("IncludeFollowTargets", true);
	ExportOptions.setIntValue("RunSilent", 1);
}

void BridgeTools::SetExportOptionsMvcProxyMesh(DzFileIOSettings &ExportOptions)
{
	SetExportOptionsAllOff(ExportOptions);
	ExportOptions.setStringValue("Format", "FBX 2012 -- Binary");
	ExportOptions.setBoolValue("IncludeSelectedOnly", true);
	ExportOptions.setBoolValue("IncludeVisibleOnly", true);
	ExportOptions.setBoolValue("IncludeFigures", true);
	ExportOptions.setBoolValue("MergeFollowers", true);
	ExportOptions.setBoolValue("AllowDegradedSkinning", true);
	ExportOptions.setBoolValue("AllowDegradedScaling", true);
	ExportOptions.setBoolValue("IncludeNodeNamesLabels", true);
	ExportOptions.setBoolValue("IncludeNodePresentation", true);
	ExportOptions.setBoolValue("IncludeSceneIDs", true);
	ExportOptions.setIntValue("RunSilent", 1);
}
