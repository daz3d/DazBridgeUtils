// UNCOMMENT FOLLOWING LINE TO STEP THROUGH MVC CALCULATIONS IN SINGLE THREAD
//#define __SINGLE_THREAD_DEBUG

#include "MvcTools.h"
#include <dzobject.h>
#include <fbxsdk.h>

#include <dzapp.h>
#include <dzprogress.h>
#include <dzscene.h>
#include <dzshape.h>
#include <dzgeometry.h>

#include "FbxTools.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

#include <QtCore>
#include <QThread>
#include <QList>

//////////////////////////////////////////////////////////////////////////////////
// Mean Value Coordinate deformation and interpolation functions

// parallel processing version of calculate_mean_value_coordinate_weights()
// all parameters are embedded as member variables of class
void JobCalculateMvcWeights::PerformJob()
{

	//bool bResult;

	const FbxMesh* pMesh = m_pMesh;
	FbxVector4 x = m_x;
	QVector<double>* pMvcWeights = m_pMvcWeights;

	if (pMesh == nullptr || pMvcWeights == nullptr)
	{
		//emit SignalJobDone(m_sJobName);
		return;
	}
	double epsilon = std::numeric_limits<double>::epsilon() * 10.0;

	int numVerts = pMesh->GetControlPointsCount();
	assert(numVerts > 0);
	FbxVector4* fbx_vertbuffer = m_pVertexBuffer;
	assert(fbx_vertbuffer != nullptr);

	QVector<FbxVector4>* vertexbuffer = new QVector<FbxVector4>(numVerts);
	assert(vertexbuffer != nullptr);
	for (int i = 0; i < numVerts; i++)
	{
		(*vertexbuffer)[i][0] = fbx_vertbuffer[i][0];
		(*vertexbuffer)[i][1] = fbx_vertbuffer[i][1];
		(*vertexbuffer)[i][2] = fbx_vertbuffer[i][2];
	}

	int polyCount = pMesh->GetPolygonCount();
	int polyVertCount = pMesh->GetPolygonVertexCount();

	int vertsPerPoly = pMesh->GetPolygonSize(0);
	bool bTriangulate = false;
	if (vertsPerPoly > 3)
	{
		bTriangulate = true;
	}
	//	assert(vertsPerPoly == 3);
	int numWeights = numVerts;
	int numSteps = (bTriangulate) ? polyCount * 2 : polyCount;
	//DzProgress* pMvcProgress = new DzProgress(QString("Building MVC weights...%1").arg(m_sJobName), numSteps, false, true);
	////pMvcProgress->enable(true);
	//pMvcProgress->step();

	int extraPolys = 0;
	QVector<int>* triangles = new QVector<int>(polyCount * 3);
	for (int i = 0; i < polyCount; i++)
	{
		int polySize = pMesh->GetPolygonSize(i);
		for (int j = 0; j < 3; j++)
		{
			(*triangles)[i * 3 + j] = pMesh->GetPolygonVertex(i, j);
		}
		if (polySize != 3)
		{
			if (polySize > 4)
			{
				//printf("ERROR: ignoring polysize %d", polySize);
			}
			// add more triangles...
			extraPolys++;
			triangles->resize((polyCount + extraPolys) * 3);
			int newPolyOffset = polyCount + extraPolys - 1;
			(*triangles)[newPolyOffset * 3 + 0] = pMesh->GetPolygonVertex(i, 2);
			(*triangles)[newPolyOffset * 3 + 1] = pMesh->GetPolygonVertex(i, 3);
			(*triangles)[newPolyOffset * 3 + 2] = pMesh->GetPolygonVertex(i, 0);
		}
	}


	const FbxVector4* pVertexBuffer = vertexbuffer->constData();

	QVector<FbxVector4> aUnitVectors(numVerts, FbxVector4(0, 0, 0));
	QVector<double> aDistance(numVerts, (double)0.0);

	QVector<FbxVector4> aFValues(numVerts, FbxVector4(0, 0, 0));

	for (int i = 0; i < numVerts; i++)
	{
		FbxVector4 pj = pVertexBuffer[i];
		double distance = FbxTools::getDistance(pj, x);
		aDistance[i] = distance;
		if (distance < epsilon)
		{
			// x == pj, return MVC weight 1.0 for vertindex
			for (int j = 0; j < numVerts; j++)
			{
				(*pMvcWeights)[j] = 0.0;
			}
			(*pMvcWeights)[i] = 1.0;
			//emit SignalJobDone(m_sJobName);
			return;
		}
		aUnitVectors[i] = (pj - x) / FbxTools::getDistance(pj, x);
	}
	//FbxVector4 totalF;
	//double totalW;

	int numPolys = triangles->count() / 3;

	//DzProgress* pMvcProgress = new DzProgress("Building MVC weights...", numPolys, false, true);
	//pMvcProgress->enable(true);
	//pMvcProgress->step();

	double sumWj = 0.0;
	for (int polyIndex = 0; polyIndex < numPolys; polyIndex++)
	{
		bool bIsCoplanar = false;
		//pMvcProgress->step();
		int numVertsInPolygon = 3;

		double* aTheta = new double[numVertsInPolygon];
		double h = 0.0;
		for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
		{
			int polyVert_plus_1 = (polyVert + 1) % 3;
			int polyVert_minus_1 = (polyVert + 2) % 3;
			if (polyVert >= 3)
			{
				//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
				continue;
			}
			int vertBufferIndex = (*triangles)[polyIndex * 3 + polyVert];
			int vertBufferIndex_plus_1 = (*triangles)[polyIndex * 3 + polyVert_plus_1];
			int vertBufferIndex_minus_1 = (*triangles)[polyIndex * 3 + polyVert_minus_1];

			FbxVector4 u1 = aUnitVectors[vertBufferIndex_plus_1];
			FbxVector4 u2 = aUnitVectors[vertBufferIndex_minus_1];
			double fLength = FbxTools::getDistance(u1, u2);
			double theta = 2.0 * asin(fLength / 2.0);
			aTheta[polyVert] = theta;
			h += theta;
		}
		h /= 2;
		double pi = M_PI;
		if (pi - h < epsilon)
		{
			// x is on plane of polygon (triangle), so use 2D barycentric coordinates to interpolate vertex f-values
			//printf("DEBUG: x is coplanar to cage mesh");
			bIsCoplanar = true;
		}
		else
		{
			bIsCoplanar = false;
		}

		if (bIsCoplanar)
		{
			for (int j = 0; j < numVerts; j++)
			{
				(*pMvcWeights)[j] = 0.0;
			}
			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;

				// use 2D barycentric weights for coplanar control points
				int vertBufferIndex = (*triangles)[polyIndex * 3 + polyVert];
				int vertBufferIndex_plus_1 = (*triangles)[polyIndex * 3 + polyVert_plus_1];
				int vertBufferIndex_minus_1 = (*triangles)[polyIndex * 3 + polyVert_minus_1];

				double theta = aTheta[polyVert];
				double distance_plus_1 = aDistance[vertBufferIndex_plus_1];
				double distance_minus_1 = aDistance[vertBufferIndex_minus_1];
				double Wj = sin(theta) * distance_plus_1 * distance_minus_1;

				(*pMvcWeights)[vertBufferIndex] += Wj;
			}
			//emit SignalJobDone(m_sJobName);
			return;
		}
		else
		{
			double* aCosinePsi = new double[numVertsInPolygon];
			double* aSinePsi = new double[numVertsInPolygon];
			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;
				if (polyVert >= 3)
				{
					//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
					continue;
				}

				double theta_0 = aTheta[polyVert];
				double theta_plus_1 = aTheta[polyVert_plus_1];
				double theta_minus_1 = aTheta[polyVert_minus_1];
				double cosine_psi = (2.0 * sin(h) * sin(h - theta_0)) / (sin(theta_plus_1) * sin(theta_minus_1)) - 1.0;
				aCosinePsi[polyVert] = cosine_psi;

				int vertBufferIndex = (*triangles)[polyIndex * 3 + polyVert];
				int vertBufferIndex_plus_1 = (*triangles)[polyIndex * 3 + polyVert_plus_1];
				int vertBufferIndex_minus_1 = (*triangles)[polyIndex * 3 + polyVert_minus_1];
				FbxVector4 unitVectors[3];
				unitVectors[0] = aUnitVectors[vertBufferIndex];
				unitVectors[1] = aUnitVectors[vertBufferIndex_plus_1];
				unitVectors[2] = aUnitVectors[vertBufferIndex_minus_1];
				FbxAMatrix unitMatrix;
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						unitMatrix[j][i] = unitVectors[i][j];
					}
				}
				double det = unitMatrix.Determinant();
				double debug_det = FbxTools::determinant_3x3(unitVectors);
				double delta = debug_det - det;
				if (delta > epsilon)
				{
					//printf("ERROR: Determinant 4 != Determinant 3");
					det = debug_det;
				}

				//double sine_psi = det * sqrt(1 - cosine_psi * cosine_psi);
				double sine_psi = sqrt(1 - cosine_psi * cosine_psi);
				sine_psi *= (det < 0) ? -1 : 1;
				aSinePsi[polyVert] = sine_psi;

				if (isnan(sine_psi))
				{
					//printf("DEBUG: sine of psi is not a number...");
					//aSinePsi[polyVert] = INFINITY;
					continue;
				}
				else if (fabs(sine_psi) <= epsilon)
				{
					// x lies outside t on the same plane
					//printf("DEBUG: sine of psi is inverted (x is outside of cage mesh)");
					continue;
				}
			}

			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;
				if (polyVert >= 3)
				{
					//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
					continue;
				}
				int vertBufferIndex = (*triangles)[polyIndex * 3 + polyVert];

				double theta_0 = aTheta[polyVert];
				double theta_plus_1 = aTheta[polyVert_plus_1];
				double theta_minus_1 = aTheta[polyVert_minus_1];

				double cosine_0 = aCosinePsi[polyVert];
				double cosine_plus_1 = aCosinePsi[polyVert_plus_1];
				double cosine_minus_1 = aCosinePsi[polyVert_minus_1];
				double sine_plus_1 = aSinePsi[polyVert_plus_1];
				double sine_minus_1 = aSinePsi[polyVert_minus_1];

				double distance = aDistance[vertBufferIndex];

				bool bOutsideCage = false;
				//if (aSinePsi[polyVert] <= epsilon)
				//{
				//	// x lies outside cage, adjust Wj to compensate
				//	bOutsideCage = true;
				//	(*pMvcWeights)[vertBufferIndex] = NAN;
				//	continue;
				//}
				double sine_psi = aSinePsi[polyVert];
				if (isnan(sine_psi))
				{
					//printf("DEBUG: sine of psi is not a number...");
					//aSinePsi[polyVert] = INFINITY;
					(*pMvcWeights)[vertBufferIndex] = NAN;
					continue;
				}
				else if (fabs(sine_psi) <= epsilon)
					//				else if (fabs(sine_psi) <= 0.1)
				{
					// x lies outside t on the same plane
					//printf("DEBUG: sine of psi is inverted (x is outside of cage mesh)");
//					(*pMvcWeights)[vertBufferIndex] = NAN;
//					(*pMvcWeights)[vertBufferIndex] = 0;
					continue;

					//for (int j = 0; j < numVerts; j++)
					//{
					//	(*pMvcWeights)[j] = 0.0;
					//}
					//return false;
				}

				double previousWeight = (*pMvcWeights)[vertBufferIndex];
				// sum wi for vertices shared by multiple triangles
				double Wj = (theta_0 - cosine_plus_1 * theta_minus_1 - cosine_minus_1 * theta_plus_1) / (distance * sin(theta_plus_1) * sine_minus_1);
				//double Wj = (theta_0 - cosine_plus_1 * theta_minus_1 - cosine_minus_1 * theta_plus_1) / (2 * sine_plus_1 * sin(theta_minus_1) * distance );
				if (bOutsideCage)
				{
					//Wj = 0;
					int nop = 0;
				}
				if (isinf(Wj))
				{
					//printf("ERROR: Wj is infinite for %d", vertBufferIndex);
					int nop = 0;
				}
				else if (fabs(Wj) > epsilon)
				{
					if (previousWeight > epsilon)
					{
						//printf("DEBUG: adding to existing weight for vertex index: %d", vertBufferIndex);
					}
					sumWj += Wj;
					(*pMvcWeights)[vertBufferIndex] += Wj;
					//(*pMvcWeights)[vertBufferIndex] = Wj;

					//double currentWj = (*pMvcWeights)[vertBufferIndex];
					//double newWj = (fabs(currentWj) < fabs(Wj)) ? Wj : currentWj;
					//(*pMvcWeights)[vertBufferIndex] = newWj;

					//double currentWj = (*pMvcWeights)[vertBufferIndex];
					//if (fabs(currentWj) < fabs(Wj))
					//{
					//	sumWj += Wj;
					//	(*pMvcWeights)[vertBufferIndex] = Wj;
					//}

				}

			}

			delete[] aSinePsi;
			delete[] aCosinePsi;
		}

		delete[] aTheta;
	}
	delete triangles;
	delete vertexbuffer;


	//emit SignalJobDone(m_sJobName);

	//pMvcProgress->finish();
}

