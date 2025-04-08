
#include "dzprogress.h"
#include "OpenFBXInterface.h"
#include "FbxTools.h"

#include <fbxsdk.h>
#include <qlist.h>
#include <qmap.h>
#include <dzapp.h>

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

	int nodeIndex = pPose->Find(pNode);
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
					dzApp->log(QString("ERROR: BakePoseToBindMatrix() could not find cluster bone: %1 in pose").arg(sSearchName));
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

void FbxTools::DetachGeometry(FbxScene* pScene)
{
	FbxNode* RootNode = pScene->GetRootNode();

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
				RootNode->AddChild(SceneNode);
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


bool FbxTools::LoadAndPose(QString poseFilePath, FbxScene* lCurrentScene, DzProgress* pProgress, bool bConvertToZUp)
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
			if (sNodeName == "RootNode")
				continue;
			if (lookupTable.find(sNodeName) != lookupTable.end())
			{
				FbxNode* pPoseNode = lookupTable[sNodeName];
				pNode->Copy(*pPoseNode);
				//pNode->LclRotation.Set(pPoseNode->LclRotation.Get());
				//pNode->LclScaling.Set(pPoseNode->LclScaling.Get());
				//pNode->LclTranslation.Set(pPoseNode->LclTranslation.Get());
				//pNode->PreRotation.Set(pPoseNode->PreRotation.Get());
				//pNode->PostRotation.Set(pPoseNode->PostRotation.Get());
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
void FbxTools::FixClusterTranformLinks(FbxScene* Scene, FbxNode* RootNode, bool bCorrectFix)
{
	FbxGeometry* NodeGeometry = static_cast<FbxGeometry*>(RootNode->GetMesh());

	// Create missing weights
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

					QString sBoneName(Cluster->GetLink()->GetName());

					// Update the rotation
					FbxDouble3 Rotation = Cluster->GetLink()->PostRotation.Get();
					if (bCorrectFix) {
						Matrix.MultRM(Rotation);
						if (sBoneName.contains("_r")) {
							Matrix.MultRM(FbxVector4(90, 0, 0));
						} else {
							Matrix.MultRM(FbxVector4(-90, 0, 0));
						}
						if (sBoneName.contains("ball_")) {
							Matrix.MultRM(FbxVector4(90, 0, 0));
						}
						else if (sBoneName.contains("thumb_")) {
							Matrix.MultRM(FbxVector4(0, 0, 0));
						}
						else if ( sBoneName.contains("hand_") ||
							HasNodeAncestor(Cluster->GetLink(), "hand_r", Qt::CaseInsensitive) ||
							HasNodeAncestor(Cluster->GetLink(), "hand_l", Qt::CaseInsensitive) )
						{
							Matrix.MultRM(FbxVector4(-90, 0, 0));
						}
						if (sBoneName.contains("_l") || sBoneName.contains("_r")) {
							if (HasNodeAncestor(Cluster->GetLink(), "spine_01", Qt::CaseInsensitive)) {
								Matrix.MultRM(FbxVector4(0, 0, 0));
							} else {
								Matrix.MultRM(FbxVector4(0, -90, 0));
							}
						} else {
							Matrix.MultRM(FbxVector4(0, -90, 0));
						}

					}
					else {
						Matrix.SetR(Rotation);
					}
					Cluster->SetTransformLinkMatrix(Matrix);
				}
			}
		}
	}

	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
		FixClusterTranformLinks(Scene, ChildNode, bCorrectFix);
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
		printf("nop");
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

#define TCHAR_TO_UTF8(a) a
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
			IKRootNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
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
			FbxVector4 FootLocation = FootLNode->EvaluateGlobalTransform().GetT();
			IKFootLNode->LclTranslation.Set(FootLocation);
			IKRootNode->AddChild(IKFootLNode);
		}

		// ik_foot_r
		FbxNode* IKFootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_foot_r")));
		FbxNode* FootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sRightFoot)));
		if (!FootRNode) FootRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_foot")));
		if (!IKFootRNode && FootRNode)
		{
			// Create IK Root
			FbxSkeleton* IKFootRNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKFootRNodeAttribute->Size.Set(1.0);
			IKFootRNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_foot_r")));
			IKFootRNode->SetNodeAttribute(IKFootRNodeAttribute);
			FbxVector4 FootLocation = FootRNode->EvaluateGlobalTransform().GetT();
			IKFootRNode->LclTranslation.Set(FootLocation);
			IKRootNode->AddChild(IKFootRNode);
		}

		// ik_hand_root
		FbxNode* IKHandRootNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_root")));
		if (!IKHandRootNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandRootNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRootNodeAttribute->Size.Set(1.0);
			IKHandRootNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_root")));
			IKHandRootNode->SetNodeAttribute(IKHandRootNodeAttribute);
			IKHandRootNode->LclTranslation.Set(FbxVector4(0.0, 00.0, 0.0));
			pRootBone->AddChild(IKHandRootNode);
		}

		// ik_hand_gun
		FbxNode* IKHandGunNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
		FbxNode* HandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT(sRightHand)));
		if (!HandRNode) HandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("r_hand")));
		if (!IKHandGunNode && HandRNode)
		{
			// Create IK Root
			FbxSkeleton* IKHandGunNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandGunNodeAttribute->Size.Set(1.0);
			IKHandGunNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_gun")));
			IKHandGunNode->SetNodeAttribute(IKHandGunNodeAttribute);
			FbxVector4 HandLocation = HandRNode->EvaluateGlobalTransform().GetT();
			IKHandGunNode->LclTranslation.Set(HandLocation);
			IKHandRootNode->AddChild(IKHandGunNode);
		}

		// ik_hand_r
		FbxNode* IKHandRNode = pScene->FindNodeByName(TCHAR_TO_UTF8(TEXT("ik_hand_r")));
		if (!IKHandRNode && HandRNode && IKHandGunNode)
		{
			// Create IK Root
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
			// Create IK Root
			FbxSkeleton* IKHandRNodeAttribute = FbxSkeleton::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandRNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			IKHandRNodeAttribute->Size.Set(1.0);
			IKHandLNode = FbxNode::Create(pScene, TCHAR_TO_UTF8(TEXT("ik_hand_l")));
			IKHandLNode->SetNodeAttribute(IKHandRNodeAttribute);
			FbxVector4 HandLocation = HandLNode->EvaluateGlobalTransform().GetT();
			FbxVector4 ParentLocation = IKHandGunNode->EvaluateGlobalTransform().GetT();
			IKHandLNode->LclTranslation.Set(HandLocation - ParentLocation);
			IKHandGunNode->AddChild(IKHandLNode);
		}
	}

}
