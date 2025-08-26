
#include "dzprogress.h"
#include "OpenFBXInterface.h"
#include "FbxTools.h"

#include <fbxsdk.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qmap.h>
#include <dzapp.h>
#include <qfile.h>
#include <qtextstream.h>

#include <dzscene.h>
#include <dznode.h>
#include <dzmaterial.h>
#include <dzdefaultmaterial.h>
#include <dzproperty.h>
#include <dzjsonreader.h>
#include <dzjsonwriter.h>

/////////////////////////////////////////////////////////////////////////////////
/// GEOMETRY FUNCTIONS
double FbxTools::getLength(double a, double b, double c)
{
	double distance = 0;
	double a2 = a * a;
	double b2 = b * b;
	double c2 = c * c;
	distance = sqrt(a2 + b2 + c2);
	return distance;
}

double FbxTools::getLength(double a, double b)
{
	double distance = 0;
	double a2 = a * a;
	double b2 = b * b;
	distance = sqrt(a2 + b2);
	return distance;
}

double FbxTools::getDistance(FbxVector4 a, FbxVector4 b)
{
	FbxVector4 ab = b - a;
	double distance = getLength(ab[0], ab[1], ab[2]);
	return distance;
}

double FbxTools::getDistance(FbxVector2 a, FbxVector2 b)
{
	FbxVector2 ab = b - a;
	double distance = getLength(ab[0], ab[1]);
	return distance;
}

double FbxTools::determinant_3x3(FbxVector4* matrix)
{
	double return_value = 0.0;

	double mat1 = matrix[0][0] * matrix[1][1] * matrix[2][2];
	double mat2 = matrix[0][1] * matrix[1][2] * matrix[2][0];
	double mat3 = matrix[0][2] * matrix[1][0] * matrix[2][1];

	double mat4 = matrix[0][2] * matrix[1][1] * matrix[2][0];
	double mat5 = matrix[0][1] * matrix[1][0] * matrix[2][2];
	double mat6 = matrix[0][0] * matrix[1][2] * matrix[2][1];

	return_value = mat1 + mat2 + mat3 - mat4 - mat5 - mat6;

	return return_value;
}

FbxVector4* FbxTools::CalculateBoundingVolume(QList<FbxVector4>& pointCloud)
{

	FbxVector4* result = new FbxVector4[3];

	if (pointCloud.isEmpty())
	{
		return result;
	}

	FbxVector4 cloudCenter;

	FbxVector4 maxBounds = pointCloud[0];
	FbxVector4 minBounds = pointCloud[0];
	FbxVector4 sum(0, 0, 0);
	for (FbxVector4 currentPoint : pointCloud)
	{
		for (int i = 0; i < 3; i++)
		{
			if (maxBounds[i] < currentPoint[i])
			{
				maxBounds[i] = currentPoint[i];
			}
			if (minBounds[i] > currentPoint[i])
			{
				minBounds[i] = currentPoint[i];
			}
		}
		sum += currentPoint;
	}

	FbxVector4 cloudAverage = sum / pointCloud.count();

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}


FbxVector4* FbxTools::CalculateBoundingVolume(FbxMesh* pMesh)
{
	FbxVector4* result = new FbxVector4[3];

	FbxVector4 cloudCenter;
	FbxVector4 minBounds;
	FbxVector4 maxBounds;
	FbxVector4 sum(0, 0, 0);

	int numPoints = pMesh->GetControlPointsCount();
	for (int vertex_index = 0; vertex_index < numPoints; vertex_index++)
	{
		FbxVector4 currentPoint = pMesh->GetControlPointAt(vertex_index);
		sum += currentPoint;
		if (vertex_index == 0)
		{
			minBounds = maxBounds = currentPoint;
			continue;
		}
		for (int axis_index = 0; axis_index < 3; axis_index++)
		{
			if (currentPoint[axis_index] < minBounds[axis_index])
				minBounds[axis_index] = currentPoint[axis_index];
			if (currentPoint[axis_index] > maxBounds[axis_index])
				maxBounds[axis_index] = currentPoint[axis_index];
		}
	}

	FbxVector4 cloudAverage = sum / numPoints;

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}

FbxVector4* FbxTools::CalculateBoundingVolume(FbxMesh* pMesh, QList<int>* pVertexIndexes)
{
	FbxVector4* result = new FbxVector4[3];

	FbxVector4 cloudAverage;
	FbxVector4 cloudCenter;
	FbxVector4 minBounds;
	FbxVector4 maxBounds;

	bool bFirstElement = true;
	double totalWeights = 0.0;
	for (int vertex_index : (*pVertexIndexes))
	{
		FbxVector4 currentPoint = pMesh->GetControlPointAt(vertex_index);
		if (bFirstElement)
		{
			bFirstElement = false;
			minBounds = maxBounds = currentPoint;
			cloudAverage = currentPoint;
			totalWeights = 1.0;
			continue;
		}
		for (int axis_index = 0; axis_index < 3; axis_index++)
		{
			cloudAverage += currentPoint;
			totalWeights += 1.0;
			if (currentPoint[axis_index] < minBounds[axis_index])
				minBounds[axis_index] = currentPoint[axis_index];
			if (currentPoint[axis_index] > maxBounds[axis_index])
				maxBounds[axis_index] = currentPoint[axis_index];
		}
	}

	cloudAverage = cloudAverage / totalWeights;

	cloudCenter[0] = (maxBounds[0] + minBounds[0]) / 2;
	cloudCenter[1] = (maxBounds[1] + minBounds[1]) / 2;
	cloudCenter[2] = (maxBounds[2] + minBounds[2]) / 2;

	FbxVector4 cloudSize;
	cloudSize[0] = abs(maxBounds[0] - minBounds[0]);
	cloudSize[1] = abs(maxBounds[1] - minBounds[1]);
	cloudSize[2] = abs(maxBounds[2] - minBounds[2]);

	result[0] = cloudSize;
	result[1] = cloudCenter;
	result[2] = cloudAverage;

	return result;
}

FbxVector4 FbxTools::CalculatePointCloudAverage(FbxMesh* pMesh, QList<int>* pVertexIndexes)
{

	if (pMesh == nullptr || pVertexIndexes == nullptr || pVertexIndexes->count() <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = FbxVector4(0, 0, 0);
	double totalWeights = 0.0;
	for (int vertex_index : (*pVertexIndexes))
	{
		FbxVector4 currentPoint = pMesh->GetControlPointAt(vertex_index);
		double currentWeight = 1.0;
		cloudCenter[0] += currentPoint[0];
		cloudCenter[1] += currentPoint[1];
		cloudCenter[2] += currentPoint[2];
		totalWeights += currentWeight;
	}
	cloudCenter[0] = cloudCenter[0] / totalWeights;
	cloudCenter[1] = cloudCenter[1] / totalWeights;
	cloudCenter[2] = cloudCenter[2] / totalWeights;

	return cloudCenter;

}

FbxVector4 FbxTools::CalculatePointCloudCenter(FbxMesh* pMesh, QList<int>* pVertexIndexes, bool bCenterWeight)
{

	if (pMesh == nullptr || pVertexIndexes == nullptr || pVertexIndexes->count() <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = pMesh->GetControlPointAt(pVertexIndexes->first());
	FbxVector4 min_bounds = cloudCenter;
	FbxVector4 max_bounds = cloudCenter;
	for (int vertex_index : (*pVertexIndexes))
	{
		FbxVector4 currentPoint = pMesh->GetControlPointAt(vertex_index);
		for (int i = 0; i < 3; i++)
		{
			if (currentPoint[i] < min_bounds[i]) min_bounds[i] = currentPoint[i];
			if (currentPoint[i] > max_bounds[i]) max_bounds[i] = currentPoint[i];
		}
	}
	double center_weight = 0;
	if (abs(max_bounds[0]) < abs(min_bounds[0]))
		center_weight = max_bounds[0];
	else
		center_weight = min_bounds[0];

	if (bCenterWeight)
		//		cloudCenter[0] = (max_bounds[0] + min_bounds[0] + center_weight) / 3;
		cloudCenter[0] = center_weight;
	else
		cloudCenter[0] = (max_bounds[0] + min_bounds[0]) / 2;
	cloudCenter[1] = (max_bounds[1] + min_bounds[1]) / 2;
	cloudCenter[2] = (max_bounds[2] + min_bounds[2]) / 2;

	FbxVector4 cloudAverage = CalculatePointCloudAverage(pMesh, pVertexIndexes);

	return cloudCenter;

}


////////////////////////////////////////////
/// FBX CLUSTER DEFORM FUNCTIONS

// Scale all the elements of a matrix.
void FbxTools::MultiplyMatrix_InPlace(FbxAMatrix& pMatrix, double pValue)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}

// Add a value to all the elements in the diagonal of the matrix.
void FbxTools::AddToScaleOfMatrix_InPlace(FbxAMatrix& pMatrix, double pValue)
{
	for (int i = 0; i < 4; i++)
	{
		pMatrix[i][i] += pValue;
	}
}

// Sum two matrices element by element
void FbxTools::AddMatrix_InPlace(FbxAMatrix& destinationMatrix, const FbxAMatrix& sourceMatrix)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			destinationMatrix[i][j] += sourceMatrix[i][j];
		}
	}
}

// Get the matrix of the given pose
FbxAMatrix FbxTools::GetPoseMatrix(FbxPose* pPose, int pNodeIndex)
{
	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

	return lPoseMatrix;
}

FbxAMatrix FbxTools::GetAffineMatrix(FbxPose* pPose, int nItemIndex, bool bReturnLocalSpace, FbxTime fbxTime)
{
	/////////////////////
	// DEFAULT CASES: Return global or local pose matrix (with matching bReturnLocalSpace)
	////////////////////////
	FbxAMatrix returnMatrix;
	FbxMatrix tempMatrix = pPose->GetMatrix(nItemIndex);
	memcpy(&returnMatrix, &tempMatrix, sizeof(tempMatrix.mData));

	/////////////////////////
	// OTHER CONDITIONS
	/////////////////////////
	if (pPose->IsLocalMatrix(nItemIndex) == true && bReturnLocalSpace == false)
	{
		FbxNode* pParentNode = pPose->GetNode(nItemIndex)->GetParent();
		if (pParentNode)
		{
			FbxAMatrix parentMatrix;
			int nParentIndex = pPose->Find(pParentNode);
			if (nParentIndex > -1)
			{
				parentMatrix = GetAffineMatrix(pPose, nParentIndex, bReturnLocalSpace, fbxTime);
			}
			else
			{
				parentMatrix = pParentNode->EvaluateGlobalTransform(fbxTime);
			}
			FbxAMatrix tempMatrix2 = parentMatrix * returnMatrix;
			returnMatrix = tempMatrix2;
		}
	}
	else if (pPose->IsLocalMatrix(nItemIndex) == false && bReturnLocalSpace == true)
	{
		FbxNode* pParentNode = pPose->GetNode(nItemIndex)->GetParent();
		if (pParentNode)
		{
			FbxAMatrix parentMatrix;
			int nParentIndex = pPose->Find(pParentNode);
			if (nParentIndex > -1)
			{
				parentMatrix = GetAffineMatrix(pPose, nParentIndex, bReturnLocalSpace, fbxTime);
			}
			else
			{
				parentMatrix = pParentNode->EvaluateGlobalTransform(fbxTime);
			}
			FbxAMatrix tempMatrix2 = parentMatrix.Inverse() * returnMatrix;
			returnMatrix = tempMatrix2;
		}
	}
	////////
	// NOTE: DEFAULT CASES ALREADY ASSIGNED ABOVE
	////////

	return returnMatrix;
}

// Return matrix of pNode, using pPose if it is present, using WS matrix by default, using time infinite by default
FbxAMatrix FbxTools::GetAffineMatrix(FbxPose* pPose, FbxNode* pNode, bool bReturnLocalSpace, FbxTime fbxTime)
{
	FbxAMatrix returnMatrix;

	if (pPose != nullptr)
	{
		int nodeIndex = pPose->Find(pNode);
		if (nodeIndex == -1)
		{
			QString sActiveNodeName(pNode->GetName());
			for (int i = 0; i < pPose->GetCount(); i++)
			{
				FbxNode* current_node = pPose->GetNode(i);
				QString sCurrentNodeName(current_node->GetName());
				if (sCurrentNodeName.contains(sActiveNodeName) == true)
				{
					nodeIndex = i;
					break;
				}
			}
		}

		if (nodeIndex > -1)
		{
			returnMatrix = FbxTools::GetAffineMatrix(pPose, nodeIndex, bReturnLocalSpace);
			return returnMatrix;
		}
	}
	if (bReturnLocalSpace == false)
	{
		returnMatrix = pNode->EvaluateGlobalTransform(fbxTime);
	}
	else
	{
		returnMatrix = pNode->EvaluateLocalTransform(fbxTime);
	}
	return returnMatrix;
}

FbxAMatrix FbxTools::GetGeometricAffineMatrix(FbxNode* pNode)
{
	FbxVector4 t = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	FbxVector4 r = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	FbxVector4 s = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	FbxAMatrix returnMatrix(t, r, s);

	return returnMatrix;
}

bool FbxTools::CalculateClusterDeformationMatrix(FbxAMatrix& clusterDeformationMatrix, FbxCluster* pCluster, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime)
{
	bool bResult = false;
	// if cluster link mode is eAdditive
	if (pCluster->GetLinkMode() == FbxCluster::eAdditive)
	{
		FbxAMatrix clusterBindMatrix_x_Geo;
		pCluster->GetTransformMatrix(clusterBindMatrix_x_Geo);
		FbxAMatrix meshGeoMatrix = GetGeometricAffineMatrix(pMesh->GetNode());
		clusterBindMatrix_x_Geo *= meshGeoMatrix;

		// associate matrix
		FbxAMatrix associateModelMatrix;
		pCluster->GetTransformAssociateModelMatrix(associateModelMatrix);

		FbxNode* pAssociateMesh = pCluster->GetAssociateModel();
		FbxAMatrix associateGeoMatrix = GetGeometricAffineMatrix(pAssociateMesh);
		FbxAMatrix associateModelPosedMatrix = GetAffineMatrix(pPose, pCluster->GetAssociateModel(), false, fbxTime);

		FbxAMatrix clusterPosedMatrix = GetAffineMatrix(pPose, pCluster->GetLink(), false, fbxTime);

		FbxAMatrix clusterLinkBindMatrix_x_Geo;
		pCluster->GetTransformLinkMatrix(clusterLinkBindMatrix_x_Geo);
		FbxAMatrix clusterLinkGeoMatrix = GetGeometricAffineMatrix(pCluster->GetLink());
		clusterLinkBindMatrix_x_Geo *= clusterLinkGeoMatrix;

		/////// Compute the shift of the link relative to the reference.
		// reference_inverse * associate * associate_geo_inverse * link_geo * link_geo_inverse * reference
		clusterDeformationMatrix = clusterBindMatrix_x_Geo.Inverse() * associateModelMatrix * associateModelPosedMatrix.Inverse() *
			clusterPosedMatrix * clusterLinkBindMatrix_x_Geo.Inverse() * clusterBindMatrix_x_Geo;
		bResult = true;
	}
	else
	{
		FbxAMatrix clusterPosedMatrix = GetAffineMatrix(pPose, pCluster->GetLink(), false, fbxTime);

		FbxAMatrix clusterLinkBindMatrix_x_Geo;
		pCluster->GetTransformLinkMatrix(clusterLinkBindMatrix_x_Geo);

		FbxAMatrix clusterBindMatrix_x_Geo;
		pCluster->GetTransformMatrix(clusterBindMatrix_x_Geo);
		FbxAMatrix meshGeoMatrix = GetGeometricAffineMatrix(pMesh->GetNode());
		clusterBindMatrix_x_Geo *= meshGeoMatrix;

		// relative_current_inverse * relative_initial
		clusterDeformationMatrix = pGlobalOffsetMatrix->Inverse() * clusterPosedMatrix *
			clusterLinkBindMatrix_x_Geo.Inverse() * clusterBindMatrix_x_Geo;
		bResult = true;
	}

	return bResult;
}

bool FbxTools::BakePoseToVertexBuffer_LinearPathway(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime)
{
	bool bResult = false;
	// get cluster link mode
	FbxSkin* pSkinDeformer = (FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin);
	FbxCluster* pCluster = pSkinDeformer->GetCluster(0);
	// 2025-05-16, DB: Bugfix for converted rig
	if (pCluster == NULL) {
		dzApp->log("WARNING: FbxTools: BakePoseToVertexBuffer_LinearPathway(): NULL skindeformer cluster encountered, skipping...");
		return false;
	}
	FbxCluster::ELinkMode clusterMode = pCluster->GetLinkMode();

	int numVerts = pMesh->GetControlPointsCount();
	// prepare cluster matrix buffer (one matrix per vertex)
	FbxAMatrix* pMatrixBuffer = new FbxAMatrix[numVerts];
	memset(pMatrixBuffer, 0, numVerts * sizeof(FbxAMatrix));
	// prepare cluster weight buffer (one weight per vertex)
	double* pWeightBuffer = new double[numVerts];
	memset(pWeightBuffer, 0, numVerts * sizeof(double));
	// if addtive cluster mode, set each matrix in matrix buffer to identity
	if (clusterMode == FbxCluster::eAdditive)
	{
		for (int matrixIndex = 0; matrixIndex < numVerts; matrixIndex++)
		{
			pMatrixBuffer[matrixIndex].SetIdentity();
		}
	}

	// for each cluster in each skindeformer of mesh, calc matrix transform and weights per vertex
	int numSkinDeformers = pMesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)pMesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			if (pCurrentSkinDeformer->GetCluster(clusterIndex)->GetLink() == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);

			FbxAMatrix clusterTransformMatrix;
			if (CalculateClusterDeformationMatrix(clusterTransformMatrix, pCurrentCluster, pGlobalOffsetMatrix, pPose, pMesh, fbxTime) == false)
			{
				//printf("ERROR: unable to calculate cluster deformation matrix, skipping cluster[%i]", clusterIndex);
				continue;
			}
			// each cluster has a list of indexes into the global vertex index buffer
			// localIndex == offset into each cluster's buffer of vertex indexes
			// globalIndex == offset into the global vertex buffer
			int numLocalIndexes = pCurrentCluster->GetControlPointIndicesCount();
			for (int localIndex = 0; localIndex < numLocalIndexes; localIndex++)
			{
				int globalIndex = pCurrentCluster->GetControlPointIndices()[localIndex];
				if (globalIndex >= numVerts)
				{
					//printf("ERROR: global vertex index is out of range of global vertex buffer: globalIndex=[%i]", globalIndex);
					continue;
				}
				double fWeightOfVertex = pCurrentCluster->GetControlPointWeights()[localIndex];
				FbxAMatrix weightedTransformMatrix = clusterTransformMatrix;
				MultiplyMatrix_InPlace(weightedTransformMatrix, fWeightOfVertex);
				if (clusterMode == FbxCluster::eAdditive)
				{
					AddToScaleOfMatrix_InPlace(weightedTransformMatrix, 1 - fWeightOfVertex);
					pMatrixBuffer[globalIndex] = weightedTransformMatrix * pMatrixBuffer[globalIndex];
					pWeightBuffer[globalIndex] = 1.0;
				}
				else
				{
					AddMatrix_InPlace(pMatrixBuffer[globalIndex], weightedTransformMatrix);
					pWeightBuffer[globalIndex] += fWeightOfVertex;
				}
			}

		}
	}

	// apply weight * matrix transform to each vertex
	for (int globalIndex = 0; globalIndex < numVerts; globalIndex++)
	{
		FbxVector4 sourceVertex = pVertexBuffer[globalIndex];
		FbxVector4 finalTargetVertex;
		double fVertexWeight = pWeightBuffer[globalIndex];
		if (fVertexWeight != 0.0)
		{
			FbxVector4 intermediateVertexValue = pMatrixBuffer[globalIndex].MultT(sourceVertex);
			if (clusterMode == FbxCluster::eNormalize)
			{
				finalTargetVertex = intermediateVertexValue / fVertexWeight;
			}
			else if (clusterMode == FbxCluster::eTotalOne)
			{
				finalTargetVertex = intermediateVertexValue + sourceVertex * (1 - fVertexWeight);
			}
			else
			{
				finalTargetVertex = intermediateVertexValue;
			}
			pVertexBuffer[globalIndex] = finalTargetVertex;
		}
	}
	bResult = true;

	// cleanup buffers
	delete[] pMatrixBuffer;
	delete[] pWeightBuffer;

	return bResult;
}

