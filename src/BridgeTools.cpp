
#include <QtCore>

#include "dzapp.h"
#include "dzfloatproperty.h"
#include "dzexporter.h"

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
		QString cleanedString = BridgeTools::CleanTrailingSeparator(sSplit[1]);
		if (cleanedString == "/windows") {
			return true;
		}
	}
#endif

	return false;
}

// 40 Options for FbxExporter (as of 2025-08-29)
void BridgeTools::SetFbxExportOptionsAllOff(DzFileIOSettings &ExportOptions)
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

bool BridgeTools::LogDefaultExportOptions(DzExporter* Exporter)
{
	if (!Exporter) return false;
	DzFileIOSettings ExportOptions;
	Exporter->getDefaultOptions(&ExportOptions);
	QString sMesg = QString("ExportOptions (DzFileIOSettings) for '%1'").arg(Exporter->className());
	dzApp->log(sMesg);
	for (int i = 0; i < ExportOptions.getNumValues(); i++)
	{
		QString sKey = ExportOptions.getKey(i);
		QString sValue = ExportOptions.getValue(i);
		QString sKVP = QString("%1 = %2").arg(sKey).arg(sValue);
		printf("%s\n", sKVP.toLocal8Bit().constData());
		dzApp->log(sKVP);
	}
	sMesg = QString("End of ExportOptions for '%1'").arg(Exporter->className());
	dzApp->log(sMesg);
	return true;
}

void BridgeTools::SetFbxExportOptionsBridgeDefaults(DzFileIOSettings &ExportOptions)
{
	SetFbxExportOptionsAllOff(ExportOptions);
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
	ExportOptions.setStringValue("Take", "Animation");
	ExportOptions.setBoolValue("IncludeSubD", true);
//	ExportOptions.setBoolValue("IncludeMorphs", false);
//	ExportOptions.setStringValue("MorphRules", "");
	ExportOptions.setBoolValue("IncludeNodeNamesLabels", true);
	ExportOptions.setBoolValue("IncludeNodePresentation", true);
	ExportOptions.setBoolValue("IncludeSceneIDs", true);
	ExportOptions.setBoolValue("IncludeFollowTargets", true);
	ExportOptions.setIntValue("RunSilent", 1);
}

