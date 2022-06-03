#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzBridgeAction.h"
#include "DzBridgeAction_Scriptable.h"
#include "dzprogress.h"

#include "dzbridge.h"

using namespace DzBridgeNameSpace;

/// <summary>
/// Constructor
/// </summary>
UnitTest_DzBridgeAction::UnitTest_DzBridgeAction()
{
	m_testObject = (QObject*) new ::DzBridgeAction();
}

/// <summary>
/// This class also tests DzBridgeAction indirectly since DzBridgeAction is an abstract base class.
/// </summary>
/// <returns></returns>
bool UnitTest_DzBridgeAction::runUnitTests()
{
	if (!m_testObject)
	{
		return false;
	}

	RUNTEST(_DzBridgeAction);
	RUNTEST(resetToDefaults);
	RUNTEST(cleanString);
	RUNTEST(getAvailableMorphs);
	RUNTEST(getActiveMorphs);
	RUNTEST(makeNormalMapFromHeightMap);
	RUNTEST(preProcessScene);
	RUNTEST(renameDuplicateMaterial);
	RUNTEST(undoRenameDuplicateMaterials);
	RUNTEST(generateMissingNormalMap);
	RUNTEST(undoGenerateMissingNormalMaps);
	RUNTEST(getActionGroup);
	RUNTEST(getDefaultMenuPath);
	RUNTEST(exportAsset);
	RUNTEST(exportNode);
	RUNTEST(writeConfiguration);
	RUNTEST(setExportOptions);
	RUNTEST(readGuiRootFolder);
	RUNTEST(writeDtuHeader);
	RUNTEST(startMaterialBlock);
	RUNTEST(finishMaterialBlock);
	RUNTEST(writeAllMaterials);
	RUNTEST(writeMaterialProperty);
	RUNTEST(writeAllMorphs);
	RUNTEST(writeMorphProperties);
	RUNTEST(writeMorphJointLinkInfo);
	RUNTEST(writeAllSubdivisions);
	RUNTEST(writeSubdivisionProperties);
	RUNTEST(writeAllDforceInfo);
	RUNTEST(writeDforceMaterialProperties);
	RUNTEST(writeDforceModifiers);
	RUNTEST(writeEnvironment);
	RUNTEST(writeInstances);
	RUNTEST(writeInstance);
	RUNTEST(writeAllPoses);
	RUNTEST(renameDuplicateMaterials2);
	RUNTEST(undoRenameDuplicateMaterials2);
	RUNTEST(getScenePropList);
	RUNTEST(disconnectNode);
	RUNTEST(reconnectNodes);
	RUNTEST(disconnectOverrideControllers);
	RUNTEST(reconnectOverrideControllers);
	RUNTEST(checkIfPoseExportIsDestructive);
	RUNTEST(unlockTransform);
	RUNTEST(getBridgeDialog);
	RUNTEST(setBridgeDialog);
	RUNTEST(getSubdivisionDialog);
	RUNTEST(setSubdivisionDialog);
	RUNTEST(getMorphSelectionDialog);
	RUNTEST(setMorphSelectionDialog);
	RUNTEST(getAssetType);
	RUNTEST(setAssetType);
	RUNTEST(getExportFilename);
	RUNTEST(setExportFilename);
	RUNTEST(getExportFolder);
	RUNTEST(setExportFolder);
	RUNTEST(getRootFolder);
	RUNTEST(setRootFolder);
	RUNTEST(getProductName);
	RUNTEST(setProductName);
	RUNTEST(getProductComponentName);
	RUNTEST(setProductComponentName);
	RUNTEST(getMorphList);
	RUNTEST(setMorphList);
	RUNTEST(getUseRelativePaths);
	RUNTEST(setUseRelativePaths);
	RUNTEST(isTemporaryFile);
	RUNTEST(exportAssetWithDtu);
	RUNTEST(writePropertyTexture);
	RUNTEST(makeUniqueFilename);
	RUNTEST(getUndoNormalMaps);
	RUNTEST(setUndoNormalMaps);
	RUNTEST(getNonInteractiveMode);
	RUNTEST(setNonInteractiveMode);
	RUNTEST(getExportFbx);
	RUNTEST(setExportFbx);
	RUNTEST(readGui);
	RUNTEST(exportHD);
	RUNTEST(upgradeToHD);
	RUNTEST(writeWeightMaps);
	RUNTEST(metaInvokeMethod);
	RUNTEST(copyFile);
	RUNTEST(getMD5);


	return true;
}