bool FbxTools::BakePoseToVertexBuffer_DualQuaternionPathway(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime fbxTime)
{
	bool bResult = false;
	// get cluster link mode
	FbxSkin* pSkinDeformer = (FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin);
	if (pSkinDeformer->GetClusterCount() < 1) {
		dzApp->log("WARNING: FbxTools::BakePoseToVertexBuffer_DualQuaternionPathway(): SkinDeformer node has zero clusters");
		return false;
	}
	FbxCluster* pCluster = pSkinDeformer->GetCluster(0);
	FbxCluster::ELinkMode clusterMode = pCluster->GetLinkMode();

	int numVerts = pMesh->GetControlPointsCount();
	// prepare dual-quaternion buffer (one DQ per vertex)
	FbxDualQuaternion* pDualQuaternionBuffer = new FbxDualQuaternion[numVerts];
	memset(pDualQuaternionBuffer, 0, numVerts * sizeof(FbxDualQuaternion));
	// prepare cluster weight buffer (one weight per vertex)
	double* pWeightBuffer = new double[numVerts];
	memset(pWeightBuffer, 0, numVerts * sizeof(double));

	// for each cluster of each skindeformer of mesh
	int numSkinDeformers = pMesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)pMesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			if (pCurrentSkinDeformer->GetCluster(clusterIndex)->GetLink() == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);

			FbxAMatrix clusterTransformMatrix;
			if (CalculateClusterDeformationMatrix(clusterTransformMatrix, pCurrentCluster, pGlobalOffsetMatrix, pPose, pMesh, fbxTime) == false)
			{
				//printf("ERROR: unable to calculate cluster deformation matrix, skipping cluster[%i]", clusterIndex);
				continue;
			}
			// compute DQ deformation and weight for each vertex
			FbxQuaternion componentQuaternion = clusterTransformMatrix.GetQ();
			FbxVector4 componentTranslation = clusterTransformMatrix.GetT();
			FbxDualQuaternion clusterDualQuaternion(componentQuaternion, componentTranslation);

			// each cluster has a list of indexes into the global vertex index buffer
			// localIndex == offset into each cluster's buffer of vertex indexes
			// globalIndex == offset into the global vertex buffer
			int numLocalIndexes = pCurrentCluster->GetControlPointIndicesCount();
			for (int localIndex = 0; localIndex < numLocalIndexes; localIndex++)
			{
				int globalIndex = pCurrentCluster->GetControlPointIndices()[localIndex];
				if (globalIndex >= numVerts)
				{
					//printf("ERROR: global vertex index is out of range of global vertex buffer: globalIndex=[%i]", globalIndex);
					continue;
				}
				double fWeightOfVertex = pCurrentCluster->GetControlPointWeights()[localIndex];
				if (fWeightOfVertex != 0.0)
				{
					FbxDualQuaternion weightedDualQuaternion = clusterDualQuaternion * fWeightOfVertex;
					if (clusterMode == FbxCluster::eAdditive)
					{
						pDualQuaternionBuffer[globalIndex] = weightedDualQuaternion;
						pWeightBuffer[globalIndex] = 1.0;
					}
					else
					{
						pWeightBuffer[globalIndex] += fWeightOfVertex;
						if (clusterIndex == 0)
						{
							pDualQuaternionBuffer[globalIndex] = weightedDualQuaternion;
						}
						else
						{
							FbxQuaternion quaternionA = pDualQuaternionBuffer[globalIndex].GetFirstQuaternion();
							FbxQuaternion quaternionB = weightedDualQuaternion.GetFirstQuaternion();
							double fSign = quaternionA.DotProduct(quaternionB);
							if (fSign >= 0.0)
							{
								pDualQuaternionBuffer[globalIndex] += weightedDualQuaternion;
							}
							else
							{
								pDualQuaternionBuffer[globalIndex] -= weightedDualQuaternion;
							}
						}
					}

				}

			}


		}
	}

	// apply weighted DQ deformation, based on cluster link mode
	for (int globalIndex = 0; globalIndex < numVerts; globalIndex++)
	{
		FbxVector4 sourceVertex = pVertexBuffer[globalIndex];
		FbxVector4 finalTargetVertex = sourceVertex;
		double fVertexWeight = pWeightBuffer[globalIndex];
		if (fVertexWeight != 0.0)
		{
			pDualQuaternionBuffer[globalIndex].Normalize();
			FbxVector4 intermediateVertexValue = pDualQuaternionBuffer[globalIndex].Deform(finalTargetVertex);
			if (clusterMode == FbxCluster::eNormalize)
			{
				finalTargetVertex = intermediateVertexValue / fVertexWeight;
			}
			else if (clusterMode == FbxCluster::eTotalOne)
			{
				finalTargetVertex = intermediateVertexValue + sourceVertex * (1.0 - fVertexWeight);
			}
			else
			{
				finalTargetVertex = intermediateVertexValue;
			}

			pVertexBuffer[globalIndex] = finalTargetVertex;
		}
	}
	bResult = true;

	// cleanup buffers
	delete[] pDualQuaternionBuffer;
	delete[] pWeightBuffer;

	return bResult;
}

bool FbxTools::BakePoseToVertexBuffer(FbxVector4* pVertexBuffer, FbxAMatrix* pGlobalOffsetMatrix, FbxPose* pPose, const FbxMesh* pMesh, FbxTime pTime)
{
	bool bResult = false;
	// get skin deformer for mesh
	FbxSkin* pSkinDeformer = (FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin);
	if (!pSkinDeformer)
	{
		// do unskinned bake
		bResult = MultiplyMatrixToVertexBuffer(pGlobalOffsetMatrix, pVertexBuffer, pMesh->GetControlPointsCount());
		return bResult;
	}
	FbxSkin::EType skinningType = pSkinDeformer->GetSkinningType();

	// choose linear, dual-quaternion or blend pathways
	switch (skinningType)
	{
	case FbxSkin::eLinear:
	case FbxSkin::eRigid:
		bResult = BakePoseToVertexBuffer_LinearPathway(pVertexBuffer, pGlobalOffsetMatrix, pPose, pMesh, pTime);
		break;
	case FbxSkin::eDualQuaternion:
		bResult = BakePoseToVertexBuffer_DualQuaternionPathway(pVertexBuffer, pGlobalOffsetMatrix, pPose, pMesh, pTime);
		break;
	case FbxSkin::eBlend:
		// create temp vertex buffers to compute linear & quaternion pathways
		// linear
		int numVerts = pMesh->GetControlPointsCount();
		FbxVector4* pVertexBuffer_Linear = new FbxVector4[numVerts];
		memcpy(pVertexBuffer_Linear, pMesh->GetControlPoints(), numVerts * sizeof(FbxVector4));
		BakePoseToVertexBuffer_LinearPathway(pVertexBuffer_Linear, pGlobalOffsetMatrix, pPose, pMesh, pTime);
		// dual-quaternion
		FbxVector4* pVertexBuffer_DQ = new FbxVector4[numVerts];
		memcpy(pVertexBuffer_DQ, pMesh->GetControlPoints(), numVerts * sizeof(FbxVector4));
		BakePoseToVertexBuffer_DualQuaternionPathway(pVertexBuffer_DQ, pGlobalOffsetMatrix, pPose, pMesh, pTime);
		// linear-interpolate between the two buffer results
		int numBlendWeights = pSkinDeformer->GetControlPointIndicesCount();
		double* pBlendWeightBuffer = pSkinDeformer->GetControlPointBlendWeights();
		for (int nVertexIndex = 0; nVertexIndex < numBlendWeights; nVertexIndex++)
		{
			double fBlendWeight = pBlendWeightBuffer[nVertexIndex];
			FbxVector4 linearResult = pVertexBuffer_Linear[nVertexIndex];
			FbxVector4 dqResult = pVertexBuffer_DQ[nVertexIndex];
			pVertexBuffer[nVertexIndex] = (linearResult * fBlendWeight) + (dqResult * (1 - fBlendWeight));
		}
		// cleanup buffers
		delete[] pVertexBuffer_Linear;
		delete[] pVertexBuffer_DQ;
		bResult = true;
		break;
	}

	return bResult;
}


////////////////////////////////////////////
/// FBX POSE FUNCTIONS
FbxAMatrix FbxTools::FindPoseMatrixOrIdentity(FbxPose* pPose, FbxNode* pNode)
{
	FbxAMatrix returnMatrix;

	int nodeIndex = pPose->Find(pNode);
	if (nodeIndex > -1)
	{
		returnMatrix = FbxTools::GetAffineMatrix(pPose, nodeIndex);
	}
	else
	{
		returnMatrix.SetIdentity();
	}

	return returnMatrix;
}

FbxAMatrix FbxTools::FindPoseMatrixOrGlobal(FbxPose* pPose, FbxNode* pNode)
{
	FbxAMatrix returnMatrix;

	int nodeIndex = -1;

	if (pPose) 
	{
		nodeIndex = pPose->Find(pNode);
	}
	if (nodeIndex > -1)
	{
		returnMatrix = FbxTools::GetAffineMatrix(pPose, nodeIndex);
	}
	else
	{
		returnMatrix = pNode->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE);
	}

	return returnMatrix;
}

void FbxTools::RemoveBindPoses(FbxScene* Scene)
{
	QList<int> poseIndexesToDelete;
	int numPoses = Scene->GetPoseCount();
	for (int PoseIndex = numPoses - 1; PoseIndex >= 0; --PoseIndex)
	{
		FbxPose* pPose = Scene->GetPose(PoseIndex);
		if (pPose->IsBindPose())
		{
			//			ApplyPose(Scene, pPose);
			for (int nGeoIndex = 0; nGeoIndex < Scene->GetGeometryCount(); nGeoIndex++)
			{
				FbxMesh* mesh = (FbxMesh*)Scene->GetGeometry(0);
				FbxVector4* vertex_buffer = mesh->GetControlPoints();
				//				ComputeSkinDeformation(GetGlobalPosition(mesh->GetNode(), FbxTime(0), pPose), mesh, FbxTime(0), vertex_buffer, NULL);
			}
			poseIndexesToDelete.append(PoseIndex);

		}
	}

	for (int i : poseIndexesToDelete)
	{
		Scene->RemovePose(i);
	}

}

FbxPose* FbxTools::SaveBindMatrixToPose(FbxScene* pScene, const char* lpPoseName, FbxNode* Argument_pMeshNode, bool bAddPose)
{
	FbxPose* pNewBindPose = FbxPose::Create(pScene->GetFbxManager(), lpPoseName);

	QList<FbxNode*> todoList;
	FbxNode* pRootNode = pScene->GetRootNode();
	if (Argument_pMeshNode != nullptr)
	{
		pRootNode = Argument_pMeshNode;
	}
	todoList.append(pRootNode);

	while (todoList.isEmpty() == false)
	{
		FbxNode* pCurrentMeshNode = todoList.front();
		todoList.pop_front();
		const char* lpCurrentMeshNodeName = pCurrentMeshNode->GetName();
		FbxGeometry* pGeometry = static_cast<FbxGeometry*>(pCurrentMeshNode->GetMesh());
		if (pGeometry)
		{
			for (int nDeformerIndex = 0; nDeformerIndex < pGeometry->GetDeformerCount(); ++nDeformerIndex)
			{
				FbxSkin* pSkin = static_cast<FbxSkin*>(pGeometry->GetDeformer(nDeformerIndex));
				if (pSkin)
				{
					for (int nClusterIndex = 0; nClusterIndex < pSkin->GetClusterCount(); ++nClusterIndex)
					{
						FbxCluster* pCluster = pSkin->GetCluster(nClusterIndex);
						FbxNode* pClusterBone = pCluster->GetLink();
						// crash protection
						if (pClusterBone == nullptr) continue;
						const char* pBoneName = pClusterBone->GetName();
						FbxAMatrix bindMatrix;
						pCluster->GetTransformLinkMatrix(bindMatrix);
						pNewBindPose->Add(pClusterBone, bindMatrix, false);

						if (QString(pBoneName).contains("lShldrBend", Qt::CaseInsensitive))
						{
							FbxVector4 rotation = bindMatrix.GetR();
							//printf("nop");
						}
						if (QString(pBoneName).contains("lForearmBend", Qt::CaseInsensitive))
						{
							FbxVector4 rotation = bindMatrix.GetR();
							//printf("nop");
						}

					}
				}
			}
		}
		for (int nChildIndex = 0; nChildIndex < pCurrentMeshNode->GetChildCount(); ++nChildIndex)
		{
			FbxNode* pChildBone = pCurrentMeshNode->GetChild(nChildIndex);
			todoList.push_back(pChildBone);
		}
	}
	if (bAddPose)
	{
		pScene->AddPose(pNewBindPose);
	}
	return pNewBindPose;
}

void FbxTools::ApplyBindPose(FbxScene* pScene, FbxPose* pPose, FbxNode* pNode, bool bRecurse, bool bClampJoints)
{
	// loop and perform for each node starting with root node
	if (pNode == nullptr)
	{
		pNode = pScene->GetRootNode();
	}
	const char* lpNodeName = pNode->GetName();

	FbxAMatrix poseMatrix;
	FbxAMatrix parentMatrix;
	FbxAMatrix localMatrix;
	// find node in main scene
	FbxNode* pParentNode = pNode->GetParent();
	if (pParentNode == NULL)
	{
		localMatrix = FindPoseMatrixOrGlobal(pPose, pNode);
	}
	else
	{
		const char* lpParentNodeName = pParentNode->GetName();
		parentMatrix = FindPoseMatrixOrGlobal(pPose, pParentNode);

		poseMatrix = FindPoseMatrixOrGlobal(pPose, pNode);

		localMatrix = parentMatrix.Inverse() * poseMatrix;
	}

	if (bClampJoints)
	{
		//		ClampTransform(pNode, &localMatrix);
	}

	//// rotation order
	FbxVector4 correctRotation = localMatrix.GetR();
	FbxRotationOrder rotationOrderFixer(pNode->RotationOrder.Get());
	rotationOrderFixer.M2V(correctRotation, localMatrix);

	pNode->SetPreRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	pNode->SetPostRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	pNode->SetRotationOffset(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));

	pNode->LclTranslation.Set(localMatrix.GetT());
	pNode->LclRotation.Set(correctRotation);
	pNode->LclScaling.Set(localMatrix.GetS());


	if (QString(lpNodeName).contains("lForearmBend", Qt::CaseInsensitive))
	{
		FbxVector4 localRot = localMatrix.GetR();
		FbxVector4 poseRot = poseMatrix.GetR();
		FbxVector4 parentRot = parentMatrix.GetR();
		const char* lpParentName = pParentNode->GetName();

		//printf("nop");
	}

	if (bRecurse == false)
		return;

	// apply to all children
	for (int childIndex = 0; childIndex < pNode->GetChildCount(); childIndex++)
	{
		FbxNode* pChildNode = pNode->GetChild(childIndex);
		ApplyBindPose(pScene, pPose, pChildNode, bRecurse, bClampJoints);
	}

}


bool FbxTools::BakePoseToBindMatrix(FbxMesh* pMesh, FbxPose* pPose)
{
	// for each cluster,
	// get link node
	// look up link node in pose
	// apply pose matrix to bindmatrix with SetTransformLinkMatrix

	int numSkinDeformers = pMesh->GetDeformerCount(FbxSkin::eSkin);
	for (int skinIndex = 0; skinIndex < numSkinDeformers; skinIndex++)
	{
		FbxSkin* pCurrentSkinDeformer = (FbxSkin*)pMesh->GetDeformer(skinIndex, FbxSkin::eSkin);
		int numClusters = pCurrentSkinDeformer->GetClusterCount();
		for (int clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)
		{
			FbxCluster* pCurrentCluster = pCurrentSkinDeformer->GetCluster(clusterIndex);
			FbxNode* clusterBone = pCurrentCluster->GetLink();
			if (clusterBone == nullptr)
			{
				//printf("DEBUG: cluster is not linked to any bone, skipping cluster[%i]", clusterIndex);
				continue;
			}

			bool bNoPoseBone = false;
			if (pPose != nullptr)
			{
				const char* lpBoneName = clusterBone->GetName();
				QString sSearchName(lpBoneName);
				int poseNodeIndex = -1;
				for (int i = 0; i < pPose->GetCount(); i++)
				{
					FbxNode* current_node = pPose->GetNode(i);
					QString sCurrentNodeName(current_node->GetName());
					if (sCurrentNodeName == sSearchName)
					{
						poseNodeIndex = i;
						break;
					}
				}
				if (poseNodeIndex != -1)
				{
					assert(pPose->IsLocalMatrix(poseNodeIndex) == false);
					FbxAMatrix poseMatrix = GetPoseMatrix(pPose, poseNodeIndex);
					pCurrentCluster->SetTransformLinkMatrix(poseMatrix);
				}
				else
				{
					dzApp->log(QString("ERROR: BakePoseToBindMatrix() could not find cluster bone: %1 in pose[%2]").arg(sSearchName).arg(pPose->GetName()));
					bNoPoseBone = true;
				}
			}
			
			if (pPose == nullptr || bNoPoseBone == true)
			{
				FbxAMatrix poseMatrix = FbxTools::GetAffineMatrix(nullptr, clusterBone);
				pCurrentCluster->SetTransformLinkMatrix(poseMatrix);
			}

		}
	}

	return true;
}



