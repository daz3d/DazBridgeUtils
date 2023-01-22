#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzBridgeDialog.h"
#include "DzBridgeDialog.h"

#include "dzbridge.h"
using namespace DzBridgeNameSpace;

UnitTest_DzBridgeDialog::UnitTest_DzBridgeDialog()
{
	m_testObject = (QObject*) new DzBridgeDialog();
}

bool UnitTest_DzBridgeDialog::runUnitTests()
{
	DzBridgeDialog* testObject = new DzBridgeDialog();

	if (!testObject)
	{
		return false;
	}

	RUNTEST(getAssetNameEdit);
	RUNTEST(getAssetTypeCombo);
	RUNTEST(getMorphsEnabledCheckBox);
	RUNTEST(getSubdivisionEnabledCheckBox);
	RUNTEST(getAdvancedSettingsGroupBox);
	RUNTEST(getFbxVersionCombo);
	RUNTEST(getShowFbxDialogCheckBox);
	RUNTEST(_DzBridgeDialog);
	RUNTEST(GetMorphString);
	RUNTEST(GetMorphMappingFromMorphSelectionDialog);
	RUNTEST(resetToDefaults);
	RUNTEST(loadSavedSettings);
	RUNTEST(Accepted);
	RUNTEST(handleSceneSelectionChanged);
	RUNTEST(HandleChooseMorphsButton);
	RUNTEST(HandleMorphsCheckBoxChange);
	RUNTEST(HandleChooseSubdivisionsButton);
	RUNTEST(HandleFBXVersionChange);
	RUNTEST(HandleShowFbxDialogCheckBoxChange);
	RUNTEST(HandleExportMaterialPropertyCSVCheckBoxChange);
	RUNTEST(HandleShowAdvancedSettingsCheckBoxChange);
	RUNTEST(refreshAsset);

	return true;
}

bool UnitTest_DzBridgeDialog::getAssetNameEdit(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getAssetNameEdit());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getAssetTypeCombo(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getAssetTypeCombo());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getMorphsEnabledCheckBox(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getMorphsEnabledCheckBox());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getSubdivisionEnabledCheckBox(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getSubdivisionEnabledCheckBox());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getAdvancedSettingsGroupBox(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getAdvancedSettingsGroupBox());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getFbxVersionCombo(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getFbxVersionCombo());
	return bResult;
}

bool UnitTest_DzBridgeDialog::getShowFbxDialogCheckBox(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->getShowFbxDialogCheckBox());
	return bResult;
}

bool UnitTest_DzBridgeDialog::_DzBridgeDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzBridgeDialog());
	return bResult;
}

bool UnitTest_DzBridgeDialog::GetMorphString(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->GetMorphString());
	return bResult;
}

bool UnitTest_DzBridgeDialog::GetMorphMappingFromMorphSelectionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->GetMorphMappingFromMorphSelectionDialog());
	return bResult;
}

bool UnitTest_DzBridgeDialog::resetToDefaults(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->resetToDefaults());
	return bResult;
}

bool UnitTest_DzBridgeDialog::loadSavedSettings(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->loadSavedSettings());
	return bResult;
}

bool UnitTest_DzBridgeDialog::Accepted(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	LOGTEST_TEXT("Accepted is Qt framework GUI method. Skipping UnitTest...");
//	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->Accepted());
	return bResult;
}

bool UnitTest_DzBridgeDialog::handleSceneSelectionChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->handleSceneSelectionChanged());
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleChooseMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleChooseMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleMorphsCheckBoxChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleMorphsCheckBoxChange(0));
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleChooseSubdivisionsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleChooseSubdivisionsButton());
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleFBXVersionChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleFBXVersionChange(0));
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleShowFbxDialogCheckBoxChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleShowFbxDialogCheckBoxChange(0));
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleExportMaterialPropertyCSVCheckBoxChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleExportMaterialPropertyCSVCheckBoxChange(0));
	return bResult;
}

bool UnitTest_DzBridgeDialog::HandleShowAdvancedSettingsCheckBoxChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->HandleShowAdvancedSettingsCheckBoxChange(0));
	return bResult;
}

bool UnitTest_DzBridgeDialog::refreshAsset(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeDialog*>(m_testObject)->refreshAsset());
	return bResult;
}

#include "moc_UnitTest_DzBridgeDialog.cpp"
#endif