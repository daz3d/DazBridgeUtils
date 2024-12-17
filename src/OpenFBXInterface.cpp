
/****************************************************************************************
 Portions of this file is based on source code from Autodesk,
   and is used under license below:

   Copyright (C) 2015 Autodesk, Inc.
   All rights reserved.
   Use of this software is subject to the terms of the Autodesk license agreement
   provided at the time of installation or download, or which otherwise accompanies
   this software in either electronic or hard copy form.
****************************************************************************************/

#include <QString>
#include <qcoreapplication.h>

#ifdef __APPLE__
#define USING_LIBSTDCPP     1
#endif
#include <fbxsdk.h>

#include <dzapp.h>

#include "OpenFBXInterface.h"

OpenFBXInterface* OpenFBXInterface::singleton = nullptr;

#ifdef __APPLE_CRASH__
QList<void*> crashfix_reallocTable;
int leakCounter = 0;
void* crashfix_realloc(void* oPtr, size_t memsize)
{
    void* newPtr;
    
    if (oPtr != NULL && crashfix_reallocTable.contains(oPtr)==false)
    {
        leakCounter++;
        newPtr = malloc(memsize);
    }
    else
    {
        newPtr = realloc(oPtr, memsize);
    }

    if (crashfix_reallocTable.contains(newPtr)==false)
        crashfix_reallocTable.append(newPtr);
    
    return newPtr;
}
#endif

// Constructor
OpenFBXInterface::OpenFBXInterface()
{
	// Create FbxManager
    FbxManager* result = FbxManager::Create();
	if (result == nullptr)
	{
		throw (std::runtime_error("OpenFBXInterface: could not create FbxManager"));
	}
    m_fbxManager = result;

#ifdef __APPLE_CRASH__
    FbxReallocProc origProc = FbxGetReallocHandler();
    FbxSetReallocHandler(crashfix_realloc);
#endif
    // Create FbxIOSettings
    m_fbxIOSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
#ifdef __APPLE_CRASH__
    FbxSetReallocHandler(origProc);
    crashfix_reallocTable.clear();
#endif
    
    m_fbxManager->SetIOSettings(m_fbxIOSettings);

	// Initialize Fbx Plugin folder
	FbxString appPath = FbxGetApplicationDirectory();
	m_fbxManager->LoadPluginsDirectory(appPath.Buffer());

	m_ErrorCode = 0;
	m_ErrorString = "";
	m_DefaultScene = CreateScene("DefaultScene");

}

// Destructor
OpenFBXInterface::~OpenFBXInterface()
{
	if (m_DefaultScene) m_DefaultScene->Destroy();
    if (m_fbxIOSettings) m_fbxIOSettings->Destroy();
    if (m_fbxManager) m_fbxManager->Destroy();
}

bool OpenFBXInterface::SaveScene(FbxScene* pScene, QString sFilename, int nFileFormat, bool bEmbedMedia)
{
	bool bStatus = true;

	// Create FbxExporter
	FbxExporter* pExporter = FbxExporter::Create(m_fbxManager, "");

	///////////////////////////////////
	// DEBUG
	///////////////////////////////////
	bool bUseAscii = false;
	if (bUseAscii)
	{
		int lFormatIndex, lFormatCount = m_fbxManager->GetIOPluginRegistry()->GetWriterFormatCount();
		for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
		{
			if (m_fbxManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
			{
				FbxString lDesc = m_fbxManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
				if (lDesc.Find("ascii") >= 0)
				{
					nFileFormat = lFormatIndex;
					break;
				}
			}
		}
	}

	// Check if fileformat is invalid
	if (nFileFormat < 0 || nFileFormat >= m_fbxManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// replace with valid format
		nFileFormat = m_fbxManager->GetIOPluginRegistry()->GetNativeWriterFormat();
	}

	m_fbxIOSettings->SetBoolProp(EXP_FBX_MATERIAL, true);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_TEXTURE, true);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_EMBEDDED, bEmbedMedia);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_SHAPE, true);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_GOBO, true);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_ANIMATION, true);
	m_fbxIOSettings->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	if (pExporter->Initialize(sFilename.toUtf8().data(), nFileFormat, m_fbxIOSettings) == false)
	{
		m_ErrorString = QString(pExporter->GetStatus().GetErrorString());
		m_ErrorCode = pExporter->GetStatus().GetCode();
		pExporter->Destroy();
		return false;
	}

	// DB 2024-09-17: Trying to fix race condition related to DCC/destination software import before DzBridgeAction::upgadeToHD()
	//    finishes writing missing bone weights for subdivided verts to file. 
	bool bNonBlocking = false;