FbxVector4 MvcTools::deform_using_mean_value_coordinates(const QVector<FbxVector4>& VertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x)
{
	if (pMvcWeights == nullptr)
	{
		return x;
	}

	double epsilon = std::numeric_limits<double>::epsilon() * 10.0;
	FbxVector4 deformed_x = x;

	const FbxVector4* pVertexBuffer = VertexBuffer.constData();
	int numVerts = VertexBuffer.count();

	assert(numVerts == pMvcWeights->count());

	bool bNoWeights = true;
	FbxVector4 sumWjPj(0, 0, 0);
	double sumWj = 0.0;
	for (int i = 0; i < numVerts; i++)
	{
		FbxVector4 pj = pVertexBuffer[i];
		double wj = (*pMvcWeights)[i];
		if (isnan(wj))
		{
			continue;
			//return FbxVector4(NAN, NAN, NAN);
		}
		else if (fabs(wj) <= epsilon)
		{
			continue;
		}
		bNoWeights = false;
		FbxVector4 wjpj = pj * wj;
		wjpj[3] = 0.0;

		sumWjPj += wjpj;
		sumWj += wj;
		//printf("nop");
	}

	if (!bNoWeights)
	{
		if ( fabs(sumWj) < 3.5 ) {
			int break_here = 0;
			// hardcode for G9
//			sumWj *= 1.6334531198954590003266906239791;
		}
		deformed_x = sumWjPj;
		if (fabs(sumWj) > epsilon)
		{
			FbxVector4 normalizedWjPj = sumWjPj / sumWj;
			deformed_x = normalizedWjPj;
		}
		else
		{
			int break_here = 0;
		}
	}

	return deformed_x;
}

