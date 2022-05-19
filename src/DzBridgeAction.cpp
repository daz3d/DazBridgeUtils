#include <dzapp.h>
#include <dzscene.h>
#include <dzexportmgr.h>
#include <dzexporter.h>
#include <dzmainwindow.h>
#include <dzmaterial.h>
#include <dzproperty.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <dzimageproperty.h>
#include <dzstringproperty.h>
#include <dznumericproperty.h>
#include <dzfloatproperty.h>
#include <dzcolorproperty.h>
#include <dzstringproperty.h>
#include <dzenumproperty.h>
#include <dzboolproperty.h>
#include <dzobject.h>
#include <dzskeleton.h>
#include <dzfigure.h>
#include <dzshape.h>
#include <dzassetmgr.h>
#include <dzuri.h>
#include <dzcontentmgr.h>
#include <dzassetmetadata.h>
#include <dzbone.h>
#include <dzskeleton.h>
#include <dzpresentation.h>
#include <dzmodifier.h>
#include <dzmorph.h>
#include <dzprogress.h>
#include <dztexture.h>
#include <dzimagemgr.h>

#include "dzgeometry.h"
#include "dzweightmap.h"
#include "dzfacetshape.h"
#include "dzfacetmesh.h"
#include "dzfacegroup.h"
#include "dzmaterial.h"

#include <QtCore/qdir.h>
#include <QtGui/qlineedit.h>
#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/QMessageBox>
#include "QtCore/qmetaobject.h"

#include "DzBridgeAction.h"
#include "DzBridgeDialog.h"
#include "DzBridgeSubdivisionDialog.h"
#include "DzBridgeMorphSelectionDialog.h"

using namespace DzBridgeNameSpace;

/// <summary>
/// Initializes general export data and settings.
/// </summary>
DzBridgeAction::DzBridgeAction(const QString& text, const QString& desc) :
	 DzAction(text, desc)
{
	resetToDefaults();
	m_bridgeDialog = nullptr;
	m_subdivisionDialog = nullptr;
	m_morphSelectionDialog = nullptr;
	m_bGenerateNormalMaps = false;
	m_pSelectedNode = nullptr;

#ifdef _DEBUG
	 m_bUndoNormalMaps = false;
#else
	 m_bUndoNormalMaps = true;
#endif

}

DzBridgeAction::~DzBridgeAction()
{
}

/// <summary>
/// Resets export settings to default values.
/// </summary>
void DzBridgeAction::resetToDefaults()
{
	m_bEnableMorphs = false;
	m_EnableSubdivisions = false;
	m_bShowFbxOptions = false;
	m_ControllersToDisconnect.clear();
	m_ControllersToDisconnect.append("facs_bs_MouthClose_div2");

	// Reset all dialog settings and script-exposed properties to Hardcoded Defaults
	// Ignore saved settings, QSettings, etc.
	DzNode* selection = dzScene->getPrimarySelection();
	DzFigure* figure = qobject_cast<DzFigure*>(selection);
	if (selection)
	{
		if (dzScene->getFilename().length() > 0)
		{
			QFileInfo fileInfo = QFileInfo(dzScene->getFilename());
			m_sAssetName = fileInfo.baseName().remove(QRegExp("[^A-Za-z0-9_]"));
		}
		else
		{
			m_sAssetName = this->cleanString(selection->getLabel());
		}
	}
	else
	{
		m_sAssetName = "";
	}
	if (figure)
	{
		m_sAssetType = "SkeletalMesh";
	}
	else
	{
		m_sAssetType = "StaticMesh";
	}

	m_sProductName = "";
	m_sProductComponentName = "";
	m_aMorphListOverride.clear();
	m_bUseRelativePaths = false;
	m_bUndoNormalMaps = true;
	m_nNonInteractiveMode = 0;
	m_undoTable_DuplicateMaterialRename.clear();
	m_undoTable_GenerateMissingNormalMap.clear();
	m_sExportFbx = "";

}

/// <summary>
/// Performs multiple pre-processing procedures prior to exporting FBX and generating DTU.
///
/// Usage: Usually called from executeAction() prior to calling Export() or ExportHD().
/// See Also: undoPreProcessScene()
/// </summary>
/// <param name="parentNode">The "root" node from which to start processing of all
/// children.  If null, then scene primary selection is used.</param>
/// <returns>true if procedure was successful</returns>
bool DzBridgeAction::preProcessScene(DzNode* parentNode)
{
	DzProgress preProcessProgress = DzProgress("Daz Bridge Pre-Processing...", 0, false, true);

	///////////////////////
	// Create JobPool
	// Iterate through all children of "parentNode", create jobpool of nodes to process later
	// Nodes are added to nodeJobList in breadth-first order (parent,children,grandchildren)
	///////////////////////
	DzNodeList nodeJobList;
	DzNodeList tempQueue;
	DzNode *node_ptr = parentNode;
	if (node_ptr == nullptr)
		node_ptr = dzScene->getPrimarySelection();
	if (node_ptr == nullptr)
		return false;

	tempQueue.append(node_ptr);
	while (!tempQueue.isEmpty())
	{
		node_ptr = tempQueue.first();
		tempQueue.removeFirst();
		nodeJobList.append(node_ptr);
		DzNodeListIterator Iterator = node_ptr->nodeChildrenIterator();
		while (Iterator.hasNext())
		{
			DzNode* tempChild = Iterator.next();
			tempQueue.append(tempChild);
		}
	}

	///////////////////////
	// Process JobPool (DzNodeList nodeJobList)
	///////////////////////
	QList<QString> existingMaterialNameList;
	for (int i = 0; i < nodeJobList.length(); i++)
	{
		DzNode *node = nodeJobList[i];
		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;
		if (shape)
		{
			for (int i = 0; i < shape->getNumMaterials(); i++)
			{
				DzMaterial* material = shape->getMaterial(i);
				if (material)
				{
					//////////////////
					// Rename Duplicate Material
					/////////////////
					renameDuplicateMaterial(material, &existingMaterialNameList);

					/////////////////
					// Generate Missing Normal Maps
					/////////////////
					if (m_bGenerateNormalMaps)
						generateMissingNormalMap(material);
				}
			}
		}
	}

	preProcessProgress.finish();

	return true;
}

/// <summary>
/// Generate Normal Map texture for for use in Target Software that doesn't support HeightMap.
/// Called by preProcessScene() for each exported material. Checks material for existing
/// HeightMap texture but missing NormalMap texture before generating NormalMap. Exports HeightMap
/// strength to NormalMap strength in DTU file.
///
/// Note: Must call undoGenerateMissingNormalMaps() to undo insertion of NormalMaps into materials.
///
/// See Also: makeNormalMapFromHeightMap(), m_undoTable_GenerateMissingNormalMap,
/// preProcessScene(), undoPreProcessScene().
/// </summary>
/// <returns>true if normalmap was generated</returns>
bool DzBridgeAction::generateMissingNormalMap(DzMaterial* material)
{
	if (material == nullptr)
		return false;

	bool bNormalMapWasGenerated = false;

	// Check if normal map missing
	if (isNormalMapMissing(material))
	{
		// Check if height map present
		if (isHeightMapPresent(material))
		{
			// Generate normal map from height map
			QString heightMapFilename = getHeightMapFilename(material);
			if (heightMapFilename != "")
			{
				// Retrieve Normap Map property
				QString propertyName = "normal map";
				DzProperty* normalMapProp = material->findProperty(propertyName, false);
				if (normalMapProp)
				{
					DzImageProperty* imageProp = qobject_cast<DzImageProperty*>(normalMapProp);
					DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(normalMapProp);

					// calculate normal map strength based on height map strength
					double conversionFactor = 0.5;
					QString shaderName = material->getMaterialName().toLower();
					if (shaderName.contains("aoa_subsurface"))
					{
						conversionFactor = 3.0;
					}
					else if (shaderName.contains("omubersurface"))
					{
						double bumpMin = -0.1;
						double bumpMax = 0.1;
						DzNumericProperty *bumpMinProp = qobject_cast<DzNumericProperty*>(material->findProperty("bump minimum", false));
						DzNumericProperty *bumpMaxProp = qobject_cast<DzNumericProperty*>(material->findProperty("bump maximum", false));
						if (bumpMinProp)
						{
							bumpMin = bumpMinProp->getDoubleValue();
						}
						if (bumpMaxProp)
						{
							bumpMax = bumpMaxProp->getDoubleValue();
						}
						double range = bumpMax - bumpMin;
						conversionFactor = range * 25;
					}
					double heightStrength = getHeightMapStrength(material);
					double normalStrength = heightStrength * conversionFactor;
					double bakeStrength = 1.0;
					// If not numeric property, then save normal map strength to external
					//   value so it can be added into the DTU file on export.
					if (!numericProp && imageProp)
					{
						// normalStrengthTable <material, normalstrength>
						m_imgPropertyTable_NormalMapStrength.insert(imageProp, normalStrength);
					}

					// create normalMap filename
					QString tempPath;
					QFileInfo fileInfo = QFileInfo(heightMapFilename);
					//QString normalMapFilename = fileInfo.completeBaseName() + "_nm." + fileInfo.suffix();
					QString normalMapFilename = fileInfo.completeBaseName() + "_nm." + "png";
					QString normalMapSavePath = dzApp->getTempPath() + "/" + normalMapFilename;
					QFileInfo normalMapInfo = QFileInfo(normalMapSavePath);

					// Generate Temp Normal Map if does not already exist.
					// If it does exist then assume it was generated for previous material
					// and re-use it for this material.
					if (!normalMapInfo.exists())
					{
						QImage normalMap = makeNormalMapFromHeightMap(heightMapFilename, bakeStrength);
						QString progressString = "Saving Normal Map: " + normalMapSavePath;
						DzProgress saveProgress = DzProgress(progressString, 2, false, true);
						saveProgress.step();
						normalMap.save(normalMapSavePath, 0, 75);
						saveProgress.step();
						saveProgress.finish();
					}

					// Insert generated NormalMap into Daz material
					if (numericProp)
					{
						numericProp->setMap(normalMapSavePath);
						numericProp->setDoubleValue(normalStrength);
					}
					else if (imageProp)
					{
						imageProp->setValue(normalMapSavePath);
					}

					if (m_bUndoNormalMaps)
					{
						// Add to Undo Table
						m_undoTable_GenerateMissingNormalMap.insert(material, normalMapProp);
					}

					bNormalMapWasGenerated = true;
				}
			}
		}
	}

	return bNormalMapWasGenerated;
}

