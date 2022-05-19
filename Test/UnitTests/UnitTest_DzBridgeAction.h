#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzBridgeAction : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzBridgeAction();

	bool runUnitTests();

private:
	bool _DzBridgeAction(UnitTest::TestResult* testResult);
	bool resetToDefaults(UnitTest::TestResult* testResult);
	bool cleanString(UnitTest::TestResult* testResult);
	bool getAvailableMorphs(UnitTest::TestResult* testResult);
	bool getActiveMorphs(UnitTest::TestResult* testResult);
	bool makeNormalMapFromHeightMap(UnitTest::TestResult* testResult);
	bool preProcessScene(UnitTest::TestResult* testResult);
	bool renameDuplicateMaterial(UnitTest::TestResult* testResult);
	bool undoRenameDuplicateMaterials(UnitTest::TestResult* testResult);
	bool generateMissingNormalMap(UnitTest::TestResult* testResult);
	bool undoGenerateMissingNormalMaps(UnitTest::TestResult* testResult);
	bool getActionGroup(UnitTest::TestResult* testResult);
	bool getDefaultMenuPath(UnitTest::TestResult* testResult);
	bool exportAsset(UnitTest::TestResult* testResult);
	bool exportNode(UnitTest::TestResult* testResult);
	bool writeConfiguration(UnitTest::TestResult* testResult);
	bool setExportOptions(UnitTest::TestResult* testResult);
	bool readGuiRootFolder(UnitTest::TestResult* testResult);
	bool writeDtuHeader(UnitTest::TestResult* testResult);
	bool startMaterialBlock(UnitTest::TestResult* testResult);
	bool finishMaterialBlock(UnitTest::TestResult* testResult);
	bool writeAllMaterials(UnitTest::TestResult* testResult);
	bool writeMaterialProperty(UnitTest::TestResult* testResult);
	bool writeAllMorphs(UnitTest::TestResult* testResult);
	bool writeMorphProperties(UnitTest::TestResult* testResult);
	bool writeMorphJointLinkInfo(UnitTest::TestResult* testResult);
	bool writeAllSubdivisions(UnitTest::TestResult* testResult);
	bool writeSubdivisionProperties(UnitTest::TestResult* testResult);
	bool writeAllDforceInfo(UnitTest::TestResult* testResult);
	bool writeDforceMaterialProperties(UnitTest::TestResult* testResult);
	bool writeDforceModifiers(UnitTest::TestResult* testResult);
	bool writeEnvironment(UnitTest::TestResult* testResult);
	bool writeInstances(UnitTest::TestResult* testResult);
	bool writeInstance(UnitTest::TestResult* testResult);
	bool writeAllPoses(UnitTest::TestResult* testResult);
	bool renameDuplicateMaterials2(UnitTest::TestResult* testResult);
	bool undoRenameDuplicateMaterials2(UnitTest::TestResult* testResult);
	bool getScenePropList(UnitTest::TestResult* testResult);
	bool disconnectNode(UnitTest::TestResult* testResult);
	bool reconnectNodes(UnitTest::TestResult* testResult);
	bool disconnectOverrideControllers(UnitTest::TestResult* testResult);
	bool reconnectOverrideControllers(UnitTest::TestResult* testResult);
	bool checkIfPoseExportIsDestructive(UnitTest::TestResult* testResult);
	bool unlockTransform(UnitTest::TestResult* testResult);
	bool getBridgeDialog(UnitTest::TestResult* testResult);
	bool setBridgeDialog(UnitTest::TestResult* testResult);
	bool getSubdivisionDialog(UnitTest::TestResult* testResult);
	bool setSubdivisionDialog(UnitTest::TestResult* testResult);
	bool getMorphSelectionDialog(UnitTest::TestResult* testResult);
	bool setMorphSelectionDialog(UnitTest::TestResult* testResult);
	bool getAssetType(UnitTest::TestResult* testResult);
	bool setAssetType(UnitTest::TestResult* testResult);
	bool getExportFilename(UnitTest::TestResult* testResult);
	bool setExportFilename(UnitTest::TestResult* testResult);
	bool getExportFolder(UnitTest::TestResult* testResult);
	bool setExportFolder(UnitTest::TestResult* testResult);
	bool getRootFolder(UnitTest::TestResult* testResult);
	bool setRootFolder(UnitTest::TestResult* testResult);
	bool getProductName(UnitTest::TestResult* testResult);
	bool setProductName(UnitTest::TestResult* testResult);
	bool getProductComponentName(UnitTest::TestResult* testResult);
	bool setProductComponentName(UnitTest::TestResult* testResult);
	bool getMorphList(UnitTest::TestResult* testResult);
	bool setMorphList(UnitTest::TestResult* testResult);
	bool getUseRelativePaths(UnitTest::TestResult* testResult);
	bool setUseRelativePaths(UnitTest::TestResult* testResult);
	bool isTemporaryFile(UnitTest::TestResult* testResult);
	bool exportAssetWithDtu(UnitTest::TestResult* testResult);
	bool writePropertyTexture(UnitTest::TestResult* testResult);
	bool makeUniqueFilename(UnitTest::TestResult* testResult);
	bool getUndoNormalMaps(UnitTest::TestResult* testResult);
	bool setUndoNormalMaps(UnitTest::TestResult* testResult);
	bool getNonInteractiveMode(UnitTest::TestResult* testResult);
	bool setNonInteractiveMode(UnitTest::TestResult* testResult);
	bool getExportFbx(UnitTest::TestResult* testResult);
	bool setExportFbx(UnitTest::TestResult* testResult);
	bool readGui(UnitTest::TestResult* testResult);
	bool exportHD(UnitTest::TestResult* testResult);
	bool upgradeToHD(UnitTest::TestResult* testResult);
	bool writeWeightMaps(UnitTest::TestResult* testResult);
	bool metaInvokeMethod(UnitTest::TestResult* testResult);
	bool copyFile(UnitTest::TestResult* testResult);
	bool getMD5(UnitTest::TestResult* testResult);

};

#endif