bool UnitTest_DzBridgeAction::_DzBridgeAction(UnitTest::TestResult *testResult)
{
	bool bResult = true;
	LOGTEST_TEXT("DzBridgeAction is an abstract class.  Can not test constructor.");

/**
	// Can not build because constructor for abstract class
	DzBridgeAction *testObj = new DzBridgeAction();
	if (!testObj)
		LOGTEST_FAILED("");
	else
		LOGTEST_PASSED("");
*/

	return bResult;
}

bool UnitTest_DzBridgeAction::resetToDefaults(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->resetToDefaults());
	return true;
}

bool UnitTest_DzBridgeAction::cleanString(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->cleanString(nullptr));

	TRY_METHODCALL_CUSTOM(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->cleanString(""), "C++ exception with empty string test.");

	return bResult;

}

bool UnitTest_DzBridgeAction::getAvailableMorphs(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getAvailableMorphs(new DzNode()));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getAvailableMorphs(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getActiveMorphs(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getActiveMorphs(new DzNode()));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getActiveMorphs(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::makeNormalMapFromHeightMap(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->makeNormalMapFromHeightMap(nullptr, 0.0));

	return bResult;
}

bool UnitTest_DzBridgeAction::preProcessScene(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->preProcessScene(new DzNode()));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->preProcessScene(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::renameDuplicateMaterial(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->renameDuplicateMaterial(nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::undoRenameDuplicateMaterials(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->undoRenameDuplicateMaterials());

	return bResult;
}

bool UnitTest_DzBridgeAction::generateMissingNormalMap(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->generateMissingNormalMap(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::undoGenerateMissingNormalMaps(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->undoGenerateMissingNormalMaps());

	return bResult;
}

bool UnitTest_DzBridgeAction::getActionGroup(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getActionGroup());

	return bResult;
}

bool UnitTest_DzBridgeAction::getDefaultMenuPath(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getDefaultMenuPath());

	return bResult;
}

bool UnitTest_DzBridgeAction::exportAsset(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->exportAsset());

	return bResult;
}

bool UnitTest_DzBridgeAction::exportNode(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->exportNode(new DzNode()));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->exportNode(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeConfiguration(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeConfiguration());

	return bResult;
}

bool UnitTest_DzBridgeAction::setExportOptions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzFileIOSettings arg;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setExportOptions(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::readGuiRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->readGuiRootFolder());

	return bResult;
}