/// <summary>
/// Revert changes to materials made by GenerateMissingNormalMaps().
/// Called by undoPreProcessScene() after Export() or exportHD().
/// </summary>
/// <returns>true if the undo process completed successfully.</returns>
bool DzBridgeAction::undoGenerateMissingNormalMaps()
{
	QMap<DzMaterial*, DzProperty*>::iterator iter;
	for (iter = m_undoTable_GenerateMissingNormalMap.begin(); iter != m_undoTable_GenerateMissingNormalMap.end(); ++iter)
	{
		DzProperty* normalMapProp = iter.value();
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(normalMapProp);
		DzImageProperty* imageProp = qobject_cast<DzImageProperty*>(normalMapProp);
		if (numericProp)
		{
			numericProp->setDoubleValue(numericProp->getDoubleDefaultValue());
			numericProp->setMap(nullptr);
		}
		else if (imageProp)
		{
			imageProp->setValue(nullptr);
		}
	}
	m_undoTable_GenerateMissingNormalMap.clear();

	return true;
}

/// <summary>
/// Retrieve HeightMap Strength from material's "bump strength" property.
/// Called by generateMissingNormalMap().
/// </summary>
/// <returns>value of heightmap strength if it exists,
/// or 0.0 if it does not exist</returns>
double DzBridgeAction::getHeightMapStrength(DzMaterial* material)
{
	if (material == nullptr)
		return false;

	QString propertyName = "bump strength";
	DzProperty* heightMapProp = material->findProperty(propertyName, false);

	if (heightMapProp)
	{
		// DEBUG
		QString propertyName = heightMapProp->getName();
		QString propertyLabel = heightMapProp->getLabel();

		// normal map property preseet
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(heightMapProp);

		if (numericProp)
		{
			double heightStrength = numericProp->getDoubleValue();
			return heightStrength;
		}

	}

	return 0.0;

}


/// <returns>filename stored in material's "bump strength" property if it exists,
/// QString("") if it does not.</returns>
QString DzBridgeAction::getHeightMapFilename(DzMaterial* material)
{
	if (material == nullptr)
		return QString("");

	QString propertyName = "bump strength";
	DzProperty* heightMapProp = material->findProperty(propertyName, false);

	if (heightMapProp)
	{
		// DEBUG
		QString propertyName = heightMapProp->getName();
		QString propertyLabel = heightMapProp->getLabel();

		// normal map property preseet
		DzImageProperty* imageProp = qobject_cast<DzImageProperty*>(heightMapProp);
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(heightMapProp);

		if (imageProp)
		{
			if (imageProp->getValue())
			{
				QString heightMapFilename = imageProp->getValue()->getFilename();
				return heightMapFilename;
			}
		}
		else if (numericProp)
		{
			if (numericProp->getMapValue())
			{
				QString heightMapFilename = numericProp->getMapValue()->getFilename();
				return heightMapFilename;
			}
		}

		// Bump Map property present, missing mapvalue
		return "";
	}

	// DEBUG
	QString materialName = material->getName();
	QString materialLabel = material->getLabel();

	return "";
}