//	bool bNonBlocking = true; // if false doesn't work, try true
	bStatus = pExporter->Export(pScene, bNonBlocking);
	if (!bStatus)
	{
		m_ErrorString = QString(pExporter->GetStatus().GetErrorString());
		m_ErrorCode = pExporter->GetStatus().GetCode();
	}
	else
	{
		// DB 2024-09-17: NonBlocking / race condition issues: try to wait for Export to really complete.
		do {
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		} while (pExporter->IsExporting(bStatus));
		// Sanity Check
		if (!bStatus) {
			m_ErrorString = QString(pExporter->GetStatus().GetErrorString());
			m_ErrorCode = pExporter->GetStatus().GetCode();
		}

	}

	pExporter->Destroy();

	return bStatus;

}

bool OpenFBXInterface::LoadScene(FbxScene* pScene, QString sFilename)
{
	bool bStatus = true;

	FbxImporter* pImporter = FbxImporter::Create(m_fbxManager, "");

	if (pImporter->Initialize(sFilename.toUtf8().data(), -1, m_fbxIOSettings) == false)
	{
		m_ErrorString = QString(pImporter->GetStatus().GetErrorString());
		m_ErrorCode = pImporter->GetStatus().GetCode();
		if (m_ErrorCode == FbxStatus::EStatusCode::eFailure) 
		{
			// wait 1 second and retry
			QString sErrorMessage = QString("OpenFbxInterface()::LoadScene(): FbxImporter::Initialize(%1) failed. [EStatusCode=%2]").arg(sFilename).arg(m_ErrorCode);
#ifdef DAZ_APP_H
			dzApp->log("ERROR: DzBridge: " + sErrorMessage + " - retrying after 1 second wait....");
#endif
			int nSleepTimeMs = 1000; // 1000 ms
#ifdef __APPLE__
			struct timespec ts = { nSleepTimeMs / 1000, (nSleepTimeMs % 1000) * 1000 * 1000 };
			nanosleep(&ts, NULL);
#else
			 _sleep(nSleepTimeMs);
#endif
			if (pImporter->Initialize(sFilename.toUtf8().data(), -1, m_fbxIOSettings) == false)
			{
				m_ErrorString = QString(pImporter->GetStatus().GetErrorString());
				m_ErrorCode = pImporter->GetStatus().GetCode();
				pImporter->Destroy();
				return false;
			}
			m_ErrorCode = 0;
			m_ErrorString = "";
		}
		else 
		{
			pImporter->Destroy();
			return false;
		}
	}

	if (pImporter->IsFBX() == false)
	{
		m_ErrorCode = -1;
		m_ErrorString = QString("OpenFBXInterface: loaded scene file has unrecognized FBX file format.");
		pImporter->Destroy();
		return false;
	}

	bStatus = pImporter->Import(pScene);
	if (!bStatus)
	{
		m_ErrorString = QString(pImporter->GetStatus().GetErrorString());
		m_ErrorCode = pImporter->GetStatus().GetCode();
	}

	pImporter->Destroy();
	return bStatus;

}

FbxScene* OpenFBXInterface::CreateScene(QString sSceneName)
{
	FbxScene* pNewScene = FbxScene::Create(m_fbxManager, sSceneName.toUtf8().data());

	return pNewScene;
}

FbxGeometry* OpenFBXInterface::FindGeometry(FbxScene* pScene, QString sGeometryName)
{
	int numGeometry = pScene->GetGeometryCount();
	for (int i = 0; i < numGeometry; i++)
	{
		FbxGeometry* geo = pScene->GetGeometry(i);
		FbxNode* node = geo->GetNode();
		auto raw_name = node->GetName();
		QString sGeoName(raw_name);
		if (sGeoName == sGeometryName)
		{
			return geo;
		}
	}
	return nullptr;
}

FbxNode* OpenFBXInterface::FindNode(FbxScene* pScene, QString sNodeName)
{
	FbxString fsName(sNodeName.toUtf8().data());
	auto result = pScene->FindNodeByName(fsName);
	return result;
}

#include "moc_OpenFBXInterface.cpp"
