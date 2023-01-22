#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzBridgeMorphSelectionDialog : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzBridgeMorphSelectionDialog();
	bool runUnitTests();

private:
	bool _DzBridgeMorphSelectionDialog(UnitTest::TestResult* testResult);
	bool PrepareDialog(UnitTest::TestResult* testResult);
	bool GetMorphString(UnitTest::TestResult* testResult);
	bool GetMorphCSVString(UnitTest::TestResult* testResult);
	bool GetMorphMapping(UnitTest::TestResult* testResult);
	bool IsAutoJCMEnabled(UnitTest::TestResult* testResult);
	bool GetActiveJointControlledMorphs(UnitTest::TestResult* testResult);
	bool GetMorphLabelFromName(UnitTest::TestResult* testResult);
	bool FilterChanged(UnitTest::TestResult* testResult);
	bool ItemSelectionChanged(UnitTest::TestResult* testResult);
	bool HandleAddMorphsButton(UnitTest::TestResult* testResult);
	bool HandleRemoveMorphsButton(UnitTest::TestResult* testResult);
	bool HandleSavePreset(UnitTest::TestResult* testResult);
	bool HandlePresetChanged(UnitTest::TestResult* testResult);
	bool HandleArmJCMMorphsButton(UnitTest::TestResult* testResult);
	bool HandleLegJCMMorphsButton(UnitTest::TestResult* testResult);
	bool HandleTorsoJCMMorphsButton(UnitTest::TestResult* testResult);
	bool HandleARKitGenesis81MorphsButton(UnitTest::TestResult* testResult);
	bool HandleFaceFXGenesis8Button(UnitTest::TestResult* testResult);
	bool HandleAutoJCMCheckBoxChange(UnitTest::TestResult* testResult);


};


#endif