/// <returns>true if "normal map" property exists AND property does not have filename set</returns>
bool DzBridgeAction::isNormalMapMissing(DzMaterial* material)
{
	if (material == nullptr)
		return false;

	QString propertyName = "normal map";
	DzProperty* normalMapProp = material->findProperty(propertyName, false);

	if (normalMapProp)
	{
		// DEBUG
		QString propertyName = normalMapProp->getName();
		QString propertyLabel = normalMapProp->getLabel();

		// normal map property preseet
		DzImageProperty* imageProp = qobject_cast<DzImageProperty*>(normalMapProp);
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(normalMapProp);

		if (imageProp)
		{
			if (imageProp->getValue())
			{
				QString normalMapFilename = imageProp->getValue()->getFilename();
				if (normalMapFilename == "")
				{
					// image-type normal map property present, value is empty string
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				// image property is missing texture
				return true;
			}
		}
		else if (numericProp)
		{
			if (numericProp->getMapValue())
			{
				QString normalMapFilename = numericProp->getMapValue()->getFilename();
				if (normalMapFilename == "")
				{
					// numeric-type normal map property present, map value is empty string
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				// numeric-type normal map property present, no map value
				return true;
			}
		}

		// normal map property exists...
		return false;
	}

#ifdef _DEBUG
	QString materialName = material->getName();
	QString materialLabel = material->getLabel();
	QString shaderName = material->getMaterialName();
#endif

	// normal map property does not exist
	return false;
}

/// <returns>true if material has a "bump strength" property</returns>
bool DzBridgeAction::isHeightMapPresent(DzMaterial* material)
{
	QString propertyName = "bump strength";
	DzProperty* heightMapProp = material->findProperty(propertyName, false);

	if (heightMapProp)
	{
		return true;
	}

	return false;
}

/// <summary>
/// Undo changes performed by preProcessScene().
/// </summary>
/// <returns>true if all undo procedures are successful</returns>
bool DzBridgeAction::undoPreProcessScene()
{
	bool bResult = true;

	if (undoRenameDuplicateMaterials() == false)
	{
		bResult = false;
	}

	// Undo Inject Normal Maps
	if (undoGenerateMissingNormalMaps() == false)
	{
		bResult = false;
	}

	return bResult;
}

/// <summary>
/// Rename material if its name is already used by existing material(s) to
/// prevent collisions in Target Software. Called by preProcessScene().
/// See also: undoRenameMaterialDuplicateMaterials().
/// </summary>
/// <returns>true if successful</returns>
bool DzBridgeAction::renameDuplicateMaterial(DzMaterial *material, QList<QString>* existingMaterialNameList)
{
	if (material == nullptr)
		return false;

	int nameIndex = 0;
	QString newMaterialName = material->getName();
	while (existingMaterialNameList->contains(newMaterialName))
	{
		newMaterialName = material->getName() + QString("_%1").arg(++nameIndex);
	}
	if (newMaterialName != material->getName())
	{
		// Add to Undo Table
		m_undoTable_DuplicateMaterialRename.insert(material, material->getName());
		material->setName(newMaterialName);
	}
	existingMaterialNameList->append(newMaterialName);

	return true;
}

/// <summary>
/// Undo any materials modified by renameDuplicaetMaterials().
/// </summary>
/// <returns>true if successful</returns>
bool DzBridgeAction::undoRenameDuplicateMaterials()
{
	QMap<DzMaterial*, QString>::iterator iter;
	for (iter = m_undoTable_DuplicateMaterialRename.begin(); iter != m_undoTable_DuplicateMaterialRename.end(); ++iter)
	{
		iter.key()->setName(iter.value());
	}
	m_undoTable_DuplicateMaterialRename.clear();

	return true;

}

/// <summary>
/// Convenience method to export base mesh and HD mesh as needed.
/// Performs weightmap fix on subdivided mesh.
/// See also: upgradeToHD(), Export().
/// </summary>
/// <param name="exportProgress">if null, exportHD will handle UI progress updates</param>
void DzBridgeAction::exportHD(DzProgress* exportProgress)
{
	if (m_subdivisionDialog == nullptr)
		return;

	bool bLocalDzProgress = false;
	if (!exportProgress)
	{
		bLocalDzProgress = true;
		exportProgress = new DzProgress(tr("DazBridge: Exporting FBX/DTU"), 8, false, true);
		exportProgress->setCloseOnFinish(true);
		exportProgress->enable(true);
		exportProgress->step();
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	if (m_EnableSubdivisions)
	{
		if (exportProgress)
		{
			exportProgress->setInfo(tr("Exporting Base Mesh..."));
			exportProgress->step();
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}
		m_subdivisionDialog->LockSubdivisionProperties(false);
		m_bExportingBaseMesh = true;
		exportAsset();
		m_subdivisionDialog->UnlockSubdivisionProperties();
		if (exportProgress)
		{
			exportProgress->setInfo(tr("Base mesh exported."));
			exportProgress->step();
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

	}

	if (exportProgress)
	{
		exportProgress->step();
		if (m_EnableSubdivisions)
			exportProgress->setInfo(tr("Exporting HD Mesh..."));
		else
			exportProgress->setInfo(tr("Exporting Mesh..."));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}
	m_subdivisionDialog->LockSubdivisionProperties(m_EnableSubdivisions);
	m_bExportingBaseMesh = false;
	exportAsset();
	if (exportProgress)
	{
		exportProgress->step();
		exportProgress->setInfo(tr("Mesh exported."));
	}

	if (m_EnableSubdivisions)
	{
		if (exportProgress)
		{
			exportProgress->step();
			exportProgress->setInfo(tr("Fixing weightmaps on HD Mesh..."));
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

		std::map<std::string, int>* pLookupTable = m_subdivisionDialog->GetLookupTable();
		QString BaseCharacterFBX = m_sDestinationPath + m_sAssetName + "_base.fbx";
		// DB 2021-10-02: Upgrade HD
		if (upgradeToHD(BaseCharacterFBX, m_sDestinationFBX, m_sDestinationFBX, pLookupTable) == false)
		{
			if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
				tr("There was an error during the Subdivision Surface refinement operation, the exported Daz model may not work correctly."), QMessageBox::Ok);
		}
		else
		{
			// remove intermediate base character fbx
			// Sanity Check
			if (QFile::exists(BaseCharacterFBX))
			{
//				QFile::remove(BaseCharacterFBX);
			}
		}
		delete(pLookupTable);
		if (exportProgress)
		{
			exportProgress->step();
			exportProgress->setInfo(tr("HD weightmaps fixed."));
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

	}

	// DB 2021-09-02: Unlock and Undo subdivision changes
	m_subdivisionDialog->UnlockSubdivisionProperties();
	if (exportProgress)
	{
		exportProgress->step();
		exportProgress->setInfo(tr("Mesh export complete."));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}

	if (bLocalDzProgress)
	{
		exportProgress->finish();
		// 2022-02-13 (DB): Generic messagebox "Export Complete"
		if (m_nNonInteractiveMode == 0)
		{
			QMessageBox::information(0, "DazBridge",
				tr("Export phase from Daz Studio complete. Please switch to Unity to begin Import phase."), QMessageBox::Ok);
		}

	}

}

void DzBridgeAction::exportAsset()
{
	// FBX Export
	m_pSelectedNode = dzScene->getPrimarySelection();
	if (m_pSelectedNode == nullptr)
		return;

	QMap<QString, DzNode*> PropToInstance;
	if (m_sAssetType == "Environment")
	{
		// Store off the original export information
		QString OriginalCharacterName = m_sAssetName;
		DzNode* OriginalSelection = m_pSelectedNode;

		// Find all the different types of props in the scene
		getScenePropList(m_pSelectedNode, PropToInstance);
		QMap<QString, DzNode*>::iterator iter;
		for (iter = PropToInstance.begin(); iter != PropToInstance.end(); ++iter)
		{
			// Override the export info for exporting this prop
			m_sAssetType = "StaticMesh";
			m_sAssetName = iter.key();
			m_sAssetName = m_sAssetName.remove(QRegExp("[^A-Za-z0-9_]"));
			m_sDestinationPath = m_sRootFolder + "/" + m_sAssetName + "/";
			m_sDestinationFBX = m_sDestinationPath + m_sAssetName + ".fbx";
			DzNode* Node = iter.value();

			// If this is a figure, send it as a skeletal mesh
			if (DzSkeleton* Skeleton = Node->getSkeleton())
			{
				if (DzFigure* Figure = qobject_cast<DzFigure*>(Skeleton))
				{
					m_sAssetType = "SkeletalMesh";
				}
			}

			//Unlock the transform controls so the node can be moved to root
			unlockTranform(Node);

			// Disconnect the asset being sent from everything else
			QList<AttachmentInfo> AttachmentList;
			disconnectNode(Node, AttachmentList);

			// Set the selection so this will be the exported asset
			m_pSelectedNode = Node;

			// Store the current transform and zero it out.
			DzVec3 Location;
			DzQuat Rotation;
			DzMatrix3 Scale;

			Node->getWSTransform(Location, Rotation, Scale);
			Node->setWSTransform(DzVec3(0.0f, 0.0f, 0.0f), DzQuat(), DzMatrix3(true));

			// Export
			exportNode(Node);

			// Put the item back where it was
			Node->setWSTransform(Location, Rotation, Scale);

			// Reconnect all the nodes
			reconnectNodes(AttachmentList);
		}

		// After the props have been exported, export the environment
		m_sAssetName = OriginalCharacterName;
		m_sDestinationPath = m_sRootFolder + "/" + m_sExportSubfolder + "/";
		// use original export fbx filestem, if exists
		if (m_sExportFbx == "") m_sExportFbx = m_sAssetName;
		m_sDestinationFBX = m_sDestinationPath + m_sExportFbx + ".fbx";
		m_pSelectedNode = OriginalSelection;
		m_sAssetType = "Environment";
		exportNode(m_pSelectedNode);
	}
	else if (m_sAssetType == "Pose")
	{
		if (checkIfPoseExportIsDestructive())
		{
			if (QMessageBox::question(0, tr("Continue"),
				tr("Proceeding will delete keyed values on some properties. Continue?"),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			{
				return;
			}
		}

		m_aPoseList.clear();
		DzNode* Selection = dzScene->getPrimarySelection();
		int poseIndex = 0;
		DzNumericProperty* previousProperty = nullptr;
		for (int index = 0; index < Selection->getNumProperties(); index++)
		{
			DzProperty* property = Selection->getProperty(index);
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			QString propName = property->getName();
			if (numericProperty)
			{
				QString propName = property->getName();
				if (m_mMorphNameToLabel.contains(propName))
				{
					poseIndex++;
					numericProperty->setDoubleValue(0.0f, 0.0f);
					for (int frame = 0; frame < m_mMorphNameToLabel.count() + 1; frame++)
					{
						numericProperty->setDoubleValue(dzScene->getTimeStep() * double(frame), 0.0f);
					}
					numericProperty->setDoubleValue(dzScene->getTimeStep() * double(poseIndex),1.0f);
					m_aPoseList.append(propName);
				}
			}
		}

		DzObject* Object = Selection->getObject();
		if (Object)
		{
			for (int index = 0; index < Object->getNumModifiers(); index++)
			{
				DzModifier* modifier = Object->getModifier(index);
				DzMorph* mod = qobject_cast<DzMorph*>(modifier);
				if (mod)
				{
					for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
					{
						DzProperty* property = modifier->getProperty(propindex);
						QString propName = property->getName();
						QString propLabel = property->getLabel();
						DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
						if (numericProperty)
						{
							QString propName = property->getName();
							//qDebug() << propName;
							if (m_mMorphNameToLabel.contains(modifier->getName()))
							{
								poseIndex++;
								numericProperty->setDoubleValue(0.0f, 0.0f);
								for (int frame = 0; frame < m_mMorphNameToLabel.count() + 1; frame++)
								{
									numericProperty->setDoubleValue(dzScene->getTimeStep() * double(frame), 0.0f);
								}
								numericProperty->setDoubleValue(dzScene->getTimeStep() * double(poseIndex), 1.0f);
								m_aPoseList.append(modifier->getName());
							}
						}
					}

				}

			}
		}

		dzScene->setAnimRange(DzTimeRange(0, poseIndex * dzScene->getTimeStep()));
		dzScene->setPlayRange(DzTimeRange(0, poseIndex * dzScene->getTimeStep()));

		exportNode(Selection);
	}
	else if (m_sAssetType == "SkeletalMesh")
	{
		QList<QString> DisconnectedModifiers = disconnectOverrideControllers();
		DzNode* Selection = dzScene->getPrimarySelection();
		exportNode(Selection);
		reconnectOverrideControllers(DisconnectedModifiers);
	}
	else
	{
		DzNode* Selection = dzScene->getPrimarySelection();
		exportNode(Selection);
	}
}

void DzBridgeAction::disconnectNode(DzNode* Node, QList<AttachmentInfo>& AttachmentList)
{
	if (Node == nullptr)
		return;

	AttachmentInfo ParentAttachment;
	if (Node->getNodeParent())
	{
		// Don't disconnect a figures bones
		if (DzBone* Bone = qobject_cast<DzBone*>(Node))
		{

		}
		else
		{
			ParentAttachment.Parent = Node->getNodeParent();
			ParentAttachment.Child = Node;
			AttachmentList.append(ParentAttachment);
			Node->getNodeParent()->removeNodeChild(Node);
		}
	}

	QList<DzNode*> ChildNodes;
	for (int ChildIndex = Node->getNumNodeChildren() - 1; ChildIndex >= 0; ChildIndex--)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		if (DzBone* Bone = qobject_cast<DzBone*>(ChildNode))
		{

		}
		else
		{
			DzNode* ChildNode = Node->getNodeChild(ChildIndex);
			AttachmentInfo ChildAttachment;
			ChildAttachment.Parent = Node;
			ChildAttachment.Child = ChildNode;
			AttachmentList.append(ChildAttachment);
			Node->removeNodeChild(ChildNode);
		}
		disconnectNode(ChildNode, AttachmentList);
	}
}

void DzBridgeAction::reconnectNodes(QList<AttachmentInfo>& AttachmentList)
{
	foreach(AttachmentInfo Attachment, AttachmentList)
	{
		Attachment.Parent->addNodeChild(Attachment.Child);
	}
}


void DzBridgeAction::exportNode(DzNode* Node)
{
	if (Node == nullptr)
		return;

	dzScene->selectAllNodes(false);
	 dzScene->setPrimarySelection(Node);

	 if (m_sAssetType == "Environment")
	 {
		 QDir dir;
		 dir.mkpath(m_sDestinationPath);
		 writeConfiguration();
		 return;
	 }

	 DzExportMgr* ExportManager = dzApp->getExportMgr();
	 DzExporter* Exporter = ExportManager->findExporterByClassName("DzFbxExporter");

	 if (Exporter)
	 {
		  DzFileIOSettings ExportOptions;
		  ExportOptions.setBoolValue("doSelected", true);
		  ExportOptions.setBoolValue("doVisible", false);
		  if (m_sAssetType == "SkeletalMesh" || m_sAssetType == "StaticMesh" || m_sAssetType == "Environment")
		  {
				ExportOptions.setBoolValue("doFigures", true);
				ExportOptions.setBoolValue("doProps", true);
		  }
		  else
		  {
				ExportOptions.setBoolValue("doFigures", true);
				ExportOptions.setBoolValue("doProps", false);
		  }
		  ExportOptions.setBoolValue("doLights", false);
		  ExportOptions.setBoolValue("doCameras", false);
		  if (m_sAssetType == "Animation")
		  {
			  ExportOptions.setBoolValue("doAnims", true);
		  }
		  else
		  {
			  ExportOptions.setBoolValue("doAnims", false);
		  }
		  if ((m_sAssetType == "Animation" || m_sAssetType == "SkeletalMesh") && m_bEnableMorphs && m_sMorphSelectionRule != "")
		  {
				ExportOptions.setBoolValue("doMorphs", true);
				ExportOptions.setStringValue("rules", m_sMorphSelectionRule);
		  }
		  else
		  {
				ExportOptions.setBoolValue("doMorphs", false);
				ExportOptions.setStringValue("rules", "");
		  }

		  ExportOptions.setStringValue("format", m_sFbxVersion);
		  ExportOptions.setIntValue("RunSilent", !m_bShowFbxOptions);

		  ExportOptions.setBoolValue("doEmbed", false);
		  ExportOptions.setBoolValue("doCopyTextures", false);
		  ExportOptions.setBoolValue("doDiffuseOpacity", false);
		  ExportOptions.setBoolValue("doMergeClothing", true);
		  ExportOptions.setBoolValue("doStaticClothing", false);
		  ExportOptions.setBoolValue("degradedSkinning", true);
		  ExportOptions.setBoolValue("degradedScaling", true);
		  ExportOptions.setBoolValue("doSubD", false);
		  ExportOptions.setBoolValue("doCollapseUVTiles", false);

		  // get the top level node for things like clothing so we don't get dupe material names
		  DzNode* Parent = Node;
		  if (m_sAssetType != "Environment")
		  {
			  while (Parent->getNodeParent() != NULL)
			  {
				  Parent = Parent->getNodeParent();
			  }
		  }

		  preProcessScene(Parent);

		  QDir dir;
		  dir.mkpath(m_sDestinationPath);

		  setExportOptions(ExportOptions);

		  if (m_EnableSubdivisions && m_bExportingBaseMesh)
		  {
			  QString CharacterBaseFBX = this->m_sDestinationFBX;
			  CharacterBaseFBX.replace(".fbx", "_base.fbx");
			  Exporter->writeFile(CharacterBaseFBX, &ExportOptions);
		  }
		  else
		  {
			  Exporter->writeFile(m_sDestinationFBX, &ExportOptions);
			  writeConfiguration();
		  }

		  undoPreProcessScene();
	 }
}

// If there are duplicate material names, save off the original and rename one
void DzBridgeAction::renameDuplicateMaterials(DzNode* Node, QList<QString>& MaterialNames, QMap<DzMaterial*, QString>& OriginalMaterialNames)
{
	if (Node == nullptr)
		return;

	 DzObject* Object = Node->getObject();
	 DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	 if (Shape)
	 {
		  for (int i = 0; i < Shape->getNumMaterials(); i++)
		  {
				DzMaterial* Material = Shape->getMaterial(i);
				if (Material)
				{
					 OriginalMaterialNames.insert(Material, Material->getName());
					 while (MaterialNames.contains(Material->getName()))
					 {
						  Material->setName(Material->getName() + "_1");
					 }
					 MaterialNames.append(Material->getName());
				}
		  }
	 }
	 DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	 while (Iterator.hasNext())
	 {
		  DzNode* Child = Iterator.next();
		  renameDuplicateMaterials(Child, MaterialNames, OriginalMaterialNames);
	 }
}

// Restore the original material names
void DzBridgeAction::undoRenameDuplicateMaterials(DzNode* Node, QList<QString>& MaterialNames, QMap<DzMaterial*, QString>& OriginalMaterialNames)
{
	 QMap<DzMaterial*, QString>::iterator iter;
	 for (iter = OriginalMaterialNames.begin(); iter != OriginalMaterialNames.end(); ++iter)
	 {
		  iter.key()->setName(iter.value());
	 }
}

void DzBridgeAction::getScenePropList(DzNode* Node, QMap<QString, DzNode*>& Types)
{
	if (Node == nullptr)
		return;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;
	DzGeometry* Geometry = Shape ? Shape->getGeometry() : NULL;
	DzSkeleton* Skeleton = Node->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;
	//QString AssetId = Node->getAssetId();
	//IDzSceneAsset::m_sAssetType Type = Node->getAssetType();

	// Use the FileName to generate a name for the prop to be exported
	QString Path = Node->getAssetFileInfo().getUri().getFilePath();
	QFile File(Path);
	QString FileName = File.fileName();
	QStringList Items = FileName.split("/");
	QStringList Parts = Items[Items.count() - 1].split(".");
	QString AssetID = Node->getAssetUri().getId();
	QString Name = AssetID.remove(QRegExp("[^A-Za-z0-9_]"));

	if (Figure)
	{
		QString FigureAssetId = Figure->getAssetId();
		if (!Types.contains(Name))
		{
			Types.insert(Name, Node);
		}
	}
	else if (Geometry)
	{
		if (!Types.contains(Name))
		{
			Types.insert(Name, Node);
		}
	}

	// Looks through the child nodes for more props
	for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		getScenePropList(ChildNode, Types);
	}
}

QList<QString> DzBridgeAction::disconnectOverrideControllers()
{
	QList<QString> ModifiedList;
	ModifiedList.clear();

	DzNode* Selection = dzScene->getPrimarySelection();
	if (Selection == nullptr)
		return ModifiedList;

	int poseIndex = 0;
	DzNumericProperty* previousProperty = nullptr;
	for (int index = 0; index < Selection->getNumProperties(); index++)
	{
		DzProperty* property = Selection->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		QString propName = property->getName();
		if (numericProperty && !numericProperty->isOverridingControllers())
		{
			QString propName = property->getName();
			if (m_mMorphNameToLabel.contains(propName) && m_ControllersToDisconnect.contains(propName))
			{
				numericProperty->setOverrideControllers(true);
				ModifiedList.append(propName);
			}
		}
	}

	DzObject* Object = Selection->getObject();
	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty && !numericProperty->isOverridingControllers())
					{
						QString propName = property->getName();
						if (m_mMorphNameToLabel.contains(modifier->getName()) && m_ControllersToDisconnect.contains(modifier->getName()))
						{
							numericProperty->setOverrideControllers(true);
							ModifiedList.append(modifier->getName());
						}
					}
				}

			}

		}
	}

	return ModifiedList;
}