bool MvcTools::calculate_mean_value_coordinate_weights(const QVector<FbxVector4>& VertexBuffer, const QVector<int>& Triangles, FbxVector4 x, QVector<double>* pMvcWeights)
{
	if (pMvcWeights == nullptr)
	{
		return false;
	}

	const FbxVector4* pVertexBuffer = VertexBuffer.constData();
	int numVerts = VertexBuffer.count();

	QVector<FbxVector4> aUnitVectors(numVerts, FbxVector4(0, 0, 0));
	QVector<double> aDistance(numVerts, (double)0.0);

	double epsilon = std::numeric_limits<double>::epsilon() * 10.0;

	QVector<FbxVector4> aFValues(numVerts, FbxVector4(0, 0, 0));

	for (int i = 0; i < numVerts; i++)
	{
		FbxVector4 pj = pVertexBuffer[i];
		double distance = FbxTools::getDistance(pj, x);
		aDistance[i] = distance;
		if (distance < epsilon)
		{
			// x == pj, return MVC weight 1.0 for vertindex
			for (int j = 0; j < numVerts; j++)
			{
				(*pMvcWeights)[j] = 0.0;
			}
			(*pMvcWeights)[i] = 1.0;
			return false;
		}
		aUnitVectors[i] = (pj - x) / FbxTools::getDistance(pj, x);
	}
	//FbxVector4 totalF;
	//double totalW;

	int numPolys = Triangles.count() / 3;

	//DzProgress* pMvcProgress = new DzProgress("Building MVC weights...", numPolys, false, true);
	//pMvcProgress->enable(true);
	//pMvcProgress->step();

	double sumWj = 0.0;
	for (int polyIndex = 0; polyIndex < numPolys; polyIndex++)
	{
		bool bIsCoplanar = false;
		//pMvcProgress->step();
		int numVertsInPolygon = 3;

		double* aTheta = new double[numVertsInPolygon];
		double h = 0.0;
		for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
		{
			int polyVert_plus_1 = (polyVert + 1) % 3;
			int polyVert_minus_1 = (polyVert + 2) % 3;
			if (polyVert >= 3)
			{
				//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
				continue;
			}
			int vertBufferIndex = Triangles[polyIndex * 3 + polyVert];
			int vertBufferIndex_plus_1 = Triangles[polyIndex * 3 + polyVert_plus_1];
			int vertBufferIndex_minus_1 = Triangles[polyIndex * 3 + polyVert_minus_1];

			FbxVector4 u1 = aUnitVectors[vertBufferIndex_plus_1];
			FbxVector4 u2 = aUnitVectors[vertBufferIndex_minus_1];
			double fLength = FbxTools::getDistance(u1, u2);
			double theta = 2.0 * asin(fLength / 2.0);
			aTheta[polyVert] = theta;
			h += theta;
		}
		h /= 2;
		double pi = M_PI;
		if (pi - h < epsilon)
		{
			// x is on plane of polygon (triangle), so use 2D barycentric coordinates to interpolate vertex f-values
			//printf("DEBUG: x is coplanar to cage mesh");
			bIsCoplanar = true;
		}
		else
		{
			bIsCoplanar = false;
		}

		if (bIsCoplanar)
		{
			for (int j = 0; j < numVerts; j++)
			{
				(*pMvcWeights)[j] = 0.0;
			}
			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;

				// use 2D barycentric weights for coplanar control points
				int vertBufferIndex = Triangles[polyIndex * 3 + polyVert];
				int vertBufferIndex_plus_1 = Triangles[polyIndex * 3 + polyVert_plus_1];
				int vertBufferIndex_minus_1 = Triangles[polyIndex * 3 + polyVert_minus_1];

				double theta = aTheta[polyVert];
				double distance_plus_1 = aDistance[vertBufferIndex_plus_1];
				double distance_minus_1 = aDistance[vertBufferIndex_minus_1];
				double Wj = sin(theta) * distance_plus_1 * distance_minus_1;

				(*pMvcWeights)[vertBufferIndex] += Wj;
			}
			return false;
		}
		else
		{
			double* aCosinePsi = new double[numVertsInPolygon];
			double* aSinePsi = new double[numVertsInPolygon];
			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;
				if (polyVert >= 3)
				{
					//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
					continue;
				}

				double theta_0 = aTheta[polyVert];
				double theta_plus_1 = aTheta[polyVert_plus_1];
				double theta_minus_1 = aTheta[polyVert_minus_1];
				double cosine_psi = (2.0 * sin(h) * sin(h - theta_0)) / (sin(theta_plus_1) * sin(theta_minus_1)) - 1.0;
				aCosinePsi[polyVert] = cosine_psi;

				int vertBufferIndex = Triangles[polyIndex * 3 + polyVert];
				int vertBufferIndex_plus_1 = Triangles[polyIndex * 3 + polyVert_plus_1];
				int vertBufferIndex_minus_1 = Triangles[polyIndex * 3 + polyVert_minus_1];
				FbxVector4 unitVectors[3];
				unitVectors[0] = aUnitVectors[vertBufferIndex];
				unitVectors[1] = aUnitVectors[vertBufferIndex_plus_1];
				unitVectors[2] = aUnitVectors[vertBufferIndex_minus_1];
				FbxAMatrix unitMatrix;
				for (int i = 0; i < 3; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						unitMatrix[j][i] = unitVectors[i][j];
					}
				}
				double det = unitMatrix.Determinant();
				double debug_det = FbxTools::determinant_3x3(unitVectors);

				double delta = debug_det - det;

				if (delta > epsilon)
				{
					//printf("ERROR: Determinant 4 != Determinant 3");
					det = debug_det;
				}

				//double sine_psi = det * sqrt(1 - cosine_psi * cosine_psi);
				double sine_psi = sqrt(1 - cosine_psi * cosine_psi);
				sine_psi *= (det < 0) ? -1 : 1;
				aSinePsi[polyVert] = sine_psi;

				if (isnan(sine_psi))
				{
					//printf("DEBUG: sine of psi is not a number...");
					//aSinePsi[polyVert] = INFINITY;
					continue;
				}
				else if (fabs(sine_psi) <= epsilon)
				{
					// x lies outside t on the same plane
					//printf("DEBUG: sine of psi is inverted (x is outside of cage mesh)");
					continue;
				}
			}

			for (int polyVert = 0; polyVert < numVertsInPolygon; polyVert++)
			{
				int polyVert_plus_1 = (polyVert + 1) % 3;
				int polyVert_minus_1 = (polyVert + 2) % 3;
				if (polyVert >= 3)
				{
					//printf("ERROR: unhandled polygon: %d points", numVertsInPolygon);
					continue;
				}
				int vertBufferIndex = Triangles[polyIndex * 3 + polyVert];

				double theta_0 = aTheta[polyVert];
				double theta_plus_1 = aTheta[polyVert_plus_1];
				double theta_minus_1 = aTheta[polyVert_minus_1];

				double cosine_0 = aCosinePsi[polyVert];
				double cosine_plus_1 = aCosinePsi[polyVert_plus_1];
				double cosine_minus_1 = aCosinePsi[polyVert_minus_1];
				double sine_plus_1 = aSinePsi[polyVert_plus_1];
				double sine_minus_1 = aSinePsi[polyVert_minus_1];

				double distance = aDistance[vertBufferIndex];

				bool bOutsideCage = false;
				//if (aSinePsi[polyVert] <= epsilon)
				//{
				//	// x lies outside cage, adjust Wj to compensate
				//	bOutsideCage = true;
				//	(*pMvcWeights)[vertBufferIndex] = NAN;
				//	continue;
				//}
				double sine_psi = aSinePsi[polyVert];
				if (isnan(sine_psi))
				{
					//printf("DEBUG: sine of psi is not a number...");
					//aSinePsi[polyVert] = INFINITY;
					(*pMvcWeights)[vertBufferIndex] = NAN;
					continue;
				}
				else if (fabs(sine_psi) <= epsilon)
					//				else if (fabs(sine_psi) <= 0.1)
				{
					// x lies outside t on the same plane
					//printf("DEBUG: sine of psi is inverted (x is outside of cage mesh)");
//					(*pMvcWeights)[vertBufferIndex] = NAN;
//					(*pMvcWeights)[vertBufferIndex] = 0;
					continue;

					//for (int j = 0; j < numVerts; j++)
					//{
					//	(*pMvcWeights)[j] = 0.0;
					//}
					//return false;
				}

				double previousWeight = (*pMvcWeights)[vertBufferIndex];
				// sum wi for vertices shared by multiple triangles
				double Wj = (theta_0 - cosine_plus_1 * theta_minus_1 - cosine_minus_1 * theta_plus_1) / (distance * sin(theta_plus_1) * sine_minus_1);
				//double Wj = (theta_0 - cosine_plus_1 * theta_minus_1 - cosine_minus_1 * theta_plus_1) / (2 * sine_plus_1 * sin(theta_minus_1) * distance );
				if (bOutsideCage)
				{
					//Wj = 0;
					//printf("nop?");
				}
				if (isinf(Wj))
				{
					//printf("ERROR: Wj is infinite for %d", vertBufferIndex);
				}
				else if (fabs(Wj) > epsilon)
				{
					if (previousWeight > epsilon)
					{
						//printf("DEBUG: adding to existing weight for vertex index: %d", vertBufferIndex);
					}
					sumWj += Wj;
					(*pMvcWeights)[vertBufferIndex] += Wj;
					//(*pMvcWeights)[vertBufferIndex] = Wj;

					//double currentWj = (*pMvcWeights)[vertBufferIndex];
					//double newWj = (fabs(currentWj) < fabs(Wj)) ? Wj : currentWj;
					//(*pMvcWeights)[vertBufferIndex] = newWj;

					//double currentWj = (*pMvcWeights)[vertBufferIndex];
					//if (fabs(currentWj) < fabs(Wj))
					//{
					//	sumWj += Wj;
					//	(*pMvcWeights)[vertBufferIndex] = Wj;
					//}

				}

			}

			delete[] aSinePsi;
			delete[] aCosinePsi;
		}

		delete[] aTheta;
	}

	//if (fabs(sumWj) > epsilon)
	//{
	//	for (int i = 0; i < numVerts; i++)
	//	{
	//		double Wj = (*pMvcWeights)[i];
	//		double normalizedWj = Wj / sumWj;
	//		(*pMvcWeights)[i] = normalizedWj;
	//	}
	//}

	//pMvcProgress->finish();
	//delete pMvcProgress;

	return false;
}

