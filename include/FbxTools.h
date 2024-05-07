#include <qlist.h>
#include <fbxsdk.h>

#define Y_TO_Z(a) FbxVector4(a[0], a[2], a[1])
#define Y_TO_NEGZ(a) FbxVector4(a[0], a[2], -a[1])
#define NEGY_TO_NEGZ(a) FbxVector4(a[0], -a[2], -a[1])
#define NEGZ(a) FbxVector4(a[0], a[1], -a[2])

class DzProgress;

class FbxTools
{
public:
	static double getLength(double a, double b);

	static double getLength(double a, double b, double c);

	static double getDistance(FbxVector2 a, FbxVector2 b);

	static double getDistance(FbxVector4 a, FbxVector4 b);

	static double determinant_3x3(FbxVector4* matrix);

	static FbxVector4* CalculateBoundingVolume(QList<FbxVector4>& pointCloud);

	static FbxVector4* CalculateBoundingVolume(FbxMesh* pMesh);

	static FbxVector4* CalculateBoundingVolume(FbxMesh* pMesh, QList<int>* pVertexIndexes);

	static void MultiplyMatrix_InPlace(FbxAMatrix& pMatrix, double pValue);

	static void AddToScaleOfMatrix_InPlace(FbxAMatrix& pMatrix, double pValue);

	static void AddMatrix_InPlace(FbxAMatrix& destinationMatrix, const FbxAMatrix& sourceMatrix);

	static FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex);

	static FbxAMatrix GetAffineMatrix(FbxPose* pPose, int nItemIndex, bool bReturnLocalSpace = false, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static FbxAMatrix GetAffineMatrix(FbxPose* pPose, FbxNode* pNode, bool bReturnLocalSpace = false, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static FbxAMatrix GetGeometricAffineMatrix(FbxNode* pNode);

	static bool CalculateClusterDeformationMatrix(FbxAMatrix& clusterDeformationMatrix, FbxCluster* pCluster, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer_LinearPathway(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer_DualQuaternionPathway(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime = FBXSDK_TIME_INFINITE);

	static bool BakePoseToVertexBuffer(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime pTime = FBXSDK_TIME_INFINITE);

	static FbxAMatrix FindPoseMatrixOrIdentity(FbxPose* pPose, FbxNode* pNode);

	static FbxAMatrix FindPoseMatrixOrGlobal(FbxPose* pPose, FbxNode* pNode);

	static void RemoveBindPoses(FbxScene* Scene);

	static FbxPose* SaveBindMatrixToPose(FbxScene* pScene, const char* lpPoseName, FbxNode* Argument_pMeshNode = nullptr, bool bAddPose = false);

	static void ApplyBindPose(FbxScene* pScene, FbxPose* pPose, FbxNode* pNode = nullptr, bool bRecurse = true, bool bClampJoints = false);


	static FbxNode* GetRootBone(FbxScene* pScene, bool bRenameRootBone = false, FbxNode* pPreviousBone = nullptr);

	static void DetachGeometry(FbxScene* pScene);

	static bool BakePoseToBindMatrix(FbxMesh* pMesh, FbxPose* pPose);

	static bool SyncDuplicateBones(FbxScene* lCurrentScene);

	static bool LoadAndPoseBelowHeadOnly(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress = nullptr, bool bConvertToZUp = false);

	static bool LoadAndPose(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress = nullptr, bool bConvertToZUp = false);

	static int ConvertToZUp(FbxMesh* mesh, FbxNode* rootNode);

	static bool FlipAndBakeVertexBuffer(FbxMesh* mesh, FbxNode* rootNode, FbxVector4* vertex_buffer);

	static FbxCluster* FindClusterFromNode(FbxNode* pNode);

	static FbxVector4 CalculatePointCloudAverage(FbxMesh* pMesh, QList<int>* pVertexIndexes);

	static FbxVector4 CalculatePointCloudCenter(FbxMesh* pMesh, QList<int>* pVertexIndexes, bool bCenterWeight = false);

	static void removeMorphExportPrefixFromNode(FbxNode* pNode, const char* prefix);
	static void removeMorphExportPrefixFromBlendShapeChannel(FbxBlendShapeChannel* pChannel, const char* prefix);

	static bool MultiplyMatrixToVertexBuffer(FbxAMatrix* pMatrix, FbxVector4* pVertexBuffer, int numVerts);
};