void DzBridgeAction::reconnectOverrideControllers(QList<QString>& DisconnetedControllers)
{
	DzNode* Selection = dzScene->getPrimarySelection();
	if (Selection == nullptr)
		return;

	int poseIndex = 0;
	DzNumericProperty* previousProperty = nullptr;
	for (int index = 0; index < Selection->getNumProperties(); index++)
	{
		DzProperty* property = Selection->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		QString propName = property->getName();
		if (numericProperty && numericProperty->isOverridingControllers())
		{
			QString propName = property->getName();
			if (DisconnetedControllers.contains(propName))
			{
				numericProperty->setOverrideControllers(false);
			}
		}
	}

	DzObject* Object = Selection->getObject();
	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty && numericProperty->isOverridingControllers())
					{
						QString propName = property->getName();
						if (DisconnetedControllers.contains(modifier->getName()))
						{
							numericProperty->setOverrideControllers(false);
						}
					}
				}

			}

		}
	}
}

bool DzBridgeAction::checkIfPoseExportIsDestructive()
{
	DzNode* Selection = dzScene->getPrimarySelection();
	if (Selection == nullptr)
		return false;

	int poseIndex = 0;
	DzNumericProperty* previousProperty = nullptr;
	for (int index = 0; index < Selection->getNumProperties(); index++)
	{
		DzProperty* property = Selection->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		QString propName = property->getName();
		if (numericProperty)
		{
			QString propName = property->getName();
			if (m_mMorphNameToLabel.contains(propName))
			{
				if (!(numericProperty->getKeyRange().getEnd() == 0.0f && numericProperty->getDoubleValue(0.0f) == 0.0f)) return true;
			}
		}
	}

	DzObject* Object = Selection->getObject();
	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty)
					{
						QString propName = property->getName();
						if (m_mMorphNameToLabel.contains(modifier->getName()))
						{
							if (!(numericProperty->getKeyRange().getEnd() == 0.0f && numericProperty->getDoubleValue(0.0f) == 0.0f)) return true;
						}
					}
				}

			}

		}
	}

	return false;
}

void DzBridgeAction::unlockTranform(DzNode* NodeToUnlock)
{
	if (NodeToUnlock == nullptr)
		return;

	DzFloatProperty* Property = nullptr;
	Property = NodeToUnlock->getXPosControl();
	Property->lock(false);
	Property = NodeToUnlock->getYPosControl();
	Property->lock(false);
	Property = NodeToUnlock->getZPosControl();
	Property->lock(false);

	Property = NodeToUnlock->getXRotControl();
	Property->lock(false);
	Property = NodeToUnlock->getYRotControl();
	Property->lock(false);
	Property = NodeToUnlock->getZRotControl();
	Property->lock(false);

	Property = NodeToUnlock->getXScaleControl();
	Property->lock(false);
	Property = NodeToUnlock->getYScaleControl();
	Property->lock(false);
	Property = NodeToUnlock->getZScaleControl();
	Property->lock(false);
}

bool DzBridgeAction::isTemporaryFile(QString sFilename)
{
	QString cleanedFilename = sFilename.toLower().replace("\\", "/");
	QString cleanedTempPath = dzApp->getTempPath().toLower().replace("\\", "/");

	if (cleanedFilename.contains(cleanedTempPath))
	{
		return true;
	}

	return false;
}

QString DzBridgeAction::exportAssetWithDtu(QString sFilename, QString sAssetMaterialName)
{
	if (sFilename.isEmpty())
		return sFilename;

	QString cleanedFilename = sFilename.toLower().replace("\\", "/");
	QString cleanedTempPath = dzApp->getTempPath().toLower().replace("\\", "/");
	QString cleanedAssetMaterialName = sAssetMaterialName;
	cleanedAssetMaterialName.remove(QRegExp("[^A-Za-z0-9_]"));

	QString exportPath = this->m_sRootFolder.replace("\\","/") + "/" + this->m_sExportSubfolder.replace("\\", "/");
	QString fileStem = QFileInfo(sFilename).fileName();

	exportPath += "/ExportTextures/";
	QDir().mkpath(exportPath);
//	QString exportFilename = exportPath + cleanedAssetMaterialName + "_" + fileStem;
	QString exportFilename = exportPath + fileStem;

	exportFilename = makeUniqueFilename(exportFilename);

	if (QFile(sFilename).copy(exportFilename) == true)
	{
		return exportFilename;
	}

	// copy method may fail if file already exists,
	// if exists and same file size, then proceed as if successful
	if ( QFileInfo(exportFilename).exists() &&
		QFileInfo(sFilename).size() == QFileInfo(exportFilename).size())
	{
		return exportFilename;
	}

	// return original source string if failed
	return sFilename;

}

QString DzBridgeAction::makeUniqueFilename(QString sFilename)
{
	if (QFileInfo(sFilename).exists() != true)
		return sFilename;

	QString newFilename = sFilename;
	int duplicate_count = 0;

	while (
		QFileInfo(newFilename).exists() &&
		QFileInfo(sFilename).size() != QFileInfo(newFilename).size()
		)
	{
		newFilename = sFilename + QString("_%i").arg(duplicate_count++);
	}

	return newFilename;

}

void DzBridgeAction::writePropertyTexture(DzJsonWriter& Writer, QString sName, QString sValue, QString sType, QString sTexture)
{
	Writer.startObject(true);
	Writer.addMember("Name", sName);
	Writer.addMember("Value", sValue);
	Writer.addMember("Data Type", sType);
	Writer.addMember("Texture", sTexture);
	Writer.finishObject();

}

void DzBridgeAction::writePropertyTexture(DzJsonWriter& Writer, QString sName, double dValue, QString sType, QString sTexture)
{
	Writer.startObject(true);
	Writer.addMember("Name", sName);
	Writer.addMember("Value", dValue);
	Writer.addMember("Data Type", sType);
	Writer.addMember("Texture", sTexture);
	Writer.finishObject();

}