/////////////////////////////////////////////////////////////////////////////////
/// FBX SCENE FUNCTIONS
FbxNode* FbxTools::GetRootBone(FbxScene* pScene, bool bRenameRootBone, FbxNode* pPreviousBone)
{
	QList<FbxNode*> todoList;

	// Find the root bone.  There should only be one bone off the scene root
	FbxNode* pRootNode = pScene->GetRootNode();
	FbxNode* pRootBone = nullptr;
	int rootChildCount = pRootNode->GetChildCount();
	int rootBoneCount = 0;
	bool bFoundPrevious = false;
	for (int nChildIndex = 0; nChildIndex < rootChildCount; ++nChildIndex)
	{
		FbxNode* pChildNode = pRootNode->GetChild(nChildIndex);
		if (pPreviousBone != nullptr && bFoundPrevious == false)
		{
			if (pChildNode == pPreviousBone)
				bFoundPrevious = true;
			continue;
		}
		FbxNodeAttribute* pAttr = pChildNode->GetNodeAttribute();
		if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			rootBoneCount++;
			pRootBone = pChildNode;
			const char* lpRootBoneName = pRootBone->GetName();
			if (bRenameRootBone)
			{
				pRootBone->SetName("root");
				pAttr->SetName("root");
			}
			break;
		}
		todoList.append(pChildNode);
	}

	// if first layer failed, search each successive layer
	if (pRootBone == nullptr)
	{
		while (todoList.isEmpty() == false)
		{
			if (pRootBone)
			{
				break;
			}
			FbxNode* pNode = todoList.front();
			todoList.pop_front();
			int nChildCount = pNode->GetChildCount();
			for (int nChildIndex = 0; nChildIndex < nChildCount; nChildIndex++)
			{
				FbxNode* pChildNode = pNode->GetChild(nChildIndex);
				if (pPreviousBone != nullptr && bFoundPrevious == false)
				{
					if (pChildNode == pPreviousBone)
						bFoundPrevious = true;
					continue;
				}
				FbxNodeAttribute* pAttr = pChildNode->GetNodeAttribute();
				if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					rootBoneCount++;
					pRootBone = pChildNode;
					const char* lpRootBoneName = pRootBone->GetName();
					if (bRenameRootBone)
					{
						pRootBone->SetName("root");
						pAttr->SetName("root");
					}
					break;
				}
				todoList.append(pChildNode);
			}
		}
	}

	return pRootBone;
}

void FbxTools::DetachGeometry(FbxScene* pScene, FbxNode* pRootNode)
{
	if (pRootNode == nullptr) pRootNode = pScene->GetRootNode();

	// Detach geometry from the skeleton
	for (int NodeIndex = 0; NodeIndex < pScene->GetNodeCount(); ++NodeIndex)
	{
		FbxNode* SceneNode = pScene->GetNode(NodeIndex);
		if (SceneNode == nullptr)
		{
			continue;
		}
		FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(SceneNode->GetMesh());
		if (NodeGeometry)
		{
			if (SceneNode->GetParent() &&
				SceneNode->GetParent()->GetNodeAttribute() &&
				SceneNode->GetParent()->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				SceneNode->GetParent()->RemoveChild(SceneNode);
				pRootNode->AddChild(SceneNode);
			}
		}
	}
}

bool FbxTools::SyncDuplicateBones(FbxScene* lCurrentScene)
{
	// for each bone with .001, sync with original bone
	for (int i = 0; i < lCurrentScene->GetNodeCount(); i++)
	{
		FbxNode* pBone = lCurrentScene->GetNode(i);
		FbxNodeAttribute* Attr = pBone->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpBoneName = pBone->GetName();
			QString sBoneName(lpBoneName);
			if (sBoneName.contains(".001"))
			{
				QString sOrigBoneName = QString(sBoneName).replace(".001", "");
				FbxNode* pOrigBone = lCurrentScene->FindNodeByName(sOrigBoneName.toUtf8().constData());
				if (pOrigBone)
				{
					//pBone->Copy(*pOrigBone);
					pBone->LclRotation.Set(pOrigBone->LclRotation.Get());
					pBone->LclScaling.Set(pOrigBone->LclScaling.Get());
					pBone->LclTranslation.Set(pOrigBone->LclTranslation.Get());
					pBone->PreRotation.Set(pOrigBone->PreRotation.Get());
					pBone->PostRotation.Set(pOrigBone->PostRotation.Get());
					
				}
				else
				{
					dzApp->log(QString("ERROR: SyncDuplicateBones(): OrigBone not found for: %1").arg(sBoneName));
				}
			}
		}
	}
	

	return true;
}

bool FbxTools::LoadAndPoseBelowHeadOnly(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress, bool bConvertToZUp)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();

	FbxScene* pPoseScene = openFBX->CreateScene("My Scene");
	if (openFBX->LoadScene(pPoseScene, poseFilePath.toUtf8().data()) == false)
	{
		return false;
	}
	if (pProgress) pProgress->step();

	// make nodename lookup table
	QMap<QString, FbxNode*> lookupTable;
	int numPoseNodes = pPoseScene->GetNodeCount();
	for (int i = 0; i < numPoseNodes; i++)
	{
		FbxNode* pNode = pPoseScene->GetNode(i);
		const char* lpNodeName = pNode->GetName();
		QString sNodeName(lpNodeName);
		lookupTable.insert(sNodeName, pNode);
	}
	if (pProgress) pProgress->step();

	// Convert Pose Scene to Zup
	if (bConvertToZUp)
	{
		FbxMesh* mesh = (FbxMesh*)pPoseScene->GetGeometry(0);
		if (mesh)
		{
			ConvertToZUp(mesh, lookupTable["root"]);
		}
	}

	int numMainNodes = lCurrentScene->GetNodeCount();
	for (int i = 0; i < numMainNodes; i++)
	{
		FbxNode* pNode = lCurrentScene->GetNode(i);
		FbxNodeAttribute* Attr = pNode->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpNodeName = pNode->GetName();
			QString sNodeName(lpNodeName);
			if (sNodeName.contains("head"))
			{
				break;
			}
			if (lookupTable.find(sNodeName) != lookupTable.end())
			{
				FbxNode* pPoseNode = lookupTable[sNodeName];
				//pNode->Copy(*pPoseNode);
				pNode->LclRotation.Set(pPoseNode->LclRotation.Get());
				pNode->LclScaling.Set(pPoseNode->LclScaling.Get());
				pNode->LclTranslation.Set(pPoseNode->LclTranslation.Get());
				pNode->PreRotation.Set(pPoseNode->PreRotation.Get());
				pNode->PostRotation.Set(pPoseNode->PostRotation.Get());
			}
		}
	}
	if (pProgress) pProgress->step();

	// close pose scene
	pPoseScene->Destroy();

	return true;
}

bool FbxTools::LoadAndPose(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress, bool bConvertToZUp, bool bRotationOnly, QList<QString> aSkipBoneNames, FbxPose *pNewPose)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();

	FbxScene* pPoseScene = openFBX->CreateScene("My Scene");
	if (openFBX->LoadScene(pPoseScene, poseFilePath.toUtf8().data()) == false)
	{
		return false;
	}
	if (pProgress) pProgress->step();

	// make nodename lookup table
	QMap<QString, FbxNode*> lookupTable;
	int numPoseNodes = pPoseScene->GetNodeCount();
	for (int i = 0; i < numPoseNodes; i++)
	{
		FbxNode* pNode = pPoseScene->GetNode(i);
		const char* lpNodeName = pNode->GetName();
		QString sNodeName(lpNodeName);
		lookupTable.insert(sNodeName, pNode);
	}
	if (pProgress) pProgress->step();

	// Convert Pose Scene to Zup
	if (bConvertToZUp)
	{
		FbxMesh* mesh = (FbxMesh*) pPoseScene->GetGeometry(0);
		if (mesh)
		{
			ConvertToZUp(mesh, lookupTable["root"]);
		}
	}

	int numMainNodes = lCurrentScene->GetNodeCount();
	for (int i = 0; i < numMainNodes; i++)
	{
		FbxNode* pNode = lCurrentScene->GetNode(i);
		FbxNodeAttribute* Attr = pNode->GetNodeAttribute();
		if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			const char* lpNodeName = pNode->GetName();
			QString sNodeName(lpNodeName);
//			if (sNodeName == "RootNode")
//				continue;
			if (aSkipBoneNames.count() > 0)
			{
				bool bSkipBoneFound = false;
				foreach(QString sSkipBoneName, aSkipBoneNames) {
					if (sNodeName.compare(sSkipBoneName, Qt::CaseInsensitive) == 0) {
						bSkipBoneFound = true;
						break;
					}
				}
				if (bSkipBoneFound) {
					dzApp->log("DEBUG: LoadAndPose(): Skipping Bone=" + sNodeName);
					continue;
				}
			}
			if (lookupTable.find(sNodeName) != lookupTable.end())
			{
				FbxNode* pPoseNode = lookupTable[sNodeName];
                if (bRotationOnly)
                {
					FbxEuler::EOrder oPoseRotationOrder = pPoseNode->RotationOrder.Get();
					pNode->SetRotationOrder(FbxNode::eSourcePivot, oPoseRotationOrder);
					pNode->PreRotation.Set(pPoseNode->PreRotation.Get());
					pNode->LclRotation.Set(pPoseNode->LclRotation.Get());
					pNode->PostRotation.Set(pPoseNode->PostRotation.Get());
				}
                else
                {
                    pNode->Copy(*pPoseNode);
                }
				if (pNewPose) {
					pNewPose->Add(pNode, pPoseNode->EvaluateGlobalTransform());
				}
			}
		}
	}
	if (pProgress) pProgress->step();

	// close pose scene
	pPoseScene->Destroy();

	return true;
}


//bool LoadAndPose(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress )
//{
//	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
//
//	FbxScene* pPoseScene = openFBX->CreateScene("My Scene");
//	if (openFBX->LoadScene(pPoseScene, poseFilePath.toUtf8().data()) == false)
//	{
//		return false;
//	}
//	if (pProgress) pProgress->step();
//
//	// make nodename lookup table
//	QMap<QString, FbxNode*> lookupTable;
//	int numMainNodes = lCurrentScene->GetNodeCount();
//	for (int i = 0; i < numMainNodes; i++)
//	{
//		FbxNode* pNode = lCurrentScene->GetNode(i);
//		const char* lpNodeName = pNode->GetName();
//		QString sNodeName(lpNodeName);
//		lookupTable.insert(sNodeName, pNode);
//	}
//	if (pProgress) pProgress->step();
//
//	FbxPose* bindPose = lCurrentScene->GetPose(0);
//
//	int numPoseNodes = pPoseScene->GetNodeCount();
//	for (int i = 0; i < numPoseNodes; i++)
//	{
//		FbxNode* poseNode = pPoseScene->GetNode(i);
//		const char* lpNodeName = poseNode->GetName();
//		QString sNodeName(lpNodeName);
//		// find node in main scene
//		FbxNode* mainNode = lookupTable[sNodeName];
//		if (mainNode)
//		{
//			mainNode->Copy(*poseNode);
//		}
//	}
//	if (pProgress) pProgress->step();
//
//	// close pose scene
//	pPoseScene->Destroy();
//
//	return true;
//}



int FbxTools::ConvertToZUp(FbxMesh* mesh, FbxNode* rootNode)
{
	int correction = 0;
	FbxVector4 eulerRotation;
	bool bRotate = false;
	if (bRotate == false)
	{
		// 1. Find Bounding Box
		FbxVector4* result = CalculateBoundingVolume(mesh);
		// 2. Check longest axis
		FbxVector4 cloudSize = result[0];
		FbxVector4 cloudCenter = result[1];
		if (cloudSize[1] > cloudSize[2])
		{
			// 3. If longest axis is not Y, then flip
			bRotate = true;
			// check Y value to figure out which direction to flip
			if (cloudCenter[1] > 0)
			{
				// mesh is +Yup
				correction = 90;
				eulerRotation = FbxVector4(correction, 0, 0);
			}
			else
			{
				correction = -90;
				eulerRotation = FbxVector4(correction, 0, 0);
			}
		}
		delete[] result;
	}
	//FbxNode* rootNode = lookupTable[QString("root")];
	if (rootNode && bRotate)
	{
		// HARD-CODED 90-deg X-axis rotation of root node....
		// TODO: detect and apply global axis correction as needed
		rootNode->LclRotation.Set(eulerRotation + rootNode->LclRotation.Get());
	}

	return correction;
}

bool FbxTools::FlipAndBakeVertexBuffer(FbxMesh* mesh, FbxNode* rootNode, FbxVector4* vertex_buffer)
{
	if (ConvertToZUp(mesh, rootNode) == false)
		return false;
    FbxAMatrix matrix = FbxTools::GetAffineMatrix(NULL, mesh->GetNode());
	BakePoseToVertexBuffer(vertex_buffer, &matrix, nullptr, mesh);

	return true;
}

FbxCluster* FbxTools::FindClusterFromNode(FbxNode* pNode)
{
	// debug
	int numDstConnections = pNode->GetDstObjectCount();
	int numSrcConnections = pNode->GetSrcObjectCount();
	const char* lpNdoeName = pNode->GetName();

	FbxCluster* pCluster1 = (FbxCluster*)pNode->GetSrcObject(FbxCriteria::ObjectType(FbxCluster::ClassId));
	FbxCluster* pCluster2 = (FbxCluster*)pNode->GetDstObject(FbxCriteria::ObjectType(FbxCluster::ClassId));
	FbxCluster* pCluster = nullptr;

	if (pCluster1)
	{
		pCluster = pCluster1;
	}
	else if (pCluster2)
	{
		pCluster = pCluster2;
	}

	return pCluster;

}

void FbxTools::removeMorphExportPrefixFromBlendShapeChannel(FbxBlendShapeChannel* pChannel, const char* prefix)
{
	// DB 2025-04-08: rename deformer (blendshapechannel)
	FbxString newChannelName = pChannel->GetName();
	newChannelName.FindAndReplace(prefix, "", 0);
	pChannel->SetName(newChannelName.Buffer());

	int shapeCount = pChannel->GetTargetShapeCount();
	for (int shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex)
	{
		FbxShape* shape = pChannel->GetTargetShape(shapeIndex);
		if (shape)
		{
			FbxString newName = shape->GetName();
			newName.FindAndReplace(prefix, "", 0);
			shape->SetName(newName.Buffer());
		}
	}
}

void FbxTools::removeMorphExportPrefixFromNode(FbxNode* pNode, const char* prefix)
{
	if (pNode)
	{
		// Check if the node has a mesh
		FbxMesh* pMesh = pNode->GetMesh();

		// Rename Shapes
		if (pMesh)
		{
			int deformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int deformerIndex = 0; deformerIndex < deformerCount; ++deformerIndex)
			{
				FbxBlendShape* blendShape = static_cast<FbxBlendShape*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eBlendShape));

				int blendShapeChannelCount = blendShape->GetBlendShapeChannelCount();
				for (int channelIndex = 0; channelIndex < blendShapeChannelCount; ++channelIndex)
				{
					FbxBlendShapeChannel* channel = blendShape->GetBlendShapeChannel(channelIndex);
					if (channel)
					{
						// Rename the shapes associated with this channel
						removeMorphExportPrefixFromBlendShapeChannel(channel, prefix);
					}
				}
			}
		}

		// Recursively process children nodes
		for (int j = 0; j < pNode->GetChildCount(); j++) {
			removeMorphExportPrefixFromNode(pNode->GetChild(j), prefix);
		}
	}
}

bool FbxTools::MultiplyMatrixToVertexBuffer(FbxAMatrix* pMatrix, FbxVector4* pVertexBuffer, int numVerts)
{
	// apply weight * matrix transform to each vertex
	for (int i = 0; i < numVerts; i++)
	{
		FbxVector4 sourceVertex = pVertexBuffer[i];
		FbxVector4 finalTargetVertex = pMatrix->MultT(sourceVertex);
		pVertexBuffer[i] = finalTargetVertex;
	}
	return true;
}


FbxVector4 FbxTools::CalculatePointCloudCenter(FbxVector4 *pVertexBuffer, int numVertices, bool bCenterWeight)
{

	if (pVertexBuffer == nullptr || numVertices <= 0)
	{
		dzApp->warning("ERROR: CalculatePointCloudCenter recieved invalid inputs");
		return nullptr;
	}

	FbxVector4 cloudCenter = pVertexBuffer[0];
	FbxVector4 min_bounds = cloudCenter;
	FbxVector4 max_bounds = cloudCenter;
	for (int vertIndex=0; vertIndex < numVertices; vertIndex++)
	{
		FbxVector4 currentPoint = pVertexBuffer[vertIndex];
		for (int i = 0; i < 3; i++)
		{
			if (currentPoint[i] < min_bounds[i]) min_bounds[i] = currentPoint[i];
			if (currentPoint[i] > max_bounds[i]) max_bounds[i] = currentPoint[i];
		}
	}
	double center_weight = 0;
	if (abs(max_bounds[0]) < abs(min_bounds[0]))
		center_weight = max_bounds[0];
	else
		center_weight = min_bounds[0];

	if (bCenterWeight)
		//		cloudCenter[0] = (max_bounds[0] + min_bounds[0] + center_weight) / 3;
		cloudCenter[0] = center_weight;
	else
		cloudCenter[0] = (max_bounds[0] + min_bounds[0]) / 2;
	cloudCenter[1] = (max_bounds[1] + min_bounds[1]) / 2;
	cloudCenter[2] = (max_bounds[2] + min_bounds[2]) / 2;

//	FbxVector4 cloudAverage = CalculatePointCloudAverage(pMesh, pVertexIndexes);

	return cloudCenter;

}

bool FbxTools::GetAllMeshes(FbxNode* pNode, QList<FbxNode*>& aFbxNodeList)
{
	if (pNode == NULL) return false;

	auto attribute = pNode->GetNodeAttribute();
	if (attribute) {
		auto attributeType = attribute->GetAttributeType();
		if (attributeType == FbxNodeAttribute::eMesh) {
			aFbxNodeList.append(pNode);
		}
	}

	for (int i = 0; i < pNode->GetChildCount(); i++) {
		FbxNode* pChild = pNode->GetChild(i);
		GetAllMeshes(pChild, aFbxNodeList);
	}

	return true;
}

