#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzBridgeMorphSelectionDialog.h"
#include "DzBridgeMorphSelectionDialog.h"

#include "dzbridge.h"
using namespace DzBridgeNameSpace;

UnitTest_DzBridgeMorphSelectionDialog::UnitTest_DzBridgeMorphSelectionDialog()
{
	m_testObject = (QObject*) new DzBridgeMorphSelectionDialog();
}

bool UnitTest_DzBridgeMorphSelectionDialog::runUnitTests()
{
	DzBridgeMorphSelectionDialog* testObject = new DzBridgeMorphSelectionDialog();

	if (!testObject)
	{
		return false;
	}

	RUNTEST(_DzBridgeMorphSelectionDialog);
	RUNTEST(PrepareDialog);
	RUNTEST(GetMorphString);
	RUNTEST(GetMorphCSVString);
	RUNTEST(GetMorphMapping);
	RUNTEST(IsAutoJCMEnabled);
	RUNTEST(GetActiveJointControlledMorphs);
	RUNTEST(GetMorphLabelFromName);
	RUNTEST(FilterChanged);
	RUNTEST(ItemSelectionChanged);
	RUNTEST(HandleAddMorphsButton);
	RUNTEST(HandleRemoveMorphsButton);
	RUNTEST(HandleSavePreset);
	RUNTEST(HandlePresetChanged);
	RUNTEST(HandleArmJCMMorphsButton);
	RUNTEST(HandleLegJCMMorphsButton);
	RUNTEST(HandleTorsoJCMMorphsButton);
	RUNTEST(HandleARKitGenesis81MorphsButton);
	RUNTEST(HandleFaceFXGenesis8Button);
	RUNTEST(HandleAutoJCMCheckBoxChange);

	return true;
}

bool UnitTest_DzBridgeMorphSelectionDialog::_DzBridgeMorphSelectionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzBridgeMorphSelectionDialog());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::PrepareDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->PrepareDialog());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::GetMorphString(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->GetMorphString());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::GetMorphCSVString(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->GetMorphCSVString());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::GetMorphMapping(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->GetMorphMapping());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::IsAutoJCMEnabled(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->IsAutoJCMEnabled());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::GetActiveJointControlledMorphs(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->GetActiveJointControlledMorphs());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::GetMorphLabelFromName(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->GetMorphLabelFromName(""));
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::FilterChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->FilterChanged(""));
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::ItemSelectionChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->ItemSelectionChanged());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleAddMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleAddMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleRemoveMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleRemoveMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleSavePreset(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleSavePreset());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandlePresetChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandlePresetChanged(""));
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleArmJCMMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleArmJCMMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleLegJCMMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleLegJCMMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleTorsoJCMMorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleTorsoJCMMorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleARKitGenesis81MorphsButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleARKitGenesis81MorphsButton());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleFaceFXGenesis8Button(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleFaceFXGenesis8Button());
	return bResult;
}

bool UnitTest_DzBridgeMorphSelectionDialog::HandleAutoJCMCheckBoxChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeMorphSelectionDialog*>(m_testObject)->HandleAutoJCMCheckBoxChange(false));
	return bResult;
}

#include "moc_UnitTest_DzBridgeMorphSelectionDialog.cpp"
#endif