// Overload of above functions, using FbxMesh* instead of QVector<FbxVector4>
bool MvcTools::calculate_mean_value_coordinate_weights(const FbxMesh* pMesh, FbxVector4 x, QVector<double>* pMvcWeights)
{
	if (pMesh == nullptr || pMvcWeights == nullptr)
	{
		return false;
	}
	double epsilon = std::numeric_limits<double>::epsilon() * 10.0;

	int numVerts = pMesh->GetControlPointsCount();
	FbxVector4* fbx_vertbuffer = pMesh->GetControlPoints();

	QVector<FbxVector4>* vertexbuffer = new QVector<FbxVector4>(numVerts);

	for (int i = 0; i < numVerts; i++)
	{
		(*vertexbuffer)[i][0] = fbx_vertbuffer[i][0];
		(*vertexbuffer)[i][1] = fbx_vertbuffer[i][1];
		(*vertexbuffer)[i][2] = fbx_vertbuffer[i][2];
	}

	int polyCount = pMesh->GetPolygonCount();
	int polyVertCount = pMesh->GetPolygonVertexCount();

	int vertsPerPoly = pMesh->GetPolygonSize(0);
	bool bTriangulate = false;
	if (vertsPerPoly > 3)
	{
		bTriangulate = true;
	}
	//	assert(vertsPerPoly == 3);
	//int numWeights = m_mBoneToMvcWeightsTable.count();
	int numWeights = 0;
	int numSteps = (bTriangulate) ? polyCount * 2 : polyCount;
	DzProgress* pMvcProgress = new DzProgress(QString("Building MVC weights...%1").arg(numWeights), numSteps, false, true);
	//pMvcProgress->enable(true);
	pMvcProgress->step();

	int extraPolys = 0;
	QVector<int>* triangles = new QVector<int>(polyCount * 3);
	for (int i = 0; i < polyCount; i++)
	{
		int polySize = pMesh->GetPolygonSize(i);
		for (int j = 0; j < 3; j++)
		{
			(*triangles)[i * 3 + j] = pMesh->GetPolygonVertex(i, j);
		}
		if (polySize != 3)
		{
			if (polySize > 4)
			{
				//printf("ERROR: ignoring polysize %d", polySize);
			}
			// add more triangles...
			extraPolys++;
			triangles->resize((polyCount + extraPolys) * 3);
			int newPolyOffset = polyCount + extraPolys - 1;
			(*triangles)[newPolyOffset * 3 + 0] = pMesh->GetPolygonVertex(i, 2);
			(*triangles)[newPolyOffset * 3 + 1] = pMesh->GetPolygonVertex(i, 3);
			(*triangles)[newPolyOffset * 3 + 2] = pMesh->GetPolygonVertex(i, 0);
		}
	}

	bool bResult = calculate_mean_value_coordinate_weights(*vertexbuffer, *triangles, x, pMvcWeights);

	pMvcProgress->finish();
	delete pMvcProgress;

	return bResult;
}

FbxVector4 MvcTools::deform_using_mean_value_coordinates(const FbxMesh* pMesh, const FbxVector4* pVertexBuffer, const QVector<double>* pMvcWeights, FbxVector4 x)
{
	if (pMesh == nullptr || pMvcWeights == nullptr)
	{
        return FbxVector4(NAN,NAN,NAN);
	}

	FbxVector4 deformed_x;

	//FbxVector4* pVertexBuffer = pMesh->GetControlPoints();
	//BakePoseToVertexBuffer(pVertexBuffer, &GetAffineMatrix(nullptr, pMesh->GetNode(), false, FbxTime(0)), nullptr, (FbxMesh*) pMesh, FbxTime(0));

	int numVerts = pMesh->GetControlPointsCount();
	assert(numVerts == pMvcWeights->count());

	QVector<FbxVector4> vertex_buffer_list;
	vertex_buffer_list.reserve(numVerts);
	std::copy(pVertexBuffer, pVertexBuffer + numVerts, std::back_inserter(vertex_buffer_list));

	// validation
	for (int i = 0; i < numVerts; i++)
	{
		assert(vertex_buffer_list[i] == pVertexBuffer[i]);
		for (int j = 0; j < 3; j++)
		{
			assert(vertex_buffer_list[i][j] == pVertexBuffer[i][j]);
		}
	}

	deformed_x = deform_using_mean_value_coordinates(vertex_buffer_list, pMvcWeights, x);

	return deformed_x;
}

FbxVector2 MvcTools::interpolate_using_mean_value_coordinates(const QVector<double>* pMvcWeights, const QVector<FbxVector2>* pFValues)
{
	assert(pMvcWeights != nullptr);
	assert(pFValues != nullptr);

	int numVerts = pMvcWeights->count();

	FbxVector2 sumWjFj(0.0,0.0);
	double sumWj=0.0;
	for (int i = 0; i < numVerts; i++)
	{
		FbxVector2 fj = (*pFValues)[i];
		double wj = (*pMvcWeights)[i];

		sumWjFj += fj;
		sumWj += wj;
	}

	FbxVector2 interpolated_uv = sumWjFj / sumWj;

	return interpolated_uv;
}