bool FbxTools::HasNodeAncestor(FbxNode* pNode, const QString sAncestorName, Qt::CaseSensitivity cs) {

	FbxNode* pParentNode = pNode->GetParent();
	if (pParentNode == NULL) return false;

	QString sParentName = pParentNode->GetName();
	if (sParentName.compare(sAncestorName, cs) == 0) {
		return true;
	}
	return HasNodeAncestor(pParentNode, sAncestorName, cs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEV TESTING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FbxTools::ModifyBindPose(FbxScene* Scene, FbxNode* RootNode, ModifyBindPoseCallback *pCustomBoneFix)
{
    if (Scene == nullptr || RootNode == nullptr)
    {
        // log error and return
        dzApp->log(dzApp->tr("ERROR: FbxTools::ModifyBindPose() invalid nullptr argument."));
        return;
    }
    
	FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(RootNode->GetMesh());

	// find all skin clusters linked to geometry and modify the bind matrix linked to those clusters
	if (NodeGeometry)
	{

		for (int DeformerIndex = 0; DeformerIndex < NodeGeometry->GetDeformerCount(); ++DeformerIndex)
		{
			FbxSkin* Skin = static_cast<FbxSkin*>(NodeGeometry->GetDeformer(DeformerIndex));
			if (Skin)
			{
				for (int ClusterIndex = 0; ClusterIndex < Skin->GetClusterCount(); ++ClusterIndex)
				{
					// Get the current tranform
					FbxAMatrix Matrix;
					FbxCluster* Cluster = Skin->GetCluster(ClusterIndex);
					Cluster->GetTransformLinkMatrix(Matrix);

					// crash protection
					if (Cluster->GetLink() == nullptr) continue;
					
					QString sBoneName(Cluster->GetLink()->GetName());

					// Update the rotation
					FbxDouble3 Rotation = Cluster->GetLink()->PostRotation.Get();
					if (pCustomBoneFix) {
                        pCustomBoneFix->performTask(Matrix, Cluster, sBoneName, Rotation);
					}
					else {
						Matrix.SetR(Rotation);
					}
					Cluster->SetTransformLinkMatrix(Matrix);

//					// DEBUGGING
//					FbxRotationOrder oRotationOrder(Cluster->GetLink()->RotationOrder.Get());
//					FbxVector4 vRotation = Matrix.GetR();
//					printf("%s, order=%i, [%f, %f, %f]\n", sBoneName.toLocal8Bit().data(), oRotationOrder.GetOrder(), vRotation[0], vRotation[1], vRotation[2]);

				}
			}
		}
	}
	else {
//		printf("ERROR! No NodeGeometry for %s\n", RootNode->GetName());
	}
	
	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
		FbxTools::ModifyBindPose(Scene, ChildNode, pCustomBoneFix);
	}
}

/*
void RemoveBindPoses(FbxScene* Scene)
{
	for (int PoseIndex = Scene->GetPoseCount() - 1; PoseIndex >= 0; --PoseIndex)
	{
		Scene->RemovePose(PoseIndex);
	}
}
*/
void FbxTools::RemovePrePostRotations(FbxNode* pNode)
{
	QString sNodeName = pNode->GetName();
	for (int nChildIndex = 0; nChildIndex < pNode->GetChildCount(); nChildIndex++)
	{
		FbxNode* pChildBone = pNode->GetChild(nChildIndex);
		RemovePrePostRotations(pChildBone);
	}
	if (sNodeName.contains("twist", Qt::CaseInsensitive) == false)
	{
		pNode->SetPreRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
		pNode->SetPostRotation(FbxNode::EPivotSet::eSourcePivot, FbxVector4(0, 0, 0));
	}
}
void FbxTools::ReparentTwistBone(FbxNode* pNode)
{
	FbxNode* pParentNode = pNode->GetParent();
	FbxNode* pGrandParentNode = pParentNode->GetParent();
	QString sNodeName = pNode->GetName();
	QString sParentName = pParentNode->GetName();
	QString sGrandParentName = pGrandParentNode->GetName();

	// Calc Position Delta to add to Child
	FbxAMatrix mNodeLocalTransform = pNode->EvaluateLocalTransform();
	FbxVector4 vDelta = pNode->EvaluateLocalTransform().GetT();
	for (int nChildIndex = 0; nChildIndex < pNode->GetChildCount(); nChildIndex++)
	{
		FbxNode* pChildBone = pNode->GetChild(nChildIndex);
		QString sChildName = pChildBone->GetName();
		pNode->RemoveChild(pChildBone);
		pParentNode->AddChild(pChildBone);
		FbxAMatrix pChildLocalTransform = pChildBone->EvaluateLocalTransform();
		FbxAMatrix mNewTransform = mNodeLocalTransform * pChildLocalTransform;
		pChildBone->LclTranslation.Set(mNodeLocalTransform.GetT());
		//pChildBone->LclRotation.Set(mNodeLocalTransform.GetR());
		//pChildBone->LclScaling.Set(mNodeLocalTransform.GetS());
	}
	if (pGrandParentNode)
	{
		//		pParentNode->RemoveChild(pNode);
		//		pGrandParentNode->AddChild(pNode);
	}
	else
	{
//		printf("nop");
	}

}
void FbxTools::FindAndProcessTwistBones(FbxNode* pNode)
{
	QString sNodeName = pNode->GetName();
	for (int nChildIndex = 0; nChildIndex < pNode->GetChildCount(); nChildIndex++)
	{
		FbxNode* pChildBone = pNode->GetChild(nChildIndex);
		FindAndProcessTwistBones(pChildBone);
	}
	if (sNodeName.contains("twist", Qt::CaseInsensitive))
	{
		ReparentTwistBone(pNode);
	}
}

#define TCHAR_TO_UTF8(a) QString(a).toUtf8().constData()
#define TEXT(a) a
void FbxTools::AddIkNodes(FbxScene* pScene, FbxNode* pRootBone, const char* sLeftFoot, const char* sRightFoot, const char* sLeftHand, const char* sRightHand)
{
	bool AddIKBones = true;
	// Add IK bones
	if (pRootBone && AddIKBones)
	{
		// ik_foot_root
		FbxNode* IKRootNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_root")));
		if (!IKRootNode)
		{
			// Create IK Root
			FbxSkeleton* IKRootNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_root")));
			IKRootNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKRootNodeAttribute->Size.Set(1.0);
			IKRootNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_root")));
			IKRootNode->SetNodeAttribute(IKRootNodeAttribute);
			IKRootNode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));
			IKRootNode->LclRotation.Set(FbxVector4(-90.0, 0.0, 0.0));
			pRootBone->AddChild(IKRootNode);
		}

		// ik_foot_l
		FbxNode* IKFootLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_l")));
		FbxNode* FootLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sLeftFoot)));
		if (!FootLNode) FootLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("l_foot")));
		if (!IKFootLNode && FootLNode)
		{
			// Create IK Root
			FbxSkeleton* IKFootLNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_l")));
			IKFootLNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKFootLNodeAttribute->Size.Set(1.0);
			IKFootLNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_l")));
			IKFootLNode->SetNodeAttribute(IKFootLNodeAttribute);
			FbxAMatrix FootTransform = FootLNode->EvaluateGlobalTransform();
			FbxAMatrix ParentTransform = IKRootNode->EvaluateGlobalTransform();
			FbxAMatrix LocalTransform = ParentTransform.Inverse() * FootTransform;
			FbxVector4 FootLocation = LocalTransform.GetT();
			FbxVector4 FootOrientation = LocalTransform.GetR();
			IKFootLNode->LclTranslation.Set(FootLocation);
			IKFootLNode->LclRotation.Set(FootOrientation);
			IKRootNode->AddChild(IKFootLNode);
		}

		// ik_foot_r
		FbxNode* IKFootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_r")));
		FbxNode* FootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sRightFoot)));
		if (!FootRNode) FootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_foot")));
		if (!IKFootRNode && FootRNode)
		{
			// Create IK FootR
			FbxSkeleton* IKFootRNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKFootRNodeAttribute->Size.Set(1.0);
			IKFootRNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNode->SetNodeAttribute(IKFootRNodeAttribute);
			FbxAMatrix FootTransform = FootRNode->EvaluateGlobalTransform();
			FbxAMatrix ParentTransform = IKRootNode->EvaluateGlobalTransform();
			FbxAMatrix LocalTransform = ParentTransform.Inverse() * FootTransform;
			FbxVector4 FootLocation = LocalTransform.GetT();
			FbxVector4 FootOrientation = LocalTransform.GetR();
			IKFootRNode->LclTranslation.Set(FootLocation);
			IKFootLNode->LclRotation.Set(FootOrientation);
			IKRootNode->AddChild(IKFootRNode);
		}

		// ik_hand_root
		FbxNode* IKHandRootNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_root")));
		if (!IKHandRootNode)
		{
			// Create IK HandRoot
			FbxSkeleton* IKHandRootNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRootNodeAttribute->Size.Set(1.0);
			IKHandRootNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNode->SetNodeAttribute(IKHandRootNodeAttribute);
			IKHandRootNode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));
			IKHandRootNode->LclRotation.Set(FbxVector4(-90.0, 0.0, 0.0));
			pRootBone->AddChild(IKHandRootNode);
		}

		// ik_hand_gun
		FbxNode* IKHandGunNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
		FbxNode* HandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sRightHand)));
		if (!HandRNode) HandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_hand")));
		if (!IKHandGunNode && HandRNode)
		{
			// Create IK GUN
			FbxSkeleton* IKHandGunNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandGunNodeAttribute->Size.Set(1.0);
			IKHandGunNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNode->SetNodeAttribute(IKHandGunNodeAttribute);
			FbxAMatrix HandTransform = HandRNode->EvaluateGlobalTransform();
			FbxAMatrix ParentTransform = IKHandRootNode->EvaluateGlobalTransform();
			FbxAMatrix LocalTransform = ParentTransform.Inverse() * HandTransform;
			FbxVector4 HandLocation = LocalTransform.GetT();
			FbxVector4 HandOrientation = LocalTransform.GetR();
			IKHandGunNode->LclTranslation.Set(HandLocation);
			IKHandGunNode->LclRotation.Set(HandOrientation);
			IKHandRootNode->AddChild(IKHandGunNode);
		}

		// ik_hand_r
		FbxNode* IKHandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_r")));
		if (!IKHandRNode && HandRNode && IKHandGunNode)
		{
			// Create IK HANDR
			FbxSkeleton* IKHandRNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_r")));
			IKHandRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRNodeAttribute->Size.Set(1.0);
			IKHandRNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_r")));
			IKHandRNode->SetNodeAttribute(IKHandRNodeAttribute);
			IKHandRNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
			IKHandGunNode->AddChild(IKHandRNode);
		}

		// ik_hand_l
		FbxNode* IKHandLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_l")));
		FbxNode* HandLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sLeftHand)));
		if (!HandLNode) HandLNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("l_hand")));
		if (!IKHandLNode && HandLNode && IKHandGunNode)
		{
			// Create IK HANDL
			FbxSkeleton* IKHandRNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRNodeAttribute->Size.Set(1.0);
			IKHandLNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandLNode->SetNodeAttribute(IKHandRNodeAttribute);
			FbxAMatrix HandTransform = HandLNode->EvaluateGlobalTransform();
			FbxAMatrix ParentTransform = IKHandGunNode->EvaluateGlobalTransform();
			FbxAMatrix LocalTransform = ParentTransform.Inverse() * HandTransform;
			FbxVector4 HandLocation = LocalTransform.GetT();
			FbxVector4 HandOrientation = LocalTransform.GetR();
			IKHandLNode->LclTranslation.Set(HandLocation);
			IKHandLNode->LclRotation.Set(HandOrientation);
			IKHandGunNode->AddChild(IKHandLNode);
		}
	}

}

// Built-in implementation of CustomBoneFix callback for use with Metahuman and Unreal Engine 5.x Mannequin rig conversion process
void FbxTools::UnrealJointFixCallback::performTask(FbxAMatrix &Matrix, FbxCluster *Cluster, QString sBoneName, FbxDouble3 Rotation)
{
    // Hard code for Unreal Engine Mannequin bone-names
    Matrix.MultRM(Rotation);
    if (sBoneName.contains("_r")) {
        Matrix.MultRM(FbxVector4(90, 0, 0));
    }
    else {
        Matrix.MultRM(FbxVector4(-90, 0, 0));
    }
    
    if (sBoneName.contains("ball_")) {
        Matrix.MultRM(FbxVector4(90, 0, 0));
    }
    else if (sBoneName.contains("thumb_")) {
        Matrix.MultRM(FbxVector4(0, 0, 0));
    }
    else if (sBoneName.contains("hand_") ||
             FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
			 FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive))
    {
        Matrix.MultRM(FbxVector4(-90, 0, 0));
    }
    
    if (sBoneName.contains("_l") || sBoneName.contains("_r")) {
        if (FbxTools::HasNodeAncestor(Cluster->GetLink(), "spine_01", Qt::CaseInsensitive)) {
            Matrix.MultRM(FbxVector4(0, 0, 0));
        }
        else {
            Matrix.MultRM(FbxVector4(0, -90, 0));
        }
    }
    else {
        Matrix.MultRM(FbxVector4(0, -90, 0));
    }

}

#include <QMessageBox>
bool FbxTools::ExLoadScene(FbxScene* pScene, QString sFilename, void (*pfLogFunction)(QString), bool bShowGuiError, QString sErrorMessageTemplate)
{
	if (pScene == nullptr) return false;

	if (sErrorMessageTemplate.isEmpty() || sErrorMessageTemplate == "") 
	{
		sErrorMessageTemplate = QObject::tr("\
ERROR: FbxTools::ExLoadScene():\n\n\
File: \"%1\"\n\n\
FbxStatusCode: %2\n\n\
Error Message: \"%3\"\n\n"
		   );
	}
	
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();

#if 0
	openFBX->LoadScene(pScene, sFilename);
#else
	if (openFBX->LoadScene(pScene, sFilename) == false)
#endif
	{
		QString sCombinedErrorMessage = QString(sErrorMessageTemplate).arg(sFilename).arg(openFBX->GetErrorCode()).arg(openFBX->GetErrorString());

		if (pfLogFunction)
		{
			QString sLogMessage = QString(sCombinedErrorMessage).replace("\n\n", "\n");
			if (sLogMessage.endsWith("\n")) {
				sLogMessage.chop(1);
			}
			sLogMessage = sLogMessage.replace("\n", ", ").replace(":,", ":");
			pfLogFunction(sLogMessage);
		}

		if (bShowGuiError)
		{
			QMessageBox::warning(0,
				QObject::tr("Error"),
				QObject::tr("An error occurred while processing the Fbx file:\n\n") + sCombinedErrorMessage,
				QMessageBox::Ok);
		}
		return false;
	}

	if (pfLogFunction)
	{
		QString sLogSuccess = QObject::tr("FbxTools::ExLoadScene(): File Loaded: \"%1\"");
		sLogSuccess = QString(sLogSuccess).arg(sFilename);
		pfLogFunction(sLogSuccess);
	}
	
	return true;
}

bool FbxTools::LoadBlendshapeMappingTable(QString sMappingFilename, QMap<QString, QString> &oMappingTable, QList<QString> &aMappingOrder)
{
	QFile oMappingFile(sMappingFilename);
	
	if (!oMappingFile.exists()) return false;
	
	if (!oMappingFile.open(QIODevice::ReadOnly)) {
		return false;
	}

	// load the selected csv from disk into the export list on the right
	QTextStream oInputStream(&oMappingFile);

	while (!oInputStream.atEnd()) {
		QString sInputLine = oInputStream.readLine();
		QStringList aKeyValuePair = sInputLine.split(",");
		aMappingOrder.append(aKeyValuePair[0]);
		oMappingTable.insert(aKeyValuePair[0], aKeyValuePair[1]);
	}

	oMappingFile.close();
	
	return true;
}