void DzBridgeAction::writeDTUHeader(DzJsonWriter& writer)
{
	QString sAssetId = "";

	if (m_pSelectedNode)
	{
		sAssetId = m_pSelectedNode->getAssetId();
	}

	writer.addMember("DTU Version", 3);
	writer.addMember("Asset Name", m_sAssetName);
	writer.addMember("Asset Type", m_sAssetType);
	writer.addMember("Asset Id", sAssetId);
	writer.addMember("FBX File", m_sDestinationFBX);
	QString CharacterBaseFBX = m_sDestinationFBX;
	CharacterBaseFBX.replace(".fbx", "_base.fbx");
	writer.addMember("Base FBX File", CharacterBaseFBX);
	QString CharacterHDFBX = m_sDestinationFBX;
	CharacterHDFBX.replace(".fbx", "_HD.fbx");
	writer.addMember("HD FBX File", CharacterHDFBX);
	writer.addMember("Import Folder", m_sDestinationPath);
	// DB Dec-21-2021: additional metadata
	writer.addMember("Product Name", m_sProductName);
	writer.addMember("Product Component Name", m_sProductComponentName);

}

// Write out all the surface properties
void DzBridgeAction::writeAllMaterials(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, bool bRecursive)
{
	if (Node == nullptr)
		return;

	if (!bRecursive)
		Writer.startMemberArray("Materials", true);

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : nullptr;

	if (Shape)
	{
		for (int i = 0; i < Shape->getNumMaterials(); i++)
		{
			DzMaterial* Material = Shape->getMaterial(i);
			if (Material)
			{
				auto propertyList = Material->propertyListIterator();
				startMaterialBlock(Node, Writer, pCVSStream, Material);
				while (propertyList.hasNext())
				{
					writeMaterialProperty(Node, Writer, pCVSStream, Material, propertyList.next());
				}
				finishMaterialBlock(Writer);
			}
		}
	}

	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		writeAllMaterials(Child, Writer, pCVSStream, true);
	}

	if (!bRecursive)
		Writer.finishArray();
}

void DzBridgeAction::startMaterialBlock(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, DzMaterial* Material)
{
	if (Node == nullptr || Material == nullptr)
		return;

	Writer.startObject(true);
	Writer.addMember("Version", 3);
	Writer.addMember("Asset Name", Node->getLabel());
	Writer.addMember("Material Name", Material->getName());
	Writer.addMember("Material Type", Material->getMaterialName());

	DzPresentation* presentation = Node->getPresentation();
	if (presentation)
	{
		const QString presentationType = presentation->getType();
		Writer.addMember("Value", presentationType);
	}
	else
	{
		Writer.addMember("Value", QString("Unknown"));
	}

	Writer.startMemberArray("Properties", true);
	// Presentation node is stored as first element in Property array for compatibility with UE plugin's basematerial search algorithm
	if (presentation)
	{
		const QString presentationType = presentation->getType();
		Writer.startObject(true);
		Writer.addMember("Name", QString("Asset Type"));
		Writer.addMember("Value", presentationType);
		Writer.addMember("Data Type", QString("String"));
		Writer.addMember("Texture", QString(""));
		Writer.finishObject();

		if (m_bExportMaterialPropertiesCSV && pCVSStream)
		{
			*pCVSStream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << "Asset Type" << ", " << presentationType << ", " << "String" << ", " << "" << endl;
		}
	}
}

void DzBridgeAction::finishMaterialBlock(DzJsonWriter& Writer)
{
	// replace with Section Stack
	Writer.finishArray();
	Writer.finishObject();

}

void DzBridgeAction::writeMaterialProperty(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, DzMaterial* Material, DzProperty* Property)
{
	if (Node == nullptr || Material == nullptr || Property == nullptr)
		return;

	QString Name = Property->getName();
	QString TextureName = "";
	QString dtuPropType = "";
	QString dtuPropValue = "";
	double dtuPropNumericValue = 0.0;
	bool bUseNumeric = false;

	DzImageProperty* ImageProperty = qobject_cast<DzImageProperty*>(Property);
	DzNumericProperty* NumericProperty = qobject_cast<DzNumericProperty*>(Property);
	DzColorProperty* ColorProperty = qobject_cast<DzColorProperty*>(Property);
	if (ImageProperty)
	{
		if (ImageProperty->getValue())
		{
			TextureName = ImageProperty->getValue()->getFilename();
		}
		dtuPropValue = Material->getDiffuseColor().name();
		dtuPropType = QString("Texture");

		// Check if this is a Normal Map with Strength stored in lookup table
		if (m_imgPropertyTable_NormalMapStrength.contains(ImageProperty))
		{
			dtuPropType = QString("Double");
			dtuPropNumericValue = m_imgPropertyTable_NormalMapStrength[ImageProperty];
			bUseNumeric = true;
		}
	}
	// DzColorProperty is subclass of DzNumericProperty
	else if (ColorProperty)
	{
		if (ColorProperty->getMapValue())
		{
			TextureName = ColorProperty->getMapValue()->getFilename();
		}
		dtuPropValue = ColorProperty->getColorValue().name();
		dtuPropType = QString("Color");
	}
	else if (NumericProperty)
	{
		if (NumericProperty->getMapValue())
		{
			TextureName = NumericProperty->getMapValue()->getFilename();
		}
		dtuPropType = QString("Double");
		dtuPropNumericValue = NumericProperty->getDoubleValue();
		bUseNumeric = true;
	}
	else
	{
		// unsupported property type
		return;
	}

	QString dtuTextureName = TextureName;
	if (TextureName != "")
	{
		if (this->m_bUseRelativePaths)
		{
			dtuTextureName = dzApp->getContentMgr()->getRelativePath(TextureName, true);
		}
		if (isTemporaryFile(TextureName))
		{
			dtuTextureName = exportAssetWithDtu(TextureName, Node->getLabel() + "_" + Material->getName());
		}
	}
	if (bUseNumeric)
		writePropertyTexture(Writer, Name, dtuPropNumericValue, dtuPropType, dtuTextureName);
	else
		writePropertyTexture(Writer, Name, dtuPropValue, dtuPropType, dtuTextureName);

	if (m_bExportMaterialPropertiesCSV && pCVSStream)
	{
		if (bUseNumeric)
			*pCVSStream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << Name << ", " << dtuPropNumericValue << ", " << dtuPropType << ", " << TextureName << endl;
		else
			*pCVSStream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << Name << ", " << dtuPropValue << ", " << dtuPropType << ", " << TextureName << endl;
	}
	return;

}

void DzBridgeAction::writeAllMorphs(DzJsonWriter& writer)
{
	writer.startMemberArray("Morphs", true);
	if (m_bEnableMorphs)
	{
		for (QMap<QString, QString>::iterator i = m_mMorphNameToLabel.begin(); i != m_mMorphNameToLabel.end(); ++i)
		{
			writeMorphProperties(writer, i.key(), i.value());
		}
	}
	writer.finishArray();

	if (m_bEnableMorphs)
	{
		if (m_morphSelectionDialog->IsAutoJCMEnabled())
		{
			writer.startMemberArray("JointLinks", true);
			QList<JointLinkInfo> JointLinks = m_morphSelectionDialog->GetActiveJointControlledMorphs(m_pSelectedNode);
			foreach(JointLinkInfo linkInfo, JointLinks)
			{
				writeMorphJointLinkInfo(writer, linkInfo);
			}
			writer.finishArray();
		}
	}

}

void DzBridgeAction::writeMorphProperties(DzJsonWriter& writer, const QString& key, const QString& value)
{
	writer.startObject(true);
	writer.addMember("Name", key);
	writer.addMember("Label", value);
	writer.finishObject();
}

void DzBridgeAction::writeMorphJointLinkInfo(DzJsonWriter& writer, const JointLinkInfo& linkInfo)
{
	writer.startObject(true);
	writer.addMember("Bone", linkInfo.Bone);
	writer.addMember("Axis", linkInfo.Axis);
	writer.addMember("Morph", linkInfo.Morph);
	writer.addMember("Scalar", linkInfo.Scalar);
	writer.addMember("Alpha", linkInfo.Alpha);
	if (linkInfo.Keys.count() > 0)
	{
		writer.startMemberArray("Keys", true);
		foreach(JointLinkKey key, linkInfo.Keys)
		{
			writer.startObject(true);
			writer.addMember("Angle", key.Angle);
			writer.addMember("Value", key.Value);
			writer.finishObject();
		}
		writer.finishArray();
	}
	writer.finishObject();
}

void DzBridgeAction::writeAllSubdivisions(DzJsonWriter& writer)
{
	writer.startMemberArray("Subdivisions", true);
	if (m_EnableSubdivisions)
	{
		//stream << "Version, Object, Subdivision" << endl;
		QObjectList objList = m_subdivisionDialog->getSubdivisionCombos();
		foreach(QObject* obj, objList)
		{
			QComboBox* combo = (QComboBox*) obj;
			QString Name = combo->property("Object").toString() + ".Shape";
			int targetValue = combo->currentText().toInt();

			writeSubdivisionProperties(writer, Name, targetValue);
			//stream << "1, " << Name << ", " << targetValue << endl;
		}

	}

	writer.finishArray();

}

void DzBridgeAction::writeSubdivisionProperties(DzJsonWriter& writer, const QString& Name, int targetValue)
{
	writer.startObject(true);
	writer.addMember("Version", 1);
	writer.addMember("Asset Name", Name);
	writer.addMember("Value", targetValue);
	writer.finishObject();
}

void DzBridgeAction::writeAllDforceInfo(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, bool bRecursive)
{
	if (Node == nullptr)
		return;

	if (!bRecursive)
		Writer.startMemberArray("dForce", true);

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : nullptr;

	bool bDForceSettingsAvailable = false;
	if (Shape)
	{
		QList<DzModifier*> dforceModifierList;
		DzModifierIterator modIter = Object->modifierIterator();
		int modifierCount = 0;
		while (modIter.hasNext())
		{
			DzModifier* modifier = modIter.next();
			QString mod_Class = modifier->className();
			if (mod_Class.toLower().contains("dforce"))
			{
				bDForceSettingsAvailable = true;
				dforceModifierList.append(modifier);
				modifierCount++;
			}
		}

		if (bDForceSettingsAvailable)
		{
			Writer.startObject(true);
			Writer.addMember("Version", 4);
			Writer.addMember("Asset Name", Node->getLabel());
			Writer.addMember("Modifier Count", modifierCount);
			Writer.addMember("Material Count", Shape->getNumMaterials());
			writeDforceModifiers(dforceModifierList, Writer, Shape);

			Writer.startMemberArray("dForce-Materials", true);
			for (int i = 0; i < Shape->getNumMaterials(); i++)
			{
				DzMaterial* Material = Shape->getMaterial(i);
				if (Material)
				{
					Writer.startObject(true);
					Writer.addMember("Version", 3);
					Writer.addMember("Asset Name", Node->getLabel());
					Writer.addMember("Material Name", Material->getName());
					Writer.addMember("Material Type", Material->getMaterialName());
					DzPresentation* presentation = Node->getPresentation();
					if (presentation != nullptr)
					{
						const QString presentationType = presentation->getType();
						Writer.addMember("Value", presentationType);
					}
					else
					{
						Writer.addMember("Value", QString("Unknown"));
					}
					writeDforceMaterialProperties(Writer, Material, Shape);
					Writer.finishObject();
				}
			}
			Writer.finishArray();
			Writer.finishObject();
		}
	}

	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		writeAllDforceInfo(Child, Writer, pCVSStream, true);
	}

	if (!bRecursive)
	{
		if (m_sAssetType == "SkeletalMesh")
		{
			bool ExportDForce = true;
			Writer.startMemberArray("dForce-WeightMaps", true);
			if (ExportDForce)
			{
				writeWeightMaps(m_pSelectedNode, Writer);
			}
			Writer.finishArray();
		}
		Writer.finishArray();
	}
}

