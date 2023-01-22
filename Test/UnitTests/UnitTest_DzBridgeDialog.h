#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzBridgeDialog : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzBridgeDialog();
	bool runUnitTests();

private:
	bool getAssetNameEdit(UnitTest::TestResult* testResult);
	bool getAssetTypeCombo(UnitTest::TestResult* testResult);
	bool getMorphsEnabledCheckBox(UnitTest::TestResult* testResult);
	bool getSubdivisionEnabledCheckBox(UnitTest::TestResult* testResult);
	bool getAdvancedSettingsGroupBox(UnitTest::TestResult* testResult);
	bool getFbxVersionCombo(UnitTest::TestResult* testResult);
	bool getShowFbxDialogCheckBox(UnitTest::TestResult* testResult);
	bool _DzBridgeDialog(UnitTest::TestResult* testResult);
	bool GetMorphString(UnitTest::TestResult* testResult);
	bool GetMorphMappingFromMorphSelectionDialog(UnitTest::TestResult* testResult);
	bool resetToDefaults(UnitTest::TestResult* testResult);
	bool loadSavedSettings(UnitTest::TestResult* testResult);
	bool Accepted(UnitTest::TestResult* testResult);
	bool handleSceneSelectionChanged(UnitTest::TestResult* testResult);
	bool HandleChooseMorphsButton(UnitTest::TestResult* testResult);
	bool HandleMorphsCheckBoxChange(UnitTest::TestResult* testResult);
	bool HandleChooseSubdivisionsButton(UnitTest::TestResult* testResult);
	bool HandleFBXVersionChange(UnitTest::TestResult* testResult);
	bool HandleShowFbxDialogCheckBoxChange(UnitTest::TestResult* testResult);
	bool HandleExportMaterialPropertyCSVCheckBoxChange(UnitTest::TestResult* testResult);
	bool HandleShowAdvancedSettingsCheckBoxChange(UnitTest::TestResult* testResult);
	bool refreshAsset(UnitTest::TestResult* testResult);

};


#endif