// Transfer blendshapes from source fbx to destination scene
bool FbxTools::TransferBlendshapes(QString sSourceFilename, FbxScene* pDestinationScene, QString sMappingFilename)
{
	if (sSourceFilename.isEmpty() || sSourceFilename == "" || pDestinationScene == nullptr) return false;

	QString sLog;
//	sLog = "*************************DEBUG: TransferBlendshapes() START HERE **************************";
//	dzApp->log(sLog);
//	printf("%s\n", sLog.toLocal8Bit().constData());
	
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	QList<FbxNode*> aDestinationMeshNodeList;
	FbxTools::GetAllMeshes(pDestinationScene->GetRootNode(), aDestinationMeshNodeList);
	
	FbxScene* pSourceScene = openFBX->CreateScene("Blendshapes Scene");
	if (FbxTools::ExLoadScene(pSourceScene, sSourceFilename) == false) {
		pSourceScene->Destroy();
		return false;
	}
	else
	{
		// get mesh nodes list
		FbxNode* pSourceRootNode = pSourceScene->GetRootNode();
		QList<FbxNode*> aSourceMeshNodeList;
		FbxTools::GetAllMeshes(pSourceRootNode, aSourceMeshNodeList);
		sLog = QString("FbxTools::TransferBlendshapes() proxy file '%1' loaded with %2 meshes.").arg(sSourceFilename).arg(aSourceMeshNodeList.count());
		dzApp->log(sLog);
		printf("%s\n", sLog.toLocal8Bit().constData());
		
		for (int nSourceMeshIndex=0; nSourceMeshIndex < aSourceMeshNodeList.count(); nSourceMeshIndex++)
		{
			// Load Ordered Blendshape Mapping Table
			QMap<QString, QString> oChannelMappingTable;
			QMap<QString, QString> oReverseLookupChannelMapping;
			QList<QString> aChannelOrderedList;
			
			LoadBlendshapeMappingTable(sMappingFilename, oChannelMappingTable, aChannelOrderedList);
			foreach(auto key, oChannelMappingTable.keys()) {
				auto value = oChannelMappingTable.value(key);
				oReverseLookupChannelMapping.insert(value, key);
			}
			
			FbxNode* pSourceNode = aSourceMeshNodeList[nSourceMeshIndex];
			FbxMesh* pSourceMesh = pSourceNode->GetMesh();
			FbxMesh* pDestinationMesh = nullptr;
			int nSourceMeshVertexCount = pSourceMesh->GetControlPointsCount();
			// Find Mesh in pScene with same vertexcount and same mesh name
			foreach(FbxNode* pDestinationNode, aDestinationMeshNodeList) {
				FbxMesh* pCurrentDestinationMesh = pDestinationNode->GetMesh();
				if (pCurrentDestinationMesh->GetControlPointsCount() == nSourceMeshVertexCount) {
					// double-check name
					QString sDestinationMeshName = QString(pCurrentDestinationMesh->GetName()).replace(".Shape", "");
					QString sSourceMeshName = QString(pSourceMesh->GetName()).replace(".Shape", "");
					if (sDestinationMeshName != sSourceMeshName) {
						continue;
					}
					pDestinationMesh = pCurrentDestinationMesh;
					break;
				}
			}
			if (pDestinationMesh == nullptr) {
				sLog = QString("FbxTools::TransferBlendshapes() Skipping mesh %1, no match found with %2 verts.").arg(pSourceNode->GetName()).arg(nSourceMeshVertexCount);
				dzApp->log(sLog);
				printf("%s\n", sLog.toLocal8Bit().constData());
				continue;
			}
//			sLog = QString("FbxTools::TransferBlendshapes() Attempting blendshape transfer for mesh %1, with %2 verts.").arg(pSourceNode->GetName()).arg(nSourceMeshVertexCount);
//			dzApp->log(sLog);
//			printf("%s\n", sLog.toLocal8Bit().constData());
			
			// First Pass to build Blendshape Proxy Lookup Table
			QMap<QString, int> oChannelIndexLookup;
			int numBlendshapes = pSourceMesh->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int nBlendshapeIndex = 0; nBlendshapeIndex < numBlendshapes; nBlendshapeIndex++)
			{
				// Blendshape Level
				FbxBlendShape* pSourceBlendshape = static_cast<FbxBlendShape*>(pSourceMesh->GetDeformer(nBlendshapeIndex, FbxDeformer::eBlendShape));
				const char* pBlendshapeName = pSourceBlendshape->GetName();
				int numBlendshapeChannels = pSourceBlendshape->GetBlendShapeChannelCount();
				for (int nBlendshapeChannelIndex = 0; nBlendshapeChannelIndex < numBlendshapeChannels; nBlendshapeChannelIndex++)
				{
					// Channel Level
					FbxBlendShapeChannel* pSourceChannel = pSourceBlendshape->GetBlendShapeChannel(nBlendshapeChannelIndex);
					const char* pChannelName = pSourceChannel->GetName();
					int numTargetShapes = pSourceChannel->GetTargetShapeCount();

					QString sCleanedMeshName = QString(pSourceMesh->GetName()).replace(".Shape", "");
					QString sCleanedChannelName = QString(pChannelName).replace(sCleanedMeshName+"__", "");
					oChannelIndexLookup.insert(sCleanedChannelName, nBlendshapeChannelIndex);
					
					// check for existing entry
					bool bInChannelMappingTable = (oReverseLookupChannelMapping.find(sCleanedChannelName) != oReverseLookupChannelMapping.end());
					if (!bInChannelMappingTable)
					{
//						printf("DEBUG: adding unmapped channel name: %s\n", pChannelName);
						oChannelMappingTable.insert(pChannelName, sCleanedChannelName);
						aChannelOrderedList.append(pChannelName);
					}

				} // for (int nBlendshapeChannelIndex = 0; nBlendshapeChannelIndex < numBlendshapeChannels; nBlendshapeChannelIndex++)
			} // for (int nBlendshapeIndex = 0; nBlendshapeIndex < numBlendshapes; nBlendshapeIndex++)

			// Second Pass to transfer shapes in correct order

			// **** CREATE BLENDSHAPE IN pDestinationScene ****
			QString sDestinationBlendshapeName = QString(pDestinationMesh->GetName()).replace(".Shape", "") + "BlendShapes";
			FbxBlendShape* pDestinationShape = FbxBlendShape::Create(pDestinationScene->GetFbxManager(), sDestinationBlendshapeName.toLocal8Bit().data());
			pDestinationMesh->AddDeformer((FbxDeformer*) pDestinationShape);
//			sLog = QString("FbxTools::TransferBlendshapes() Adding blendshape[%1]: %2, numChannels: %3").arg(0).arg(sDestinationBlendshapeName).arg(aChannelOrderedList.count());
//			dzApp->log(sLog);
//			printf("%s\n", sLog.toLocal8Bit().constData());

			FbxBlendShape* pSourceBlendshape = static_cast<FbxBlendShape*>(pSourceMesh->GetDeformer(0, FbxDeformer::eBlendShape));

			for (int nDestinationBlendshapeChannelIndex=0; nDestinationBlendshapeChannelIndex < aChannelOrderedList.count(); nDestinationBlendshapeChannelIndex++)
			{					
				// ***** CREATE CHANNEL IN pScene *****
				QString sDestinationChannelName = aChannelOrderedList[nDestinationBlendshapeChannelIndex];
				FbxBlendShapeChannel* pDestinationChannel = FbxBlendShapeChannel::Create(pDestinationScene->GetFbxManager(), sDestinationChannelName.toLocal8Bit().data());
				pDestinationShape->AddBlendShapeChannel(pDestinationChannel);
//				sLog = QString("FbxTools::TransferBlendshapes() Adding channel [%1]: %2").arg(nDestinationBlendshapeChannelIndex).arg(sDestinationChannelName);
//				dzApp->log(sLog);
//				printf("%s\n", sLog.toLocal8Bit().constData());

				// Lookup Correct Source Channel
				QString sMappedChannelName = oChannelMappingTable[sDestinationChannelName];
				if (oChannelIndexLookup.find(sMappedChannelName) == oChannelIndexLookup.end()) {
//					sLog = QString("FbxTools: ERROR: unable to lookup channel index for: " + sMappedChannelName + ", skipping...");
//					dzApp->log(sLog);
//					printf("%s\n", sLog.toLocal8Bit().constData());
					continue;
				}
				int nSourceChannel = oChannelIndexLookup[sMappedChannelName];

				FbxBlendShapeChannel* pSourceChannel = pSourceBlendshape->GetBlendShapeChannel(nSourceChannel);
				FbxShape* pSourceShape = pSourceChannel->GetTargetShape(0);
				int numVertsShapeBuffer = pSourceShape->GetControlPointsCount();

				// ***** CREATE SHAPE IN pScene ******
				FbxShape* pDestinationShape = FbxShape::Create(pDestinationScene->GetFbxManager(), sDestinationChannelName.toLocal8Bit().data());
				pDestinationChannel->AddTargetShape(pDestinationShape);
//				sLog = QString("FbxTools::TransferBlendshapes() Adding target shape [%1]: %2").arg(0).arg(sDestinationChannelName);
//				dzApp->log(sLog);
//				printf("%s\n", sLog.toLocal8Bit().constData());

				// prepare source
				FbxVector4* pSourceBasisBuffer = pSourceMesh->GetControlPoints();
				FbxVector4* pSourceBuffer = pSourceShape->GetControlPoints();
				// prepare destionation
				pDestinationShape->SetControlPointCount(numVertsShapeBuffer);
				FbxVector4* pDestinationBasisBuffer = pDestinationMesh->GetControlPoints();
				FbxVector4* pDestinationBuffer = pDestinationShape->GetControlPoints();

				// iterate and transfer each vertex delta
				for (int nBufferIndex=0; nBufferIndex < numVertsShapeBuffer; nBufferIndex++) {
					// compute vertex deltas
					FbxVector4 oVertexDelta = pSourceBuffer[nBufferIndex] - pSourceBasisBuffer[nBufferIndex];
					// add vertex deltas to destination buffer
					pDestinationBuffer[nBufferIndex] = pDestinationBasisBuffer[nBufferIndex] + oVertexDelta;
				}

			} // for (int nDestinationBlendshapeIndex=0; nDestinationBlendshapeIndex < aBlendshapeNameOrderedList.count(); nDestinationBlendshapeIndex++)
			
		} // foreach(aBlendshapeMeshNodeList)
	}

	pSourceScene->Destroy();

	return true;
}

bool FbxTools::BakeMeshesToSingleBindPose(FbxScene* pScene)
{
	if (pScene == nullptr) return false;

	QList<FbxNode*> nodeList;
	FbxNode* pFbxRootNode = pScene->GetRootNode();
	FbxTools::GetAllMeshes(pFbxRootNode, nodeList);
	
	// Bake all meshes to use the same bind pose (blender work-around -- does not support separate bind matrix in follower meshes)
	FbxTools::RemoveBindPoses(pScene);
	foreach(FbxNode * pNode, nodeList) {
		QString debugName(pNode->GetName());
		FbxMesh* pMesh = pNode->GetMesh();
		FbxAMatrix matrix = pNode->EvaluateGlobalTransform();
		FbxVector4* pVertexBuffer = pMesh->GetControlPoints();
		if (pVertexBuffer == NULL) continue;
		FbxTools::BakePoseToVertexBuffer(pVertexBuffer, &matrix, nullptr, pMesh);
		
		// bake for each blendshape
		int numBlendshapes = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
		for (int nBlendshapeIndex = 0; nBlendshapeIndex < numBlendshapes; nBlendshapeIndex++)
		{
			// Blendshape Level
			FbxBlendShape* pBlendshape = static_cast<FbxBlendShape*>(pMesh->GetDeformer(nBlendshapeIndex, FbxDeformer::eBlendShape));
			int numBlendshapeChannels = pBlendshape->GetBlendShapeChannelCount();
			for (int nBlendshapeChannelIndex = 0; nBlendshapeChannelIndex < numBlendshapeChannels; nBlendshapeChannelIndex++)
			{
				// Channel Level
				FbxBlendShapeChannel* pChannel = pBlendshape->GetBlendShapeChannel(nBlendshapeChannelIndex);
				int numTargetShapes = pChannel->GetTargetShapeCount();
				for (int nTargetShapeIndex = 0; nTargetShapeIndex < numTargetShapes; nTargetShapeIndex++)
				{
					FbxShape* pTargetShape = pChannel->GetTargetShape(nTargetShapeIndex);
					pVertexBuffer = pTargetShape->GetControlPoints();
					if (pVertexBuffer == NULL) continue;
					FbxTools::BakePoseToVertexBuffer(pVertexBuffer, &matrix, nullptr, pMesh);
				}
			}
		}
	}
	foreach(FbxNode* pNode, nodeList) {
		FbxMesh* pMesh = pNode->GetMesh();
		FbxTools::BakePoseToBindMatrix(pMesh, nullptr);
	}

	return true;
}

////////////////////////////////////////////////

QString SanitizeName(QString OriginalName)
{
	return OriginalName.replace(TEXT(" "), TEXT(""))
		.replace(TEXT("("), TEXT("_"))
		.replace(TEXT(")"), TEXT("_"))
		.replace(TEXT("."), TEXT("_"))
		.replace(TEXT("&"), TEXT("_"))
		.replace(TEXT("!"), TEXT("_"))
		.replace(TEXT("*"), TEXT("_"))
		.replace(TEXT("<"), TEXT("_"))
		.replace(TEXT(">"), TEXT("_"))
		.replace(TEXT("?"), TEXT("_"))
		.replace(TEXT("\\"), TEXT("_"))
		.replace(TEXT(":"), TEXT("_"))
		.replace(TEXT("'"), TEXT("_"));
}

void RenameDuplicateBones(FbxNode* pRootNode, QMap<QString, int>& oExistingBones)
{
	if (pRootNode == nullptr) return;

	FbxNodeAttribute* pAttr = pRootNode->GetNodeAttribute();
	if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		QString sBoneName = QString(pRootNode->GetName());
		if (oExistingBones.contains(sBoneName))
		{
			oExistingBones[sBoneName] += 1;
			sBoneName = QString("%1_RENAMED_%2").arg(sBoneName).arg(oExistingBones[sBoneName]);
			pRootNode->SetName(sBoneName.toLocal8Bit().constData());
		}
		else
		{
			oExistingBones.insert(sBoneName, 1);
		}
	}

	for (int nChildIndex = 0; nChildIndex < pRootNode->GetChildCount(); ++nChildIndex)
	{
		FbxNode* pChildNode = pRootNode->GetChild(nChildIndex);
		RenameDuplicateBones(pChildNode, oExistingBones);
	}
}

void FbxTools::RenameDuplicateBones(FbxNode* pRootNode)
{
	QMap<QString, int> oExistingBones;
	::RenameDuplicateBones(pRootNode, oExistingBones);
}

// Some accesories attached in ways like using the DzRigidFollowNode become additional meshes.
// This function attached them to the skeleton of the primary mesh so the don't break the skeleton.
void FbxTools::MergeFollowerRigs(FbxScene* pScene)
{
	FbxNode* pRootNode = pScene->GetRootNode();
	for (int RootNodeIndex = pScene->GetNodeCount() -1; RootNodeIndex >= 0; --RootNodeIndex)
	{
		FbxNode* pOtherRootNode = pScene->GetNode(RootNodeIndex);

		if (pOtherRootNode != pRootNode)
		{
			if (FbxSkeleton* pOtherRootNodeSkeleton = pOtherRootNode->GetSkeleton())
			{
				pOtherRootNodeSkeleton->SetSkeletonType(FbxSkeleton::eLimbNode);
			}
			else if(pOtherRootNode->GetMesh() == nullptr)
			{
				FbxSkeleton* pSkeletonAttribute = FbxSkeleton::Create(pScene, pOtherRootNode->GetName());
				pSkeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
				pOtherRootNode->SetNodeAttribute(pSkeletonAttribute);
			}
		}
	}
}

FbxNode* FbxTools::AddRootBone(FbxNode* pRootNode, FbxScene* pScene)
{
	FbxNode* pRootBone = nullptr;

	// If this is a skeleton mesh, but a root bone wasn't found, it may be a scene under a group node or something similar
	// So create a root node.
	if (pRootBone == nullptr)
	{
		FbxSkeleton* NewRootNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("root")));
		NewRootNodeAttribute->SetSkeletonType(FbxSkeleton::eRoot);
		NewRootNodeAttribute->Size.Set(1.0);
		pRootBone = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("root")));
		pRootBone->SetNodeAttribute(NewRootNodeAttribute);
		pRootBone->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));


		for (int ChildIndex = pRootNode->GetChildCount() - 1; ChildIndex >= 0; --ChildIndex)
		{
			FbxNode* ChildNode = pRootNode->GetChild(ChildIndex);
			pRootBone->AddChild(ChildNode);
			if (FbxSkeleton* ChildSkeleton = ChildNode->GetSkeleton())
			{
				if (ChildSkeleton->GetSkeletonType() == FbxSkeleton::eRoot)
				{
					ChildSkeleton->SetSkeletonType(FbxSkeleton::eLimb);
				}
			}
		}

		pRootNode->AddChild(pRootBone);
	}

	return pRootBone;
}

FbxNode* FbxTools::FindRootBone(FbxNode* pRootNode, FbxScene* pScene)
{
	FbxNode* pRootBone = nullptr;

	for (int nChildIndex = 0; nChildIndex < pRootNode->GetChildCount(); ++nChildIndex)
	{
		FbxNode* pChildNode = pRootNode->GetChild(nChildIndex);
		FbxNodeAttribute* pAttr = pChildNode->GetNodeAttribute();
		if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			pRootBone = pChildNode;
			break;
		}
	}

	return pRootBone;
}

// Takes twist bones "out of line".  G3 and G8 have twist bones between some joints like thigh and knee.
void FbxTools::FixTwistBones(FbxNode* pNode)
{
	if (pNode == nullptr) return;

	// Process Children first since they'll get reparented
	for (int nChildIndex = pNode->GetChildCount() - 1; nChildIndex >= 0; --nChildIndex)
	{
		FbxNode* pChildNode = pNode->GetChild(nChildIndex);
		FixTwistBones(pChildNode);
	}

	FbxNodeAttribute* pAttr = pNode->GetNodeAttribute();
	if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		QString sBoneName = QString(pNode->GetName());
		if (sBoneName.contains(TEXT("twist")))
		{
			for (int nChildIndex = pNode->GetChildCount() - 1; nChildIndex >= 0; --nChildIndex)
			{
				FbxNode* pChildNode = pNode->GetChild(nChildIndex);
				if (pNode->GetParent())
				{
					pNode->RemoveChild(pChildNode);
					pNode->GetParent()->AddChild(pChildNode);
				}
			}
		}
	}
}

QString GetFriendlyObjectName(FbxNode* pNode)
{
	if (!pNode) return QString();

	QString sObjectName = "";

//	FbxProperty oNameProperty   = pNode->FindProperty("StudioNodeName");
	FbxProperty oLabelProperty  = pNode->FindProperty("StudioNodeLabel");
//	FbxProperty oSceneIdProperty = pNode->FindProperty("StudioSceneID");

	// If the label property is missing or invalid, walk up the hierarchy to find a valid label.
	if (!oLabelProperty.IsValid())
	{
		FbxNode* pParent = pNode->GetParent();
		while (pParent && (!oLabelProperty.IsValid()))
		{
			oLabelProperty = pParent->FindProperty("StudioNodeLabel");
			if (oLabelProperty.IsValid()) {
				break;
			}
			pParent = pParent->GetParent();
		}
	}

	if (oLabelProperty.IsValid()) 
	{
		FbxString oLabelValue = oLabelProperty.Get<FbxString>();
		sObjectName = QString(oLabelValue.Buffer());
	}
	else if (!oLabelProperty.IsValid()) 
	{
		// Search Daz Scene
		QString sSearchString = QString(pNode->GetName()).replace(".Shape", "", Qt::CaseInsensitive);
		DzNode* pNodeSearchResult = dzScene->findNode(sSearchString);
		if (pNodeSearchResult) {
			sObjectName = pNodeSearchResult->getLabel();
		}
	}

	return sObjectName;
}