void DzBridgeAction::writeDforceModifiers(const QList<DzModifier*>& dforceModifierList, DzJsonWriter& Writer, DzShape* Shape)
{
	Writer.startMemberArray("DForce-Modifiers", true);

	foreach(auto modifier, dforceModifierList)
	{
		Writer.startObject(true);
		Writer.addMember("Modifier Name", modifier->getName());
		Writer.addMember("Modifier Class", modifier->className());
		/////////////////////////////////////
		// TODO: DUMP MODIFIER PROPERTIES
		/////////////////////////////////////
		Writer.finishObject();
	}

	Writer.finishArray();
}

void DzBridgeAction::writeDforceMaterialProperties(DzJsonWriter& Writer, DzMaterial* Material, DzShape* Shape)
{
	if (Material == nullptr || Shape == nullptr)
		return;

	Writer.startMemberArray("Properties", true);

	DzElement* elSimulationSettingsProvider;
	bool ret = false;
	int methodIndex = -1;
	methodIndex = Shape->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("findSimulationSettingsProvider(QString)"));
	if (methodIndex != -1)
	{
		QMetaMethod method = Shape->metaObject()->method(methodIndex);
		QGenericReturnArgument returnArgument(
			method.typeName(),
			&elSimulationSettingsProvider
		);
		ret = method.invoke(Shape, returnArgument, Q_ARG(QString, Material->getName()));
		if (elSimulationSettingsProvider)
		{
			int numProperties = elSimulationSettingsProvider->getNumProperties();
			DzPropertyListIterator propIter = elSimulationSettingsProvider->propertyListIterator();
			QString propString = "";
			int propIndex = 0;
			while (propIter.hasNext())
			{
				DzProperty* Property = propIter.next();
				DzNumericProperty* NumericProperty = qobject_cast<DzNumericProperty*>(Property);
				if (NumericProperty)
				{
					QString Name = Property->getName();
					QString TextureName = "";
					if (NumericProperty->getMapValue())
					{
						TextureName = NumericProperty->getMapValue()->getFilename();
					}
					Writer.startObject(true);
					Writer.addMember("Name", Name);
					Writer.addMember("Value", QString::number(NumericProperty->getDoubleValue()));
					Writer.addMember("Data Type", QString("Double"));
					Writer.addMember("Texture", TextureName);
					Writer.finishObject();
				}
			}

		}

	}

	Writer.finishArray();
}

void DzBridgeAction::writeAllPoses(DzJsonWriter& writer)
{
	writer.startMemberArray("Poses", true);
	for (QList<QString>::iterator i = m_aPoseList.begin(); i != m_aPoseList.end(); ++i)
	{
		writer.startObject(true);
		writer.addMember("Name", *i);
		writer.addMember("Label", m_mMorphNameToLabel[*i]);
		writer.finishObject();
	}
	writer.finishArray();
}

void DzBridgeAction::writeEnvironment(DzJsonWriter& writer)
{
	writer.startMemberArray("Instances", true);
	QMap<QString, DzMatrix3> WritingInstances;
	QList<DzGeometry*> ExportedGeometry;
	writeInstances(m_pSelectedNode, writer, WritingInstances, ExportedGeometry);
	writer.finishArray();
}

void DzBridgeAction::writeInstances(DzNode* Node, DzJsonWriter& Writer, QMap<QString, DzMatrix3>& WritenInstances, QList<DzGeometry*>& ExportedGeometry, QUuid ParentID)
{
	if (Node == nullptr)
		return;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;
	DzGeometry* Geometry = Shape ? Shape->getGeometry() : NULL;
	DzBone* Bone = qobject_cast<DzBone*>(Node);

	if (Bone == nullptr && Geometry)
	{
		ExportedGeometry.append(Geometry);
		ParentID = writeInstance(Node, Writer, ParentID);
	}

	for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		writeInstances(ChildNode, Writer, WritenInstances, ExportedGeometry, ParentID);
	}
}

QUuid DzBridgeAction::writeInstance(DzNode* Node, DzJsonWriter& Writer, QUuid ParentID)
{
	if (Node == nullptr)
#ifdef __APPLE__
        return 0;
#else
		return false;
#endif

	QString Path = Node->getAssetFileInfo().getUri().getFilePath();
	QFile File(Path);
	QString FileName = File.fileName();
	QStringList Items = FileName.split("/");
	QStringList Parts = Items[Items.count() - 1].split(".");
	QString AssetID = Node->getAssetUri().getId();
	QString Name = AssetID.remove(QRegExp("[^A-Za-z0-9_]"));
	QUuid Uid = QUuid::createUuid();

	Writer.startObject(true);
	Writer.addMember("Version", 1);
	Writer.addMember("InstanceLabel", Node->getLabel());
	Writer.addMember("InstanceAsset", Name);
	Writer.addMember("ParentID", ParentID.toString());
	Writer.addMember("Guid", Uid.toString());
	Writer.addMember("TranslationX", Node->getWSPos().m_x);
	Writer.addMember("TranslationY", Node->getWSPos().m_y);
	Writer.addMember("TranslationZ", Node->getWSPos().m_z);

	DzQuat RotationQuat = Node->getWSRot();
	DzVec3 Rotation;
	RotationQuat.getValue(Node->getRotationOrder(), Rotation);
	Writer.addMember("RotationX", Rotation.m_x);
	Writer.addMember("RotationY", Rotation.m_y);
	Writer.addMember("RotationZ", Rotation.m_z);

	DzMatrix3 Scale = Node->getWSScale();

	Writer.addMember("ScaleX", Scale.row(0).length());
	Writer.addMember("ScaleY", Scale.row(1).length());
	Writer.addMember("ScaleZ", Scale.row(2).length());
	Writer.finishObject();

	return Uid;
}

void DzBridgeAction::readGui(DzBridgeDialog* BridgeDialog)
{
	if (BridgeDialog == nullptr)
		return;

	// Collect the values from the dialog fields
	if (m_sAssetName == "" || m_nNonInteractiveMode == 0) m_sAssetName = BridgeDialog->getAssetNameEdit()->text();
	if (m_sRootFolder == "" || m_nNonInteractiveMode == 0) m_sRootFolder = readGuiRootFolder();
	if (m_sExportSubfolder == "" || m_nNonInteractiveMode == 0) m_sExportSubfolder = m_sAssetName;
	m_sDestinationPath = m_sRootFolder + "/" + m_sExportSubfolder + "/";
	if (m_sExportFbx == "" || m_nNonInteractiveMode == 0) m_sExportFbx = m_sAssetName;
	m_sDestinationFBX = m_sDestinationPath + m_sExportFbx + ".fbx";

	if (m_nNonInteractiveMode == 0)
	{
		// TODO: consider removing once findData( ) method above is completely implemented
		m_sAssetType = cleanString(BridgeDialog->getAssetTypeCombo()->currentText());

		m_sMorphSelectionRule = BridgeDialog->GetMorphString();
		m_mMorphNameToLabel = BridgeDialog->GetMorphMapping();
		m_bEnableMorphs = BridgeDialog->getMorphsEnabledCheckBox()->isChecked();
	}

	m_EnableSubdivisions = BridgeDialog->getSubdivisionEnabledCheckBox()->isChecked();
	m_bShowFbxOptions = BridgeDialog->getShowFbxDialogCheckBox()->isChecked();
	if (m_subdivisionDialog == nullptr)
	{
		m_subdivisionDialog = DzBridgeSubdivisionDialog::Get(BridgeDialog);
	}
	if (m_morphSelectionDialog == nullptr)
	{
		m_morphSelectionDialog = DzBridgeMorphSelectionDialog::Get(BridgeDialog);
	}
	m_sFbxVersion = BridgeDialog->getFbxVersionCombo()->currentText();
	m_bGenerateNormalMaps = BridgeDialog->getEnableNormalMapGenerationCheckBox()->isChecked();

}

// ------------------------------------------------
// PixelIntensity
// ------------------------------------------------
double DzBridgeAction::getPixelIntensity(const  QRgb& pixel)
{
	const double r = double(qRed(pixel));
	const double g = double(qGreen(pixel));
	const double b = double(qBlue(pixel));
	const double average = (r + g + b) / 3.0;
	return average / 255.0;
}

// ------------------------------------------------
// MapComponent
// ------------------------------------------------
uint8_t DzBridgeAction::getNormalMapComponent(double pX)
{
	return (pX + 1.0) * (255.0 / 2.0);
}

// ------------------------------------------------
// intclamp
// ------------------------------------------------
int DzBridgeAction::getIntClamp(int x, int low, int high)
{
	if (x < low) { return low; }
	else if (x > high) { return high; }
	return x;
}

// ------------------------------------------------
// map_component
// ------------------------------------------------
QRgb DzBridgeAction::getSafePixel(const QImage& img, int x, int y)
{
	int ix = this->getIntClamp(x, 0, img.size().width() - 1);
	int iy = this->getIntClamp(y, 0, img.size().height() - 1);
	return img.pixel(ix, iy);
}

