#pragma once
#include <dznode.h>
#include <fbxsdk.h>

class DzProgress;
class QThread;

class JobCalculateMvcWeights;

class MvcTools
{
public:
	static bool calculate_mean_value_coordinate_weights(const FbxMesh* pMesh, FbxVector4 x, QVector<double>* pMvcWeights);
	static FbxVector4 deform_using_mean_value_coordinates(const FbxMesh* pMesh, const FbxVector4* pVertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x = FbxVector4(NAN, NAN, NAN));

	static bool calculate_mean_value_coordinate_weights(const QVector<FbxVector4>& VertexBuffer, const QVector<int>& Triangles, FbxVector4 x, QVector<double>* pMvcWeights);
	static FbxVector4 deform_using_mean_value_coordinates(const QVector<FbxVector4>& VertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x);

	static FbxVector2 interpolate_using_mean_value_coordinates(const QVector<double>* pMvcWeights, const QVector<FbxVector2>* pFValues);
};

struct mvcweights_header
{
	//char SIG[5] = "MVCW\0";
	char SIG[5] = { 0, 0, 0, 0, 0 };
	int numElements = 0;
	int numArrays = 0;
	int offsetLookupTable = 0;
	int offsetRawData = 0;
	int fileSize = 0;
};

class MvcHelper
{
public:
	MvcHelper() {};
	~MvcHelper() { clearWeights(); };

	bool createMvcWeightsTable(FbxMesh* pMesh, FbxNode* pRootNode, DzProgress* pProgress);
	bool validateMvcWeights(const FbxMesh* pMesh, FbxNode* pRootBone);

	QMap<QString, QVector<double>*> m_mBoneToMvcWeightsTable;
	QMap<QString, JobCalculateMvcWeights*> m_JobQueue;

	FbxVector4 calibrate_bone(const FbxMesh* pMorphedMesh, const FbxVector4* pVertexBuffer, QString sBoneName);
	FbxVector4 calibrate_bone(const FbxMesh* pMesh, QString sBoneName);
	bool loadMvcWeightsCache(QString sMvcWeightsFilename);
	bool saveMvcWeightsCache(QString sMvcWeightsFilename);
	void clearWeights();

};

class JobCalculateMvcWeights : public QObject
{
	Q_OBJECT
public:
	JobCalculateMvcWeights(QString sJobName, const FbxMesh* pMesh, FbxVector4 vec, FbxVector4* pVertexBuffer, QVector<double>* pWeights)
	{
		m_sJobName = sJobName;
		m_pMesh = pMesh;
		m_x = vec;
		m_pVertexBuffer = pVertexBuffer;
		m_pMvcWeights = pWeights;
	}
	QString m_sJobName;
	const FbxMesh* m_pMesh;
	FbxVector4* m_pVertexBuffer;
	FbxVector4 m_x;
	QVector<double>* m_pMvcWeights;
	void PerformJob();

	static void StaticPerformJob(JobCalculateMvcWeights* job)
	{
		job->PerformJob();
	};

};

//class JobBundle : public QObject
//{
//	Q_OBJECT
//public:
//	void addJob(JobCalculateMvcWeights* job) { m_JobList.append(job); if (m_sBundleName == "") m_sBundleName = job->m_sJobName; };
//	JobCalculateMvcWeights* takeJob() { auto job = m_JobList.first(); m_JobList.pop_front(); return job; };
//	int count() { return m_JobList.count(); };
//	QString getName() { return m_sBundleName; };
//
//	QString m_sBundleName;
//	QList<JobCalculateMvcWeights*> m_JobList;
//	int debug_PerformJob_Called = 0;
//
//	static void StaticPerformJob(JobBundle* job)
//	{
//		job->PerformJob();
//	};
//
//public slots:
//	void PerformJob() 
//	{ 
//		assert(debug_PerformJob_Called==0);
//		debug_PerformJob_Called++;
//		for (auto job : m_JobList) 
//		{ 
//			job->PerformJob(); 
//		} 
//		emit SignalJobDone(getName()); 
//	};
//
//signals:
//	void SignalJobDone(QString);
//};