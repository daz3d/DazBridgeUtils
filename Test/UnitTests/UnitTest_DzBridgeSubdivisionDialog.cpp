#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzBridgeSubdivisionDialog.h"
#include "DzBridgeSubdivisionDialog.h"

#include "dzbridge.h"
using namespace DzBridgeNameSpace;

UnitTest_DzBridgeSubdivisionDialog::UnitTest_DzBridgeSubdivisionDialog()
{
	m_testObject = (QObject*) new DzBridgeSubdivisionDialog();
}

bool UnitTest_DzBridgeSubdivisionDialog::runUnitTests()
{
	DzBridgeSubdivisionDialog* testObject = new DzBridgeSubdivisionDialog();

	if (!testObject)
	{
		return false;
	}

	RUNTEST(_DzBridgeSubdivisionDialog);
	RUNTEST(getSubdivisionCombos);
	RUNTEST(PrepareDialog);
	RUNTEST(LockSubdivisionProperties);
	RUNTEST(WriteSubdivisions);
	RUNTEST(FindObject);
	RUNTEST(setSubdivisionLevelByNode);
	RUNTEST(UnlockSubdivisionProperties);
	RUNTEST(GetLookupTable);
	RUNTEST(HandleSubdivisionLevelChanged);

	return true;
}

bool UnitTest_DzBridgeSubdivisionDialog::_DzBridgeSubdivisionDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzBridgeSubdivisionDialog());
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::getSubdivisionCombos(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->getSubdivisionCombos());
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::PrepareDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->PrepareDialog());
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::LockSubdivisionProperties(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->LockSubdivisionProperties(false));
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::WriteSubdivisions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzJsonWriter arg(nullptr);
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->WriteSubdivisions(arg));
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::FindObject(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->FindObject(nullptr, ""));
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::setSubdivisionLevelByNode(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL_NULLPTR(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->setSubdivisionLevelByNode(nullptr, 0));
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::UnlockSubdivisionProperties(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->UnlockSubdivisionProperties());
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::GetLookupTable(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->GetLookupTable());
	return bResult;
}

bool UnitTest_DzBridgeSubdivisionDialog::HandleSubdivisionLevelChanged(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzBridgeSubdivisionDialog*>(m_testObject)->HandleSubdivisionLevelChanged(""));
	return bResult;
}


#include "moc_UnitTest_DzBridgeSubdivisionDialog.cpp"
#endif