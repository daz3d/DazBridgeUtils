#pragma once
#include <dznode.h>
#include <fbxsdk.h>

class DzProgress;
class QThread;
class DzGeometry;
class DzFacetMesh;
class DzBone;

class JobCalculateMvcWeights;

class MvcTools
{
public:
	// Db 2024-06-18: better support for Daz Classes
	static bool calculate_mean_value_coordinate_weights(const DzFacetMesh* pMesh, DzVec3 x, QVector<double>* pMvcWeights);
	static DzVec3 deform_using_mean_value_coordinates(const DzFacetMesh* pMesh, const DzPnt3* pVertexBuffer, const QVector<double>* pMvcWeights, DzVec3 x = DzVec3(NAN, NAN, NAN));

	static bool calculate_mean_value_coordinate_weights(const FbxMesh* pMesh, FbxVector4 x, QVector<double>* pMvcWeights);
	static FbxVector4 deform_using_mean_value_coordinates(const FbxMesh* pMesh, const FbxVector4* pVertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x = FbxVector4(NAN, NAN, NAN));

	static bool calculate_mean_value_coordinate_weights(const QVector<FbxVector4>& VertexBuffer, const QVector<int>& Triangles, FbxVector4 x, QVector<double>* pMvcWeights);
	static FbxVector4 deform_using_mean_value_coordinates(const QVector<FbxVector4>& VertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x);

	static FbxVector2 interpolate_using_mean_value_coordinates(const QVector<double>* pMvcWeights, const QVector<FbxVector2>* pFValues);

	// modify a geometry node using Mvc Cage deform
	static bool testMvc(DzNode* selected);
	// private use by testMvc()
	static bool makeTestMvcCage(QVector<FbxVector4>* &mvc_test_cage_vertexbuffer, QVector<int>* &mvc_test_cage_triangles);
	static bool makeTestMvcCage_2(QVector<FbxVector4>* &mvc_test_cage_vertexbuffer, QVector<int>* &mvc_test_cage_triangles);

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

template <typename TKEY> class MvcHelper
{
public:
	MvcHelper() {};
	~MvcHelper() { clearWeights(); };

	QMap<TKEY, QVector<double>*> m_MvcWeightsTable;
	QMap<TKEY, JobCalculateMvcWeights*> m_JobQueue;

	virtual void clearWeights() {
		for (TKEY key : m_MvcWeightsTable.keys())
		{
			auto mvc_weights = m_MvcWeightsTable[key];
			mvc_weights->clear();
			delete mvc_weights;
			m_MvcWeightsTable.remove(key);
		}
		m_MvcWeightsTable.clear();
	};

};

class MvcCageRetargeter
{
public:
	QMap<int, QVector<double>*> m_MvcWeightsTable;
	QMap<int, JobCalculateMvcWeights*> m_JobQueue;

	virtual bool createMvcWeights(const FbxMesh* pMesh, const FbxMesh* pCage, DzProgress* pProgress);
//	bool validateMvcWeights(const FbxMesh* pMesh);
	virtual bool deformCage(const FbxMesh* pMorphedMesh, const FbxMesh* pCage, FbxVector4* pVertexBuffer);

};

class MvcFbxBoneRetargeter
{
public:
	MvcFbxBoneRetargeter() {};
	~MvcFbxBoneRetargeter() { clearWeights(); };

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

class MvcDzBoneRetargeter
{
public:
	MvcDzBoneRetargeter() {};
	~MvcDzBoneRetargeter() { clearWeights(); };

	bool createMvcWeightsTable(DzGeometry* pMesh, DzBone* pRootNode, DzProgress* pProgress);
	//bool validateMvcWeights(const FbxMesh* pMesh, FbxNode* pRootBone);

	QMap<QString, QVector<double>*> m_mBoneToMvcWeightsTable;
	QMap<QString, JobCalculateMvcWeights*> m_JobQueue;

	DzVec3 calibrate_bone(const DzGeometry* pMorphedMesh, QString sBoneName);
	//FbxVector4 calibrate_bone(const FbxMesh* pMorphedMesh, const FbxVector4* pVertexBuffer, QString sBoneName);
	//FbxVector4 calibrate_bone(const FbxMesh* pMesh, QString sBoneName);
	//bool loadMvcWeightsCache(QString sMvcWeightsFilename);
	//bool saveMvcWeightsCache(QString sMvcWeightsFilename);
	void clearWeights() {};

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