bool MvcBoneRetargeter::createMvcWeightsTable(FbxMesh* pMesh, FbxNode* pRootNode, DzProgress* pProgress)
{
	pProgress->step();
	// for each bone, calculte mvc weights, add to mvc weights table

	if (pMesh == nullptr || pRootNode == nullptr)
		return false;

	QList<FbxNode*> todoQueue;
	todoQueue.append(pRootNode);

	const char* rootName = pRootNode->GetName();
	int numVerts = pMesh->GetControlPointsCount();
	int numBones = 1500;
	DzProgress* pMvcProgress = new DzProgress(QString("Analyzing... %1").arg(rootName), numBones, false, true);
	pMvcProgress->enable(true);
	pMvcProgress->step();

	FbxVector4* pTempBuffer = pMesh->GetControlPoints();
	FbxVector4* pVertexBuffer = new FbxVector4[numVerts];
	memcpy(pVertexBuffer, pTempBuffer, sizeof(FbxVector4)*numVerts);
    FbxAMatrix matrix = FbxTools::GetAffineMatrix(nullptr, pMesh->GetNode());
	FbxTools::BakePoseToVertexBuffer(pVertexBuffer, &matrix, nullptr, pMesh);

	//while (todoQueue.isEmpty() == false)
	//{
	//	pMvcProgress->step();
	//	FbxNode* pNode = todoQueue.front();
	//	todoQueue.pop_front();

	//	const char* lpBoneName = pNode->GetName();
	//	QString sBoneName(lpBoneName);
	//	// calculate mvc weights
	//	FbxVector4 bonePosition = GetAffineMatrix(nullptr, pNode).GetT();
	//	QVector<double>* pMvcWeights = new QVector<double>(numVerts, (double)0.0);
	//	DzProgress::setCurrentInfo(QString("Computing MVC weights for %1").arg(sBoneName));
	//	calculate_mean_value_coordinate_weights(pMesh, bonePosition, pMvcWeights);
	//	// add mvc weights to table
	//	m_mBoneToMvcWeightsTable.insert(sBoneName, pMvcWeights);

	//	// add children
	//	for (int i = 0; i < pNode->GetChildCount(); i++)
	//	{
	//		FbxNode* pChild = pNode->GetChild(i);
	//		FbxNodeAttribute* attributes = pChild->GetNodeAttribute();
	//		if (attributes && attributes->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	//		{
	//			todoQueue.append(pChild);
	//		}
	//	}
	//}

	// create jobs
	int nJobID = 0;
	while (todoQueue.isEmpty() == false)
	{
		pMvcProgress->step();
		FbxNode* pNode = todoQueue.front();
		todoQueue.pop_front();

		const char* lpBoneName = pNode->GetName();
		QString sBoneName(lpBoneName);
		// calculate mvc weights
		FbxVector4 bonePosition = FbxTools::GetAffineMatrix(nullptr, pNode).GetT();
		QVector<double>* pMvcWeights = new QVector<double>(numVerts, (double)0.0);
		DzProgress::setCurrentInfo(QString("Computing MVC weights for %1").arg(sBoneName));

		//calculate_mean_value_coordinate_weights(pMesh, bonePosition, pMvcWeights);
		//// add mvc weights to table
		//m_mBoneToMvcWeightsTable.insert(sBoneName, pMvcWeights);

		auto job = new JobCalculateMvcWeights(sBoneName, pMesh, bonePosition, pVertexBuffer, pMvcWeights);
		m_JobQueue.insert(sBoneName, job);
		nJobID++;
		m_mBoneToMvcWeightsTable.insert(sBoneName, pMvcWeights);

		// add children
		for (int i = 0; i < pNode->GetChildCount(); i++)
		{
			FbxNode* pChild = pNode->GetChild(i);
			FbxNodeAttribute* attributes = pChild->GetNodeAttribute();
			if (attributes && attributes->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				todoQueue.append(pChild);
			}
		}
	}

	// spawn threads and process jobs
	int numLogicalCores = QThread::idealThreadCount();
	//int numLogicalCores = 1;

	//if (numLogicalCores > 1) numLogicalCores *= 0.85;
	if (numLogicalCores >= 4)
	{
		numLogicalCores--;
	}

	/*
		int numJobs = m_JobQueue.count();
		int bundleSize = numJobs / numLogicalCores;
		bool bDoHack = true;
		if (bDoHack)
		{
			bundleSize *= 2;
		}
		while (m_JobQueue.count() > 0)
		{
			// if active workers are not at max, hire new workers
			// else wait for number active workers to decrease (HandleJobDone())
			if (m_WorkerThreads.count() < numLogicalCores)
			{
				// get jobs (gather list of clusterSize of jobs)
				JobBundle* jobBundle = new JobBundle();
				for (int i = 0; i < bundleSize; i++)
				{
					auto kvp = m_JobQueue.begin();
					if (kvp != m_JobQueue.end())
					{
						QString key = kvp.key();
						JobCalculateMvcWeights* job = kvp.value();
						jobBundle->addJob(job);
						// remove job from queue
						m_JobQueue.remove(key);
					}
					else
					{
						break;
					}
				}
				spawnThread(jobBundle);
				QThread::yieldCurrentThread();

				int index = 0;
				QString sJobLog = QString("d:/temp/joblog_%1_%2.log").arg(jobBundle->getName()).arg(index);
				QFile fileJobLog(sJobLog);
				while (fileJobLog.exists())
				{
					index++;
					sJobLog = QString("d:/temp/joblog_%1_%2.log").arg(jobBundle->getName()).arg(index);
					fileJobLog.setFileName(sJobLog);
				}
				if (fileJobLog.open(QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Text))
				{
					for (auto job : jobBundle->m_JobList)
					{
						QString jobname = job->m_sJobName + "\n";
						fileJobLog.write(jobname.toLocal8Bit());
					}
					fileJobLog.close();
				}
			}
			else
			{
				auto first = m_WorkerThreads.begin().value();
				if (first->isRunning() == false)
				{
					printf("nop");
				}
				//first->wait(500);
				Sleep(50);
				//DzProgress::setCurrentInfo("Waiting on threads...");
			}
			if (m_WorkerThreads.count() > 0)
			{
				QThread::yieldCurrentThread();
				//m_WorkerThreads.begin().value()->wait(250);
				Sleep(50);
				QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers | QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
			}
		}

		// flush out any remaining threads & active jobs
		while (m_WorkerThreads.count() > 0)
		{
			QThread::yieldCurrentThread();
			//m_WorkerThreads.begin().value()->wait(250);
			Sleep(50);

			//QCoreApplication::processEvents();
			QCoreApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeSocketNotifiers | QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
			//DzProgress::setCurrentInfo("Waiting on threads...");
		}
	*/

	/////// QtConcurrent
#ifdef __SINGLE_THREAD_DEBUG
	for (auto job : m_JobQueue.values())
	{
		job->PerformJob();
	}
#elif defined(__APPLE__)
    std::vector<JobCalculateMvcWeights*> jobs;
    for (JobCalculateMvcWeights* job : m_JobQueue.values())
    {
        jobs.push_back(job);
    }
    QtConcurrent::blockingMap(jobs, JobCalculateMvcWeights::StaticPerformJob);
#else
    QtConcurrent::blockingMap(m_JobQueue.values(), JobCalculateMvcWeights::StaticPerformJob);
#endif

	delete[] pVertexBuffer;

	pMvcProgress->finish();
	delete pMvcProgress;

	return true;
}

bool MvcBoneRetargeter::validateMvcWeights(const FbxMesh* pMesh, FbxNode* pRootBone)
{
	bool bResult = true;

	// for each bone_name, calculate morphed mesh * mvc weights (aka deformed bones)
	// apply deformed bones to pose
	// bake pose to bind matrix

	int numVerts = pMesh->GetControlPointsCount();
	FbxVector4* pTempBuffer = pMesh->GetControlPoints();
	FbxVector4* pVertexBuffer = new FbxVector4[numVerts];
	memcpy(pVertexBuffer, pTempBuffer, sizeof(FbxVector4) * numVerts);
    FbxAMatrix matrix = FbxTools::GetAffineMatrix(nullptr, pMesh->GetNode());
	FbxTools::BakePoseToVertexBuffer(pVertexBuffer, &matrix, nullptr, (FbxMesh*) pMesh);

	double epsilon = 0.001;
	QList<FbxNode*> todoQueue;
	todoQueue.append(pRootBone);

	while (todoQueue.isEmpty() == false)
	{
		FbxNode* pNode = todoQueue.front();
		todoQueue.pop_front();

		const char* lpBoneName = pNode->GetName();
		QString sBoneName(lpBoneName);
		// lookup mvc weights in table
		auto results = m_mBoneToMvcWeightsTable.find(sBoneName);
		if (results == m_mBoneToMvcWeightsTable.end())
		{
			//printf("ERROR: unable to find bonename in MvcWeights lookup table: %s", lpBoneName);
			continue;
		}
		QVector<double>* pMvcWeights = results.value();
		FbxVector4 bonePosition = FbxTools::GetAffineMatrix(nullptr, pNode).GetT();
		FbxVector4 newBonePosition = MvcTools::deform_using_mean_value_coordinates(pMesh, pVertexBuffer, pMvcWeights, bonePosition);

		FbxVector4 delta = newBonePosition - bonePosition;
		if (fabs(delta[0]) > epsilon ||
			fabs(delta[1]) > epsilon ||
			fabs(delta[2]) > epsilon)
		{
			//printf("ERROR: MVC validation failed, unable to reproduce same position using original values.");
			delete [] pVertexBuffer;
			bResult = false;
			return bResult;
		}
		else
		{
			bResult = true;
		}

		// add children
		for (int i = 0; i < pNode->GetChildCount(); i++)
		{
			FbxNode* pChild = pNode->GetChild(i);
			FbxNodeAttribute* attributes = pChild->GetNodeAttribute();
			if (attributes && attributes->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				todoQueue.append(pChild);
			}
		}
	}
	delete[] pVertexBuffer;
	return bResult;
}