// ------------------------------------------------
// makeNormalMapFromBumpMap
// ------------------------------------------------
QImage DzBridgeAction::makeNormalMapFromHeightMap(QString heightMapFilename, double normalStrength)
{
	// load qimage
	QImage image;
	image.load(heightMapFilename);
	int imageWidth = image.size().width();
	int imageHeight = image.size().height();

	QImage result = QImage(imageWidth, imageHeight, QImage::Format_ARGB32_Premultiplied);
	QRect rect = result.rect();
	int r1 = rect.top();
	int r2 = rect.bottom();
	int c1 = rect.left();
	int c2 = rect.right();

	QFileInfo fileInfo = QFileInfo(heightMapFilename);
	QString progressString = QString("Generating Normal Map: %1 (%2 x %3)").arg(fileInfo.fileName()).arg(imageWidth).arg(imageHeight);

	DzProgress progress = DzProgress(progressString, 100, false, true);
	float step = ((float)(r2 - r1)) / 100.0;
	float current = 0;

	// row loop
	for (int row = r1; row <= r2; row++) {
		current++;
		if (current >= step) {
			progress.step();
			current = 0;
		}

		// col loop
		for (int col = c1; col <= c2; col++) {

			// skip blank pixels to speed conversion
			QRgb rgbMask = image.pixel(col, row);
			int mask = QColor(rgbMask).value();
			if (mask == 0 || mask == 255)
			{
				const QColor color = QColor(128, 127, 255);
				result.setPixel(col, row, color.rgb());
				continue;
			}

			// Pixel Picker
			const QRgb topLeft = this->getSafePixel(image, col - 1, row - 1);
			const QRgb top = this->getSafePixel(image, col, row - 1);
			const QRgb topRight = this->getSafePixel(image, col + 1, row - 1);
			const QRgb right = this->getSafePixel(image, col + 1, row);
			const QRgb bottomRight = this->getSafePixel(image, col + 1, row + 1);
			const QRgb bottom = this->getSafePixel(image, col, row + 1);
			const QRgb bottomLeft = this->getSafePixel(image, col - 1, row + 1);
			const QRgb left = this->getSafePixel(image, col - 1, row);

			// calculating pixel intensities
			const double tl = this->getPixelIntensity(topLeft);
			const double t = this->getPixelIntensity(top);
			const double tr = this->getPixelIntensity(topRight);
			const double r = this->getPixelIntensity(right);
			const double br = this->getPixelIntensity(bottomRight);
			const double b = this->getPixelIntensity(bottom);
			const double bl = this->getPixelIntensity(bottomLeft);
			const double l = this->getPixelIntensity(left);

			//// skip edge cases to reduce seam
			//if (tl == 0 || t == 0 || tr == 0 || r == 0 || br == 0 || b == 0 || bl == 0 || l == 0)
			//{
			//	const QColor color = QColor(128, 127, 255);
			//	result.setPixel(col, row, color.rgb());
			//	continue;
			//}

			// Sobel filter
			const double dX = (tr + 2.0 * r + br) - (tl + 2.0 * l + bl);
			const double dY = 1.0 / normalStrength;
			const double dZ = (bl + 2.0 * b + br) - (tl + 2.0 * t + tr);
			const DzVec3 vec = DzVec3(dX, dY, dZ).normalized();

			// DS uses Y as up, not Z, Normalmaps uses Z
			const QColor color = QColor(this->getNormalMapComponent(vec.m_x), this->getNormalMapComponent(vec.m_z), this->getNormalMapComponent(vec.m_y));
			result.setPixel(col, row, color.rgb());
		}
	}

	return result;
}

QStringList DzBridgeAction::getAvailableMorphs(DzNode* Node)
{
	QStringList newMorphList;
	newMorphList.clear();

	if (Node == nullptr)
		return newMorphList;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		QString propName = property->getName();
		QString propLabel = property->getLabel();
		DzPresentation* presentation = property->getPresentation();
		if (presentation && presentation->getType() == "Modifier/Shape")
		{
			newMorphList.append(propName);
		}
	}

	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			QString modName = modifier->getName();
			QString modLabel = modifier->getLabel();
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzPresentation* presentation = property->getPresentation();
					if (presentation)
					{
						newMorphList.append(modName);
					}
				}
			}
		}
	}

	return newMorphList;
}

QStringList DzBridgeAction::getActiveMorphs(DzNode* Node)
{
	QStringList newMorphList;
	newMorphList.clear();

	if (Node == nullptr)
		return newMorphList;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		QString propName = property->getName();
		QString propLabel = property->getLabel();
		DzPresentation* presentation = property->getPresentation();
		if (presentation && presentation->getType() == "Modifier/Shape")
		{
			DzNumericProperty *numericProp = qobject_cast<DzNumericProperty*>(property);
			if (numericProp->getDoubleValue() > 0)
			{
				newMorphList.append(propName);
			}
		}
	}

	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			QString modName = modifier->getName();
			QString modLabel = modifier->getLabel();
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzPresentation* presentation = property->getPresentation();
					if (presentation)
					{
						DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(property);
						if (numericProp->getDoubleValue() > 0)
						{
							newMorphList.append(modName);
						}
					}
				}
			}
		}
	}

	return newMorphList;
}

bool DzBridgeAction::setBridgeDialog(DzBasicDialog* arg_dlg)
{
	m_bridgeDialog = qobject_cast<DzBridgeDialog*>(arg_dlg);

	if (m_bridgeDialog == nullptr)
	{
		if (arg_dlg == nullptr)
			return true;

		if (arg_dlg->inherits("DzBridgeDialog"))
		{
			m_bridgeDialog = (DzBridgeDialog*)arg_dlg;
			// WARNING
			printf("WARNING: DzBridge version mismatch detected! Crashes may occur.");
		}

		// return false to signal version mismatch
		return false;
	}

	return true;
}

bool DzBridgeAction::setSubdivisionDialog(DzBasicDialog* arg_dlg)
{
	m_subdivisionDialog = qobject_cast<DzBridgeSubdivisionDialog*>(arg_dlg);

	if (m_subdivisionDialog == nullptr)
	{
		if (arg_dlg == nullptr)
			return true;

		if (arg_dlg->inherits("DzBridgeSubdivisionDialog"))
		{
			m_subdivisionDialog = (DzBridgeSubdivisionDialog*)arg_dlg;
			// WARNING
			printf("WARNING: DzBridge version mismatch detected! Crashes may occur.");
		}

		// return false to signal version mismatch
		return false;
	}

	return true;
}

bool DzBridgeAction::setMorphSelectionDialog(DzBasicDialog* arg_dlg)
{
	m_morphSelectionDialog = qobject_cast<DzBridgeMorphSelectionDialog*>(arg_dlg);

	if (m_morphSelectionDialog == nullptr)
	{
		if (arg_dlg == nullptr)
			return true;

		if (arg_dlg->inherits("DzBridgeMorphSelectionDialog"))
		{
			m_morphSelectionDialog = (DzBridgeMorphSelectionDialog*)arg_dlg;
			// WARNING
			printf("WARNING: DzBridge version mismatch detected! Crashes may occur.");
		}

		// return false to signal version mismatch
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// START: DFORCE WEIGHTMAPS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Write weightmaps - recursively traverse parent/children, and export all associated weightmaps
void DzBridgeAction::writeWeightMaps(DzNode* Node, DzJsonWriter& Writer)
{
	if (Node == nullptr)
		return;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	bool bDForceSettingsAvailable = false;

	if (Shape && Shape->inherits("DzFacetShape"))
	{
		DzModifier* dforceModifier;
		DzModifierIterator modIter = Object->modifierIterator();
		while (modIter.hasNext())
		{
			DzModifier* modifier = modIter.next();
			QString mod_Class = modifier->className();
			if (mod_Class.toLower().contains("dforce"))
			{
				bDForceSettingsAvailable = true;
				dforceModifier = modifier;
				break;
			}
		}

		if (bDForceSettingsAvailable)
		{
			///////////////////////////////////////////////
			// Method for obtaining weightmaps, grab directly from dForce Modifier Node
			///////////////////////////////////////////////
			int methodIndex = dforceModifier->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("getInfluenceWeights()"));
			if (methodIndex != -1)
			{
				QMetaMethod method = dforceModifier->metaObject()->method(methodIndex);
				DzWeightMap* weightMap;
				QGenericReturnArgument returnArg(
					method.typeName(),
					&weightMap
				);
				int result = method.invoke((QObject*)dforceModifier, returnArg);
				if (result != -1)
				{
					if (weightMap)
					{
						int numVerts = Shape->getAssemblyGeometry()->getNumVertices();
						unsigned short* daz_weights = weightMap->getWeights();
						int byte_length = numVerts * sizeof(unsigned short);

						char* buffer = new char[byte_length];
						unsigned short* unity_weights = (unsigned short*)buffer;

						// load material groups to remap weights to unity's vertex order
						DzFacetMesh* facetMesh = dynamic_cast<DzFacetShape*>(Shape)->getFacetMesh();
						if (facetMesh)
						{
							// sanity check
							if (numVerts != facetMesh->getNumVertices())
							{
								// throw error if needed
								dzApp->log("DazBridge: ERROR Exporting Weight Map to file.");
								return;
							}
							int numMaterials = facetMesh->getNumMaterialGroups();
							std::list<MaterialGroupExportOrderMetaData> exportQueue;
							DzFacet* facetPtr = facetMesh->getFacetsPtr();

							// generate export order queue
							// first, populate export queue with materialgroups
							for (int i = 0; i < numMaterials; i++)
							{
								DzMaterialFaceGroup* materialGroup = facetMesh->getMaterialGroup(i);
								int numFaces = materialGroup->count();
								const int* indexPtr = materialGroup->getIndicesPtr();
								int offset = facetPtr[indexPtr[0]].m_vertIdx[0];
								int count = -1;
								MaterialGroupExportOrderMetaData* metaData = new MaterialGroupExportOrderMetaData(i, offset);
								exportQueue.push_back(*metaData);
							}

							// sort: uses operator< to order by vertex_offset
							exportQueue.sort();

							/////////////////////////////////////////
							// for building vertex index lookup tables
							/////////////////////////////////////////
							int material_vertex_count = 0;
							int material_vertex_offset = 0;
							int* DazToUnityLookup = new int[numVerts];
							for (int i = 0; i < numVerts; i++) { DazToUnityLookup[i] = -1; }
							int* UnityToDazLookup = new int[numVerts];
							for (int i = 0; i < numVerts; i++) { UnityToDazLookup[i] = -1; }

							int unity_weightmap_vertexindex = 0;
							// iterate through sorted material groups...
							for (std::list<MaterialGroupExportOrderMetaData>::iterator export_iter = exportQueue.begin(); export_iter != exportQueue.end(); export_iter++)
							{
								// update the vert_offset for each materialGroup
								material_vertex_offset = material_vertex_offset + material_vertex_count;
								material_vertex_count = 0;
								int check_offset = export_iter->vertex_offset;

								// retrieve material group based on sorted material index list
								int materialGroupIndex = export_iter->materialIndex;
								DzMaterialFaceGroup* materialGroup = facetMesh->getMaterialGroup(materialGroupIndex);
								int numFaces = materialGroup->count();
								// pointer for faces in materialGroup
								const int* indexPtr = materialGroup->getIndicesPtr();

								// get each face in materialGroup, then iterate through all vertex indices in the face
								// copy out weights into buffer using material group's vertex ordering, but cross-referenced with internal vertex array indices

								// get the i-th index into the index array of faces, then retrieve the j-th index into the vertex index array
								// i is 0 to number of faces (aka facets), j is 0 to number of vertices in the face
								for (int i = 0; i < numFaces; i++)
								{
									int vertsPerFacet = (facetPtr->isQuad()) ? 4 : 3;
									for (int j = 0; j < vertsPerFacet; j++)
									{
										// retrieve vertex index into daz internal vertex array (probably a BST in array format)
										int vert_index = facetPtr[indexPtr[i]].m_vertIdx[j];

										///////////////////////////////////
										// NOTE: Since the faces will often share/re-use the same vertex, we need to skip
										// any vertex that has already been recorded, since we want ONLY unique vertices
										// in weightmap.  This is done by creating checking back into a DazToUnity vertex index lookup table
										///////////////////////////////////
										// unique vertices will not yet be written and have default -1 value
										if (DazToUnityLookup[vert_index] == -1)
										{
											// This vertex is unique, record into the daztounity lookup table and proceed with other operations
											// to be performend on unqiue verts.
											DazToUnityLookup[vert_index] = unity_weightmap_vertexindex;

											// use the vertex index to cross-reference to the corresponding weightmap value and copy out to buffer for exporting
											// (only do this for unique verts)
											unity_weights[unity_weightmap_vertexindex] = weightMap->getWeight(vert_index);
											//unity_weights[unity_weightmap_vertexindex] = daz_weights[vert_index];

											// Create the unity to daz vertex lookup table (only do this for unique verts)
											UnityToDazLookup[unity_weightmap_vertexindex] = vert_index;

											// increment the unity weightmap vertex index (only do this for unique verts)
											unity_weightmap_vertexindex++;
										}
									} //for (int j = 0; j < vertsPerFace; j++)
								} // for (int i = 0; i < numFaces; i++)
							} // for (std::list<MaterialGroupExportOrderMetaData>::iterator export_iter = exportQueue.begin(); export_iter != exportQueue.end(); export_iter++)

							// export to dforce_weightmap file
							QString filename = QString("%1.dforce_weightmap.bytes").arg(cleanString(Node->getLabel()));
							QFile rawWeight(m_sDestinationPath + filename);
							if (rawWeight.open(QIODevice::WriteOnly))
							{
								int bytesWritten = rawWeight.write(buffer, byte_length);
								if (bytesWritten != byte_length)
								{
									// write error
									QString errString = rawWeight.errorString();
									if (m_nNonInteractiveMode == 0) QMessageBox::warning(0,
										tr("Error writing dforce weightmap. Incorrect number of bytes written."),
										errString, QMessageBox::Ok);
								}
								rawWeight.close();

								// Write entry into DTU for weightmap lookup
								Writer.startObject(true);
								Writer.addMember("Asset Name", Node->getLabel());
								Writer.addMember("Weightmap Filename", filename);
								Writer.finishObject();

							}

						} // if (facetMesh) /** facetMesh null? */
					} // if (weightMap) /** weightmap null? */
				} // if (result != -1) /** invokeMethod failed? */
			} // if (methodIndex != -1) /** findMethod failed? */
		} // if (bDForceSettingsAvailable) /** no dforce data found */
	} // if (Shape)

	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		writeWeightMaps(Child, Writer);
	}

}