FbxNode* GetObjectForMaterial(FbxSurfaceMaterial* Material)
{
	FbxScene* Scene = Material->GetScene();

	for (int MeshIndex = Scene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
	{
		FbxGeometry* Geometry = Scene->GetGeometry(MeshIndex);
		FbxNode* GeometryNode = Geometry->GetNode();
		int MaterialCount = GeometryNode->GetMaterialCount();
		for (int MaterialIndex = 0; MaterialIndex < MaterialCount; MaterialIndex++)
		{
			FbxSurfaceMaterial* NodeMaterial = GeometryNode->GetMaterial(MaterialIndex);
			if (NodeMaterial == Material)
			{
				return GeometryNode;
			}
		}
	}
	
	return nullptr;
}

QString GetObjectNameForMaterial(FbxSurfaceMaterial* Material)
{
	FbxScene* Scene = Material->GetScene();

	for (int MeshIndex = Scene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
	{
		FbxGeometry* Geometry = Scene->GetGeometry(MeshIndex);
		FbxNode* GeometryNode = Geometry->GetNode();
		int MaterialCount = GeometryNode->GetMaterialCount();
		for (int MaterialIndex = 0; MaterialIndex < MaterialCount; MaterialIndex++)
		{
			FbxSurfaceMaterial* NodeMaterial = GeometryNode->GetMaterial(MaterialIndex);
			if (NodeMaterial == Material)
			{
				QString ObjectName = QString(Geometry->GetName());
				return ObjectName;
			}
		}
	}

	return QString();
}



bool RenameBlendshapeChannel(FbxBlendShapeChannel* pChannel, QString sNewName)
{
	if (pChannel == nullptr) return false;
	
	QString sChannelName(pChannel->GetName());
	pChannel->SetName(sNewName.toLocal8Bit().constData());

	int numShapes = pChannel->GetTargetShapeCount();
	for (int nShapeIndex = 0; nShapeIndex < numShapes; ++nShapeIndex)
	{
		FbxShape* pTargetShape = pChannel->GetTargetShape(nShapeIndex);
		if (pTargetShape) {
			QString sTargetShapeName(pTargetShape->GetName());
			if (sTargetShapeName.compare(sChannelName) == 0) {
				pTargetShape->SetName(sNewName.toLocal8Bit().constData());
			}
		}
	}

	return true;
}

#include "MorphTools.h"
bool FbxTools::RenameMorphs(FbxScene* pScene, QMap<QString, MorphInfo> &MorphMappings, bool bUseLabels)
{
	if (pScene == nullptr) return false;
	QList<FbxNode*> aMeshNodeList;
	FbxTools::GetAllMeshes(pScene->GetRootNode(), aMeshNodeList);

	foreach(FbxNode * pNode, aMeshNodeList)
	{
		if (pNode == nullptr) continue;
		QString sNodeName(pNode->GetName());
		// Check if the node has a mesh
		FbxMesh* pMesh = pNode->GetMesh();
		if (pMesh == nullptr) continue;
		QString sMeshName = QString(pMesh->GetName()).replace(".Shape", "");
		// Rename Shapes
		int numBlendshapes = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
		for (int nBlendshapeIndex = 0; nBlendshapeIndex < numBlendshapes; ++nBlendshapeIndex)
		{
			FbxBlendShape* pBlendShape = static_cast<FbxBlendShape*>(pMesh->GetDeformer(nBlendshapeIndex, FbxDeformer::eBlendShape));
			if (pBlendShape == nullptr) continue;
			int numChannels = pBlendShape->GetBlendShapeChannelCount();
			for (int nChannelIndex = 0; nChannelIndex < numChannels; ++nChannelIndex)
			{
				FbxBlendShapeChannel* pChannel = pBlendShape->GetBlendShapeChannel(nChannelIndex);
				if (pChannel == nullptr) continue;
				QString sChannelName = QString(pChannel->GetName());
				QString sChannelNameCleaned = QString(sChannelName).replace(sMeshName + "__", "");
//				printf("DEBUG: sChannelName = %s, changing to %s\n", sChannelName.toLocal8Bit().constData(), sChannelNameCleaned.toLocal8Bit().constData());
				QString sNewName = MorphMappings.value(sChannelNameCleaned).Label;
				if (bUseLabels && !sNewName.isEmpty()) {
//					printf("DEBUG: Renaming sChannelName: %s to %s\n", sChannelNameCleaned.toLocal8Bit().constData(), sNewName.toLocal8Bit().constData());
					RenameBlendshapeChannel(pChannel, sNewName);
				}
				else
				{
					RenameBlendshapeChannel(pChannel, sChannelNameCleaned);
				}
			}
		}
	}	
	
	return true;
}

bool ProcessMorphs(FbxScene* Scene
//	const UDazToUnrealSettings* CachedSettings, 
//	TSharedPtr<FJsonObject>& JsonObject
	)
{
	// Get a list of morph name mappings
	QMap<QString, QString> MorphMappings;
//	TArray<TSharedPtr<FJsonValue>> morphList = JsonObject->GetArrayField(TEXT("Morphs"));
//	for (int i = 0; i < morphList.Num(); i++)
//	{
//		TSharedPtr<FJsonObject> morph = morphList[i]->AsObject();
//		QString MorphName = morph->GetStringField(TEXT("Name"));
//		QString MorphLabel = morph->GetStringField(TEXT("Label"));
//
//		// Daz Studio seems to strip the part of the name before a period when exporting the morph to FBX
//		if (MorphName.Contains(TEXT(".")))
//		{
//			QString Left;
//			MorphName.Split(TEXT("."), &Left, &MorphName);
//		}
//
//		if (CachedSettings->UseInternalMorphName)
//		{
//			MorphMappings.Add(MorphName, MorphName);
//		}
//		else
//		{
//			MorphMappings.Add(MorphName, MorphLabel);
//		}
//	}

	// Combine clothing and body morphs
/***************************************************************************
	Progress.EnterProgressFrame(1, LOCTEXT("CombiningMorphs", "Combining Morphs")); 
*****************************************************************************/

	// Remove undocumented morphs
	for (int NodeIndex = 0; NodeIndex < Scene->GetNodeCount(); ++NodeIndex)
	{
		FbxNode* SceneNode = Scene->GetNode(NodeIndex);
		if (SceneNode == nullptr)
		{
			continue;
		}
		FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(SceneNode->GetMesh());
		if (NodeGeometry)
		{

			const int BlendShapeDeformerCount = NodeGeometry->GetDeformerCount(FbxDeformer::eBlendShape);
			for (int BlendShapeIndex = 0; BlendShapeIndex < BlendShapeDeformerCount; ++BlendShapeIndex)
			{
				FbxBlendShape* BlendShape = (FbxBlendShape*)NodeGeometry->GetDeformer(BlendShapeIndex, FbxDeformer::eBlendShape);
				const int BlendShapeChannelCount = BlendShape->GetBlendShapeChannelCount();

				QList<FbxBlendShapeChannel*> ChannelsToRemove;
				for (int ChannelIndex = 0; ChannelIndex < BlendShapeChannelCount; ++ChannelIndex)
				{
					FbxBlendShapeChannel* Channel = BlendShape->GetBlendShapeChannel(ChannelIndex);
					if (Channel)
					{
						QString ChannelName = QString(Channel->GetNameOnly());
						QString NewChannelName, Extra;
						auto aSplitChannelNames = ChannelName.split(QString("__"));
						Extra = aSplitChannelNames[0];
						NewChannelName = aSplitChannelNames[1];
						if (MorphMappings.contains(NewChannelName))
						{
							NewChannelName = MorphMappings[NewChannelName];
							Channel->SetName(NewChannelName.toLocal8Bit().constData());
						}
						else
						{
							if (!ChannelsToRemove.contains(Channel)) ChannelsToRemove.append(Channel);
						}
					}
				}

				for (FbxBlendShapeChannel* ChannelToRemove : ChannelsToRemove)
				{
					BlendShape->RemoveBlendShapeChannel(ChannelToRemove);
				}
			}
		}
	}

	return true;
}

bool FbxTools::PostProcessRigForUnreal(QString FBXFile, bool bFixTwistBones)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("");
	if (openFBX->LoadScene(pScene, FBXFile.toLocal8Bit().constData()) == false)
	{
		printf("ERROR! Can't load scene: %s\n", FBXFile.toLocal8Bit().constData());
		pScene->Destroy();
		return false;
	}
//	printf("DEBUG: Loaded file: %s\n", FBXFile.toLocal8Bit().constData());
	
	FbxNode* RootNode = pScene->GetRootNode();

	// Find the root bone.  There should only be one bone off the scene root
	FbxNode* RootBone = nullptr;

	RootBone = FindRootBone(RootNode, pScene);
	if (RootBone == NULL) RootBone = AddRootBone(RootNode, pScene);
	QString RootBoneName = QString(RootBone->GetName());

	// Rename Root Bone
	FbxNodeAttribute* pAttr = RootBone->GetNodeAttribute();
	RootBone->SetName("root");
	pAttr->SetName("root");

	// Daz characters sometimes have additional skeletons inside the character for accesories
	FbxTools::MergeFollowerRigs(pScene);
	
	RenameDuplicateBones(RootBone);
	FbxTools::DetachGeometry(pScene, RootNode);

	if (bFixTwistBones)
	{
		FixTwistBones(RootBone);
	}

	FbxTools::UnrealJointFixCallback2 oUnrealJointFixer;
	FbxTools::ModifyBindPose(pScene, RootNode, &oUnrealJointFixer);

	FbxTools::RemoveBindPoses(pScene);
	FbxPose* pTempBindPose = FbxTools::SaveBindMatrixToPose(pScene, "TempBindPose", nullptr, true);
	FbxTools::ApplyBindPose(pScene, pTempBindPose);

	//ProcessMorphs(Scene, CachedSettings, JsonObject);
	
	if (openFBX->SaveScene(pScene, FBXFile.toLocal8Bit().constData()) == false) {
		printf("ERROR! Can't **SAVE** scene: %s\n", FBXFile.toLocal8Bit().constData());
		pScene->Destroy();
		return false;
	}
//	printf("DEBUG: Saved to file: %s\n", FBXFile.toLocal8Bit().constData());

	pScene->Destroy();

	return true;
}

bool FbxTools::PostProcessMaterialsForUnreal(
	QString& FBXFile,
	QString& AssetName,
	QMap<DzMaterial*, DzMaterial*>& DuplicateMaterials,
	QList<QString>& MaterialSlotNames,
	int nCombineMethod)
{

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Process Materials");
	if (openFBX->LoadScene(pScene, FBXFile.toLocal8Bit().constData()) == false)
	{
		pScene->Destroy();
		return false;
	}
	
//	FbxNode* RootNode = pScene->GetRootNode();
//
//	// Find the root bone.  There should only be one bone off the scene root
//	FbxNode* RootBone = nullptr;
//
//	bool bProcessRig = true;
//	if (bProcessRig)
//	{
//		QString RootBoneName;
//		RootBone = FindRootBone(RootBoneName, RootNode, pScene);
//	}

	// Get FBX scene materials
	FbxArray<FbxSurfaceMaterial*> FbxMaterialArray;
	pScene->FillMaterialArray(FbxMaterialArray);

	if (nCombineMethod == 1)
	{
		// Create a mapping of the names of duplicate (identical) materials
		QMap<QString, QString> DuplicateToOriginalName;
		foreach (DzMaterial* DuplicateMaterial, DuplicateMaterials.keys())
		{
			QString DuplicateMaterialName = DuplicateMaterial->getName();
			DzMaterial* OriginalMaterial = DuplicateMaterials[DuplicateMaterial];
			QString OriginalMaterialName = OriginalMaterial->getName();
			DuplicateToOriginalName.insert(DuplicateMaterialName, OriginalMaterialName);
		}

		// Remap FBX Surfaces to remove references to duplicate materials
		QMap<QString, FbxSurfaceMaterial*> MaterialNameToFbxMaterial;
		for (int MaterialIndex = FbxMaterialArray.Size() - 1; MaterialIndex >= 0; --MaterialIndex)
		{
			FbxSurfaceMaterial* Material = FbxMaterialArray[MaterialIndex];
			QString OriginalMaterialName = QString(Material->GetName());
			MaterialNameToFbxMaterial.insert(OriginalMaterialName, Material);
		}

		for (int MeshIndex = pScene->GetGeometryCount() - 1; MeshIndex >= 0; --MeshIndex)
		{
			FbxArray<FbxSurfaceMaterial*> NewMaterialArray;
			FbxGeometry* Geometry = pScene->GetGeometry(MeshIndex);
			FbxNode* GeometryNode = Geometry->GetNode();
			int MaterialCount = GeometryNode->GetMaterialCount();
			for (int AddIndex = 0; AddIndex < MaterialCount; AddIndex++)
			{
				FbxSurfaceMaterial* MaterialToReplace = GeometryNode->GetMaterial(AddIndex);
				QString MaterialToReplaceName = QString(MaterialToReplace->GetName());
				if (DuplicateToOriginalName.contains(MaterialToReplaceName) && MaterialNameToFbxMaterial.contains(DuplicateToOriginalName[MaterialToReplaceName]))
				{
					NewMaterialArray.Add(MaterialNameToFbxMaterial[DuplicateToOriginalName[MaterialToReplaceName]]);
				}
				else
				{
					NewMaterialArray.Add(MaterialToReplace);
				}
			}

			GeometryNode->RemoveAllMaterials();
			for (int AddIndex = 0; AddIndex < MaterialCount; AddIndex++)
			{
				GeometryNode->AddMaterial(NewMaterialArray[AddIndex]);
			}
		}

	}

	FbxArray<FbxSurfaceMaterial*> MaterialsToDelete;

	// Rename Material Slots
	for (int MaterialIndex = FbxMaterialArray.Size() - 1; MaterialIndex >= 0; --MaterialIndex)
	{
		FbxSurfaceMaterial* FbxMaterial = FbxMaterialArray[MaterialIndex];
		QString OriginalMaterialName = QString(FbxMaterial->GetName());
		FbxNode* pMaterialObject = GetObjectForMaterial(FbxMaterial);
		if (!pMaterialObject) {
			printf("ERROR: FbxMaterial %s has no geometry node, removing...\n", FbxMaterial->GetName());
			if (MaterialsToDelete.Find(FbxMaterial) == -1) {
				MaterialsToDelete.Add(FbxMaterial);
			}
			continue;
		}
		QString MaterialObjectName = GetFriendlyObjectName(pMaterialObject);
		if (MaterialObjectName.isEmpty()) {
			printf("ERROR: FbxMaterial %s - Unable to find friendly name, reverting to geometry name: %s\n", FbxMaterial->GetName(), pMaterialObject->GetName());
			MaterialObjectName = QString(pMaterialObject->GetName()).replace(".Shape", "", Qt::CaseInsensitive);
		}

		bool bUseOriginalMaterialName = false;
		QString NewMaterialName;
		if (bUseOriginalMaterialName)
		{
			NewMaterialName = OriginalMaterialName;
		}
		else
		{
			NewMaterialName = MaterialObjectName + TEXT("_") + OriginalMaterialName;
		}

		NewMaterialName = SanitizeName(NewMaterialName);
//		printf("DEBUG: FbxMaterial %s - Renaming to %s\n", FbxMaterial->GetName(), NewMaterialName.toLocal8Bit().constData());
		FbxMaterial->SetName(NewMaterialName.toLocal8Bit().constData());
		MaterialSlotNames.append(NewMaterialName);
	}

	// Remove unused materials
	for (int i=0; i < MaterialsToDelete.GetCount(); i++) {
		FbxSurfaceMaterial* pMaterial = MaterialsToDelete[i];
		if (pMaterial) {
			pScene->RemoveMaterial(pMaterial);
		}
	}

	if (openFBX->SaveScene(pScene, FBXFile.toLocal8Bit().constData()) == false) {
		pScene->Destroy();
		return false;
	}

	pScene->Destroy();

	return true;
}

////////////////////////////////////////////////////////////////////

// Built-in implementation of CustomBoneFix callback for use with Metahuman and Unreal Engine 5.x Mannequin rig conversion process
void FbxTools::UnrealJointFixCallback2::performTask(FbxAMatrix &Matrix, FbxCluster *Cluster, QString sBoneName, FbxDouble3 Rotation)
{
//	printf("DEBUG: UnrealBoneFix2::performTask(): sBoneName=%s....\n", sBoneName.toLocal8Bit().constData());
	
	// Set Base Matrix Rotation
	Matrix.SetR(Rotation);

	// Apply Rotation Modifier based on Rotation Order, Name, etc
	FbxRotationOrder oRotationOrder(Cluster->GetLink()->RotationOrder.Get());
	FbxAMatrix RotationMatrix;
	RotationMatrix.SetIdentity();

	switch (oRotationOrder.GetOrder())
	{
		case FbxEuler::eOrderXYZ:
		case FbxEuler::eOrderXZY:
		{
			// SPECIAL CASE: HANDS AND FINGERS
			bool bIsHand = sBoneName.contains("hand_") ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive);
			bool bIsThumb = sBoneName.contains("thumb_");
			if ( bIsHand && !bIsThumb)
			{
				RotationMatrix.SetIdentity();
				if (sBoneName.contains("_l")) {
					RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
					RotationMatrix.SetRow(1, FbxVector4(0, -1, 0));
					RotationMatrix.SetRow(2, FbxVector4(0, 0, -1));
				}
				break;
			}
			// UPPER EXTREMITIES
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(0, -1, 0));
			}
			else {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(0, 1, 0));
			}
			break;
		}

		case FbxEuler::eOrderYZX:
			// TORSO AND LOWER EXTREMITIES
			if (sBoneName.contains("_r")) {
				// right lower extremity
				RotationMatrix.SetRow(0, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else if (sBoneName.contains("_l")) {
				// left lower extremity and spine
				RotationMatrix.SetRow(0, FbxVector4(0, 1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else {
				RotationMatrix.SetIdentity();
				Matrix.SetRow(0, FbxVector4(0, 1, 0));
				Matrix.SetRow(1, FbxVector4(0, 0, -1));
				Matrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
//			if (sBoneName == "neck_01") {
//				RotationMatrix.SetR(FbxVector4(0, 0, -35));
//			}
//			if (sBoneName == "neck_02") {
//				RotationMatrix.SetR(FbxVector4(0, 0, -20));
//			}
//			if (sBoneName == "head") {
//				RotationMatrix.SetR(FbxVector4(0, 0, -10));
//			}
			break;

		case FbxEuler::eOrderYXZ:
			// UNUSED
			break;
		case FbxEuler::eOrderZXY:
			// EYES
			break;

		case FbxEuler::eOrderZYX:
		{
			// FEET
			// Hardcode feet to be perpendicular to ground like legs (but facing towards toes)
			Matrix.SetR(FbxVector4(0, Rotation[1], 0));
			// SPECIAL CASE: TOES / BALL OF FEET (hardcode to point forward)
			if (sBoneName.contains("ball_")) {
				Matrix.SetR(FbxVector4(90, 0, 0));
			}
			if (sBoneName.contains("_r")) {
				// right foot
				RotationMatrix.SetRow(0, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else {
				// left foot
				RotationMatrix.SetRow(0, FbxVector4(0, 1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			break;
		}

		default:
			break;
	}

	Matrix *= RotationMatrix;

//	printf("Reordering joint: %s\n", sBoneName.toLocal8Bit().constData());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eSourcePivot, oRotationOrder.GetOrder());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderXYZ);

}


// Built-in implementation of CustomBoneFix callback for use with Metahuman and Unreal Engine 5.x Mannequin rig conversion process
void UnrealBoneFix2_performTask_0(FbxAMatrix &Matrix, FbxCluster *Cluster, QString sBoneName, FbxDouble3 Rotation)
{
	printf("DEBUG: UnrealBoneFix2::performTask(): sBoneName=%s....\n", sBoneName.toLocal8Bit().constData());
	
	// Set Base Matrix Rotation
	Matrix.SetR(Rotation);

	// Apply Rotation Modifier based on Rotation Order, Name, etc
	FbxRotationOrder oRotationOrder(Cluster->GetLink()->RotationOrder.Get());
	FbxAMatrix RotationMatrix;
	RotationMatrix.SetIdentity();

	switch (oRotationOrder.GetOrder())
	{
		case FbxEuler::eOrderXYZ:
		case FbxEuler::eOrderXZY:
			// UPPER EXTREMITIES
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetR(FbxVector4(90, 0, 0));
			}
			else {
				RotationMatrix.SetR(FbxVector4(-90, 0, 0));
			}
			// SPECIAL CASE: HANDS
			if (sBoneName.contains("thumb_")) {
				// no op
			}
			else if (sBoneName.contains("hand_") ||
					 FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
					 FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive))
			{
				RotationMatrix.MultRM(FbxVector4(-90, 0, 0));
			}
			break;

		case FbxEuler::eOrderYZX:
			// TORSO AND LOWER EXTREMITIES
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetR(FbxVector4(90, 0, -90));
			}
			else {
				RotationMatrix.SetR(FbxVector4(-90, 0, 90));
			}
			// Hardcodde pelvis
			if (sBoneName.contains("pelvis")) {
				Matrix.SetR(FbxVector4(-90, 0, 90));
				RotationMatrix.SetIdentity();
			}
			break;

		case FbxEuler::eOrderYXZ:
			// UNUSED
			break;
		case FbxEuler::eOrderZXY:
			// EYES
			break;
		case FbxEuler::eOrderZYX:
			// FEET
			// Hardcode feet to start pointing up like legs (but facing towards toes)
			Matrix.SetR(FbxVector4(
				0,
				Rotation[1],
				0)
			);
			// SPECIAL CASE: BALL OF FEET (hardcode to point forward)
			if (sBoneName.contains("ball_")) {
				Matrix.SetR(FbxVector4(90, 0, 0));
			}
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetR(FbxVector4(90, 0, -90));
			}
			else {
				RotationMatrix.SetR(FbxVector4(-90, 0, 90));
			}
			break;
		default:
			break;
	}

	Matrix *= RotationMatrix;

//	printf("Reordering joint: %s\n", sBoneName.toLocal8Bit().constData());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eSourcePivot, oRotationOrder.GetOrder());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderXYZ);

}

// Built-in implementation of CustomBoneFix callback for use with Metahuman and Unreal Engine 5.x Mannequin rig conversion process
void UnrealBoneFix2_performTask_1(FbxAMatrix &Matrix, FbxCluster *Cluster, QString sBoneName, FbxDouble3 Rotation)
{
	printf("DEBUG: UnrealBoneFix2::performTask(): sBoneName=%s....\n", sBoneName.toLocal8Bit().constData());
	
	// Set Base Matrix Rotation
	Matrix.SetR(Rotation);

	// Apply Rotation Modifier based on Rotation Order, Name, etc
	FbxRotationOrder oRotationOrder(Cluster->GetLink()->RotationOrder.Get());
	FbxAMatrix RotationMatrix;
	RotationMatrix.SetIdentity();

	switch (oRotationOrder.GetOrder())
	{
		case FbxEuler::eOrderXYZ:
		case FbxEuler::eOrderXZY:
		{
			// SPECIAL CASE: HANDS AND FINGERS
			bool bIsHand = sBoneName.contains("hand_") ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive);
			bool bIsThumb = sBoneName.contains("thumb_");
			if ( bIsHand && !bIsThumb)
			{
				RotationMatrix.SetIdentity();
				if (sBoneName.contains("_l")) {
					RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
					RotationMatrix.SetRow(1, FbxVector4(0, -1, 0));
					RotationMatrix.SetRow(2, FbxVector4(0, 0, -1));
				}
				break;
			}
			// UPPER EXTREMITIES
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(0, -1, 0));
			}
			else {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(0, 1, 0));
			}
			break;
		}

		case FbxEuler::eOrderYZX:
			// TORSO AND LOWER EXTREMITIES
			if (sBoneName.contains("_r")) {
				// right lower extremity
				RotationMatrix.SetRow(0, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else {
				// left lower extremity and spine
				RotationMatrix.SetRow(0, FbxVector4(0, 1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			break;

		case FbxEuler::eOrderYXZ:
			// UNUSED
			break;
		case FbxEuler::eOrderZXY:
			// EYES
			break;

		case FbxEuler::eOrderZYX:
		{
			// FEET
			// Hardcode feet to be perpendicular to ground like legs (but facing towards toes)
			Matrix.SetR(FbxVector4(0, Rotation[1], 0));
			// SPECIAL CASE: TOES / BALL OF FEET (hardcode to point forward)
			if (sBoneName.contains("ball_")) {
				Matrix.SetR(FbxVector4(90, 0, 0));
			}
			if (sBoneName.contains("_r")) {
				// right foot
				RotationMatrix.SetRow(0, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else {
				// left foot
				RotationMatrix.SetRow(0, FbxVector4(0, 1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			break;
		}

		default:
			break;
	}

	Matrix *= RotationMatrix;

//	printf("Reordering joint: %s\n", sBoneName.toLocal8Bit().constData());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eSourcePivot, oRotationOrder.GetOrder());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderXYZ);

}

// Built-in implementation of CustomBoneFix callback for use with Metahuman and Unreal Engine 5.x Mannequin rig conversion process
void UnrealBoneFix2_performTask3_broken(FbxAMatrix &Matrix, FbxCluster *Cluster, QString sBoneName, FbxDouble3 Rotation)
{
	printf("DEBUG: UnrealBoneFix2::performTask(): sBoneName=%s....\n", sBoneName.toLocal8Bit().constData());
	
	// Set Base Matrix Rotation
	Matrix.SetR(Rotation);

	// Apply Rotation Modifier based on Rotation Order, Name, etc
	FbxRotationOrder oRotationOrder(Cluster->GetLink()->RotationOrder.Get());
	FbxAMatrix RotationMatrix;
	RotationMatrix.SetIdentity();

	switch (oRotationOrder.GetOrder())
	{
		case FbxEuler::eOrderXYZ:
		case FbxEuler::eOrderXZY:
		{
			// SPECIAL CASE: HANDS AND FINGERS
			bool bIsHand = sBoneName.contains("hand_") ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
			FbxTools::HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive);
			bool bIsThumb = sBoneName.contains("thumb_");
			if ( bIsHand && !bIsThumb)
			{
				RotationMatrix.SetIdentity();
				if (sBoneName.contains("_l")) {
					RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
					RotationMatrix.SetRow(1, FbxVector4(0, -1, 0));
					RotationMatrix.SetRow(2, FbxVector4(0, 0, -1));
				}
				break;
			}
			// UPPER EXTREMITIES
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(0, -1, 0));
			}
			else {
				RotationMatrix.SetRow(0, FbxVector4(1, 0, 0));				
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(0, 1, 0));
			}
			break;
		}

		case FbxEuler::eOrderYZX:
			// TORSO AND LOWER EXTREMITIES
			Matrix.SetRow(0, FbxVector4(0, Rotation[1], 0));
			Matrix.SetRow(1, FbxVector4(0, 0, -1));
			Matrix.SetRow(2, FbxVector4(-1, 0, 0));
			if (sBoneName.contains("_r")) {
				RotationMatrix.SetRow(0, FbxVector4(-1, 0, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(2, FbxVector4(0, 0, 1));
			}
			break;

		case FbxEuler::eOrderYXZ:
			// UNUSED
			break;
		case FbxEuler::eOrderZXY:
			// EYES
			break;

		case FbxEuler::eOrderZYX:
		{
			// FEET
			// Hardcode feet to be perpendicular to ground like legs (but facing towards toes)
			Matrix.SetR(FbxVector4(0, Rotation[1], 0));
			// SPECIAL CASE: TOES / BALL OF FEET (hardcode to point forward)
			if (sBoneName.contains("ball_")) {
				Matrix.SetR(FbxVector4(90, 0, 0));
			}
			if (sBoneName.contains("_r")) {
				// right foot
				RotationMatrix.SetRow(0, FbxVector4(0, -1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, 1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			else {
				// left foot
				RotationMatrix.SetRow(0, FbxVector4(0, 1, 0));
				RotationMatrix.SetRow(1, FbxVector4(0, 0, -1));
				RotationMatrix.SetRow(2, FbxVector4(-1, 0, 0));
			}
			break;
		}

		default:
			break;
	}

	Matrix *= RotationMatrix;

//	printf("Reordering joint: %s\n", sBoneName.toLocal8Bit().constData());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eSourcePivot, oRotationOrder.GetOrder());
//	Cluster->GetLink()->SetRotationOrder(FbxNode::eDestinationPivot, FbxEuler::eOrderXYZ);

}

bool DeepCopyNode(FbxNode* pDestinationRoot, FbxNode* pSourceNode)
{
	// get children
	if (!pSourceNode)
	{
		dzApp->log("DeepCopyNode: pSourceNode is null.");
		return false;
	}
	if (!pDestinationRoot)
	{
		dzApp->log("DeepCopyNode: pDestinationRoot is null.");
		return false;
	}

	std::vector<FbxNode*> lChildren;
	int lNumChildren = pSourceNode->GetChildCount();
	int debug_pdestinationroot_numchildren = pDestinationRoot->GetChildCount();
	for (int i = 0; i < lNumChildren; i++) {
		lChildren.push_back(pSourceNode->GetChild(i));
	}
	int debug_lchildren_size = (int) lChildren.size();
	for (int c = 0; c < lChildren.size(); c++)
		pDestinationRoot->AddChild(lChildren[c]);

	int debug_pdestinationroot_numchildren_2 = pDestinationRoot->GetChildCount();

	return true;
}

bool FbxTools::MergeScenes(FbxScene* pDestinationScene, FbxScene* pSourceScene)
{

	DeepCopyNode(pDestinationScene->GetRootNode(), pSourceScene->GetRootNode());

	pSourceScene->GetRootNode()->DisconnectAllSrcObject();

	int lNumSceneObjects = pSourceScene->GetSrcObjectCount();
	for (int i = 0; i < lNumSceneObjects; i++) {
		FbxObject* lObj = pSourceScene->GetSrcObject(i);
		if (lObj == pSourceScene->GetRootNode() || *lObj == pSourceScene->GetGlobalSettings()) {
			// Don't move the root node or the scene's global settings; these
			// objects are created for every scene.
			continue;
		}
		/*************************/
		// DEBUG
		FbxObject* pObjGlobalSettings = &pSourceScene->GetGlobalSettings();
		QString globalSettingsName = QString(pObjGlobalSettings->GetName());
		QString objName = QString(lObj->GetName());
		if (objName == "GlobalSettings")
			continue;
		if (lObj->GetClassId() == FbxAnimCurveNode::ClassId ||
			lObj->GetClassId() == FbxAnimCurve::ClassId ||
			lObj->GetClassId() == FbxAnimLayer::ClassId ||
			lObj->GetClassId() == FbxAnimStack::ClassId ||
			lObj->GetClassId() == FbxAnimEvalClassic::ClassId ||
			lObj->GetClassId() == FbxSkeleton::ClassId)
		{
//            printf("DEBUG: skipping FbxAnimCurve, FbxAnimCurveNode");
			continue;
		}
		FbxClassId classID = lObj->GetClassId();
		QString className = classID.GetName();
//        dzApp->log("FbxTools::MergeScenes() DEBUG: adding object=" + objName + " [" + className + "]  to destination scene.");
		/*************************/

		// Attach the object to the reference scene.
		lObj->ConnectDstObject(pDestinationScene);

	}

	pSourceScene->DisconnectAllSrcObject();

	return true;
}

bool FbxTools::RemoveAllPoses(FbxScene* pScene)
{
	if (pScene == nullptr) return false;

	int numPoses = pScene->GetPoseCount();
	for (int nPoseIndex = numPoses - 1; nPoseIndex >= 0; nPoseIndex--)
	{
		FbxPose* pPose = pScene->GetPose(nPoseIndex);
		if (pScene->RemovePose(nPoseIndex) == false) {
			return false;
		}
		pPose->Destroy();
	}

	return true;
}

FbxPose* FbxTools::SaveCurrentPose(FbxScene* pScene, FbxNode* pRootNode, FbxPose* pCurrentPose)
{
	if (pScene == nullptr || pRootNode == nullptr) return nullptr;

	if (pCurrentPose == nullptr) {
		pCurrentPose = FbxPose::Create(pScene->GetFbxManager(), "New Pose");
	}
	pCurrentPose->Add(pRootNode, pRootNode->EvaluateGlobalTransform());

	for (int i = 0; i < pRootNode->GetChildCount(); i++)
	{
		FbxNode* pChildNode = pRootNode->GetChild(i);
		SaveCurrentPose(pScene, pChildNode, pCurrentPose);
	}

	return pCurrentPose;
}

#include "dzbone.h"
#include "dzfloatproperty.h"
void exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale, bool bFixTwistBones=true)
{
	DzTimeRange PlayRange = dzScene->getPlayRange();

	DzProgress exportProgress = DzProgress("DazBridge: Exporting Animation", PlayRange.getEnd() - PlayRange.getStart(), false, true);
	exportProgress.setCloseOnFinish(true);
	exportProgress.enable(true);
	exportProgress.step();
	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

	// Get a tick size for the progress bar
	int progressTickSize = (PlayRange.getEnd() - PlayRange.getStart() + 50) / 50;

	QString Name = Bone->getName();

	FbxNode* Node = BoneMap.value(Bone);
	if (Node == nullptr) return;

	//qDebug() << Bone->getName() << " Order: " << Bone->getRotationOrder().toString();

	// Create a curve node for this bone
	FbxAnimCurveNode* AnimCurveNode = Node->LclRotation.GetCurveNode(AnimBaseLayer, true);

	// For each frame, write a key (equivalent of bake)
	for (DzTime CurrentTime = PlayRange.getStart(); CurrentTime <= PlayRange.getEnd(); CurrentTime += dzScene->getTimeStep())
	{
		DzTime Frame = CurrentTime / dzScene->getTimeStep();

		// Need this for the UI to update, but it's very slow, so run every 100th frame.
		if (Frame % progressTickSize == 0)
		{
			exportProgress.update(Frame);
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

		DzVec3 DefaultPosition;
		DefaultPosition.m_x = Bone->getOriginXControl()->getValue(CurrentTime);
		DefaultPosition.m_y = Bone->getOriginYControl()->getValue(CurrentTime);
		DefaultPosition.m_z = Bone->getOriginZControl()->getValue(CurrentTime);
		DzMatrix3 Scale = Bone->getWSScale();
		//qDebug() << Bone->getName() << " Scale: " << Scale.row(0).length() << "," << Scale.row(1).length() << "," << Scale.row(2).length();

		//qDebug() << Bone->getName() << " Default Position: " << DefaultPosition.m_x << "," << DefaultPosition.m_y << "," << DefaultPosition.m_z;
		DzVec3 Position;
		Position.m_x = Bone->getXPosControl()->getValue(CurrentTime);
		Position.m_y = Bone->getYPosControl()->getValue(CurrentTime);
		Position.m_z = Bone->getZPosControl()->getValue(CurrentTime);
		//qDebug() << Bone->getName() << " Position: " << Position.m_x << "," << Position.m_y << "," << Position.m_z;

		// Get an initial rotation via the controls
		DzVec3 ControlRotation;
		ControlRotation.m_x = Bone->getXRotControl()->getValue(CurrentTime);
		ControlRotation.m_y = Bone->getYRotControl()->getValue(CurrentTime);
		ControlRotation.m_z = Bone->getZRotControl()->getValue(CurrentTime);

		// If fixing twist bones, add any child twist bone rotations
		if (bFixTwistBones)
		{
			// Looks through the child nodes for more bones
			for (int ChildIndex = 0; ChildIndex < Bone->getNumNodeChildren(); ChildIndex++)
			{
				DzNode* ChildNode = Bone->getNodeChild(ChildIndex);
				if (ChildNode->getName().contains("twist", Qt::CaseInsensitive))
				{
					if (DzBone* ChildBone = qobject_cast<DzBone*>(ChildNode))
					{
						ControlRotation.m_x += ChildBone->getXRotControl()->getValue(CurrentTime);
						ControlRotation.m_y += ChildBone->getYRotControl()->getValue(CurrentTime);
						ControlRotation.m_z += ChildBone->getZRotControl()->getValue(CurrentTime);
					}
				}
			}

			// If this is a twist bone, zero it's rotation
			if (Bone->getName().contains("twist", Qt::CaseInsensitive))
			{
				ControlRotation.m_x = 0.0f;
				ControlRotation.m_y = 0.0f;
				ControlRotation.m_z = 0.0f;
			}
		}

		DzVec3 VectorRotation = ControlRotation;

		// Scale
		DzVec3 ControlScale(1.0f, 1.0f, 1.0f);
		//float FigureScale = 1.0f;
		if (bool bAnimationApplyBoneScale = false)
		{
			//DzSkeleton* Skeleton = m_pSelectedNode->getSkeleton();
			//DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;
			FigureScale = Bone->getScaleControl()->getValue(CurrentTime);

			ControlScale.m_x = Bone->getXScaleControl()->getValue(CurrentTime) * FigureScale;
			ControlScale.m_y = Bone->getYScaleControl()->getValue(CurrentTime) * FigureScale;
			ControlScale.m_z = Bone->getZScaleControl()->getValue(CurrentTime) * FigureScale;

			//DzMatrix3 Scale = Bone->getLocalScale(CurrentTime);
			//qDebug() << Bone->getName() << " Scale: " << ControlScale.m_x << "," << ControlScale.m_y << "," << ControlScale.m_z;
		}

		// Get the rotation and position relative to the parent
		if (DzNode* ParentBone = Bone->getNodeParent())
		{

			// Get the local orientation
			DzQuat Orientation = Bone->getOrientation(true) * ParentBone->getOrientation(true).inverse();

			// Fix the rotation order
			VectorRotation = ControlRotation;
			DzQuat ReorderQuat;
			VectorRotation.m_x = VectorRotation.m_x / FBXSDK_180_DIV_PI;
			VectorRotation.m_y = VectorRotation.m_y / FBXSDK_180_DIV_PI;
			VectorRotation.m_z = VectorRotation.m_z / FBXSDK_180_DIV_PI;
			//qDebug() << Bone->getName() << " ControlRot: " << VectorRotation.m_x << "," << VectorRotation.m_y << "," << VectorRotation.m_z;
			ReorderQuat.setValue(Bone->getRotationOrder().order(), VectorRotation);
			ReorderQuat = ReorderQuat * Orientation;

			ReorderQuat.getValue(DzRotationOrder::RotOrder::XYZ, VectorRotation);
			VectorRotation.m_x = VectorRotation.m_x * FBXSDK_180_DIV_PI;
			VectorRotation.m_y = VectorRotation.m_y * FBXSDK_180_DIV_PI;
			VectorRotation.m_z = VectorRotation.m_z * FBXSDK_180_DIV_PI;

			//qDebug() << Bone->getName() << " Reorder LocalRot: " << VectorRotation.m_x << "," << VectorRotation.m_y << "," << VectorRotation.m_z;

			//qDebug() << Bone->getName() << " Parent Default Position: " << DefaultParentPosition.m_x << "," << DefaultParentPosition.m_y << "," << DefaultParentPosition.m_z;
			DzVec3 ParentPosition;
			ParentPosition.m_x = ParentBone->getXPosControl()->getValue(CurrentTime);
			ParentPosition.m_y = ParentBone->getYPosControl()->getValue(CurrentTime);
			ParentPosition.m_z = ParentBone->getZPosControl()->getValue(CurrentTime);
			//qDebug() << Bone->getName() << " Parent Position: " << ParentPosition.m_x << "," << ParentPosition.m_y << "," << ParentPosition.m_z;

			DzVec3 DefaultParentPosition;
			DefaultParentPosition.m_x = ParentBone->getOriginXControl()->getValue(CurrentTime);
			DefaultParentPosition.m_y = ParentBone->getOriginYControl()->getValue(CurrentTime);
			DefaultParentPosition.m_z = ParentBone->getOriginZControl()->getValue(CurrentTime);
			//qDebug() << Bone->getName() << " Parent Default Position: " << DefaultParentPosition.m_x << "," << DefaultParentPosition.m_y << "," << DefaultParentPosition.m_z;

			DzVec3 RelativeDefaultPosition = DefaultPosition - DefaultParentPosition;
			//float Length = RelativeDefaultPosition.length();
			DzVec3 OrientedRelativeDefaultPosition = ParentBone->getOrientation(true).inverse().multVec(RelativeDefaultPosition);

			//qDebug() << Bone->getName() << " RelativeDefaultPosition: " << OrientedRelativeDefaultPosition.m_x << "," << OrientedRelativeDefaultPosition.m_y << "," << OrientedRelativeDefaultPosition.m_z;
			DzVec3 RelativeMovement = Position - ParentPosition;
			//qDebug() << Bone->getName() << " RelativeMovement: " << RelativeMovement.m_x << "," << RelativeMovement.m_y << "," << RelativeMovement.m_z;
			//qDebug() << Bone->getName() << " Position: " << Position.m_x << "," << Position.m_y << "," << Position.m_z;
			if (ParentBone->isRootNode())
			{
				Position = (Position + OrientedRelativeDefaultPosition) * FigureScale;
			}
			else
			{
				Position = OrientedRelativeDefaultPosition + RelativeMovement;
			}
		}

		// Set the frame
		FbxTime Time;
		int KeyIndex = 0;
		Time.SetFrame(Frame);

		// Write X Rot
		FbxAnimCurve* RotXCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "X", true);
		RotXCurve->KeyModifyBegin();
		KeyIndex = RotXCurve->KeyAdd(Time);
		RotXCurve->KeySet(KeyIndex, Time, VectorRotation.m_x);
		RotXCurve->KeyModifyEnd();

		// Write Y Rot
		FbxAnimCurve* RotYCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "Y", true);
		RotYCurve->KeyModifyBegin();
		KeyIndex = RotYCurve->KeyAdd(Time);
		RotYCurve->KeySet(KeyIndex, Time, VectorRotation.m_y);
		RotYCurve->KeyModifyEnd();

		// Write Z Rot
		FbxAnimCurve* RotZCurve = Node->LclRotation.GetCurve(AnimBaseLayer, "Z", true);
		RotZCurve->KeyModifyBegin();
		KeyIndex = RotZCurve->KeyAdd(Time);
		RotZCurve->KeySet(KeyIndex, Time, VectorRotation.m_z);
		RotZCurve->KeyModifyEnd();

		// Write X Pos
		FbxAnimCurve* PosXCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "X", true);
		PosXCurve->KeyModifyBegin();
		KeyIndex = PosXCurve->KeyAdd(Time);
		PosXCurve->KeySet(KeyIndex, Time, Position.m_x);
		PosXCurve->KeyModifyEnd();

		// Write Y Pos
		FbxAnimCurve* PosYCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "Y", true);
		PosYCurve->KeyModifyBegin();
		KeyIndex = PosYCurve->KeyAdd(Time);
		PosYCurve->KeySet(KeyIndex, Time, Position.m_y);
		PosYCurve->KeyModifyEnd();

		// Write Z Pos
		FbxAnimCurve* PosZCurve = Node->LclTranslation.GetCurve(AnimBaseLayer, "Z", true);
		PosZCurve->KeyModifyBegin();
		KeyIndex = PosZCurve->KeyAdd(Time);
		PosZCurve->KeySet(KeyIndex, Time, Position.m_z);
		PosZCurve->KeyModifyEnd();

		// Write X Scale
		FbxAnimCurve* ScaleXCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "X", true);
		ScaleXCurve->KeyModifyBegin();
		KeyIndex = ScaleXCurve->KeyAdd(Time);
		ScaleXCurve->KeySet(KeyIndex, Time, ControlScale.m_x);
		ScaleXCurve->KeyModifyEnd();

		// Write Y Scale
		FbxAnimCurve* ScaleYCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "Y", true);
		ScaleYCurve->KeyModifyBegin();
		KeyIndex = ScaleYCurve->KeyAdd(Time);
		ScaleYCurve->KeySet(KeyIndex, Time, ControlScale.m_y);
		ScaleYCurve->KeyModifyEnd();

		// Write Z Scale
		FbxAnimCurve* ScaleZCurve = Node->LclScaling.GetCurve(AnimBaseLayer, "Z", true);
		ScaleZCurve->KeyModifyBegin();
		KeyIndex = ScaleZCurve->KeyAdd(Time);
		ScaleZCurve->KeySet(KeyIndex, Time, ControlScale.m_z);
		ScaleZCurve->KeyModifyEnd();
	}
}


#include "dzfigure.h"
#include "dzproperty.h"
#include "dzfloatproperty.h"
bool FbxTools::ExportAnimation(DzNode* pNode, QString sFilename, bool bIncludeFaceBones, bool bFixTwistBones)
{
	if (!pNode) return false;

	DzSkeleton* Skeleton = pNode->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;

	if (!Figure) return false;

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Animation Scene");
	
	// Get the Figure Scale
	float FigureScale = pNode->getScaleControl()->getValue();

	FbxAnimStack* AnimStack = FbxAnimStack::Create(pScene, "AnimStack");
	FbxAnimLayer* AnimBaseLayer = FbxAnimLayer::Create(pScene, "Layer0");
	AnimStack->AddMember(AnimBaseLayer);

	// Add the skeleton to the scene
	QMap<DzNode*, FbxNode*> BoneMap;
	GenerateSkeleton(Figure, pNode, nullptr, nullptr, pScene, BoneMap, bIncludeFaceBones, bFixTwistBones);

	// Get the play range
	DzTimeRange PlayRange = dzScene->getPlayRange();

	// Root Node
	exportNodeAnimation(Figure, BoneMap, AnimBaseLayer, FigureScale /*, bExportingForMLDeformer*/);

	// Iterate the bones
	DzBoneList Bones; // = getAllBones(pNode);
	Skeleton->getAllBones(Bones);
	for (auto Bone : Bones)
	{
//		exportNodeAnimation(Bone, BoneMap, AnimBaseLayer, FigureScale /*, bExportingForMLDeformer*/);
	}

	// Get a list of animated properties
//	if (m_bAnimationExportActiveCurves)
	{
		QList<DzNumericProperty*> animatedProperties; // = getAnimatedProperties(pNode);
//		exportAnimatedProperties(animatedProperties, Scene, AnimBaseLayer);
	}

	bool bAsciiMode = false;
#if VODSVERSION
	bAsciiMode = true;
#endif
	bool bSaveResult = openFBX->SaveScene(pScene, sFilename, bAsciiMode);
	
	return bSaveResult;
}

bool FbxTools::ExportSkeleton(DzNode* pNode, QString sFilename, bool bIncludeFaceBones, bool bFixTwistBones)
{
	if (!pNode) return false;

	DzSkeleton* Skeleton = pNode->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;

	if (!Figure) return false;

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Animation Scene");
	
	// Add the skeleton to the scene
	QMap<DzNode*, FbxNode*> BoneMap;
	GenerateSkeleton(Figure, pNode, nullptr, nullptr, pScene, BoneMap, bIncludeFaceBones, bFixTwistBones);

	bool bAsciiMode = false;
	bool bSaveResult = openFBX->SaveScene(pScene, sFilename, bAsciiMode);

	pScene->Destroy();
	return bSaveResult;
}

#include "dzbone.h"
#include "dzrotationorder.h"
void FbxTools::GenerateSkeleton(DzFigure* pFigure, DzNode* pDazNode, DzNode* pDazParent, FbxNode* pFbxParent, FbxScene* pScene, QMap<DzNode*, FbxNode*>& oBoneMap, bool bIncludeFaceBones, bool bFixTwistBones)
{
	// Only transfer face bones if requested.  MLDeformer doesn't like missing bones in UE5.3 and earlier
	if (pDazParent != nullptr && pDazParent->getName() == "head" && bIncludeFaceBones == false) return;

	FbxNode* pFbxBone;

	// null parent is the root bone
	if (pFbxParent == nullptr)
	{
		// Create a root bone.  Always named root so we don't have to fix it in Unreal
		FbxSkeleton* pSkeletonAttribute = FbxSkeleton::Create(pScene, "root");
		pSkeletonAttribute->SetSkeletonType(FbxSkeleton::eRoot);
		pFbxBone = FbxNode::Create(pScene, "root");
		pFbxBone->SetNodeAttribute(pSkeletonAttribute);

		FbxNode* pRootNode = pScene->GetRootNode();
		pRootNode->AddChild(pFbxBone);

		// Looks through the child nodes for more bones
		for (int nChildIndex = 0; nChildIndex < pDazNode->getNumNodeChildren(); nChildIndex++)
		{
			DzNode* pDazChild = pDazNode->getNodeChild(nChildIndex);
			GenerateSkeleton(pFigure, pDazChild, pDazNode, pFbxBone, pScene, oBoneMap, bIncludeFaceBones, bFixTwistBones);
		}
	}
	else
	{
		// Child nodes need to be bones
		if (DzBone* pDazBone = qobject_cast<DzBone*>(pDazNode))
		{
			// create the bone
			FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(pScene, pDazBone->getName().toUtf8().data());
			SkeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			pFbxBone = FbxNode::Create(pScene, pDazBone->getName().toUtf8().data());
			pFbxBone->SetNodeAttribute(SkeletonAttribute);

			DzRotationOrder oRotOrder = pDazBone->getRotationOrder();
			switch (oRotOrder.order())
			{
				case DzRotationOrder::RotOrder::XYZ:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderXYZ);
					break;
				case DzRotationOrder::RotOrder::XZY:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderXZY);
					break;
				case DzRotationOrder::RotOrder::YXZ:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderYXZ);
					break;
				case DzRotationOrder::RotOrder::YZX:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderYZX);
					break;
				case DzRotationOrder::RotOrder::ZXY:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderZXY);
					break;
				case DzRotationOrder::RotOrder::ZYX:
					pFbxBone->SetRotationOrder(FbxNode::eSourcePivot, FbxEuler::EOrder::eOrderZYX);
					break;
				default:
					break;
			}

			// find the bones position
			DzVec3 Position = pDazBone->getWSPos(DzTime(0), false);
			DzVec3 ParentPosition = pDazParent->getWSPos(DzTime(0), false);
			DzVec3 LocalPosition = Position - ParentPosition;

			// find the bone's rotation
			DzQuat Rotation = pDazBone->getWSRot(DzTime(0), false);
			DzQuat ParentRotation = pDazParent->getWSRot(DzTime(0), false);
			DzQuat LocalRotation = Rotation * ParentRotation.inverse();
			DzVec3 VectorRotation;
			LocalRotation.getValue(VectorRotation);

			// set the position and rotation properties
			pFbxBone->LclTranslation.Set(FbxVector4(LocalPosition.m_x, LocalPosition.m_y, LocalPosition.m_z));
			pFbxBone->LclRotation.Set(FbxVector4(VectorRotation.m_x, VectorRotation.m_y, VectorRotation.m_z));

			// if fixing twist bones, reparent their children
			if (bFixTwistBones && pDazBone->getNodeParent() != nullptr && pDazBone->getNodeParent()->getName().contains("twist", Qt::CaseInsensitive)) {
				pFbxParent->GetParent()->AddChild(pFbxBone);
			} else {
				pFbxParent->AddChild(pFbxBone);
			}

			// Looks through the child nodes for more bones
			QList<QString> DirectChildBones;
			for (int nChildIndex = 0; nChildIndex < pDazBone->getNumNodeChildren(); nChildIndex++)
			{
				DzNode* pChildNode = pDazBone->getNodeChild(nChildIndex);
				if (pChildNode && pChildNode->inherits("DzBone"))
				{
					DirectChildBones.append(pChildNode->getName());
				}
				GenerateSkeleton(pFigure, pChildNode, pDazBone, pFbxBone, pScene, oBoneMap, bIncludeFaceBones, bFixTwistBones);
			}

			// Add child figure bones
			for (int nChildFigureIndex = 0; nChildFigureIndex < pFigure->getNumNodeChildren(); nChildFigureIndex++)
			{
				DzNode* pTempPointer = pFigure->getNodeChild(nChildFigureIndex);
				if (DzFigure* pChildFigure = qobject_cast<DzFigure*>(pTempPointer))
				{
					// Find matching parent bone in child figures
					if (DzNode* pChildFigureMatchingParentBone = pChildFigure->findBone(pDazBone->getName()))
					{
						// Look for new child bones
						for (int nChildBoneIndex = 0; nChildBoneIndex < pChildFigureMatchingParentBone->getNumNodeChildren(); nChildBoneIndex++)
						{
							DzNode* pTempPointer = pChildFigureMatchingParentBone->getNodeChild(nChildBoneIndex);
							if (DzBone* pChildBone = qobject_cast<DzBone*>(pTempPointer))
							{
								if (!DirectChildBones.contains(pChildBone->getName()))
								{
									DirectChildBones.append(pChildBone->getName());
									GenerateSkeleton(pFigure, pChildBone, pDazBone, pFbxBone, pScene, oBoneMap, bIncludeFaceBones, bFixTwistBones);
//									printf("DEBUG: Found Extra Bone: %s\n", pChildBone->getName().toLocal8Bit().constData());
								}
							}
						}
					}
				}
			}
		}
	}

	// Add the bone to the map
	oBoneMap.insert(pDazNode, pFbxBone);
}


#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

QMap<QString, QVariant> readJsonToMap(const QString& sFilename)
{
	QFile oFile(sFilename);
	if (!oFile.open(QIODevice::ReadOnly))
		return {};

	const QByteArray aData = oFile.readAll();
	oFile.close();

	const QString sJson = QString::fromUtf8(aData);

	QScriptEngine oEngine;
	// Wrap in parentheses so it's parsed as an expression, not a block
	QScriptValue oVal = oEngine.evaluate("(" + sJson + ")");

	if (oEngine.hasUncaughtException() || !oVal.isObject())
		return {};

	QVariant oVar = oVal.toVariant();               // nested objects → QVariantMap, arrays → QVariantList
	if (oVar.type() == QVariant::Map)
		return oVar.toMap();                         // typedef of QMap<QString,QVariant>
	return {};
}

bool FbxTools::ProxyMeshBoneRenamer(QString sProxyFbxFilename, QString sRigConversionJsonFilename)
{
	
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Animation Scene");
	bool bLoadResult = FbxTools::ExLoadScene(pScene, sProxyFbxFilename);
	if (!bLoadResult) {
		return false;
	}

	QMap<QString, FbxNode*> oBoneMap;
	for (int i=0; i < pScene->GetNodeCount(); i++) {
		FbxNode* pNode = pScene->GetNode(i);
		FbxNodeAttribute* pAttr = pNode->GetNodeAttribute();
		if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
			QString sNodeName(pNode->GetName());
			oBoneMap.insert(sNodeName, pNode);
		}
	}
	
	QList<FbxNode*> aBonesToDelete;
	QMap<QString, QVariant> oRigConversionDictionary = readJsonToMap(sRigConversionJsonFilename);
	QMap<QString, QString> oReverseLookup;
	
	foreach(QString sKey, oRigConversionDictionary.keys()) {
		QVariant oValue = oRigConversionDictionary.value(sKey);
		oReverseLookup.insert(oValue.toString(), sKey);
	}

	foreach(QString sKey, oRigConversionDictionary.keys()) {
		FbxNode* pFbxNode = oBoneMap.value(sKey);
		if (pFbxNode == nullptr) continue;
		QVariant oValue = oRigConversionDictionary.value(sKey);
		QString sValue = oValue.toString();
		if (oValue.type() == QVariant::Type::String) {
			if (sValue == sKey) continue;
			if (oReverseLookup.contains(sValue)) {
				// rename original bone
//				FbxNode* pOriginalBone = pScene->FindNodeByName(sValue.toLocal8Bit().constData());
				FbxNode* pOriginalBone = oBoneMap.value(sValue);
				if (pOriginalBone) {
					QString sNewName = sValue + "__original";
					pOriginalBone->SetName(sNewName.toLocal8Bit().constData());
				}
			}
			printf("ProxyMeshBoneRenamer: Renaming %s to %s\n", pFbxNode->GetName(), sValue.toLocal8Bit().constData());
			pFbxNode->SetName(sValue.toLocal8Bit().constData());
		}
		else if (oValue.type() == QVariant::Type::Int) {
			if (oValue.toInt() == -1) {
				printf("ProxyMeshBoneRenamer: marking for deletion: %s\n", pFbxNode->GetName());
				aBonesToDelete.append(pFbxNode);				
			}
		}
	}

	foreach (FbxNode* pBoneToDelete, aBonesToDelete)
	{
		if (pBoneToDelete == nullptr) continue;
		// reparent
		FbxNode* pParent = pBoneToDelete->GetParent();
		if (pParent) {
			int numChildren = pBoneToDelete->GetChildCount();
			for (int i=numChildren; i >= 0 ; i--) {
				FbxNode* pChild = pBoneToDelete->GetChild(i);
				if (pChild == nullptr) continue;
				printf("ProxyMeshBoneRenamer: reparenting child: %s\n", pChild->GetName());
				pParent->AddChild(pChild);
			}
		}
		// remove bone
		printf("ProxyMeshBoneRenamer: deleting %s\n", pBoneToDelete->GetName());
		pScene->RemoveNode(pBoneToDelete);
	}
	
	bool bAsciiMode = false;
	bool bSaveResult = openFBX->SaveScene(pScene, sProxyFbxFilename, bAsciiMode);

	pScene->Destroy();
	
	return bSaveResult;
}