FbxVector4 MvcBoneRetargeter::calibrate_bone(const FbxMesh* pMorphedMesh, const FbxVector4* pVertexBuffer, QString sBoneName)
{
	auto results = m_mBoneToMvcWeightsTable.find(sBoneName);
	if (results == m_mBoneToMvcWeightsTable.end())
	{
		return FbxVector4(NAN, NAN, NAN);
	}
	QVector<double>* pMvcWeights = results.value();
	FbxVector4 newBonePosition = MvcTools::deform_using_mean_value_coordinates(pMorphedMesh, pVertexBuffer, pMvcWeights);
	return newBonePosition;
};


FbxVector4 MvcBoneRetargeter::calibrate_bone(const FbxMesh* pMorphedMesh, QString sBoneName)
{
	auto results = m_mBoneToMvcWeightsTable.find(sBoneName);
	if (results == m_mBoneToMvcWeightsTable.end())
	{
		return FbxVector4(NAN, NAN, NAN);
	}
	QVector<double>* pMvcWeights = results.value();

	FbxVector4* pVertexBuffer = pMorphedMesh->GetControlPoints();
    FbxAMatrix matrix = FbxTools::GetAffineMatrix(nullptr, pMorphedMesh->GetNode());
    FbxTools::BakePoseToVertexBuffer(pVertexBuffer, &matrix, nullptr, (FbxMesh*)pMorphedMesh);

	FbxVector4 newBonePosition = MvcTools::deform_using_mean_value_coordinates(pMorphedMesh, pVertexBuffer, pMvcWeights);
	return newBonePosition;
};

bool MvcBoneRetargeter::loadMvcWeightsCache(QString mvcWeightsFilename)
{
	// LOAD WEIGHTS...
	QFile fileMvcWeights(mvcWeightsFilename);
	if (fileMvcWeights.open(QIODevice::OpenModeFlag::ReadOnly))
	{
		// Load Header
		QByteArray buffer;
		buffer = fileMvcWeights.read(sizeof(mvcweights_header));

		mvcweights_header* pHeader;
		pHeader = (mvcweights_header*)buffer.data();
		const char* SIG = pHeader->SIG;
		assert(strcmp(SIG, "MVCW") == 0);
		int numElements = pHeader->numElements;
		int numArrays = pHeader->numArrays;
		int offsetLookupTable = pHeader->offsetLookupTable;
		int offsetRawData = pHeader->offsetRawData;
		int fileSize = pHeader->fileSize;
		// Look up Table
		int sizeLookupTable = offsetRawData - offsetLookupTable;
		buffer.clear();
		buffer = fileMvcWeights.read(sizeLookupTable);
		int offset = 0;
		for (int i = 0; i < numArrays; i++)
		{
			uint32_t table_index = (uint32_t) * (buffer.data() + offset);
			offset += sizeof(uint32_t);
			uint32_t buffer_len = (uint32_t) * (buffer.data() + offset);
			offset += sizeof(uint32_t);
			char* lpBoneName = new char[buffer_len + 1];
			memcpy(lpBoneName, buffer.data() + offset, buffer_len);
			lpBoneName[buffer_len] = '\0';
			QString sBoneName = QString::fromUtf8(lpBoneName);
			delete[] lpBoneName;
			offset += buffer_len;
			QVector<double>* mvcWeights = new QVector<double>();
			m_mBoneToMvcWeightsTable.insert(sBoneName, mvcWeights);
		}
		buffer.clear();
		// Load weights
		for (QString key : m_mBoneToMvcWeightsTable.keys())
		{
			QVector<double>* mvc_weights = m_mBoneToMvcWeightsTable[key];
			assert(mvc_weights->count() == 0);
			mvc_weights->reserve(numElements);
			buffer = fileMvcWeights.read(sizeof(double) * numElements);
			double* pDoubleArray = (double*)buffer.data();
			std::copy(pDoubleArray, pDoubleArray + numElements, std::back_inserter(*mvc_weights));
			buffer.clear();
		}
		fileMvcWeights.close();
	}
	else
	{
		return false;
	}

	return true;
};

bool MvcBoneRetargeter::saveMvcWeightsCache(QString mvcWeightsFilename)
{
	QFile fileMvcWeights(mvcWeightsFilename);
	if (fileMvcWeights.open(QIODevice::OpenModeFlag::WriteOnly | QIODevice::OpenModeFlag::Truncate))
	{
		mvcweights_header header;
		header.SIG[0] = 'M';
		header.SIG[1] = 'V';
		header.SIG[2] = 'C';
		header.SIG[3] = 'W';
		header.SIG[4] = '\0';
		header.numElements = m_mBoneToMvcWeightsTable.values().first()->count();
		header.numArrays = m_mBoneToMvcWeightsTable.count();
		fileMvcWeights.write((char*)&header, sizeof(header));
		uint32_t table_index = 0;
		header.offsetLookupTable = fileMvcWeights.pos();
		for (QString key : m_mBoneToMvcWeightsTable.keys())
		{
			fileMvcWeights.write((char*)&table_index, sizeof(table_index));
			QByteArray keyBuffer = key.toUtf8();
			uint32_t bufferLen = keyBuffer.size();
			fileMvcWeights.write((char*)&bufferLen, sizeof(bufferLen));
			fileMvcWeights.write(keyBuffer);
			table_index++;
		}
		header.offsetRawData = fileMvcWeights.pos();
		for (QString key : m_mBoneToMvcWeightsTable.keys())
		{
			QVector<double>* mvcArray = m_mBoneToMvcWeightsTable[key];
			assert(mvcArray->count() == header.numElements);
			const double* rawArray = mvcArray->constData();
			fileMvcWeights.write((char*)rawArray, sizeof(double) * header.numElements);
		}
		header.fileSize = fileMvcWeights.pos();
		if (fileMvcWeights.seek(0) == false)
		{
			printf("ERROR: unable to rewrite header");
		}
		fileMvcWeights.write((char*)&header, sizeof(header));
		fileMvcWeights.close();
	}
	else
	{
		return false;
	}

	return true;
}

void MvcBoneRetargeter::clearWeights()
{
	for (QString key : m_mBoneToMvcWeightsTable.keys())
	{
		auto mvc_weights = m_mBoneToMvcWeightsTable[key];
		mvc_weights->clear();
		delete mvc_weights;
		m_mBoneToMvcWeightsTable.remove(key);
	}
	m_mBoneToMvcWeightsTable.clear();

}

