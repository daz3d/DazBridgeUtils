#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzBridgeSubdivisionDialog : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzBridgeSubdivisionDialog();
	bool runUnitTests();

private:
	bool _DzBridgeSubdivisionDialog(UnitTest::TestResult* testResult);
	bool getSubdivisionCombos(UnitTest::TestResult* testResult);
	bool PrepareDialog(UnitTest::TestResult* testResult);
	bool LockSubdivisionProperties(UnitTest::TestResult* testResult);
	bool WriteSubdivisions(UnitTest::TestResult* testResult);
	bool FindObject(UnitTest::TestResult* testResult);
	bool setSubdivisionLevelByNode(UnitTest::TestResult* testResult);
	bool UnlockSubdivisionProperties(UnitTest::TestResult* testResult);
	bool GetLookupTable(UnitTest::TestResult* testResult);
	bool HandleSubdivisionLevelChanged(UnitTest::TestResult* testResult);

};


#endif