void BridgeTools::SetFbxExportOptionsMvcProxyMesh(DzFileIOSettings &ExportOptions)
{
	SetFbxExportOptionsAllOff(ExportOptions);
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

#include "dzobject.h"
#include "dzfigure.h"
#include "dzgeometry.h"
#include "dzmodifier.h"
#include "dzpushmodifier.h"
#include "dzintproperty.h"
#include "dzprogress.h"
#include "dzscript.h"

bool addSmoothingModifier(DzNode* pNode)
{
	DzScript* Script = new DzScript();

	Script->addLine("function myFunction(oRawNode) {");
	Script->addLine("var oSmoothing = new DzMeshSmoothModifier();");
	Script->addLine("oSmoothing.setName(\"DzMeshSmoothModifier\");");
	Script->addLine("Scene.setPrimarySelection(oRawNode);");
	Script->addLine("var oNode = Scene.getPrimarySelection();");
	Script->addLine("oNode.getObject().addModifier(oSmoothing);");
	Script->addLine("var result = oSmoothing;");
	Script->addLine("};");
	QVariantList Args;
	Args.append(QVariant(QMetaType::QObjectStar, &pNode));
	return Script->call("myFunction", Args);
}

bool addPushModifier(DzNode* pNode)
{
	DzScript* Script = new DzScript();

	Script->addLine("function myFunction(oRawNode) {");
	Script->addLine("print(\"test\");");
	Script->addLine("App.log(\"test\");");
	Script->addLine("var oPushMod = new DzPushModifier();");
	Script->addLine("oPushMod.setName(\"PushModifier\");");
	Script->addLine("oPushMod.getProperty(0).setValue(100);");
	Script->addLine("Scene.setPrimarySelection(oRawNode);");
	Script->addLine("var oNode = Scene.getPrimarySelection();");
	Script->addLine("oNode.getObject().addModifier(oPushMod);");
	Script->addLine("var result = oPushMod;");
	Script->addLine("};");
	QVariantList Args;
	Args.append(QVariant(QMetaType::QObjectStar, &pNode));
	return Script->call("myFunction", Args);
}

#include "dzscene.h"
#include "dzvertexmesh.h"
#include "dzfacetmesh.h"
#include "dzboolproperty.h"
bool BridgeTools::ExpandClothingFit(DzNode* pNode)
{
	if (!pNode) return false;

	QList<DzFigure*>aClothingFollowers;
	auto aAllChildren = pNode->getNodeChildren(/*scanHierarchy*/ true);
	foreach(QObject * pQObject, aAllChildren)
	{
		DzFigure* pChildFigure = qobject_cast<DzFigure*>(pQObject);
		if (!pChildFigure) continue;
		if (pChildFigure->getSkeleton()->getFollowTarget() == pNode->getSkeleton())
		{
			aClothingFollowers.append(pChildFigure);
		}
	}

	if (aClothingFollowers.isEmpty()) return false;

	foreach(DzFigure* pClothing, aClothingFollowers)
	{
		// adjust mesh smoothing
		DzModifier* pModifier = pClothing->getObject()->findModifier("DzMeshSmoothModifier");
		if (!pModifier)
		{
			// add smoothing modifier
			addSmoothingModifier(pClothing);
			pModifier = pClothing->getObject()->findModifier("DzMeshSmoothModifier");
			if (!pModifier) {
				continue;
			}
		}
		DzIntProperty* pSmoothingProp = (DzIntProperty*)pModifier->findProperty("Smoothing Iterations");
		pSmoothingProp->setValue(10);
		DzIntProperty* pCollisionProp = (DzIntProperty*)pModifier->findProperty("Collision Iterations");
		pCollisionProp->setValue(50);
		auto prop = (DzBoolProperty*)pModifier->findProperty("Enable Smoothing");
		if (prop) prop->setBoolValue(1);
		// Must Re-Fit Clothing
		pClothing->setFollowTarget(nullptr);
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		pClothing->setFollowTarget(pNode->getSkeleton());
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	DzObject* pObject = pNode->getObject();
	DzGeometry* pMesh = (DzGeometry*) pObject->getCachedGeom();
	addPushModifier(pNode);
	DzModifier* pPushMod = pObject->findModifier("PushModifier");
	if (pPushMod) {
		DzFloatProperty* pPushValue = (DzFloatProperty*)pPushMod->findProperty("Value");
		pPushValue->setValue(0.5);
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	// wait for background
	while (DzBackgroundProgress::isActive()) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	QMap<DzFigure*, DzVertexMesh*> bakedMeshLookup;
	foreach(DzFigure* pClothing, aClothingFollowers)
	{
		//pClothing->getObject()->forceCacheUpdate(pClothing, true);
		auto pMesh = pClothing->getObject()->getCachedGeom();
		DzFacetMesh* pCachedMesh = new DzFacetMesh();
		pCachedMesh->copyFrom(pMesh, false, false);
		bakedMeshLookup.insert(pClothing, (DzVertexMesh*) pCachedMesh);
	}

	if (pPushMod) {
		DzFloatProperty* pPushValue = (DzFloatProperty*)pPushMod->findProperty("Value");
		pPushValue->setValue(0);
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//		pObject->removeModifier(pPushMod);
	}
//
	foreach(DzFigure* pClothing, aClothingFollowers)
	{
		DzModifier* pModifier = pClothing->getObject()->findModifier("DzMeshSmoothModifier");
		if (!pModifier) continue;
		auto prop = (DzBoolProperty*) pModifier->findProperty("Enable Smoothing");
		if (prop) prop->setBoolValue(0);
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	// wait for background
	while (DzBackgroundProgress::isActive()) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	foreach(DzFigure* pClothing, aClothingFollowers)
	{
		DzModifier* pModifier = pClothing->getObject()->findModifier("DzMeshSmoothModifier");
		if (!pModifier) continue;
		auto pCachedMesh = bakedMeshLookup.value(pClothing);
		if (!pCachedMesh) continue;
		MorphTools::createMorph("better_fit", pCachedMesh, pClothing);
		DzFloatProperty* prop = (DzFloatProperty*) MorphTools::BruteForceFindMorph(pClothing, "better_fit");
		if (!prop) continue;
		prop->setValue(1.0);
	}
	
	dzScene->setPrimarySelection(pNode);

	return true;
}