bool MvcCageRetargeter::createMvcWeights(const FbxMesh* pMesh, const FbxMesh* pCage, DzProgress* pProgress)
{
	pProgress->step();
	// for each bone, calculte mvc weights, add to mvc weights table

	if (pMesh == nullptr || pCage == nullptr)
		return false;

	const char* cageName = pCage->GetName();
	int numWeights = pMesh->GetControlPointsCount();
	int numCageVerts = pCage->GetControlPointsCount();
	DzProgress* pMvcProgress = new DzProgress(QString("Analyzing... %1").arg(cageName), numCageVerts, false, true);
	pMvcProgress->enable(true);
	pMvcProgress->step();

	FbxVector4* pActiveMeshBuffer = pMesh->GetControlPoints();
	FbxVector4* pSourceMeshBuffer = new FbxVector4[numWeights];
	memcpy(pSourceMeshBuffer, pActiveMeshBuffer, sizeof(FbxVector4) * numWeights);
	if (pMesh->GetNode())
	{
		FbxAMatrix matrix = FbxTools::GetAffineMatrix(nullptr, pMesh->GetNode());
		FbxTools::BakePoseToVertexBuffer(pSourceMeshBuffer, &matrix, nullptr, pMesh);
	}

	FbxVector4* pActiveCageBuffer = pCage->GetControlPoints();
	FbxVector4* pCageBuffer = new FbxVector4[numCageVerts];
	memcpy(pCageBuffer, pActiveCageBuffer, sizeof(FbxVector4) * numCageVerts);
	if (pCage->GetNode())
	{
		FbxAMatrix matrix2 = FbxTools::GetAffineMatrix(nullptr, pCage->GetNode());
		FbxTools::BakePoseToVertexBuffer(pCageBuffer, &matrix2, nullptr, pCage);
	}

	// create jobs
	int nJobID = 0;
	for (int i=0; i < numCageVerts; i++)
	{
		pMvcProgress->step();

		// calculate mvc weights
		FbxVector4 vertexPosition = pCageBuffer[i];
		QVector<double>* pMvcWeights = new QVector<double>(numWeights, (double)0.0);
#ifdef _DEBUG
			DzProgress::setCurrentInfo(QString("Computing MVC weights for vertex %1").arg(i));
#endif
		QString sJobName = QString("Job#%1").arg(i);
		auto job = new JobCalculateMvcWeights(sJobName, pMesh, vertexPosition, pSourceMeshBuffer, pMvcWeights);
		m_JobQueue.insert(i, job);
		m_MvcWeightsTable.insert(i, pMvcWeights);
	}

	// spawn threads and process jobs
	int numLogicalCores = QThread::idealThreadCount();
	//int numLogicalCores = 1;

	//if (numLogicalCores > 1) numLogicalCores *= 0.85;
	if (numLogicalCores >= 4)
	{
		numLogicalCores--;
	}

	/////// QtConcurrent
#ifdef __SINGLE_THREAD_DEBUG
	for (auto job : m_JobQueue.values())
	{
		job->PerformJob();
	}
#elif defined(__APPLE__)
	std::vector<JobCalculateMvcWeights*> jobs;
	for (JobCalculateMvcWeights* job : m_JobQueue.values())
	{
		jobs.push_back(job);
	}
	QtConcurrent::blockingMap(jobs, JobCalculateMvcWeights::StaticPerformJob);
#else
	QtConcurrent::blockingMap(m_JobQueue.values(), JobCalculateMvcWeights::StaticPerformJob);
#endif

	// cleanup memory
	delete[] pSourceMeshBuffer;
	delete[] pCageBuffer;

	pMvcProgress->finish();
	delete pMvcProgress;
	return true;
}