bool UnitTest_DzBridgeAction::writeDtuHeader(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeDTUHeader(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::startMaterialBlock(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->startMaterialBlock(nullptr, arg, nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::finishMaterialBlock(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->finishMaterialBlock(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeAllMaterials(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeAllMaterials(nullptr, arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeMaterialProperty(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeMaterialProperty(nullptr, arg, nullptr, nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeAllMorphs(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeAllMorphs(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeMorphProperties(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeMorphProperties(arg, "", ""));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeMorphJointLinkInfo(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg1(nullptr);
	JointLinkInfo arg2;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeMorphJointLinkInfo(arg1, arg2));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeAllSubdivisions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeAllSubdivisions(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeSubdivisionProperties(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeSubdivisionProperties(arg, "", 0));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeAllDforceInfo(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeAllDforceInfo(nullptr, arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeDforceMaterialProperties(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeDforceMaterialProperties(arg, nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeDforceModifiers(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg1(nullptr);
	DzModifierList arg2;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeDforceModifiers(arg2, arg1, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeEnvironment(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeEnvironment(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeInstances(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg1(nullptr);
	QMap<QString, DzMatrix3> arg2;
	QList<DzGeometry*> arg3;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeInstances(nullptr, arg1, arg2, arg3));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeInstance(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeInstance(nullptr, arg, 0));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeAllPoses(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeAllPoses(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::renameDuplicateMaterials2(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QList<QString> arg1;
	QMap<DzMaterial*, QString> arg2;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->renameDuplicateMaterials(nullptr, arg1, arg2));

	return bResult;
}

bool UnitTest_DzBridgeAction::undoRenameDuplicateMaterials2(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QList<QString> arg1;
	QMap<DzMaterial*, QString> arg2;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->undoRenameDuplicateMaterials(nullptr, arg1, arg2));

	return bResult;
}

bool UnitTest_DzBridgeAction::getScenePropList(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QMap<QString, DzNode*> arg;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getScenePropList(nullptr, arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::disconnectNode(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QList<DzBridgeNameSpace::DzBridgeAction::AttachmentInfo> arg;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->disconnectNode(nullptr, arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::reconnectNodes(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QList<DzBridgeNameSpace::DzBridgeAction::AttachmentInfo> arg;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->reconnectNodes(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::disconnectOverrideControllers(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->disconnectOverrideControllers());

	return bResult;

}

bool UnitTest_DzBridgeAction::reconnectOverrideControllers(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QList<QString> arg;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->reconnectOverrideControllers(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::checkIfPoseExportIsDestructive(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->checkIfPoseExportIsDestructive());

	return bResult;
}

bool UnitTest_DzBridgeAction::unlockTransform(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->unlockTranform(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getBridgeDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getBridgeDialog());

	return bResult;

}

bool UnitTest_DzBridgeAction::setBridgeDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setBridgeDialog(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getSubdivisionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getSubdivisionDialog());

	return bResult;
}

bool UnitTest_DzBridgeAction::setSubdivisionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setSubdivisionDialog(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getMorphSelectionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getMorphSelectionDialog());

	return bResult;
}

bool UnitTest_DzBridgeAction::setMorphSelectionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setMorphSelectionDialog(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getAssetType(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getAssetType());

	return bResult;
}

bool UnitTest_DzBridgeAction::setAssetType(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setAssetType(""));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setAssetType(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getExportFilename(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getExportFilename());

	return bResult;
}

bool UnitTest_DzBridgeAction::setExportFilename(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setExportFilename(""));

	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setExportFilename(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getExportFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getExportFolder());

	return bResult;
}

bool UnitTest_DzBridgeAction::setExportFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setExportFolder(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::getRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getRootFolder());

	return bResult;
}

bool UnitTest_DzBridgeAction::setRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setRootFolder(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::getProductName(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getProductName());

	return bResult;
}

bool UnitTest_DzBridgeAction::setProductName(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setProductName(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::getProductComponentName(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getProductComponentName());

	return bResult;
}

bool UnitTest_DzBridgeAction::setProductComponentName(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setProductComponentName(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::getMorphList(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getMorphList());

	return bResult;
}

bool UnitTest_DzBridgeAction::setMorphList(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	QStringList arg;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setMorphList(arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::getUseRelativePaths(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getUseRelativePaths());

	return bResult;
}

bool UnitTest_DzBridgeAction::setUseRelativePaths(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setUseRelativePaths(0));

	return bResult;
}

bool UnitTest_DzBridgeAction::isTemporaryFile(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->isTemporaryFile(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::exportAssetWithDtu(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->exportAssetWithDtu(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::writePropertyTexture(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writePropertyTexture(arg,"",0,"","",""));

	return bResult;
}

bool UnitTest_DzBridgeAction::makeUniqueFilename(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->makeUniqueFilename(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::getUndoNormalMaps(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getUndoNormalMaps());

	return bResult;
}

bool UnitTest_DzBridgeAction::setUndoNormalMaps(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setUndoNormalMaps(0));

	return bResult;
}

bool UnitTest_DzBridgeAction::getNonInteractiveMode(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getNonInteractiveMode());

	return bResult;
}

bool UnitTest_DzBridgeAction::setNonInteractiveMode(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setNonInteractiveMode(0));

	return bResult;
}

bool UnitTest_DzBridgeAction::getExportFbx(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getExportFbx());

	return bResult;
}

bool UnitTest_DzBridgeAction::setExportFbx(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setExportFbx(""));

	return bResult;
}

bool UnitTest_DzBridgeAction::readGui(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->readGui(nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::exportHD(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzProgress* arg = new DzProgress("", 0, true, true);
	arg->enable(false);
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->exportHD(arg));
	arg->finish();

	return bResult;
}

bool UnitTest_DzBridgeAction::upgradeToHD(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->setNonInteractiveMode(true));
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->upgradeToHD("","", "", nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::writeWeightMaps(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->writeWeightMaps(nullptr, arg));

	return bResult;
}

bool UnitTest_DzBridgeAction::metaInvokeMethod(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->metaInvokeMethod(nullptr, nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::copyFile(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->copyFile(nullptr, nullptr));

	return bResult;
}

bool UnitTest_DzBridgeAction::getMD5(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeNameSpace::DzBridgeAction*>(m_testObject)->getMD5(""));

	return bResult;
}




#include "moc_UnitTest_DzBridgeAction.cpp"
#endif