bool FbxTools::GetBoneList(FbxNode* pRootNode, QList<FbxNode*> &aBoneList )
{
	if (pRootNode == nullptr) return false;

	QList<FbxNode*> aTodoList;
	aTodoList.push_back(pRootNode);

	while (aTodoList.isEmpty() == false) 
	{
		FbxNode* pNode = aTodoList.front(); aTodoList.pop_front();
		if (pNode == nullptr) continue;
		FbxNodeAttribute* pAttr = pNode->GetNodeAttribute();
		if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
			QString sNodeName(pNode->GetName());
			if (aBoneList.contains(pNode) == false) {
				aBoneList.append(pNode);
			}
		}
		for (int i = 0; i < pNode->GetChildCount(); i++) {
			FbxNode* pChild = pNode->GetChild(i);
			aTodoList.push_back(pChild);
		}
	}

	return true;
}

bool FbxTools::AddRotationCurve(FbxNode* pNode, FbxAnimLayer* pAnimLayer, FbxTime oTime, const char *pChannel, float fValue, bool bCreate)
{
	if (pNode == nullptr || pAnimLayer == nullptr) return false;

	FbxAnimCurve* pAnimCurve = pNode->LclRotation.GetCurve(pAnimLayer, pChannel, bCreate);
	pAnimCurve->KeyModifyBegin();
	int nKeyIndex = pAnimCurve->KeyAdd(oTime);
	pAnimCurve->KeySet(nKeyIndex, oTime, fValue);

	return true;
}