// OLD Method for obtaining weightmap, relying on dForce Weight Modifier Node
DzWeightMapPtr DzBridgeAction::getWeightMapPtr(DzNode* Node)
{
	if (Node == nullptr)
		return nullptr;

	// 1. check if weightmap modifier present
	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		if (Child->className().contains("DzDForceModifierWeightNode"))
		{
			QObject* handler;
			if (metaInvokeMethod(Child, "getWeightMapHandler()", (void**)&handler))
			{
				QObject* weightGroup;
				if (metaInvokeMethod(handler, "currentWeightGroup()", (void**)&weightGroup))
				{
					QObject* context;
					if (metaInvokeMethod(weightGroup, "currentWeightContext()", (void**)&context))
					{
						DzWeightMapPtr weightMap;
						// DzWeightMapPtr
						QMetaMethod metaMethod = context->metaObject()->method(30); // getWeightMap()
						QGenericReturnArgument returnArgument(
							metaMethod.typeName(),
							&weightMap
						);
						int result = metaMethod.invoke((QObject*)context, returnArgument);
						if (result != -1)
						{
							return weightMap;
						}
					}
				}
			}
		}

	}

	return nullptr;

}

bool DzBridgeAction::metaInvokeMethod(QObject* object, const char* methodSig, void** returnPtr)
{
	if (object == nullptr)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////
	// REFERENCE Signatures obtained by QMetaObject->method() query
	//////////////////////////////////////////////////////////////////
	//
	// DzDForceModifierWeightNode::getWeightMapHandler() = 372
	//
	// DzDForceModifierWeightHandler::getWeightGroup(int) = 18
	// DzDForceModifierWeightHandler::currentWeightGroup() = 20
	//
	// DzDForceModifierWeightGroup::getWeightMapContext(int) = 19
	// DzDForceModifierWeightGroup::currentWeightContext() = 22
	//
	// DzDForceModiferMapContext::getWeightMap() = 30
	/////////////////////////////////////////////////////////////////////////

	// find the metamethod
	const QMetaObject* metaObject = object->metaObject();
	int methodIndex = metaObject->indexOfMethod(QMetaObject::normalizedSignature(methodSig));
	if (methodIndex == -1)
	{
		// use fuzzy search
		// look up all methods, find closest match for methodSig
		int searchResult = -1;
		QString fuzzySig = QString(QMetaObject::normalizedSignature(methodSig)).toLower().remove("()");
		for (int i = 0; i < metaObject->methodCount(); i++)
		{
			const char* sig = metaObject->method(i).signature();
			if (QString(sig).toLower().contains(fuzzySig))
			{
				searchResult = i;
				break;
			}
		}
		if (searchResult == -1)
		{
			return false;
		}
		else
		{
			methodIndex = searchResult;
		}

	}

	// invoke metamethod
	QMetaMethod metaMethod = metaObject->method(methodIndex);
	void* returnVal;
	QGenericReturnArgument returnArgument(
		metaMethod.typeName(),
		&returnVal
	);
	int result = metaMethod.invoke((QObject*)object, returnArgument);
	if (result)
	{
		// set returnvalue
		*returnPtr = returnVal;

		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// END: DFORCE WEIGHTMAPS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// START: SUBDIVISION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __APPLE__
#define USING_LIBSTDCPP 1
#endif
#include "OpenFBXInterface.h"
#include "OpenSubdivInterface.h"


bool DzBridgeAction::upgradeToHD(QString baseFilePath, QString hdFilePath, QString outFilePath, std::map<std::string, int>* pLookupTable)
{
	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* baseMeshScene = openFBX->CreateScene("Base Mesh Scene");
	if (openFBX->LoadScene(baseMeshScene, baseFilePath.toLocal8Bit().data()) == false)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, "Error",
			"An error occurred while loading the base scene...", QMessageBox::Ok);
		printf("\n\nAn error occurred while loading the base scene...");
		return false;
	}
	SubdivideFbxScene subdivider = SubdivideFbxScene(baseMeshScene, pLookupTable);
	subdivider.ProcessScene();
	FbxScene* hdMeshScene = openFBX->CreateScene("HD Mesh Scene");
	if (openFBX->LoadScene(hdMeshScene, hdFilePath.toLocal8Bit().data()) == false)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, "Error",
			"An error occurred while loading the base scene...", QMessageBox::Ok);
		printf("\n\nAn error occurred while loading the base scene...");
		return false;
	}
	subdivider.SaveClustersToScene(hdMeshScene);
	if (openFBX->SaveScene(hdMeshScene, outFilePath.toLocal8Bit().data()) == false)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, "Error",
			"An error occurred while saving the scene...", QMessageBox::Ok);

		printf("\n\nAn error occurred while saving the scene...");
		return false;
	}

	return true;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// END: SUBDIVISION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QString DzBridgeAction::getMD5(const QString& path)
{
	auto algo = QCryptographicHash::Md5;
	QFile sourceFile(path);
	qint64 fileSize = sourceFile.size();
	const qint64 bufferSize = 10240;

	if (sourceFile.open(QIODevice::ReadOnly))
	{
		char buffer[bufferSize];
		int bytesRead;
		int readSize = qMin(fileSize, bufferSize);

		QCryptographicHash hash(algo);
		while (readSize > 0 && (bytesRead = sourceFile.read(buffer, readSize)) > 0)
		{
			fileSize -= bytesRead;
			hash.addData(buffer, bytesRead);
			readSize = qMin(fileSize, bufferSize);
		}

		sourceFile.close();
		return QString(hash.result().toHex());
	}
	return QString();
}

bool DzBridgeAction::copyFile(QFile* file, QString* dst, bool replace, bool compareFiles)
{
	if (file == nullptr || dst == nullptr)
		return false;

	bool dstExists = QFile::exists(*dst);

	if (replace)
	{
		if (compareFiles && dstExists)
		{
			auto srcFileMD5 = getMD5(file->fileName());
			auto dstFileMD5 = getMD5(*dst);

			if (srcFileMD5.length() > 0 && dstFileMD5.length() > 0 && srcFileMD5.compare(dstFileMD5) == 0)
			{
				return false;
			}
		}

		if (dstExists)
		{
			QFile::remove(*dst);
		}
	}

	auto result = file->copy(*dst);

	if (QFile::exists(*dst))
	{
#if __APPLE__
		QFile::setPermissions(*dst, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther);
#else
		QFile::setPermissions(*dst, QFile::ReadOther | QFile::WriteOther);
#endif
	}

	return result;
}


#include "moc_DzBridgeAction.cpp"
