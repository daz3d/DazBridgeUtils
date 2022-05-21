/*****************************************************************
* 
* For Visual Studio: In order to catch memory access violations, 
* you must set following option in Properties -> C/C++ -> Code Generation:
* Enable C++ Exceptions: "Yes With SEH Exceptions"
* 
******************************************************************/

#pragma once

#ifdef UNITTEST_DZBRIDGE

#include <qobject.h>
#include <qregexp.h>

#undef CPP_Export
#define CPP_Export Q_DECL_IMPORT
#ifdef DZ_BRIDGE_SHARED
	#undef CPP_Export
	#define CPP_Export Q_DECL_EXPORT
#elif DZ_BRIDGE_STATIC
	#undef CPP_Export
	#define CPP_Export
#endif

#define DECLARE_TEST(method_name) \
bool method_name(UnitTest::TestResult* testResult);

#define RUNTEST RUNTEST_1ARG

#define RUNTEST_0ARG(method_name) \
UnitTest::TestResult *method_name ## _testResult = createTestResult(#method_name); \
method_name ## _testResult->bResult = method_name();

#define RUNTEST_1ARG(method_name) \
UnitTest::TestResult *method_name ## _testResult = createTestResult(#method_name); \
method_name ## _testResult->bResult = method_name(method_name ## _testResult);

#define LOGTEST_TEXT(text) \
logToTestResult(testResult, QString(text));

#define LOGTEST_FAILED(text) \
LOGTEST_TEXT(testResult->sName + ": failed. " + text);

#define LOGTEST_PASSED(text) \
LOGTEST_TEXT(testResult->sName + ": passed. " + text);

#define TRY_METHODCALL(method_call) \
try { method_call; } \
catch (...) { LOGTEST_TEXT("C++ exception caught."); bResult = false; }

#define TRY_METHODCALL_CUSTOM(method_call, error_string) \
try { method_call; } \
catch (...) { LOGTEST_TEXT(error_string); bResult = false; }

#define TRY_METHODCALL_NULLPTR(method_call) \
try { method_call; } \
catch (...) { LOGTEST_TEXT("C++ exception caught. Failed nullptr C++ exception test."); bResult = false; } 

class QStringList;

class CPP_Export UnitTest : public QObject {
	Q_OBJECT
public:
	Q_INVOKABLE virtual bool runUnitTests()=0;

	UnitTest();

	Q_INVOKABLE bool writeAllTestResults(QString outputPath="");
	Q_INVOKABLE bool convertTestResutlsToXls();
	Q_INVOKABLE bool convertTestResultsToHtml();
	Q_INVOKABLE QObject* getTestObject();
	Q_INVOKABLE QString cleanString(QString argString) { return argString.remove(QRegExp("[^A-Za-z0-9_]")); };

protected:
	struct TestResult {
		int nId;
		QString sName;
		QStringList* aLog;
		bool bResult;
	};

	UnitTest::TestResult* createTestResult(QString methodName);
	bool logToTestResult(UnitTest::TestResult *testResult, QString text);

	QObject* m_testObject;

private:
	QList<TestResult*> m_testResultList;

};

#endif