bool FbxTools::AddTranslationCurve(FbxNode* pNode, FbxAnimLayer* pAnimLayer, FbxTime oTime, const char* pChannel, float fValue, bool bCreate)
{
	if (pNode == nullptr || pAnimLayer == nullptr) return false;

	FbxAnimCurve* pAnimCurve = pNode->LclTranslation.GetCurve(pAnimLayer, pChannel, bCreate);
	pAnimCurve->KeyModifyBegin();
	int nKeyIndex = pAnimCurve->KeyAdd(oTime);
	pAnimCurve->KeySet(nKeyIndex, oTime, fValue);

	return true;
}

bool FbxTools::AddKeyCurrentNode(FbxNode* pNode, FbxAnimLayer* pAnimLayer, FbxTime oTime)
{
	if (pNode == nullptr || pAnimLayer == nullptr) return false;

	FbxVector4 oPosition = pNode->LclTranslation.Get();
	FbxVector4 oRotation = pNode->LclRotation.Get();

	AddTranslationCurve(pNode, pAnimLayer, oTime, "X", oPosition[0], true);
	AddTranslationCurve(pNode, pAnimLayer, oTime, "Y", oPosition[1], true);
	AddTranslationCurve(pNode, pAnimLayer, oTime, "Z", oPosition[2], true);

	AddRotationCurve(pNode, pAnimLayer, oTime, "X", oRotation[0], true);
	AddRotationCurve(pNode, pAnimLayer, oTime, "Y", oRotation[1], true);
	AddRotationCurve(pNode, pAnimLayer, oTime, "Z", oRotation[2], true);

	return true;
}