bool MvcCageRetargeter::deformCage(const FbxMesh* pMorphedMesh, const FbxMesh* pCage, FbxVector4* pVertexBuffer)
{
	if (pMorphedMesh == nullptr || pCage == nullptr)
		return false;

	int numCageVerts = pCage->GetControlPointsCount();
	for (int i = 0; i < numCageVerts; i++)
	{
		auto results = m_MvcWeightsTable.find(i);
		if (results == m_MvcWeightsTable.end())
		{
			pVertexBuffer[i] = FbxVector4(NAN, NAN, NAN);
			continue;
		}
		QVector<double>* pMvcWeights = results.value();
		FbxVector4* pMorphedVertexBuffer = pMorphedMesh->GetControlPoints();

		FbxVector4 newVertexPosition = MvcTools::deform_using_mean_value_coordinates(pMorphedMesh, pMorphedVertexBuffer, pMvcWeights);

		pVertexBuffer[i] = newVertexPosition;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////


bool MvcTools::makeTestMvcCage(QVector<FbxVector4>* &mvc_test_cage_vertexbuffer, QVector<int>* &mvc_test_cage_triangles)
{
	double cage_width = 140;
	double cage_depth = 50;
	double cage_height = 180;
	double cage_floor = -0;

	//double cage_width = 200;
	//double cage_depth = 200;
	//double cage_height = 200;
	//double cage_floor = -200;

	QVector<FbxVector4> *vertexbuffer = new QVector<FbxVector4>(8);
	(*vertexbuffer)[0] = FbxVector4(-cage_width, cage_floor, cage_depth);
	(*vertexbuffer)[1] = FbxVector4(-cage_width, cage_floor, -cage_depth);
	(*vertexbuffer)[2] = FbxVector4(cage_width, cage_floor, -cage_depth);
	(*vertexbuffer)[3] = FbxVector4(cage_width, cage_floor, cage_depth);

	(*vertexbuffer)[4] = FbxVector4(-cage_width, cage_height, cage_depth);
	(*vertexbuffer)[5] = FbxVector4(-cage_width, cage_height, -cage_depth);
	(*vertexbuffer)[6] = FbxVector4(cage_width, cage_height, -cage_depth);
	(*vertexbuffer)[7] = FbxVector4(cage_width, cage_height, cage_depth);

	QVector<int> *triangles = new QVector<int>(12*3);
	int v=0;
	// bottom
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 3;
	(*triangles)[v * 3 + 2] = 1;
	v++;
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 1;
	v++;

	// top
	(*triangles)[v * 3 + 0] = 4;
	(*triangles)[v * 3 + 1] = 5;
	(*triangles)[v * 3 + 2] = 7;
	v++;
	(*triangles)[v * 3 + 0] = 7;
	(*triangles)[v * 3 + 1] = 5;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	// back
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 4;
	(*triangles)[v * 3 + 2] = 3;
	v++;
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 4;
	(*triangles)[v * 3 + 2] = 7;
	v++;

	// front
	(*triangles)[v * 3 + 0] = 1;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 5;
	v++;
	(*triangles)[v * 3 + 0] = 5;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	// right
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 1;
	(*triangles)[v * 3 + 2] = 4;
	v++;
	(*triangles)[v * 3 + 0] = 4;
	(*triangles)[v * 3 + 1] = 1;
	(*triangles)[v * 3 + 2] = 5;
	v++;

	// left
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 7;
	(*triangles)[v * 3 + 2] = 2;
	v++;
	(*triangles)[v * 3 + 0] = 2;
	(*triangles)[v * 3 + 1] = 7;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	mvc_test_cage_vertexbuffer = vertexbuffer;
	mvc_test_cage_triangles = triangles;

	return true;
}

bool MvcTools::makeTestMvcCage_2(QVector<FbxVector4>* &mvc_test_cage_vertexbuffer, QVector<int>* &mvc_test_cage_triangles)
{
	//double cage_width = 140;
	//double cage_depth = 50;
	//double cage_height = 180;
	//double cage_floor = -0;

	double cage_width = 140;
	double cage_depth = 50;
	double cage_height = 200;
	double cage_floor = -0;
	QVector<FbxVector4>* vertexbuffer = new QVector<FbxVector4>(8);
	(*vertexbuffer)[0] = FbxVector4(-cage_width, cage_floor, cage_depth);
	(*vertexbuffer)[1] = FbxVector4(-cage_width, cage_floor, -cage_depth);
	(*vertexbuffer)[2] = FbxVector4(cage_width, cage_floor, -cage_depth);
	(*vertexbuffer)[3] = FbxVector4(cage_width, cage_floor, cage_depth);

	cage_width = 14;
	cage_depth = 5;
	(*vertexbuffer)[4] = FbxVector4(-cage_width, cage_height, cage_depth);
	(*vertexbuffer)[5] = FbxVector4(-cage_width, cage_height, -cage_depth);
	(*vertexbuffer)[6] = FbxVector4(cage_width, cage_height, -cage_depth);
	(*vertexbuffer)[7] = FbxVector4(cage_width, cage_height, cage_depth);

	QVector<int>* triangles = new QVector<int>(12 * 3);
	int v = 0;
	// bottom
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 3;
	(*triangles)[v * 3 + 2] = 1;
	v++;
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 1;
	v++;

	// top
	(*triangles)[v * 3 + 0] = 4;
	(*triangles)[v * 3 + 1] = 5;
	(*triangles)[v * 3 + 2] = 7;
	v++;
	(*triangles)[v * 3 + 0] = 7;
	(*triangles)[v * 3 + 1] = 5;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	// back
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 4;
	(*triangles)[v * 3 + 2] = 3;
	v++;
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 4;
	(*triangles)[v * 3 + 2] = 7;
	v++;

	// front
	(*triangles)[v * 3 + 0] = 1;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 5;
	v++;
	(*triangles)[v * 3 + 0] = 5;
	(*triangles)[v * 3 + 1] = 2;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	// right
	(*triangles)[v * 3 + 0] = 0;
	(*triangles)[v * 3 + 1] = 1;
	(*triangles)[v * 3 + 2] = 4;
	v++;
	(*triangles)[v * 3 + 0] = 4;
	(*triangles)[v * 3 + 1] = 1;
	(*triangles)[v * 3 + 2] = 5;
	v++;

	// left
	(*triangles)[v * 3 + 0] = 3;
	(*triangles)[v * 3 + 1] = 7;
	(*triangles)[v * 3 + 2] = 2;
	v++;
	(*triangles)[v * 3 + 0] = 2;
	(*triangles)[v * 3 + 1] = 7;
	(*triangles)[v * 3 + 2] = 6;
	v++;

	mvc_test_cage_vertexbuffer = vertexbuffer;
	mvc_test_cage_triangles = triangles;

	return true;
}
//
//bool MakeTestMvcCage_3()
//{
//	QString sFbxFilePath = "c:/dev/sphere2.fbx";
//	auto oFBX = new OpenFBXInterface();
//	FbxScene* pScene = oFBX->CreateScene("mvc control cage");
//	if (oFBX->LoadScene(pScene, sFbxFilePath.toLocal8Bit().data()) == false)
//	{
//		return false;
//	}
//	FbxMesh* pMesh = (FbxMesh*) pScene->GetGeometry(0);
//	if (pMesh == nullptr)
//	{
//		return false;
//	}
//	int numVerts = pMesh->GetControlPointsCount();
//	FbxVector4* fbx_vertbuffer = pMesh->GetControlPoints();
//
//	QVector<FbxVector4>* vertexbuffer = new QVector<FbxVector4>(numVerts);
//
//	for (int i = 0; i < numVerts; i++)
//	{
//		(*vertexbuffer)[i][0] = fbx_vertbuffer[i][0];
//		(*vertexbuffer)[i][1] = fbx_vertbuffer[i][1]-15.0;
//		(*vertexbuffer)[i][2] = fbx_vertbuffer[i][2];
//	}
//
//	int polyCount = pMesh->GetPolygonCount();
//	int polyVertCount = pMesh->GetPolygonVertexCount();
//
//	int vertsPerPoly = pMesh->GetPolygonSize(0);
//	bool bTriangulate = false;
//	if (vertsPerPoly > 3)
//	{
//		bTriangulate = true;
//	}
//	assert(vertsPerPoly == 3);
//
//	QVector<int>* triangles = new QVector<int>(polyVertCount);
//	for (int i = 0; i < polyCount; i++)
//	{
//		int polySize = pMesh->GetPolygonSize(i);
//		if (polySize != 3)
//		{
//			//printf("ERROR: ignoring polysize %d", polySize);
//		}
//		for (int j = 0; j < 3; j++)
//		{
//			(*triangles)[i*3 + j] = pMesh->GetPolygonVertex(i, j);
//		}
//	}
//
//	mvc_test_cage_vertexbuffer = vertexbuffer;
//	mvc_test_cage_triangles = triangles;
//
//	return true;
//}

bool MvcTools::testMvc(DzNode *selected)
{

	DzPnt3* pVerts = nullptr;
	DzGeometry* pGeo = nullptr;
	int numVerts = 0;

	QVector<FbxVector4>* mvc_test_cage_vertexbuffer;
	QVector<int>* mvc_test_cage_triangles;
	QVector<FbxVector4>* mvc_test_cage_vertexbuffer_2;
	QVector<int>* mvc_test_cage_triangles_2;

	//MakeTestMvcCage_3();
	makeTestMvcCage(mvc_test_cage_vertexbuffer, mvc_test_cage_triangles);
	makeTestMvcCage_2(mvc_test_cage_vertexbuffer_2, mvc_test_cage_triangles_2);

	FbxVector4 origin(0,0,0);

	QVector<FbxVector4> vertList;

	if (selected)
	{
		pGeo = selected->getObject()->getShape(0)->getGeometry();
		pVerts = pGeo->getVerticesPtr();
		numVerts = pGeo->getNumVertices();
		pGeo->beginEdit();
		vertList.resize(numVerts);
	}
	else
	{
		vertList.append(origin);
		vertList.append(FbxVector4(-100, -100, -100));
		vertList.append(FbxVector4(-110, -110, -110));
		vertList.append(FbxVector4(100, 0, 100));
		vertList.append(FbxVector4(110, 0, 110));
		vertList.append(FbxVector4(1000, 0, 100));
	}

	for (int i = 0; i < numVerts; i++)
	{
		vertList[i][0] = pVerts[i][0];
		vertList[i][1] = pVerts[i][1];
		vertList[i][2] = pVerts[i][2];
	}

	double epsilon = std::numeric_limits<double>::epsilon() * 100.0;
	for (int i=0; i < vertList.count(); i++)
	{
		FbxVector4 point = vertList[i];
		QVector<double>* pMvcWeights = new QVector<double>(mvc_test_cage_vertexbuffer->count(), 0.0);
		calculate_mean_value_coordinate_weights(*mvc_test_cage_vertexbuffer, *mvc_test_cage_triangles, point, pMvcWeights);
		//recalibrate_mean_value_coordinate_weights(*mvc_test_cage_vertexbuffer, point, pMvcWeights);
		FbxVector4 newPoint = deform_using_mean_value_coordinates(*mvc_test_cage_vertexbuffer, pMvcWeights, point);
		FbxVector4 newPoint_2 = deform_using_mean_value_coordinates(*mvc_test_cage_vertexbuffer_2, pMvcWeights, point);

		FbxVector4 delta = newPoint - point;
		FbxVector4 delta2 = newPoint_2 - point;

		if (fabs(delta[0]) > epsilon ||
			fabs(delta[1]) > epsilon ||
			fabs(delta[2]) > epsilon)
		{
			//printf("failed restore");
		}
		else
		{
			//printf("successly restored");
		}

		//if (fabs(delta2[0]) > epsilon ||
		//	fabs(delta2[1]) > epsilon ||
		//	fabs(delta2[2]) > epsilon)
		//{
		//	printf("successly morphed");
		//}
		//else
		//{
		//	printf("failed morph");
		//}

		//vertList[i] = newPoint;
		vertList[i] = newPoint_2;

		//if (!isnan(newPoint[0]) &&
		//	!isnan(newPoint[1]) &&
		//	!isnan(newPoint[2]))
		//{
		//	vertList[i] = newPoint;
		//}
		//else
		//{
		//	printf("nop");
		//}
	}

	for (int i = 0; i < numVerts; i++)
	{
		pVerts[i][0] = vertList[i][0];
		pVerts[i][1] = vertList[i][1];
		pVerts[i][2] = vertList[i][2];

		pGeo->finishEdit();
	}

	return true;
}

#include "moc_MvcTools.cpp"
