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
#include <dzscript.h>

#include "dzgeometry.h"
#include "dzweightmap.h"
#include "dzfacetshape.h"
#include "dzfacetmesh.h"
#include "dzfacegroup.h"
#include "dzmaterial.h"
#include <dzvec3.h>
#include <dzskinbinding.h>
#include <dzbonebinding.h>
#include <dzerclink.h>
#include <dzpropertygroup.h>

#include "dzgroupnode.h"
#include "dzinstancenode.h"

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

#include <qimage.h>
#include "ImageTools.h"
#include "MorphTools.h"
#include "FbxTools.h"
#include "dzlayeredtexture.h"

#include "zip.h"

// miniz-lib
extern "C"
{
	unsigned long mz_crc32(unsigned long crc, const unsigned char* ptr, size_t buf_len);
}

using namespace DzBridgeNameSpace;

/// <summary>
/// Initializes general export data and settings.
/// </summary>
DzBridgeAction::DzBridgeAction(const QString& text, const QString& desc) :
	 DzAction(text, desc)
{
	this->setObjectName("DzBridge_Base_Action");

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

	 m_bPostProcessFbx = true;
	 m_bRemoveDuplicateGeografts = true;
	 m_bExperimental_FbxPostProcessing = false;

	 // LOD settings
	 m_bEnableLodGeneration = false;
	 m_bCreateLodGroup = false;

	 m_ImageToolsJobsManager = new ImageToolsJobsManager();

	 // Create List of Known Intermediate Folder File Extensions
	 char* acharExtensionList[] = {
		 "fbx", "dtu", "obj",
		 "py", "pyc", "bat", "sh",
		 "jpg", "jpeg", "png", "bmp", "tif", "gif"
	 };
	 int numExtensions = 11;
	 for (int i = 0; i < numExtensions; i++) {
		 try {
			 m_aKnownIntermediateFileExtensionsList += QString(acharExtensionList[i]);
		 }
		 catch (...) {
			 dzApp->log("DEBUG: StringListMaker: end of char array");
			 break;
		 }
		 dzApp->log("DEBUG: StringListMaker: added string: " + m_aKnownIntermediateFileExtensionsList[i]);
	 }

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
	m_bExportMaterialPropertiesCSV = false;
	m_bEnableLodGeneration = false;
	m_bCreateLodGroup = false;
	resetArray_ControllersToDisconnect();

	// Reset all dialog settings and script-exposed properties to Hardcoded Defaults
	// Ignore saved settings, QSettings, etc.
	DzNode* selection = dzScene->getPrimarySelection();
	DzFigure* figure = qobject_cast<DzFigure*>(selection);
	if (selection)
	{
		if (dzScene->getFilename().length() > 0)
		{
			QFileInfo fileInfo = QFileInfo(dzScene->getFilename());
			m_sExportFilename = fileInfo.baseName().remove(QRegExp("[^A-Za-z0-9_]"));
		}
		else
		{
			m_sExportFilename = this->cleanString(selection->getLabel());
		}
	}
	else
	{
		m_sExportFilename = "";
	}
	if (figure)
	{
		m_sAssetType = "SkeletalMesh";
	}
	else
	{
		m_sAssetType = "StaticMesh";
	}

	m_sAssetName = m_sExportFilename;
	m_sProductName = "";
	m_sProductComponentName = "";
	m_aMorphListOverride.clear();
	m_bUseRelativePaths = false;
	m_bUndoNormalMaps = true;
	m_nNonInteractiveMode = 0;
	m_undoTable_DuplicateMaterialRename.clear();
	m_undoTable_GenerateMissingNormalMap.clear();
	m_undoTable_DuplicateClothingRename.clear();
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

	if (m_sAssetType == "Environment") {
		tempQueue = BuildRootNodeList();
	}
	else {
		if (node_ptr == nullptr)
			node_ptr = dzScene->getPrimarySelection();
		if (node_ptr == nullptr)
			return false;
		tempQueue.append(node_ptr);
	}

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
	QStringList existingNodeNameList;
	for (int i = 0; i < nodeJobList.length(); i++)
	{
		preProcessProgress.step();
		DzNode *node = nodeJobList[i];

		// bone conversion incompatibility fix (see line 226 below)
		if (node->inherits("DzBone")) continue;

		DzObject* object = node->getObject();
		DzShape* shape = object ? object->getCurrentShape() : NULL;

		if (shape || node->inherits("DzGroupNode") || node->inherits("DzInstanceNode")) {
			// 2024-SEP-10, DB: moved renameDuplicateNodeName inside if (shape) to fix bone conversion and other potential node name change incompatibilities
			renameDuplicateNodeName(node, existingNodeNameList);
		}

		if (shape)
		{
			for (int i = 0; i < shape->getNumMaterials(); i++)
			{
				DzMaterial* material = shape->getMaterial(i);
				if (material)
				{
					//////////////////
					// Force LIE Update
					//////////////////
					forceLieUpdate(material);

					//////////////////
					// Rename Duplicate Material
					/////////////////
					preProcessProgress.setInfo("Renaming Duplicate Materials: " + material->getLabel());
					renameDuplicateMaterial(material, &existingMaterialNameList);

					//////////////////
					// Rename Duplicate Clothing
					/////////////////
					if (m_sAssetType == "SkeletalMesh")
					{
						preProcessProgress.setInfo("Renaming Duplicate Clothing...");
						renameDuplicateClothing();
					}

					/////////////////
					// Generate Missing Normal Maps
					/////////////////
					if (m_bGenerateNormalMaps)
					{
						preProcessProgress.setInfo("Generating Missing Normal Maps: " + material->getLabel());
						generateMissingNormalMap(material);
					}

					///////////////////
					// Multiply Texture Values
					// This should be performed before Combine Diffuse + Alpha so that the correctly modulated alpha map is added to diffuse
					//////////////////
					if (m_bMultiplyTextureValues)
					{
						preProcessProgress.setInfo("Multiplying Texture Values: " + material->getLabel());
						multiplyTextureValues(material);
					}

					////////////////////
					// Combine Diffuse and Alpha Maps
					////////////////////
					if (m_bCombineDiffuseAndAlphaMaps)
					{
						preProcessProgress.setInfo("Combining Diffuse and Alpha Maps: " + material->getLabel());
						combineDiffuseAndAlphaMaps(material);
					}

					/////////////////////
					// Bake Translucency
					/////////////////////
					if (m_bBakeTranslucency)
					{
						preProcessProgress.setInfo("Baking Translucency Maps to Diffuse: " + material->getLabel());
						bakeTranslucency(material);
					}

					////////////////////
					// Bake Makeup
					////////////////////
					if (m_bBakeMakeupOverlay)
					{
						preProcessProgress.setInfo("Baking Makeup Overlay to Diffuse: " + material->getLabel());
						bakeMakeup(material);
					}

				}
			}

			// Look for geografts, add to geograft list
			if (isGeograft(node))
			{
				m_aGeografts.append(node->getName());
				//QString shapeName = shape->getName();
				//if (shapeName != "" && shapeName != node->getName())
				//{
				//	m_aGeografts.append(shapeName);
				//}
			}

		}
	}

	auto availables = MorphTools::GetAvailableMorphs(m_pSelectedNode);
	int debug_num_availables = availables.count();
	int debug_num_AvailableMorphsTable = m_AvailableMorphsTable.count();
	if (debug_num_availables > debug_num_AvailableMorphsTable)
	{
		// This code block used when FakeDualQuat (dq2lb) is enabled and _dq2lb morphs are generated
		m_AvailableMorphsTable.clear();
		m_AvailableMorphsTable = availables;
	}
	QStringList morphNamesToExport = MorphTools::getCombinedMorphList(m_MorphNamesToExport, m_AvailableMorphsTable, m_bEnableAutoJcm);

	// PreProcess MorphsToExport
//	foreach (MorphInfo &morphInfo, m_MorphsToExport)
	foreach (QString key, morphNamesToExport)
	{
		if (key.isEmpty())
			continue;

		if (m_AvailableMorphsTable.contains(key) == false) 
			continue;

		// rename Morph Property with prefix, if prefix is not already present
		if (m_AvailableMorphsTable[key].Name.contains(MORPH_EXPORT_PREFIX) == false)
		{
			MorphInfo morphInfo = m_AvailableMorphsTable[key];
			QString sExportName = MORPH_EXPORT_PREFIX + m_AvailableMorphsTable[key].Name;
			DzProperty* morphProperty = m_AvailableMorphsTable[key].Property;
			DzElement* owner = nullptr;
			if (morphProperty)
			{
				m_AvailableMorphsTable[key].ExportString = sExportName;
				owner = morphProperty->getOwner();
				if (owner && owner->inherits("DzModifier"))
				{
					if (morphInfo.Property != morphProperty)
					{
						dzApp->debug(QString("DzBridge: ERROR: preProcessScene(): morphInfo.Property mismatch found while renaming Morph: owner=%1").arg(owner->getName()));
					}
					owner->setName(sExportName);
					m_undoTable_MorphRename.insert(owner, morphInfo);
				}
				else
				{
					morphProperty->setName(sExportName);
					// add to undo table
					m_undoTable_MorphRename.insert(morphProperty, morphInfo);
				}
			}
		}

		// check to see if has pose
		//if (m_MorphsToExport[key].hasPoseErc())
		//{
		//	QString sNewBakedName = sExportName + "_baked";
		//	preProcessProgress.setInfo("Baking Pose Morph: " + m_MorphsToExport[key].Name);
		//	sNewBakedName = bakePoseMorph(qobject_cast<DzFloatProperty*>(m_MorphsToExport[key].Property), sNewBakedName);
		//	// replace morphinfo name with new baked name
		//	m_MorphsToExport[key].Name = sNewBakedName;
		//}
	}

    preProcessProgress.setInfo("DazBridge: Pre-Processing Completed.");
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
    DzProgress::setCurrentInfo("Daz Bridge: Undoing Missing Normal Map Generation...");

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

    DzProgress::setCurrentInfo("Daz Bridge: Undo Missing Normal Map Generation Complete.");

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
    DzProgress::setCurrentInfo("Daz Bridge: Undoing Export Processing...");
    
	bool bResult = true;

	// DB 2024-09-18: CrashFix: Undo Integrity Error: Re-order Undo operations so they remain in LIFO / stack ordering

	// Undo Morph Rename
	for (int i=0; i < m_undoTable_MorphRename.size(); i++)
	{
		DzBase* morph = m_undoTable_MorphRename.keys()[i];
		DzProperty* morphProperty = qobject_cast<DzProperty*>(morph);
		DzElement* morphElement = qobject_cast<DzElement*>(morph);
		MorphInfo morphInfo = m_undoTable_MorphRename[morph];
		QString originalName = morphInfo.Name;
		QString originalLabel = morphInfo.Label;
		if (morphProperty)
		{
			morphProperty->setName(originalName);
			morphProperty->setLabel(originalLabel);
		}
		else if (morphElement)
		{
			morphElement->setName(originalName);
			morphElement->setLabel(originalLabel);
			DzProperty *ownedProperty = morphInfo.Property;
			if (ownedProperty)
			{
				ownedProperty->setLabel(originalLabel);
			}
		}
		else
		{
			morph->setName(originalName);
		}
	}

	// Undo Combine Diffuse and ALpha Maps
	if (undoCombineDiffuseAndAlphaMaps() == false)
	{
		bResult = false;
	}

	// DB 2024-09-18: ?? Why was this not being called before ??
	if (undoMultiplyTextureValues() == false)
	{
		bResult = false;
	}

	// Undo Inject Normal Maps
	if (undoGenerateMissingNormalMaps() == false)
	{
		bResult = false;
	}

	// DB 2024-09-18: ?? Why was this not being called before ??
	if (undoRenameDuplicateClothing() == false)
	{
		bResult = false;
	}

	if (undoRenameDuplicateMaterials() == false)
	{
		bResult = false;
	}

	if (undoDuplicateNodeRename() == false)
	{
		bResult = false;
	}

	// Clear Override Tables
//	m_overrideTable_MaterialImageMaps.clear();
	m_overrideTable_MaterialProperties.clear();
	m_undoTable_MorphRename.clear();

    DzProgress::setCurrentInfo("Daz Bridge: Undo Export Processing Complete.");

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
    DzProgress::setCurrentInfo("Daz Bridge: Undoing Duplicate Material Renaming...");

    QMap<DzMaterial*, QString>::iterator iter;
	for (iter = m_undoTable_DuplicateMaterialRename.begin(); iter != m_undoTable_DuplicateMaterialRename.end(); ++iter)
	{
		iter.key()->setName(iter.value());
	}
	m_undoTable_DuplicateMaterialRename.clear();

    DzProgress::setCurrentInfo("Daz Bridge: Undo Duplicate Material Renaming Complete.");

    return true;
}

/// <summary>
/// Rename clothing if its name is already used by other clothing to
/// prevent collisions in Target Software. Called by preProcessScene().
/// See also: undoRenameDuplicateClothing().
/// </summary>
/// <returns>true if successful</returns>
bool DzBridgeAction::renameDuplicateClothing()
{
	// first get the figures.  We only need to check these
	QList<DzFigure*> figureNodes;
	int nodeCount = dzScene->getNumNodes();
	for (int i = 0; i < nodeCount; i++)
	{
		DzNode* node = dzScene->getNode(i);
		if (DzFigure* figure = qobject_cast<DzFigure*>(node))
		{
			figureNodes.append(figure);
		}
		/*if (DzSkeleton* skeleton = node->getSkeleton())
		{
			if (DzFigure* figure = qobject_cast<DzFigure*>(skeleton))
			{
				figureNodes.append(figure);
			}
		}*/
	}

	int figureCount = figureNodes.length();
	//qDebug() << "Count: " << figureCount;
	for (int i = 0; i < figureCount; i++)
	{
		DzFigure* primaryFigure = figureNodes[i];
		for (int j = i + 1; j < figureCount; j++)
		{
			DzFigure* secondaryFigure = figureNodes[j];
			if (primaryFigure->getName() == secondaryFigure->getName())
			{
				//qDebug() << "Match:" << primaryFigure->getName();
				m_undoTable_DuplicateClothingRename.insert(secondaryFigure, secondaryFigure->getName());
				QString newClothingName = secondaryFigure->getName() + QString("_%1").arg(j);
				secondaryFigure->setName(newClothingName);
			}
		}
	}
	return true;
}

/// <summary>
/// Undo any clothing modified by renameDuplicateClothing().
/// </summary>
/// <returns>true if successful</returns>
bool DzBridgeAction::undoRenameDuplicateClothing()
{
	QMap<DzFigure*, QString>::iterator iter;
	for (iter = m_undoTable_DuplicateClothingRename.begin(); iter != m_undoTable_DuplicateClothingRename.end(); ++iter)
	{
		iter.key()->setName(iter.value());
	}
	m_undoTable_DuplicateClothingRename.clear();

	return true;
}

/// <summary>
/// Convenience method to export base mesh and HD mesh as needed.
/// Performs weightmap fix on subdivided mesh.
/// See also: upgradeToHD(), Export().
/// </summary>
/// <param name="exportProgress">if null, exportHD will handle UI progress updates</param>
bool DzBridgeAction::exportHD(DzProgress* exportProgress)
{
    DzProgress::setCurrentInfo("Preparing asset for export via Daz Bridge Library...");

	// DB 2024-08-25: Ensure Primary Selection Integrity
	DzNode* pPrimarySelection = dzScene->getPrimarySelection();

    if (m_subdivisionDialog == nullptr)
		return false;

	// Update the dialog so subdivision locks later find all the meshes
	m_subdivisionDialog->PrepareDialog();

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

	// look for geograft morphs for export, and prepare
	if (m_bEnableMorphs)
	{
		prepareGeograftMorphsToExport(pPrimarySelection, true);
	}

	if (m_EnableSubdivisions && m_sAssetType != "MLDeformer")
	{
		if (exportProgress)
		{
			exportProgress->setInfo(tr("Exporting Asset as Base Mesh..."));
			exportProgress->step();
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}
		m_subdivisionDialog->LockSubdivisionProperties(false);
		m_bExportingBaseMesh = true;
		exportAsset(); // basemesh
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
			exportProgress->setInfo(tr("Exporting Asset as HD Mesh..."));
		else
			exportProgress->setInfo(tr("Exporting Asset..."));
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	}
	// DB 2024-08-25: Ensure Primary Selection Integrity
	dzScene->setPrimarySelection(pPrimarySelection);
	m_subdivisionDialog->LockSubdivisionProperties(m_EnableSubdivisions);
	m_bExportingBaseMesh = false;
	bool bExportResult = exportAsset();
	if (exportProgress)
	{
		exportProgress->step();
		exportProgress->setInfo(tr("Exporting Asset: Completed."));
	}

	// Export any geograft morphs if exist
	if (m_bEnableMorphs && bExportResult)
	{
        if (exportGeograftMorphs(pPrimarySelection, m_sDestinationPath))
		{
			exportProgress->step();
			exportProgress->setInfo(tr("Geograft morphs exported."));
		}
	}

	if (m_EnableSubdivisions && m_sAssetType != "MLDeformer" && bExportResult)
	{
		if (exportProgress)
		{
			exportProgress->step();
			exportProgress->setInfo(tr("Fixing bone weightmaps on HD Mesh..."));
			QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		}

		std::map<std::string, int>* pLookupTable = m_subdivisionDialog->GetLookupTable();
		// DB 2022-06-02: Fixed Upgrade HD bug with Bridges using __LEGACY_PATHS__ mode
		QString BaseCharacterFBX = this->m_sDestinationFBX;
		BaseCharacterFBX.replace(".fbx", "_base.fbx");
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

	// DB 2024-08-25: Ensure Primary Selection Integrity
	dzScene->setPrimarySelection(pPrimarySelection);
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

		if (bExportResult)
		{
			// 2022-02-13 (DB): Generic messagebox "Export Complete"
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::information(0, "DazBridge",
					tr("Export phase from Daz Studio complete. Please switch to Unity to begin Import phase."), QMessageBox::Ok);
			}
		}
		else
		{
			// 2022-02-13 (DB): Generic messagebox "Export Complete"
			if (m_nNonInteractiveMode == 0)
			{
				QMessageBox::information(0, "DazBridge",
					tr("Export cancelled."), QMessageBox::Ok);
			}
		}

	}

	return bExportResult;
}

bool DzBridgeAction::exportAsset()
{
	bool bReturnResult = false;

	// FBX Export
	m_pSelectedNode = dzScene->getPrimarySelection();
	if (m_pSelectedNode == nullptr)
		return false;

	QMap<QString, DzNode*> PropToInstance;
	if (m_sAssetType == "Environment")
	{
		// Store off the original export information
		QString OriginalFileName = m_sExportFilename;
		QString OriginalCharacterName = m_sAssetName;
		DzNode* OriginalSelection = m_pSelectedNode;

		// Find all the different types of props in the scene
		getScenePropList(m_pSelectedNode, PropToInstance);
		QMap<QString, DzNode*>::iterator iter;
		for (iter = PropToInstance.begin(); iter != PropToInstance.end(); ++iter)
		{
			// Override the export info for exporting this prop
			m_sAssetType = "StaticMesh";
			m_sExportFilename = iter.key();
			m_sExportFilename = m_sExportFilename.remove(QRegExp("[^A-Za-z0-9_]"));
			m_sAssetName = m_sExportFilename;
			m_sDestinationPath = m_sRootFolder + "/" + m_sExportFilename + "/";
			m_sDestinationFBX = m_sDestinationPath + m_sExportFilename + ".fbx";
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
            DzProgress::setCurrentInfo("Environment Export: Exporting Node: " + Node->getLabel());
			bReturnResult = exportNode(Node);

			// Put the item back where it was
			Node->setWSTransform(Location, Rotation, Scale);

			// Reconnect all the nodes
			reconnectNodes(AttachmentList);
		}

		// After the props have been exported, export the environment
		m_sExportFilename = OriginalFileName;
		m_sAssetName = OriginalCharacterName;
		m_sDestinationPath = m_sRootFolder + "/" + m_sExportSubfolder + "/";
		// use original export fbx filestem, if exists
		if (m_sExportFbx == "") m_sExportFbx = m_sExportFilename;
		m_sDestinationFBX = m_sDestinationPath + m_sExportFbx + ".fbx";
		m_pSelectedNode = OriginalSelection;
		m_sAssetType = "Environment";
		bReturnResult = exportNode(m_pSelectedNode);
	}
	else if (m_sAssetType == "Pose")
	{
		if (checkIfPoseExportIsDestructive())
		{
			if (QMessageBox::question(0, tr("Continue"),
				tr("Proceeding will delete keyed values on some properties. Continue?"),
				QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
			{
				return false;
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
				if (m_aPoseExportList.contains(propName))
				{
					poseIndex++;
					numericProperty->setDoubleValue(0.0f, 0.0f);
//					for (int frame = 0; frame < m_mMorphNameToLabel.count() + 1; frame++)
					for (int frame = 0; frame < m_MorphNamesToExport.count() + 1; frame++)
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
							if (m_aPoseExportList.contains(modifier->getName()))
							{
								poseIndex++;
								numericProperty->setDoubleValue(0.0f, 0.0f);
								for (int frame = 0; frame < m_aPoseExportList.count() + 1; frame++)
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

        DzProgress::setCurrentInfo("Pose Export: Exporting Selection: " + Selection->getLabel());
		bReturnResult = exportNode(Selection);
	}
	else if (m_sAssetType == "SkeletalMesh")
	{
		QList<QString> DisconnectedModifiers;
		if (m_bEnableMorphs)
		{
			if (m_bEnableAutoJcm && m_bEnableFakeDualQuat) 
			{
				exportJCMDualQuatDiff();
			}
			if (m_bAllowMorphDoubleDipping == false) 
			{
				DzProgress::setCurrentInfo("SkeletalMesh Export: Disconnecting Override Controllers... ");
				DisconnectedModifiers = disconnectOverrideControllers();
			}
		}
		DzNode* Selection = dzScene->getPrimarySelection();
        DzProgress::setCurrentInfo("SkeletalMesh Export: Exporting Selection: " + Selection->getLabel());
		bReturnResult = exportNode(Selection);
		if (m_bEnableMorphs)
		{
			reconnectOverrideControllers(DisconnectedModifiers);
		}
	}
	else
	{
		DzNode* Selection = dzScene->getPrimarySelection();
        DzProgress::setCurrentInfo("SkeletalMesh Export: Exporting Selection: " + Selection->getLabel());
		bReturnResult = exportNode(Selection);
	}

	return bReturnResult;
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

void overrideExportOptions(DzFileIOSettings &ExportOptions)
{

	//// TODO: REMOVE OVERRIDE WHEN WORKING
	//// DEBUG: Override
	ExportOptions.setBoolValue("doSelected", false);
	ExportOptions.setBoolValue("doVisible", true);
	ExportOptions.setBoolValue("doFigures", true);
	ExportOptions.setBoolValue("doProps", false);
	ExportOptions.setBoolValue("doLights", false);
	ExportOptions.setBoolValue("doCameras", false);
	ExportOptions.setBoolValue("doAnims", false);
	ExportOptions.setBoolValue("doMorphs", true);
	ExportOptions.setBoolValue("doFps", true);
//	ExportOptions.setStringValue("rules", m_sMorphSelectionRule);
	ExportOptions.setStringValue("format", "FBX 2014 -- Binary");
	ExportOptions.setIntValue("RunSilent", true);
	ExportOptions.setBoolValue("doEmbed", false);
	ExportOptions.setBoolValue("doCopyTextures", false);
	ExportOptions.setBoolValue("doDiffuseOpacity", false);
	ExportOptions.setBoolValue("doMergeClothing", true);
	ExportOptions.setBoolValue("doStaticClothing", false);
	ExportOptions.setBoolValue("degradedSkinning", false);
	ExportOptions.setBoolValue("degradedScaling", false);
	ExportOptions.setBoolValue("doSubD", false);
	//ExportOptions.setBoolValue("doCollapseUVTiles", false);
	ExportOptions.setBoolValue("doLocks", false);
	ExportOptions.setBoolValue("doLimits", false);
	ExportOptions.setBoolValue("doBaseFigurePoseOnly", false);
	ExportOptions.setBoolValue("doHelperScriptScripts", false);
	ExportOptions.setBoolValue("doMentalRayMaterials", false);
	//// DEBUG: Override

}

bool DzBridgeAction::exportNode(DzNode* Node)
{
	bool bReturnResult = false;

	if (Node == nullptr)
		return false;

	dzScene->selectAllNodes(false);
	dzScene->setPrimarySelection(Node);

	// DB 2023-11-15: Custom Asset Type Support
	if (isAssetEnvironmentCompatible(m_sAssetType))
	{
		QDir dir;
		dir.mkpath(m_sDestinationPath);
		writeConfiguration();
		return true;
	}

	// DB 2023-11-15: Custom Asset Type Support
	if ((isAssetAnimationCompatible(m_sAssetType) || isAssetPoseCompatible(m_sAssetType)) && m_bAnimationUseExperimentalTransfer)
	{
		QDir dir;
		dir.mkpath(m_sDestinationPath);
		exportAnimation(/*bExportingForMLDeformer*/);
		writeConfiguration();
		return true;
	}

	DzExportMgr* ExportManager = dzApp->getExportMgr();
	DzExporter* Exporter = ExportManager->findExporterByClassName("DzFbxExporter");

	if (Exporter)
	{
		// get the top level node for things like clothing so we don't get dupe material names
		DzNode* Parent = Node;
		if (m_sAssetType != "Environment")
		{
			while (Parent->getNodeParent() != NULL)
			{
				Parent = Parent->getNodeParent();
			}
		}

		// DB, 2023-11-08: Morph Selection Overhaul, defer generation of Morph Selection Rule string
		//  until after preprocessing stage.
		preProcessScene(Parent);
		if (m_bMorphLockBoneTranslation)
		{
			lockBoneControls(Parent);
		}

		QDir dir;
		dir.mkpath(m_sDestinationPath);

		DzFileIOSettings ExportOptions;
		ExportOptions.setBoolValue("doSelected", true);
		ExportOptions.setBoolValue("doVisible", false);
		// DB 2023-11-15: Custom Asset Type Support
		if (isAssetMeshCompatible(m_sAssetType))
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
		// DB 2023-11-15: Custom Asset Type Support
		if (isAssetAnimationCompatible(m_sAssetType))
		{
			ExportOptions.setBoolValue("doAnims", true);
		}
		else
		{
			ExportOptions.setBoolValue("doAnims", false);
		}
		// DB 2023-11-15: Morph Selection Overhaul
		m_sMorphSelectionRule = MorphTools::getMorphString(m_MorphNamesToExport, m_AvailableMorphsTable, m_bEnableAutoJcm);
		// DB 2023-11-15: Custom Asset Type Support
		if (isAssetMorphCompatible(m_sAssetType) && m_bEnableMorphs && m_sMorphSelectionRule != "")
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
		// DB 5-26-2023: enable doSubD to export crease-info
		ExportOptions.setBoolValue("doSubD", true);
		ExportOptions.setBoolValue("doCollapseUVTiles", false);

		// DB 2023-11-15: Custom Asset Type Support
		if (isAssetMeshCompatible(m_sAssetType) && m_EnableSubdivisions)
		{
			ExportOptions.setBoolValue("doCollapseUVTiles", true);
		}

		setExportOptions(ExportOptions);

		// DB 2024-08-24: Fix selected node before exporting to Fbx
		dzScene->selectAllNodes(false);
		dzScene->setPrimarySelection(Node);
		if (m_EnableSubdivisions && m_bExportingBaseMesh)
		{
			QString CharacterBaseFBX = this->m_sDestinationFBX;
			CharacterBaseFBX.replace(".fbx", "_base.fbx");
			// Bugfix for double fbx options dialog
			ExportOptions.setIntValue("RunSilent", true);
			// BASE MESH FBX EXPORT OPERATION
			Exporter->writeFile(CharacterBaseFBX, &ExportOptions);
		}
		else
		{
			// MAIN FBX EXPORT OPERATION
			DzError result;
			dzApp->log("DEBUG: Preparing to export FBX using DzExporter::writeFile()...");
			try {
				result = Exporter->writeFile(m_sDestinationFBX, &ExportOptions);
				dzApp->log("DEBUG: Export using DzExporter::writeFile() completed successfuly with result code=" + QString("%1").arg((int)result));
			}
			catch (std::exception& e) {
				result = -1;
				dzApp->warning("ERROR: Exception caught during FBX Export for: " + m_sDestinationPath + ", e=" + QString(e.what()));
				QMessageBox::critical(0, tr("DzBridge Critical Exception"),
					tr("A critical exception error occured during the FBX export operation using DzExporter::writeFile().") + "\n" +
					tr("Please refer to the Daz Studio log.txt file for details."), QMessageBox::Ok);
				bReturnResult = false;
			}

			if (result == DZ_USER_CANCELLED_OPERATION || result != DZ_NO_ERROR)
			{
				// handle cancel operation
				if (result == DZ_USER_CANCELLED_OPERATION) {
					dzApp->log("INFO: User cancelled FBX Export for: " + m_sDestinationFBX);
				}
				else {
					dzApp->warning("ERROR: Error occured during FBX Export for: " + m_sDestinationFBX + " , DzError=" + QString("%1").arg((int)result));
				}
				bReturnResult = false;
			}
			else
			{
				bReturnResult = true;

				// DB 2022-09-26: Post-Process FBX file
				DzProgress::setCurrentInfo("Daz Bridge: PostProcessing FBX: " + m_sDestinationFBX);
				postProcessFbx(m_sDestinationFBX);

				// composing dtu filename string here just to use with set current info
				QString DTUfilename = m_sDestinationPath + m_sAssetName + ".dtu";
				DzProgress::setCurrentInfo("Daz Bridge: Writing DTU Configuration File: " + DTUfilename);
				writeConfiguration();
			}
		}
		if (m_bMorphLockBoneTranslation)
		{
			unlockBoneControl(Parent);
		}
		undoPreProcessScene();
	}

	return bReturnResult;
}

void DzBridgeAction::exportAnimation()
{
	if (!m_pSelectedNode) return;

	DzSkeleton* Skeleton = m_pSelectedNode->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;

	if (!Figure) return;

	// Setup FBX Exporter
	FbxManager* SdkManager = FbxManager::Create();

	FbxIOSettings* ios = FbxIOSettings::Create(SdkManager, IOSROOT);
	SdkManager->SetIOSettings(ios);

	int FileFormat = -1;
#ifdef VODSVERSION
    FileFormat = SdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");
#else
    FileFormat = SdkManager->GetIOPluginRegistry()->GetNativeWriterFormat();
#endif
    //qDebug() << "FileName: " << this->m_sDestinationFBX;
	FbxExporter* Exporter = FbxExporter::Create(SdkManager, "");
	if (!Exporter->Initialize(this->m_sDestinationFBX.toUtf8().data(), FileFormat, SdkManager->GetIOSettings()))
	{
		return;
	}

	// Get the Figure Scale
	float FigureScale = m_pSelectedNode->getScaleControl()->getValue();

	// Create the Scene
	FbxScene* Scene = FbxScene::Create(SdkManager, "");

	FbxAnimStack* AnimStack = FbxAnimStack::Create(Scene, "AnimStack");
	FbxAnimLayer* AnimBaseLayer = FbxAnimLayer::Create(Scene, "Layer0");
	AnimStack->AddMember(AnimBaseLayer);

	// Add the skeleton to the scene
	QMap<DzNode*, FbxNode*> BoneMap;
	exportSkeleton(Figure, m_pSelectedNode, nullptr, nullptr, Scene, BoneMap);

	// Get the play range
	DzTimeRange PlayRange = dzScene->getPlayRange();

	// Root Node
	exportNodeAnimation(Figure, BoneMap, AnimBaseLayer, FigureScale /*, bExportingForMLDeformer*/);

	// Iterate the bones
	DzBoneList Bones = getAllBones(m_pSelectedNode);
	//Skeleton->getAllBones(Bones);
	for (auto Bone : Bones)
	{
		exportNodeAnimation(Bone, BoneMap, AnimBaseLayer, FigureScale /*, bExportingForMLDeformer*/);
	}

	// Get a list of animated properties
	if (m_bAnimationExportActiveCurves)
	{
		QList<DzNumericProperty*> animatedProperties = getAnimatedProperties(m_pSelectedNode);
		exportAnimatedProperties(animatedProperties, Scene, AnimBaseLayer);
	}

	// Write the FBX
	Exporter->Export(Scene);
	Exporter->Destroy();
}

void DzBridgeAction::exportNodeAnimation(DzNode* Bone, QMap<DzNode*, FbxNode*>& BoneMap, FbxAnimLayer* AnimBaseLayer, float FigureScale)
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
		if (m_bFixTwistBones)
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
		if (m_bAnimationApplyBoneScale)
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

void DzBridgeAction::exportSkeleton(DzFigure* Figure, DzNode* Node, DzNode* Parent, FbxNode* FbxParent, FbxScene* Scene, QMap<DzNode*, FbxNode*>& BoneMap)
{
	// Only transfer face bones if requested.  MLDeformer doesn't like missing bones in UE5.3 and earlier
	bool bIncludeFaceBones = m_bAnimationTransferFace;
	if (m_sAssetType == "MLDeformer") bIncludeFaceBones = m_bMLDeformerExportFace;
	if (Parent != nullptr && Parent->getName() == "head" && bIncludeFaceBones == false) return;

	FbxNode* BoneNode;

	// null parent is the root bone
	if (FbxParent == nullptr)
	{
		// Create a root bone.  Always named root so we don't have to fix it in Unreal
		FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(Scene, "root");
		SkeletonAttribute->SetSkeletonType(FbxSkeleton::eRoot);
		BoneNode = FbxNode::Create(Scene, "root");
		BoneNode->SetNodeAttribute(SkeletonAttribute);

		FbxNode* RootNode = Scene->GetRootNode();
		RootNode->AddChild(BoneNode);

		// Looks through the child nodes for more bones
		for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
		{
			DzNode* ChildNode = Node->getNodeChild(ChildIndex);
			exportSkeleton(Figure, ChildNode, Node, BoneNode, Scene, BoneMap);
		}
	}
	else
	{
		// Child nodes need to be bones
		if (DzBone* Bone = qobject_cast<DzBone*>(Node))
		{
			// create the bone
			FbxSkeleton* SkeletonAttribute = FbxSkeleton::Create(Scene, Node->getName().toUtf8().data());
			SkeletonAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
			BoneNode = FbxNode::Create(Scene, Node->getName().toUtf8().data());
			BoneNode->SetNodeAttribute(SkeletonAttribute);

			// find the bones position
			DzVec3 Position = Node->getWSPos(DzTime(0), true);
			DzVec3 ParentPosition = Parent->getWSPos(DzTime(0), true);
			DzVec3 LocalPosition = Position - ParentPosition;

			// find the bone's rotation
			DzQuat Rotation = Node->getWSRot(DzTime(0), true);
			DzQuat ParentRotation = Parent->getWSRot(DzTime(0), true);
			DzQuat LocalRotation = Node->getOrientation(true);//Rotation * ParentRotation.inverse();
			DzVec3 VectorRotation;
			LocalRotation.getValue(VectorRotation);

			// set the position and rotation properties
			BoneNode->LclTranslation.Set(FbxVector4(LocalPosition.m_x, LocalPosition.m_y, LocalPosition.m_z));
			BoneNode->LclRotation.Set(FbxVector4(VectorRotation.m_x, VectorRotation.m_y, VectorRotation.m_z));

			// if fixing twist bones, reparent their children
			if (m_bFixTwistBones && Node->getNodeParent() != nullptr && Node->getNodeParent()->getName().contains("twist", Qt::CaseInsensitive))
			{
				FbxParent->GetParent()->AddChild(BoneNode);
			}
			else
			{
				FbxParent->AddChild(BoneNode);
			}

			// Looks through the child nodes for more bones
			QList<QString> DirectChildBones;
			for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
			{
				DzNode* ChildNode = Node->getNodeChild(ChildIndex);
				if (DzBone* ChildBone = qobject_cast<DzBone*>(ChildNode))
				{
					DirectChildBones.append(ChildBone->getName());
				}
				exportSkeleton(Figure, ChildNode, Node, BoneNode, Scene, BoneMap);
			}

			// Add child figure bones
			for (int ChildFigureIndex = 0; ChildFigureIndex < Figure->getNumNodeChildren(); ChildFigureIndex++)
			{
				DzNode* ChildNode = Figure->getNodeChild(ChildFigureIndex);
				if (DzFigure* ChildFigure = qobject_cast<DzFigure*>(ChildNode))
				{
					// Find matching parent bone in child figures
					if (DzNode* ChildFigureMatchingParentBone = ChildFigure->findBone(Node->getName()))
					{
						// Look for new child bones
						for (int ChildBoneIndex = 0; ChildBoneIndex < ChildFigureMatchingParentBone->getNumNodeChildren(); ChildBoneIndex++)
						{
							DzNode* ChildNode = ChildFigureMatchingParentBone->getNodeChild(ChildBoneIndex);
							if (DzBone* ChildBone = qobject_cast<DzBone*>(ChildNode))
							{
								if (!DirectChildBones.contains(ChildBone->getName()))
								{
									DirectChildBones.append(ChildBone->getName());
									exportSkeleton(Figure, ChildNode, Node, BoneNode, Scene, BoneMap);
									qDebug() << "Found Extra Bone: " << ChildBone->getName();
								}
							}
						}
					}
				}
			}
		}
	}

	// Add the bone to the map
	BoneMap.insert(Node, BoneNode);
}

QList<DzNumericProperty*> DzBridgeAction::getAnimatedProperties(DzNode* Node)
{
	QList<DzNumericProperty*> animatedPropertyList;
	animatedPropertyList.clear();

	if (Node == nullptr)
		return animatedPropertyList;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(property);
		if (numericProp)
		{
			for (int keyIndex = 0; keyIndex < property->getNumKeys(); keyIndex++)
			{
				if (numericProp->getDoubleValue(property->getKeyTime(keyIndex)) > 0.001f)
				{
					animatedPropertyList.append(numericProp);
					QString propName = property->getLabel();
					//qDebug() << "Animated Property: " << propName;
				}
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
					DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(property);
					if (numericProp)
					{
						for (int keyIndex = 0; keyIndex < property->getNumKeys(); keyIndex++)
						{
							if (numericProp->getDoubleValue(property->getKeyTime(keyIndex)) > 0.001f)
							{
								animatedPropertyList.append(numericProp);
								QString propName = property->getLabel();
								//qDebug() << "Animated Property: " << propName;
							}
						}
					}
				}
			}
		}
	}

	return animatedPropertyList;
}

void DzBridgeAction::exportAnimatedProperties(QList<DzNumericProperty*>& Properties, FbxScene* Scene, FbxAnimLayer* AnimBaseLayer)
{
	DzTimeRange PlayRange = dzScene->getPlayRange();

	foreach(DzNumericProperty * numericProperty, Properties)
	{
		FbxNode* RootNode = Scene->FindNodeByName("root");
		FbxProperty fbxProperty = FbxProperty::Create(RootNode, FbxDoubleDT, numericProperty->getLabel().toUtf8().data());
		if (fbxProperty.IsValid())
		{
			FbxAnimCurveNode* CurveNode = fbxProperty.CreateCurveNode(AnimBaseLayer);
			fbxProperty.ModifyFlag(FbxPropertyFlags::eAnimatable, true);
			fbxProperty.ModifyFlag(FbxPropertyFlags::eUserDefined, true);
			FbxAnimCurve* lAnimCurve = fbxProperty.GetCurve(AnimBaseLayer, true);

			if (lAnimCurve == nullptr)
			{
				//qDebug() << "Animated Property: FbxAnimCurve is invalid: " << numericProperty->getLabel();
				continue;
			}
			lAnimCurve->KeyModifyBegin();

			// For each frame, write a key (equivalent of bake)
			for (DzTime CurrentTime = PlayRange.getStart(); CurrentTime <= PlayRange.getEnd(); CurrentTime += dzScene->getTimeStep())
			{
				DzTime Frame = CurrentTime / dzScene->getTimeStep();
				double Value = numericProperty->getDoubleValue(CurrentTime);

				FbxTime Time;
				int KeyIndex = 0;
				Time.SetFrame(Frame);

				int lKeyIndex = lAnimCurve->KeyAdd(Time);
				lAnimCurve->KeySet(lKeyIndex, Time, Value, FbxAnimCurveDef::eInterpolationLinear);
				//qDebug() << "Animated Property: " << numericProperty->getLabel() << " Time: " << Frame << "Value:" << Value;
			}

			lAnimCurve->KeyModifyEnd();
		}
	}
}

void DzBridgeAction::exportJCMDualQuatDiff()
{
	if (!m_pSelectedNode) return;

	// Set the resolution level to Base
	int ResolutionLevel = 1; // High Resolution
	DzEnumProperty* ResolutionLevelProperty = NULL;
	if (m_pSelectedNode->getObject() && m_pSelectedNode->getObject()->getCurrentShape())
	{
		DzShape* Shape = m_pSelectedNode->getObject()->getCurrentShape();
		if (DzProperty* Property = Shape->findProperty("lodlevel"))
		{
			if (ResolutionLevelProperty = qobject_cast<DzEnumProperty*>(Property))
			{
				ResolutionLevel = ResolutionLevelProperty->getValue();
				ResolutionLevelProperty->setValue(0); // Base
			}
		}
	}

	//QList<DzNode*> FigureNodeList = GetFigureNodeList(m_pSelectedNode);
	DzNode* FigureNode = m_pSelectedNode;
	//foreach(DzNode * FigureNode, FigureNodeList)
	{
		QList<JointLinkInfo> JointLinks = MorphTools::GetActiveJointControlledMorphs( m_pSelectedNode );
		for (auto JointLink : JointLinks)
		{
			if (JointLink.IsBaseJCM)
			{
				exportNodeexportJCMDualQuatDiff(JointLink);
			}
		}
	}

	// Set the resolution level back to what it was
	if (ResolutionLevelProperty)
	{
		ResolutionLevelProperty->setValue(ResolutionLevel);
	}
}
void DzBridgeAction::exportNodeexportJCMDualQuatDiff(const JointLinkInfo& JointInfo)
{
	//if (JointInfo.Bone != "l_upperarm") return;

	float RotationAmount = 90.0f;

	if (JointInfo.Scalar != 0.0f && JointInfo.Keys.count() == 0)
	{
		RotationAmount = 1.0f / JointInfo.Scalar;
	}

	if (JointInfo.Keys.count() > 0)
	{
		for (JointLinkKey Key : JointInfo.Keys)
		{
			if (Key.Value == 1)
			{
				RotationAmount = Key.Angle;
			}
		}

		//Remove the rotation for the largest 0 key
		// We're only rotating by the applied amount of the JCM, not the max angle
		for (JointLinkKey Key : JointInfo.Keys)
		{
			if (Key.Value == 0 && abs(Key.Angle) > 0.1f)
			{
				float Ratio = pow(abs(sinf(0.01745329251 * (RotationAmount - Key.Angle))), 0.5f);
				RotationAmount *= Ratio;

				// Extreme rotations turn inside out in spots.
				if (RotationAmount > 120.0f) RotationAmount = 120.0f;
				if (RotationAmount < -120.0f) RotationAmount = -120.0f;
			}
		}
	}



	DzSkeleton* Skeleton = m_pSelectedNode->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;

	DzSkinBinding* Binding = Figure->getSkinBinding();

	DzObject* Object = m_pSelectedNode->getObject();

	DzGeometry* Geometry = Object->getCurrentShape()->getGeometry();

	DzBone* Bone = Skeleton->findBone(JointInfo.Bone);
	if (Bone == nullptr) return;
	if (JointInfo.Axis == "XRotate")
	{
		Bone->getXRotControl()->setValue(RotationAmount);
	}
	if (JointInfo.Axis == "YRotate")
	{
		Bone->getYRotControl()->setValue(RotationAmount);
	}
	if (JointInfo.Axis == "ZRotate")
	{
		Bone->getZRotControl()->setValue(RotationAmount);
	}

	setDualQuaternionBlending(Binding);

	DzVertexMesh* DualQuaternionMesh = Object->getCachedGeom();
	DzFacetMesh* CachedDualQuaternionMesh = new DzFacetMesh();
	CachedDualQuaternionMesh->copyFrom(DualQuaternionMesh, false, false);

	setLinearBlending(Binding);

	createDualQuaternionToLinearBlendDiffMorph(JointInfo.MorphName, CachedDualQuaternionMesh, dzScene->getPrimarySelection());

	Bone->getXRotControl()->setValue(0.0f);
	Bone->getYRotControl()->setValue(0.0f);
	Bone->getZRotControl()->setValue(0.0f);

	setDualQuaternionBlending(Binding);
}

void DzBridgeAction::setLinearBlending(DzSkinBinding* Binding)
{
	int methodIndex = Binding->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("setGeneralMapMode(int)"));
	if (methodIndex != -1)
	{
		QMetaMethod method = Binding->metaObject()->method(methodIndex);
		int result = method.invoke((QObject*)Binding, Q_ARG(int, 0));
		if (result != -1)
		{

		}
	}

	m_pSelectedNode->update();
	m_pSelectedNode->finalize();
}
void DzBridgeAction::setDualQuaternionBlending(DzSkinBinding* Binding)
{
	int methodIndex = Binding->metaObject()->indexOfMethod(QMetaObject::normalizedSignature("setGeneralMapMode(int)"));
	if (methodIndex != -1)
	{
		QMetaMethod method = Binding->metaObject()->method(methodIndex);
		int result = method.invoke((QObject*)Binding, Q_ARG(int, 1));
		if (result != -1)
		{

		}
	}

	m_pSelectedNode->update();
	m_pSelectedNode->finalize();
}

void DzBridgeAction::createDualQuaternionToLinearBlendDiffMorph(const QString BaseMorphName, DzVertexMesh* Mesh, DzNode* Node)
{
	DzScript* Script = new DzScript();

	Script->addLine("function myFunction(oNode, oSavedGeom, sName) {");
	Script->addLine("var oMorphLoader = new DzMorphLoader();");
	Script->addLine("oMorphLoader.setMorphName(sName);");
	Script->addLine("oMorphLoader.setMorphSubdivision(true);");
	Script->addLine("oMorphLoader.setSubdivisionBuiltResolution(0);");
	Script->addLine("oMorphLoader.setSubdivisionMinResolution(0);");
	Script->addLine("oMorphLoader.setSubdivisionMaxResolution(0);");
	Script->addLine("oMorphLoader.setSubdivisionSmoothCage(false);");
	Script->addLine("oMorphLoader.setSubdivisionMapping(DzMorphLoader.Catmark);");
	Script->addLine("oMorphLoader.setDeltaTolerance(0.01);");
	Script->addLine("oMorphLoader.setCreateControlProperty(true);");
	Script->addLine("oMorphLoader.setPropertyGroupPath(\"Morph Loader Pro\");");
	Script->addLine("oMorphLoader.setReverseDeformations(true);");
	Script->addLine("oMorphLoader.setOverwriteExisting(DzMorphLoader.DeltasOnly);");
	Script->addLine("oMorphLoader.setCleanUpOrphans(true);");
	Script->addLine("oMorphLoader.setMorphMirroring(DzMorphLoader.DoNotMirror);");
	//Script->addLine("oMorphLoader.applyReverseDeformationsPose();");
	Script->addLine("var result = oMorphLoader.createMorphFromMesh(oSavedGeom, oNode);");
	Script->addLine("};");

	QString NewMorphName = BaseMorphName + "_dq2lb";

	QVariantList Args;
	QVariant v(QMetaType::QObjectStar, &Node);
	Args.append(QVariant(QMetaType::QObjectStar, &Node));
	Args.append(QVariant(QMetaType::QObjectStar, &Mesh));
	Args.append(QVariant(NewMorphName));
	Script->call("myFunction", Args);
}

QList<DzNode*> DzBridgeAction::GetFigureNodeList(DzNode* Node)
{
	QList<DzNode*> FigureList;
	DzSkeleton* Skeleton = Node->getSkeleton();
	DzFigure* Figure = Skeleton ? qobject_cast<DzFigure*>(Skeleton) : NULL;
	if (Figure != NULL)
	{
		FigureList.append(Node);
	}

	// Looks through the child nodes for more bones
	for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		FigureList.append(GetFigureNodeList(ChildNode));
	}
	return FigureList;
}

void DzBridgeAction::lockBoneControls(DzNode* Bone)
{
	Bone->getScaleControl()->lock(true);
	Bone->getScaleControl()->setOverrideControllers(true);

	Bone->getOriginXControl()->lock(true);
	Bone->getOriginXControl()->setOverrideControllers(true);
	Bone->getEndXControl()->lock(true);
	Bone->getEndXControl()->setOverrideControllers(true);

	Bone->getOriginYControl()->lock(true);
	Bone->getOriginYControl()->setOverrideControllers(true);
	Bone->getEndYControl()->lock(true);
	Bone->getEndYControl()->setOverrideControllers(true);

	Bone->getOriginZControl()->lock(true);
	Bone->getOriginZControl()->setOverrideControllers(true);
	Bone->getEndZControl()->lock(true);
	Bone->getEndZControl()->setOverrideControllers(true);

	// Looks through the child nodes for more bones
	for (int ChildIndex = 0; ChildIndex < Bone->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Bone->getNodeChild(ChildIndex);
		lockBoneControls(ChildNode);
	}
}

void DzBridgeAction::unlockBoneControl(DzNode* Bone)
{
	Bone->getScaleControl()->lock(false);
	Bone->getScaleControl()->setOverrideControllers(false);

	Bone->getOriginXControl()->lock(false);
	Bone->getOriginXControl()->setOverrideControllers(false);
	Bone->getEndXControl()->lock(false);
	Bone->getEndXControl()->setOverrideControllers(false);

	Bone->getOriginYControl()->lock(false);
	Bone->getOriginYControl()->setOverrideControllers(false);
	Bone->getEndYControl()->lock(false);
	Bone->getEndYControl()->setOverrideControllers(false);

	Bone->getOriginZControl()->lock(false);
	Bone->getOriginZControl()->setOverrideControllers(false);
	Bone->getEndZControl()->lock(false);
	Bone->getEndZControl()->setOverrideControllers(false);

	// Looks through the child nodes for more bones
	for (int ChildIndex = 0; ChildIndex < Bone->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Bone->getNodeChild(ChildIndex);
		lockBoneControls(ChildNode);
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

	DzNumericProperty* previousProperty = nullptr;
	for (int index = 0; index < Selection->getNumProperties(); index++)
	{
		DzProperty* property = Selection->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		if (numericProperty && !numericProperty->isOverridingControllers())
		{
			QString propName = property->getName();
			if (m_MorphNamesToExport.contains(propName) && m_ControllersToDisconnect.contains(propName))
			{
				numericProperty->setOverrideControllers(true);

				double propValue = numericProperty->getDoubleValue();
				m_undoTable_ControllersToDisconnect.insert(propName, propValue);
				numericProperty->setDoubleValue(numericProperty->getDoubleDefaultValue());

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
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty && !numericProperty->isOverridingControllers())
					{
						QString propName = MorphTools::GetMorphPropertyName(property);
						if (m_MorphNamesToExport.contains(modifier->getName()) && m_ControllersToDisconnect.contains(modifier->getName()))
						{
							numericProperty->setOverrideControllers(true);

							double propValue = numericProperty->getDoubleValue();
							m_undoTable_ControllersToDisconnect.insert(propName, propValue);
							numericProperty->setDoubleValue(numericProperty->getDoubleDefaultValue());

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

	// DB, 2022-06-13: Must reconnect in REVERSE ORDER, since items are inter-connected
	for (int index = 0; index < Selection->getNumProperties(); index++)
//	for (int index = Selection->getNumProperties() - 1; index >= 0; index--)
	{
		DzProperty* property = Selection->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		QString propName = property->getName();
		if (numericProperty && numericProperty->isOverridingControllers())
		{
			QString propName = property->getName();
			if (DisconnetedControllers.contains(propName))
			{
				double propValue = m_undoTable_ControllersToDisconnect.value(propName);
				numericProperty->setDoubleValue(propValue);

				numericProperty->setOverrideControllers(false);
			}
		}
	}

	DzObject* Object = Selection->getObject();
	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
//		for (int index = Object->getNumModifiers(); index >= 0; index--)
		{
			DzModifier* modifier = Object->getModifier(index);
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty && numericProperty->isOverridingControllers())
					{
						QString propName = MorphTools::GetMorphPropertyName(property);
						if (DisconnetedControllers.contains(modifier->getName()))
						{
							double propValue = m_undoTable_ControllersToDisconnect.value(propName);
							numericProperty->setDoubleValue(propValue);

							numericProperty->setOverrideControllers(false);
						}
					}
				}

			}

		}
	}

	// reset undo table
	m_undoTable_ControllersToDisconnect.clear();

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
//			if (m_mMorphNameToLabel.contains(propName))
			if (m_MorphNamesToExport.contains(propName))
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
//						if (m_mMorphNameToLabel.contains(modifier->getName()))
						if (m_MorphNamesToExport.contains(modifier->getName()))
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

QString DzBridgeAction::generateExportAssetFilename(QString sFilename, QString sAssetMaterialName)
{
	if (sFilename.isEmpty())
		return sFilename;

	QString cleanedFilename = sFilename.toLower().replace("\\", "/");
	QString cleanedTempPath = dzApp->getTempPath().toLower().replace("\\", "/");
	QString cleanedAssetMaterialName = sAssetMaterialName;
	cleanedAssetMaterialName.remove(QRegExp("[^A-Za-z0-9_]"));

	QString exportPath = this->m_sRootFolder.replace("\\", "/") + "/" + this->m_sExportSubfolder.replace("\\", "/");
	QString fileStem = QFileInfo(sFilename).fileName();
	// DB 2023-Oct-20: add partial path for short filenames
	QString sNameTest = QFileInfo(sFilename).baseName().remove(QRegExp("[^A-Za-z]")).remove("base").remove("color").remove("normal").remove("roughness").remove("metallic").remove("height").remove("opengl");
	if (isTemporaryFile(sFilename) == false &&
		sNameTest.length() < 3 &&
		QFileInfo(sFilename).baseName().length() < 20)
	{
		QStringList filePathArray = cleanedFilename.remove(" ").split("/");
		int len = filePathArray.count();
		for (int i = 2; i < 5; i++)
		{
			if (i > len) break;
			fileStem = filePathArray[len - i] + "_" + fileStem;
		}
	}

	exportPath += "/ExportTextures/";
	QDir().mkpath(exportPath);
	//	QString exportFilename = exportPath + cleanedAssetMaterialName + "_" + fileStem;
	QString exportFilename = exportPath + fileStem;

	return exportFilename;
}

QString DzBridgeAction::exportAssetWithDtu(QString sFilename, QString sAssetMaterialName)
{
	if (sFilename.isEmpty())
		return sFilename;

/*
	QString cleanedFilename = sFilename.toLower().replace("\\", "/");
	QString cleanedTempPath = dzApp->getTempPath().toLower().replace("\\", "/");
	QString cleanedAssetMaterialName = sAssetMaterialName;
	cleanedAssetMaterialName.remove(QRegExp("[^A-Za-z0-9_]"));

	QString exportPath = this->m_sRootFolder.replace("\\","/") + "/" + this->m_sExportSubfolder.replace("\\", "/");
	QString fileStem = QFileInfo(sFilename).fileName();
	// DB 2023-Oct-20: add partial path for short filenames
	QString sNameTest = QFileInfo(sFilename).baseName().remove(QRegExp("[^A-Za-z]")).remove("base").remove("color").remove("normal").remove("roughness").remove("metallic").remove("height").remove("opengl");
	if (isTemporaryFile(sFilename) == false &&
		sNameTest.length() < 3 &&
		QFileInfo(sFilename).baseName().length() < 20)
	{
		QStringList filePathArray = cleanedFilename.remove(" ").split("/");
		int len = filePathArray.count();
		for (int i = 2; i < 5; i++)
		{
			if (i > len) break;
			fileStem = filePathArray[len - i] + "_" + fileStem;
		}
	}

	exportPath += "/ExportTextures/";
	QDir().mkpath(exportPath);
//	QString exportFilename = exportPath + cleanedAssetMaterialName + "_" + fileStem;
	QString exportFilename = exportPath + fileStem;

	exportFilename = makeUniqueFilename(exportFilename, sFilename);
*/
	QString exportFilename = generateExportAssetFilename(sFilename, sAssetMaterialName);
	exportFilename = makeUniqueFilename(exportFilename, sFilename);

	if (QFile(sFilename).copy(exportFilename) == true)
	{
		return exportFilename;
	}

	// copy method may fail if file already exists,
	// if exists and same file size, then proceed as if successful
	ulong crc32_Source = calcCRC32(sFilename);
	int bFirstCRCResult = getCalcCRC32ResultCode();
	if (bFirstCRCResult != CalcCRC32ResultCodes::SUCCESS)
	{
		dzApp->warning("ERROR: exportAssetWithDTU(): CalcCRC32 was not successful for: " + sFilename);
	}
	ulong crc32_Dest = calcCRC32(exportFilename);
	int bSecondCRCResult = getCalcCRC32ResultCode();
	if (bSecondCRCResult != CalcCRC32ResultCodes::SUCCESS)
	{
		dzApp->warning("ERROR: exportAssetWithDTU(): CalcCRC32 was not successful for: " + exportFilename);
	}
	if ( QFileInfo(exportFilename).exists() &&
		QFileInfo(sFilename).size() == QFileInfo(exportFilename).size() &&
		bFirstCRCResult == CalcCRC32ResultCodes::SUCCESS &&
		bSecondCRCResult == CalcCRC32ResultCodes::SUCCESS &&
		crc32_Source == crc32_Dest )
	{
		return exportFilename;
	}

	// return original source string if failed
	dzApp->warning("ERROR: exportAssetWithDTU(): Unexpected error occured while preparing file: " + sFilename);
	return sFilename;

}

unsigned int DzBridgeAction::calcCRC32(QString sFilename)
{
	ulong crc32Result = 0;
	m_nCalcCRC32ResultCode = CalcCRC32ResultCodes::SUCCESS;

	QFile oFile(sFilename);
	if (oFile.open(QIODevice::OpenModeFlag::ReadOnly) == false)
	{
		m_nCalcCRC32ResultCode = CalcCRC32ResultCodes::ERROR_OPENING_FILE;
		return 0;
	}

	QByteArray byteArray = oFile.readAll();
	crc32Result = (ulong) ::mz_crc32((ulong)crc32Result, (const unsigned char*) byteArray.constData(), byteArray.size());

	oFile.close();

	return crc32Result;
}

// DB NOTES: use sOriginalFilename to check if sTargetFilename is the same file
// 2023-Oct-23: Method now works (mostly), size file check supplemented with CRC32 as file content hash.
QString DzBridgeAction::makeUniqueFilename(QString sTargetFilename, QString sOriginalFilename)
{
	if (QFileInfo(sTargetFilename).exists() != true)
		return sTargetFilename;

	QString newFilename = sTargetFilename;
	int duplicate_count = 0;	

	while (QFileInfo(newFilename).exists())
	{
		if (sOriginalFilename != "")
		{
			if ( QFileInfo(sOriginalFilename).size() == QFileInfo(newFilename).size() &&
				calcCRC32(sOriginalFilename) == calcCRC32(newFilename) )
			{
				// OK to use as unique, because original and new file are equivalent (per CRC32)
				// TODO: replace with MD5 or SHA to reduce frequency of false positive
				break;
			}
		}
		QFileInfo fileInfo(sTargetFilename);
		QString sExt = fileInfo.suffix();
		QString sNameWtihoutExt = fileInfo.completeBaseName();
		QString sPath = fileInfo.path();
		newFilename = sPath + "/" + sNameWtihoutExt + QString("_%1").arg(duplicate_count++) + "." + sExt;
	}

	return newFilename;

}

void DzBridgeAction::writePropertyTexture(DzJsonWriter& Writer, QString sName, QString sLabel, QString sValue, QString sType, QString sTexture)
{
	Writer.startObject(true);
	Writer.addMember("Name", sName);
	Writer.addMember("Label", sLabel);
	Writer.addMember("Value", sValue);
	Writer.addMember("Data Type", sType);
	Writer.addMember("Texture", sTexture);
	Writer.finishObject();

}

void DzBridgeAction::writePropertyTexture(DzJsonWriter& Writer, QString sName, QString sLabel, double dValue, QString sType, QString sTexture)
{
	Writer.startObject(true);
	Writer.addMember("Name", sName);
	Writer.addMember("Label", sLabel);
	Writer.addMember("Value", dValue);
	Writer.addMember("Data Type", sType);
	Writer.addMember("Texture", sTexture);
	Writer.finishObject();

}

void DzBridgeAction::writeDTUHeader(DzJsonWriter& writer)
{
	QString sAssetId = "";
	QString sContentType = QString("Unknown");
	QString sImportName = "";

	if (m_pSelectedNode)
	{
		sAssetId = m_pSelectedNode->getAssetId();
		sImportName = m_pSelectedNode->getName();
		DzPresentation* presentation = m_pSelectedNode->getPresentation();
		if (presentation)
		{
			sContentType = presentation->getType();
		}
	}

	writer.addMember("DTU Version", 4);
	writer.addMember("Asset Name", m_sAssetName);
	writer.addMember("Import Name", sImportName); // Blender Compatibility
	writer.addMember("Asset Type", m_sAssetType);
	writer.addMember("Use Experimental Animation Transfer", m_bAnimationUseExperimentalTransfer);
	writer.addMember("Asset Id", sAssetId); // Unity Compatibility
	writer.addMember("Content Type", sContentType);
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
void DzBridgeAction::writeAllMaterials(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCSVstream, bool bRecursive)
{
	if (Node == nullptr)
		return;

	if (!bRecursive)
	{
		if (m_nNonInteractiveMode != eNonInteractiveMode::ScriptMode) {
			m_mapProcessedFiles.clear();
		}
		Writer.startMemberArray("Materials", true);
	}

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
				startMaterialBlock(Node, Writer, pCSVstream, Material);
				while (propertyList.hasNext())
				{
					writeMaterialProperty(Node, Writer, pCSVstream, Material, propertyList.next());
				}
				finishMaterialBlock(Writer);
			}
		}
	}

	// Check if Node is a geograft, add extra material block with parent's header info
	if (isGeograft(Node))
	{
		// get parent node
		DzNode* ParentNode = Node->getNodeParent();
		if (ParentNode)
		{
			for (int i = 0; i < Shape->getNumMaterials(); i++)
			{
				DzMaterial* Material = Shape->getMaterial(i);
				if (Material)
				{
					auto propertyList = Material->propertyListIterator();
					// Custom Header
					startMaterialBlock(ParentNode, Writer, pCSVstream, Material);
					while (propertyList.hasNext())
					{
						writeMaterialProperty(ParentNode, Writer, pCSVstream, Material, propertyList.next());
					}
					finishMaterialBlock(Writer);
				}
			}
		}
	}

	DzNodeListIterator Iterator = Node->nodeChildrenIterator();
	while (Iterator.hasNext())
	{
		DzNode* Child = Iterator.next();
		writeAllMaterials(Child, Writer, pCSVstream, true);
	}

	if (!bRecursive)
	{
		Writer.finishArray();
	}

}

void DzBridgeAction::startMaterialBlock(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCSVstream, DzMaterial* Material)
{
	if (Node == nullptr || Material == nullptr)
		return;

	Writer.startObject(true);
	Writer.addMember("Version", 4);
//	Writer.addMember("Asset Name", Node->getLabel());
	Writer.addMember("Asset Name", Node->getName());
	Writer.addMember("Asset Label", Node->getLabel());
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
		Writer.addMember("Label", QString("Asset Type"));
		Writer.addMember("Value", presentationType);
		Writer.addMember("Data Type", QString("String"));
		Writer.addMember("Texture", QString(""));
		Writer.finishObject();

		if (m_bExportMaterialPropertiesCSV && pCSVstream)
		{
			*pCSVstream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << "Asset Type" << ", " << presentationType << ", " << "String" << ", " << "" << endl;
		}
	}
}

void DzBridgeAction::finishMaterialBlock(DzJsonWriter& Writer)
{
	// replace with Section Stack
	Writer.finishArray();
	Writer.finishObject();

}

void DzBridgeAction::writeMaterialProperty(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCSVstream, DzMaterial* Material, DzProperty* Property)
{
	if (Node == nullptr || Material == nullptr || Property == nullptr)
		return;

	QString Name = Property->getName();
	QString sLabel = Property->getLabel();
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
	// Check for Override Image Map
	//if (m_overrideTable_MaterialImageMaps.contains(Property))
	//{
	//	TextureName = m_overrideTable_MaterialImageMaps[Property];
	//}
	if (m_overrideTable_MaterialProperties.contains(Property))
	{
		MaterialOverride overrideData = m_overrideTable_MaterialProperties[Property];
		if (overrideData.bHasFilename) TextureName = overrideData.filename;
		if (overrideData.bHasColor) dtuPropValue = overrideData.color.name();
		if (overrideData.bHasNumericValue) dtuPropNumericValue = overrideData.numericValue;
	}

	QString dtuTextureName = TextureName;
	if (!TextureName.isEmpty() && m_mapProcessedFiles.contains(TextureName.toLower()) == false)
	{
		dtuTextureName = scaleAndReEncodeMaterialProperties(Node, Material, Property);
	}
	else if (!TextureName.isEmpty())
	{
		dtuTextureName = m_mapProcessedFiles.value(TextureName.toLower());
	}

	if (bUseNumeric)
		writePropertyTexture(Writer, Name, sLabel, dtuPropNumericValue, dtuPropType, dtuTextureName);
	else
		writePropertyTexture(Writer, Name, sLabel, dtuPropValue, dtuPropType, dtuTextureName);

	if (m_bExportMaterialPropertiesCSV && pCSVstream)
	{
		if (bUseNumeric)
			*pCSVstream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << Name << ", " << dtuPropNumericValue << ", " << dtuPropType << ", " << TextureName << endl;
		else
			*pCSVstream << "2, " << Node->getLabel() << ", " << Material->getName() << ", " << Material->getMaterialName() << ", " << Name << ", " << dtuPropValue << ", " << dtuPropType << ", " << TextureName << endl;
	}
	return;

}

QStringList DzBridgeAction::checkForMorphOnChild(DzNode* pNode, QString sMorphName, QStringList& controlledMeshList)
{
	for (auto childIter = pNode->nodeChildrenIterator(); childIter.hasNext(); )
	{
		DzFigure* childFigure = qobject_cast<DzFigure*>(childIter.next());
		if (childFigure != nullptr)
		{
			DzObject *oObject = childFigure->getObject();
			if (oObject != nullptr)
			{
				DzModifier *oModifier = oObject->findModifier( sMorphName );
				if (oModifier != nullptr)
				{
					QString meshName = childFigure->getName() + ".Shape";
					if (controlledMeshList.contains(meshName) == false)
						controlledMeshList.append(meshName);
				}
			}
		}
	}
	return controlledMeshList;
}

QStringList DzBridgeAction::checkForBoneInChild(DzNode* pNode, QString sBoneName, QStringList &controlledMeshList)
{
	for (auto childIter = pNode->nodeChildrenIterator(); childIter.hasNext(); )
	{
		DzFigure *childFigure = qobject_cast<DzFigure*>(childIter.next());
		if (childFigure)
		{
			DzSkeleton *childSkeleton = childFigure->getSkeleton();
			DzBone *childBone = childSkeleton->findBone( sBoneName );
			if (childBone == nullptr)
				continue;

			DzSkinBinding *skinBinding = DzSkinBinding::findSkin(childFigure);
			if (skinBinding)
			{
				DzBoneBinding *boneBinding = skinBinding->findBoneBinding( sBoneName );
				if (boneBinding)
				{
					QString meshName = childFigure->getName() + ".Shape";
					if (controlledMeshList.contains(meshName) == false)
						controlledMeshList.append(meshName);
				}
			}
		}
	}
	return controlledMeshList;
}

QStringList DzBridgeAction::checkForBoneInAlias(DzNode* pNode, DzProperty* pMorphProperty, QStringList& controlledMeshList)
{
	for (int i = 0; i < pMorphProperty->getNumAliases(); i++)
	{
		DzProperty* propertyAlias = pMorphProperty->getAlias(i);
		DzBone* boneAlias = qobject_cast<DzBone*>(propertyAlias->getOwner());
		QString sBoneName;
		if (boneAlias)
		{
			sBoneName = boneAlias->getName();
			controlledMeshList = checkForBoneInChild(pNode, sBoneName, controlledMeshList);
		}
	}
	return controlledMeshList;
}

QStringList DzBridgeAction::checkMorphControlsChildren(DzNode* pNode, DzProperty* pMorphProperty)
{
	QString sBoneName;
	QStringList controlledMeshList;

	if (pMorphProperty == nullptr) return controlledMeshList;

	controlledMeshList.append(pNode->getName() + ".Shape");
	//int numSlaveControllers = pMorphProperty->getNumSlaveControllers();
	for(auto iter = pMorphProperty->slaveControllerListIterator(); iter.hasNext(); )
	{
		DzController *slaveController = iter.next();
		if (slaveController)
		{
			DzProperty *ownerProperty = slaveController->getOwner();
			if (ownerProperty)
			{
				DzElement* ownerElement = ownerProperty->getOwner();
				// 2025-05-15, DB: bugfix for modified rig/skeleton causing invalid JCM data -> ownerProperty->getOwner() == NULL
				if (ownerElement && ownerElement->inherits("DzBone"))
				{
					sBoneName = ownerElement->getName();
					controlledMeshList = checkForBoneInChild( pNode, sBoneName, controlledMeshList );
					break;
				}
			}
		}
	}
	if (sBoneName.isEmpty())
	{
		controlledMeshList = checkForBoneInAlias( pNode, pMorphProperty, controlledMeshList );
	}

	return controlledMeshList;
}

void DzBridgeAction::writeMorphLinks(DzJsonWriter& writer)
{
	writer.startMemberObject("MorphLinks");

	if (m_bEnableMorphs)
	{
		// iterate through each exported morph
		foreach (QString morphName, m_MorphNamesToExport)
		{
			MorphInfo morphInfo = m_AvailableMorphsTable[morphName];
			QString sMorphName = morphInfo.Name;
			QString sMorphLabel = morphInfo.Label;

			DzProperty *morphProperty = morphInfo.Property;
			if (morphProperty == nullptr) continue; // The added dq2lb morphs won't have properties yet
			QStringList controlledMeshList = checkMorphControlsChildren(m_pSelectedNode, morphProperty);
			if (morphInfo.Node != nullptr)
			{
				QString meshName = morphInfo.Node->getName() + ".Shape";
				if (controlledMeshList.contains(meshName) == false)
					controlledMeshList.push_back(meshName);
			}
			controlledMeshList = checkForMorphOnChild(m_pSelectedNode, morphInfo.Name, controlledMeshList);

			writer.startMemberObject(sMorphName);

			writer.addMember("Label", sMorphLabel);

			// DB 2022-June-6: Blender JCM, Morph Controllers support
			writer.startMemberArray("Links");
			for (auto iterator = morphProperty->controllerListIterator(); iterator.hasNext(); )
			{
				DzERCLink *ercLink = qobject_cast<DzERCLink*>(iterator.next());
				if (ercLink == nullptr)
					continue;
				auto controllerProperty = ercLink->getProperty();
				auto controllerOwner = controllerProperty->getOwner();
				QString sLinkBone = "None";
				QString sLinkProperty = controllerProperty->getName();
				int iLinkType = ercLink->getType();
				double fLinkScalar = ercLink->getScalar();
				double fLinkAddend = ercLink->getAddend();
				if (controllerOwner->inherits("DzBone"))
				{
					sLinkBone = controllerOwner->getName();
					controlledMeshList = checkForBoneInChild(m_pSelectedNode, sLinkBone, controlledMeshList);
				}
				if (controllerOwner->inherits("DzMorph"))
				{
					sLinkProperty = controllerOwner->getName();
				}
				writer.startObject();
				writer.addMember("Bone", sLinkBone);
				// DB 2025-May-2: Bugfix of April Bugfix: Remove any MORPH_EXPORT_PREFIX ("export____")
				sLinkProperty = QString(sLinkProperty).replace(MORPH_EXPORT_PREFIX, "");
				writer.addMember("Property", sLinkProperty);
				writer.addMember("Type", iLinkType);
				writer.addMember("Scalar", fLinkScalar);
				writer.addMember("Addend", fLinkAddend);
				if (iLinkType == 6)
				{
					// Keys
					int iKeyType = ercLink->getKeyInterpolation();
					writer.addMember("Key Type", iKeyType);
					writer.startMemberObject("Keys");
					for (int key_index = 0; key_index < ercLink->getNumKeyValues(); key_index++)
					{
						QString sKeyDataLabel = QString("Key %1").arg(key_index);
						double fKeyDataRotate = ercLink->getKey(key_index);
						double fKeyDataValue = ercLink->getKeyValue(key_index);
						writer.startMemberObject(sKeyDataLabel);
						writer.addMember("Rotate", fKeyDataRotate);
						writer.addMember("Value", fKeyDataValue);
						writer.finishObject();
					}
					writer.finishObject();
				}
				writer.finishObject();
			}
			writer.finishArray();

			writer.startMemberArray("SubLinks");
			for (auto iterator = morphProperty->slaveControllerListIterator(); iterator.hasNext(); )
			{
				DzERCLink *ercLink = qobject_cast<DzERCLink*>(iterator.next());
				if (ercLink == nullptr)
					continue;
				auto ercOwnerNode = ercLink->getOwner();
				if (ercOwnerNode == nullptr)
					continue;
				auto ercBoneNode = ercOwnerNode->getOwner();
				if (ercBoneNode && ercBoneNode->inherits("DzBone"))
				{
					QString sLinkBone = ercBoneNode->getName();
					QString sLinkProperty = ercOwnerNode->getName();
					int iLinkType = ercLink->getType();
					double fLinkScalar = ercLink->getScalar();
					double fLinkAddend = ercLink->getAddend();
					writer.startObject();
					writer.addMember("Bone", sLinkBone);
					// DB 2025-May-2: Bugfix of April Bugfix: Remove any MORPH_EXPORT_PREFIX ("export____")
					sLinkProperty = QString(sLinkProperty).replace(MORPH_EXPORT_PREFIX, "");
					writer.addMember("Property", sLinkProperty);
					writer.addMember("Type", iLinkType);
					writer.addMember("Scalar", fLinkScalar);
					writer.addMember("Addend", fLinkAddend);
					writer.finishObject();
				}
			}
			writer.finishArray();

			double minVal = 0.0;
			double maxVal = 1.0;
			if (morphProperty->inherits("DzFloatProperty"))
			{
				minVal = qobject_cast<DzFloatProperty*>(morphProperty)->getMin();
				maxVal = qobject_cast<DzFloatProperty*>(morphProperty)->getMax();
			}
			writer.addMember("Minimum", minVal);
			writer.addMember("Maximum", maxVal);
			bool bIsHidden = morphProperty->isHidden();
			writer.addMember("isHidden", bIsHidden);
			QString sMorphPath = morphInfo.Path;
			writer.addMember("Path", sMorphPath);
			writer.startMemberArray("Controlled Meshes");
			foreach(QString meshname, controlledMeshList)
			{
				writer.addItem(meshname);
			}
			writer.finishArray();

			writer.finishObject();
		}

	}

	writer.finishObject();
}

void DzBridgeAction::writeMorphNames(DzJsonWriter& writer)
{
	writer.startMemberArray("MorphNames");
	if (m_bEnableMorphs)
	{
		// iterate through each exported morph
//		for (QMap<QString, QString>::iterator morphNameToLabel = m_mMorphNameToLabel.begin(); morphNameToLabel != m_mMorphNameToLabel.end(); ++morphNameToLabel)
//		{
//			QString sMorphName = morphNameToLabel.key();
//			writer.addItem(sMorphName);
//		}
		foreach (QString morphName, m_MorphNamesToExport)
		{
			writer.addItem(morphName);
		}

	}
	writer.finishArray();
}

void DzBridgeAction::writeAllMorphs(DzJsonWriter& writer)
{
	writer.startMemberArray("Morphs", true);
	if (m_bEnableMorphs)
	{
//		for (QMap<QString, QString>::iterator i = m_mMorphNameToLabel.begin(); i != m_mMorphNameToLabel.end(); ++i)
		foreach (QString morphName, m_MorphNamesToExport)
		{
			MorphInfo morphInfo = m_AvailableMorphsTable[morphName];
			QString sMorphName = morphInfo.Name;
			QString sMorphLabel = morphInfo.Label;
			//writeMorphProperties(writer, i.key(), i.value());
			writeMorphProperties(writer, sMorphName, sMorphLabel);
		}
	}
	writer.finishArray();

	if (m_bEnableMorphs)
	{
		if (m_bEnableAutoJcm)
		{
			writer.startMemberArray("JointLinks", true);
			QList<JointLinkInfo> JointLinks = MorphTools::GetActiveJointControlledMorphs( m_pSelectedNode );
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
	// DB 2025-May-2: Bugfix of April Bugfix: Remove any MORPH_EXPORT_PREFIX ("export____")
	QString sCorrectedMorphName = linkInfo.MorphName;
	sCorrectedMorphName = QString(sCorrectedMorphName).replace(MORPH_EXPORT_PREFIX, "");
	writer.addMember("Morph", sCorrectedMorphName);
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

void DzBridgeAction::writeAllDforceInfo(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCSVstream, bool bRecursive)
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
		writeAllDforceInfo(Child, Writer, pCSVstream, true);
	}

	if (!bRecursive)
	{
		if (m_sAssetType == "SkeletalMesh")
		{
			bool ExportDForce = false;
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
	for (QList<QString>::iterator it = m_aPoseList.begin(); it != m_aPoseList.end(); ++it)
	{
		writer.startObject(true);
		writer.addMember("Name", *it);
//		writer.addMember("Label", m_mMorphNameToLabel[*i]);
		writer.addMember("Label", m_AvailableMorphsTable[*it].Label);
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
	DzGroupNode* GroupNode = qobject_cast<DzGroupNode*>(Node);
	DzInstanceNode* InstanceNode = qobject_cast<DzInstanceNode*>(Node);

	if (Bone == nullptr && Geometry)
	{
		ExportedGeometry.append(Geometry);
		ParentID = writeInstance(Node, Writer, ParentID);
	}

	if (GroupNode)
	{
		ParentID = writeInstance(Node, Writer, ParentID);
	}

	if (InstanceNode)
	{
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
	QString AssetID = Node->getAssetUri().getId();
	QString Name = AssetID.remove(QRegExp("[^A-Za-z0-9_]"));
	QUuid Uid = QUuid::createUuid();

	// Group Node needs an empty InstanceAsset
	DzGroupNode* GroupNode = qobject_cast<DzGroupNode*>(Node);
	if (GroupNode)
	{
		Name = "";
	}

	// Instance Node needs an InstanceAsset
	DzInstanceNode* InstanceNode = qobject_cast<DzInstanceNode*>(Node);
	if (InstanceNode && InstanceNode->getTarget())
	{
		AssetID = InstanceNode->getTarget()->getAssetUri().getId();
		Name = AssetID.remove(QRegExp("[^A-Za-z0-9_]"));
	}

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

bool DzBridgeAction::readGui(DzBridgeDialog* BridgeDialog)
{
	if (BridgeDialog == nullptr)
		return false;

	if (m_subdivisionDialog == nullptr)
	{
		m_subdivisionDialog = DzBridgeSubdivisionDialog::Get(BridgeDialog);
	}
	if (m_morphSelectionDialog == nullptr)
	{
		m_morphSelectionDialog = DzBridgeMorphSelectionDialog::Get(BridgeDialog);
	}
	m_morphSelectionDialog->PrepareDialog();

	// Collect the values from the dialog fields
	if (m_sAssetName == "" || isInteractiveMode() ) m_sAssetName = BridgeDialog->getAssetNameEdit()->text();
	if (m_sExportFilename == "" || isInteractiveMode() ) m_sExportFilename = m_sAssetName;
	if (m_sRootFolder == "" || isInteractiveMode() ) m_sRootFolder = readGuiRootFolder();
	if (m_sExportSubfolder == "" || isInteractiveMode() ) m_sExportSubfolder = m_sExportFilename;
	m_sDestinationPath = m_sRootFolder + "/" + m_sExportSubfolder + "/";
	if (m_sExportFbx == "" || isInteractiveMode() ) m_sExportFbx = m_sExportFilename;
	m_sDestinationFBX = m_sDestinationPath + m_sExportFbx + ".fbx";

	if ( isInteractiveMode() )
	{
		// TODO: consider removing once findData( ) method above is completely implemented
		//m_sAssetType = cleanString(BridgeDialog->getAssetTypeCombo()->currentText());
		// TEMP WORKAROUND FOR NEW ASSET TYPES
		QComboBox* wAssetCombo = BridgeDialog->getAssetTypeCombo();
		int nIndex = wAssetCombo->currentIndex();
		QVariant varItemData = wAssetCombo->itemData(nIndex);
		if (varItemData.isNull())
		{
			m_sAssetType = cleanString(wAssetCombo->currentText());
		}
		else if (varItemData.type() == QVariant::Type::String)
		{
			m_sAssetType = wAssetCombo->itemData(nIndex).toString();
		}
		else if (varItemData.type() == QVariant::Type::Int)
		{
			switch (varItemData.toInt())
			{
			case EAssetType::SkeletalMesh:
				m_sAssetType = "SkeletalMesh";
				break;

			case EAssetType::StaticMesh:
				m_sAssetType = "StaticMesh";
				break;

			case EAssetType::Scene:
				m_sAssetType = "Environment";
				break;

			case EAssetType::Animation:
				m_sAssetType = "Animation";
				break;

			case EAssetType::Pose:
				m_sAssetType = "Pose";
				break;

			}
		}

		m_bEnableMorphs = BridgeDialog->getMorphsEnabledCheckBox()->isChecked();

		// DB, 2023-11-08: Morph Selection Overhaul: Defer generation of Morph Selection Rule until time of export (after preprocessing stage)
//		m_sMorphSelectionRule = BridgeDialog->GetMorphString();

//		resetArray_ControllersToDisconnect();
//		m_ControllersToDisconnect.append(m_morphSelectionDialog->getMorphNamesToDisconnectList());
//		m_mMorphNameToLabel = BridgeDialog->GetMorphMappingFromMorphSelectionDialog();
		m_aPoseExportList = BridgeDialog->GetPoseList();
	}

	m_EnableSubdivisions = BridgeDialog->getSubdivisionEnabledCheckBox()->isChecked();
	m_bShowFbxOptions = BridgeDialog->getShowFbxDialogCheckBox()->isChecked();
	m_sFbxVersion = BridgeDialog->getFbxVersionCombo()->currentText();
	m_bGenerateNormalMaps = BridgeDialog->getEnableNormalMapGenerationCheckBox()->isChecked();

	m_bAnimationUseExperimentalTransfer = BridgeDialog->getExperimentalAnimationExportCheckBox()->isChecked();
	m_bAnimationBake = BridgeDialog->getBakeAnimationExportCheckBox()->isChecked();
	m_bAnimationTransferFace = BridgeDialog->getFaceAnimationExportCheckBox()->isChecked();
	m_bAnimationExportActiveCurves = BridgeDialog->getAnimationExportActiveCurvesCheckBox()->isChecked();
	m_bAnimationApplyBoneScale = BridgeDialog->getAnimationApplyBoneScaleCheckBox()->isChecked();

	m_bMorphLockBoneTranslation = BridgeDialog->getMorphLockBoneTranslationCheckBox()->isChecked();
	m_bEnableAutoJcm = BridgeDialog->getAutoJCM();
	m_bEnableFakeDualQuat = BridgeDialog->getFakeDualQuat();
	m_bAllowMorphDoubleDipping = BridgeDialog->getAllowMorphDoubleDipping();

	/////////////////////////////////////////////
	//// POPULATE m_MorphNamesToExport ////
	/////////////////////////////////////////////
	m_MorphNamesToExport.clear();
	m_AvailableMorphsTable.clear();
	m_AvailableMorphsTable = MorphTools::GetAvailableMorphs(m_pSelectedNode, true);
	QList<QString> aUnfinalizedMorphNamesToExport = m_morphSelectionDialog->GetMorphNamesToExport();
	QList<QString> aFinalizedMorphList = MorphTools::getFinalizedMorphList(aUnfinalizedMorphNamesToExport, m_AvailableMorphsTable, m_bEnableAutoJcm);
	m_MorphNamesToExport = aFinalizedMorphList;

	//////////////////////////////////////////////////
	//// POPULATE m_ControllersToDisconnect ////
	//////////////////////////////////////////////////
	resetArray_ControllersToDisconnect();
	if (m_bAllowMorphDoubleDipping == false) {
		m_ControllersToDisconnect.append(MorphTools::GetMorphNamesToDisconnectList(aFinalizedMorphList, m_pSelectedNode));
	}

	// LOD settings
	m_bEnableLodGeneration = BridgeDialog->getEnableLodCheckBox()->isChecked();

	// DB 2024-11-07: NOTE: The below codeblock will not run for Scripts or FileExportRunSilent pathway. **This will be an issue for interactive "RunSilent" where the user may expect the previous GUI settings to be preserved.**
	// CORRECT Behavior would be to preserve GUI settings but allow override from DzFileIOSettings or scripted API;
	// IDEALLY, we need to refactor to split export function from executeAction and split settings from gui dialog
	if (isInteractiveMode())
	{
		// Texture Resizing options
		bool bSkipTextureResizing = BridgeDialog->getSkipResizeTextures();

		if (bSkipTextureResizing == false)
		{
			bool bEnableTextureResizing = BridgeDialog->getResizeTextures();
			if (bEnableTextureResizing)
			{
				int maxTextureFileSize = BridgeDialog->getMaxTextureFileSize();
				if (maxTextureFileSize > 0) {
					m_nFileSizeThresholdToInitiateRecompression = maxTextureFileSize * 1024;
					m_bRecompressIfFileSizeTooBig = true;
				}
				else {
					m_bRecompressIfFileSizeTooBig = false;
				}
				int maxDimension = BridgeDialog->getMaxTextureResolution();
				if (maxDimension != -1) {
					m_qTargetTextureSize = QSize(maxDimension, maxDimension);
					m_bResizeTextures = true;
				}
				else {
					m_bResizeTextures = false;
				}
				QString formatString = BridgeDialog->getExportTextureFileFormat();
				if (formatString == "any") {
					m_bConvertToJpg = true;
					m_bConvertToPng = true;
					m_bForceReEncoding = false;
				}
				else if (formatString == "png+jpg") {
					m_bConvertToJpg = true;
					m_bConvertToPng = true;
					m_bForceReEncoding = true;
				}
				else if (formatString == "png") {
					m_bConvertToJpg = false;
					m_bConvertToPng = true;
					m_bForceReEncoding = true;
				}
				else if (formatString == "jpg") {
					m_bConvertToJpg = true;
					m_bConvertToPng = false;
					m_bForceReEncoding = true;
				}
			}
			else
			{
				m_bRecompressIfFileSizeTooBig = false;
				m_bResizeTextures = false;
	//			m_bConvertToJpg = true;
	//			m_bConvertToPng = true;
				m_bForceReEncoding = false;
			}

		}

		// Texture Baking options
		if (BridgeDialog->getIsBakeEnabled())
		{
			m_bCombineDiffuseAndAlphaMaps = BridgeDialog->getBakeAlpha();
			m_bBakeMakeupOverlay = BridgeDialog->getBakeMakeup();
			m_bMultiplyTextureValues = BridgeDialog->getBakeColorTint();
			m_bBakeTranslucency = BridgeDialog->getBakeTranslucency();
			m_bBakeSpecularToMetallic = BridgeDialog->getBakeSpecularToMetallic();
			m_bBakeRefractionWeight = BridgeDialog->getBakeRefractionWeight();

		}

		// Object Baking options
		QString sBakeInstancesMode = BridgeDialog->getBakeInstances();
		if (sBakeInstancesMode == "always") {
			m_eBakeInstancesMode = EBakeMode::AlwaysBake;
		}
		else if (sBakeInstancesMode == "ask") {
			m_eBakeInstancesMode = EBakeMode::Ask;
		}
		else if (sBakeInstancesMode == "never") {
			m_eBakeInstancesMode = EBakeMode::NeverBake;
		}
		QString sBakePivotPointsMode = BridgeDialog->getBakePivotPoints();
		if (sBakePivotPointsMode == "always") {
			m_eBakePivotPointsMode = EBakeMode::AlwaysBake;
		}
		else if (sBakePivotPointsMode == "ask") {
			m_eBakePivotPointsMode = EBakeMode::Ask;
		}
		else if (sBakePivotPointsMode == "never") {
			m_eBakePivotPointsMode = EBakeMode::NeverBake;
		}
		QString sBakeRigidFollowNodesMode = BridgeDialog->getBakeRigidFollowNodes();
		if (sBakeRigidFollowNodesMode == "always") {
			m_eBakeRigidFollowNodesMode = EBakeMode::AlwaysBake;
		}
		else if (sBakeRigidFollowNodesMode == "ask") {
			m_eBakeRigidFollowNodesMode = EBakeMode::Ask;
		}
		else if (sBakeRigidFollowNodesMode == "never") {
			m_eBakeRigidFollowNodesMode = EBakeMode::NeverBake;
		}

	}

	return true;
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

// DB, 2023-11-02: This may be an unused method.  TODO: re-evaluate the usage of this method by other projects.
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

// DB, 2023-11-02: This may be an unused method.  TODO: re-evaluate the usage of this method by other projects.
// If is being used, then address issue on the TODO comment line below.
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
		// DB, 2023-11-02: TODO: assumes the presentation metadata presented by getType() is always correct, but this is just
		// user-facing information and can be arbitrarily defined or reassigned.  Make new method to query the actual data
		// connected to the property.
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

DzBridgeDialog* DzBridgeAction::getBridgeDialog() 
{
	if (m_bridgeDialog == nullptr)
	{
		DzMainWindow* mw = dzApp->getInterface();
		if (!mw)
		{
			return nullptr;
		}
		m_bridgeDialog = new DzBridgeDialog(mw);
		m_bridgeDialog->setBridgeActionObject(this);
	}

	return m_bridgeDialog;
}

DzBridgeSubdivisionDialog* DzBridgeAction::getSubdivisionDialog() 
{ 
	if (m_subdivisionDialog == nullptr)
	{
		m_subdivisionDialog = DzBridgeSubdivisionDialog::Get(this->getBridgeDialog());
	}

	return m_subdivisionDialog;
}

DzBridgeMorphSelectionDialog* DzBridgeAction::getMorphSelectionDialog() 
{ 
	if (m_morphSelectionDialog == nullptr)
	{
		m_morphSelectionDialog = DzBridgeMorphSelectionDialog::Get(this->getBridgeDialog());
	}

	return m_morphSelectionDialog;
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
	if (openFBX->LoadScene(baseMeshScene, baseFilePath.toUtf8().data()) == false)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, "Error",
			"An error occurred while loading the base scene...", QMessageBox::Ok);
		printf("\n\nAn error occurred while loading the base scene...");
		return false;
	}
	SubdivideFbxScene subdivider = SubdivideFbxScene(baseMeshScene, pLookupTable);
	subdivider.ProcessScene();
	FbxScene* hdMeshScene = openFBX->CreateScene("HD Mesh Scene");
	if (openFBX->LoadScene(hdMeshScene, hdFilePath.toUtf8().data()) == false)
	{
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, "Error",
			"An error occurred while loading the base scene...", QMessageBox::Ok);
		printf("\n\nAn error occurred while loading the base scene...");
		return false;
	}
	subdivider.SaveClustersToScene(hdMeshScene);
	if (openFBX->SaveScene(hdMeshScene, outFilePath.toUtf8().data()) == false)
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEV TESTING
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FixClusterTranformLinks(FbxScene* Scene, FbxNode* RootNode)
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

					// Update the rotation
					FbxDouble3 Rotation = Cluster->GetLink()->PostRotation.Get();
					Matrix.SetR(Rotation);
					Cluster->SetTransformLinkMatrix(Matrix);
				}
			}
		}
	}

	for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
	{
		FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
		FixClusterTranformLinks(Scene, ChildNode);
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
void RemovePrePostRotations(FbxNode* pNode)
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
void ReparentTwistBone(FbxNode* pNode)
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
void FindAndProcessTwistBones(FbxNode* pNode)
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


// Perform post-processing of Fbx after export
// NOTE: must be called prior to writeconfiguration, otherwise can not guarantee that it will be executed before
// import process is started in target software
bool DzBridgeAction::postProcessFbx(QString fbxFilePath)
{
	if (m_bPostProcessFbx == false)
		return false;

	OpenFBXInterface* openFBX = OpenFBXInterface::GetInterface();
	FbxScene* pScene = openFBX->CreateScene("Base Mesh Scene");
	if (openFBX->LoadScene(pScene, fbxFilePath) == false)
	{
		QString sFbxErrorMessage = tr("ERROR: DzBridge: openFBX->LoadScene(): ")
			+ QString("(File: \"%1\") ").arg(fbxFilePath)
			+ QString("[%1] %2").arg(openFBX->GetErrorCode()).arg(openFBX->GetErrorString());
		dzApp->log(sFbxErrorMessage);
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("An error occurred while processing the Fbx file:\n\n") + sFbxErrorMessage, QMessageBox::Ok);
		return false;
	}

    // Remove Morph Export Prefix from FBX
    FbxTools::removeMorphExportPrefixFromNode(pScene->GetRootNode(), MORPH_EXPORT_PREFIX);

	// Remove Extra Geograft nodes and geometry
	if (m_bRemoveDuplicateGeografts)
	{
		for (QString searchString : m_aGeografts)
		{
			auto geo = openFBX->FindGeometry(pScene, searchString + ".Shape");
			if (geo)
			{
				auto node = geo->GetNode();
				node->RemoveAllMaterials();
				pScene->RemoveGeometry(geo);
				pScene->RemoveNode(node);
				geo->Destroy();
				node->Destroy();
			}
			auto node = openFBX->FindNode(pScene, searchString);
			if (node)
			{
				pScene->RemoveNode(node);
				node->Destroy();
			}
		}
	}

	if (m_bExperimental_FbxPostProcessing)
	{
		// Find the root bone.  There should only be one bone off the scene root
		FbxNode* RootNode = pScene->GetRootNode();
		FbxNode* RootBone = nullptr;
		QString RootBoneName("");
		for (int ChildIndex = 0; ChildIndex < RootNode->GetChildCount(); ++ChildIndex)
		{
			FbxNode* ChildNode = RootNode->GetChild(ChildIndex);
			FbxNodeAttribute* Attr = ChildNode->GetNodeAttribute();
			if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				RootBone = ChildNode;
				RootBoneName = RootBone->GetName();
				RootBone->SetName("root");
				Attr->SetName("root");
				break;
			}
		}

		//// Daz characters sometimes have additional skeletons inside the character for accesories
		//if (AssetType == DazAssetType::SkeletalMesh)
		//{
		//	FDazToUnrealFbx::ParentAdditionalSkeletalMeshes(Scene);
		//}
		// Daz Studio puts the base bone rotations in a different place than Unreal expects them.
		//if (CachedSettings->FixBoneRotationsOnImport && AssetType == DazAssetType::SkeletalMesh && RootBone)
		//{
		//	FDazToUnrealFbx::RemoveBindPoses(Scene);
		//	FDazToUnrealFbx::FixClusterTranformLinks(Scene, RootBone);
		//}
		if (RootBone)
		{
			// remove pre and post rotations
			RemovePrePostRotations(RootBone);
	//		FindAndProcessTwistBones(RootBone);
	//		RemoveBindPoses(pScene);
	//		FixClusterTranformLinks(pScene, RootBone);
		}
	}

	if (openFBX->SaveScene(pScene, fbxFilePath) == false)
	{
		QString sFbxErrorMessage = tr("ERROR: DzBridge: openFBX->SaveScene(): ")
			+ QString("(File: \"%1\") ").arg(fbxFilePath)
			+ QString("[%1] %2").arg(openFBX->GetErrorCode()).arg(openFBX->GetErrorString());
		dzApp->log(sFbxErrorMessage);
		if (m_nNonInteractiveMode == 0) QMessageBox::warning(0, tr("Error"),
			tr("An error occurred while processing the Fbx file:\n\n") + sFbxErrorMessage, QMessageBox::Ok);
		return false;
	}

	return true;

}

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

void DzBridgeAction::writeSkeletonData(DzNode* Node, DzJsonWriter& writer)
{
	if (Node == nullptr)
		return;

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : nullptr;

	writer.startMemberObject("SkeletonData");

	writer.startMemberArray("skeletonScale", true);
	writer.addItem(QString("skeletonScale"));
	writer.addItem(double(1.0));
	writer.finishArray();

	writer.startMemberArray("offset", true);
	writer.addItem(QString("offset"));
	writer.addItem(double(0.0));
	writer.finishArray();

	writer.finishObject();

	return;
}

// DB 2023-July-9: bugfix for rest pose, all getValue() replaced with getDoubleDefaultValue(), etc.
DzVec3 calculatePrimaryAxis(DzBone* pBone, DzVec3 &vecEndVector, double nBoneLength)
{
	DzVec3 vecFirstAxis(0.0f, 0.0f, 0.0f);
	double nNodeScale = pBone->getScaleControl()->getDoubleDefaultValue();
	int nSign = 1;
	double nAxisScale = 1.0;
	switch (pBone->getRotationOrder()[0])
	{
	case 0:
		nSign = (vecEndVector.m_x >= 0) ? 1 : -1;
		nAxisScale = pBone->getXScaleControl()->getDoubleDefaultValue();
		vecFirstAxis.m_x = nBoneLength * nSign * nAxisScale * nNodeScale;
		break;
	case 1:
		nSign = (vecEndVector.m_y >= 0) ? 1 : -1;
		nAxisScale = pBone->getYScaleControl()->getDoubleDefaultValue();
		vecFirstAxis.m_y = nBoneLength * nSign * nAxisScale * nNodeScale;
		break;
	case 2:
		nSign = (vecEndVector.m_z >= 0) ? 1 : -1;
		nAxisScale = pBone->getZScaleControl()->getDoubleDefaultValue();
		vecFirstAxis.m_z = nBoneLength * nSign * nAxisScale * nNodeScale;
		break;
	}
	DzQuat quatOrientation = pBone->getOrientation(true);
	DzVec3 vecPrimaryAxis = quatOrientation.multVec(vecFirstAxis);

	return vecPrimaryAxis;
}

DzVec3 calculateSecondaryAxis(DzBone* pBone, DzVec3& vecEndVector)
{
	DzVec3 vecSecondAxis(0.0f, 0.0f, 0.0f);
	int nSign = 1;
	switch (pBone->getRotationOrder()[0])
	{
	case 0:
		nSign = (vecEndVector.m_x >= 0) ? 1 : -1;
		break;
	case 1:
		nSign = (vecEndVector.m_y >= 0) ? 1 : -1;
		break;
	case 2:
		nSign = (vecEndVector.m_z >= 0) ? 1 : -1;
		break;
	}
	switch (pBone->getRotationOrder()[1])
	{
	case 0:
		vecSecondAxis.m_x = nSign;
		break;
	case 1:
		vecSecondAxis.m_y = nSign;
		break;
	case 2:
		vecSecondAxis.m_z = nSign;
		break;
	}
	// DB 2023-July-9: bugfix for rest pose
	DzQuat quatOrientation = pBone->getOrientation(true);
	vecSecondAxis = quatOrientation.multVec(vecSecondAxis);

	return vecSecondAxis;
}


/**
 * \Summates ERC Y-translations to bone, iterates through all of the bones' controllers
 * \TODO: consider renaming function to calculateERCOffsetForBone()?
 * \param DzBone* pBone - bone of interest
 * \return DzVec3 vecBoneOffset - final vector summation of all ERC y-translations
 */
DzVec3 calculateBoneOffset(DzBone* pBone)
{
	DzVec3 vecBoneOffset(0.0f, 0.0f, 0.0f);
	for (auto pObject : pBone->getPropertyList())
	{
		DzProperty* property = qobject_cast<DzProperty*>(pObject);
		if (property && property->getName() == "YTranslate")
		{
			DzControllerListIterator controlIterator = property->controllerListIterator();
			while (controlIterator.hasNext())
			{
				DzController* pObject2 = controlIterator.next();
				DzERCLink* pERCLink = qobject_cast<DzERCLink*>(pObject2);
				if (pERCLink)
				{
					auto controlerProperty = pERCLink->getProperty();
					if (controlerProperty->getDoubleValue() != 0)
					{
						vecBoneOffset.m_y += pERCLink->getScalar() * controlerProperty->getDoubleValue();
					}
				}
			}
		}

	}

	return vecBoneOffset;
}

DzPropertyList getAllProperties(DzBone* pBone)
{
	DzPropertyList aPropertyList;

	DzPropertyGroupList aPropertyGroup_ToDoList;
	DzPropertyGroupTree* pGroupTree = pBone->getPropertyGroups();
	if (pGroupTree)
	{
		DzPropertyGroup* propertyGroup = pGroupTree->getFirstChild();
		if (propertyGroup)
			aPropertyGroup_ToDoList.append(propertyGroup);
	}
	// Iterative stack-based algorithm to traverse GroupTree breadth-first
	while (aPropertyGroup_ToDoList.isEmpty() == false)
	{
		DzPropertyGroup* propertyGroup = aPropertyGroup_ToDoList.takeFirst();
		if (propertyGroup)
		{
			// get all properties on this node
			DzPropertyListIterator propertyListIterator = propertyGroup->getProperties();
			while (propertyListIterator.hasNext())
			{
				DzProperty* pProperty = propertyListIterator.next();
				if (pProperty)
				{
					if (!aPropertyList.contains(pProperty))
						aPropertyList.append(pProperty);
				}
			}

			// add all sibling nodes to todo-list
			DzPropertyGroup* firstChild = nullptr;
			DzPropertyGroup* parentGroup = propertyGroup->getParent();
			if (parentGroup)
			{
				firstChild = parentGroup->getFirstChild();
			}
			else
			{
				firstChild = pGroupTree->getFirstChild();
			}
			if (firstChild == propertyGroup)
			{
				DzPropertyGroup* siblingGroup = propertyGroup->getNextSibling();
				while (siblingGroup)
				{
					if (!aPropertyGroup_ToDoList.contains(siblingGroup))
						aPropertyGroup_ToDoList.append(siblingGroup);
					siblingGroup = siblingGroup->getNextSibling();
				}
			}
			// add all child nodes to todo-list
			DzPropertyGroup* childGroup = propertyGroup->getFirstChild();
			while (childGroup)
			{
				if (!aPropertyGroup_ToDoList.contains(childGroup))
					aPropertyGroup_ToDoList.append(childGroup);
				childGroup = childGroup->getNextSibling();
			}
		}
	}

	return aPropertyList;
}

/**
 * \Write DzBone head and tail data, along with orientation.  NOTE: This is used by BlenderBridge to overwrite the bindpose
 * \so the skeleton configuration must be in in rest/zero configuration AND any baked morphs must have ERC bone values applied.
 * \param Node 
 * \param writer 
 */
void DzBridgeAction::writeHeadTailData(DzNode* Node, DzJsonWriter& writer)
{
	if (Node == nullptr)
		return;

	Node->update();
	Node->finalize();

	// get skeleton and initial bone list
	DzSkeleton* pSkeleton = Node->getSkeleton();
	if (pSkeleton == nullptr)
	{
		writer.startMemberObject("HeadTailData", true);
		writer.finishObject();
		return;
	}
	QObjectList aBoneList = pSkeleton->getAllBones();
	// Create boneName Lookup
	QMap<QString, bool> aBoneNameLookup;
	for (auto item : aBoneList)
	{
		DzBone* boneItem = qobject_cast<DzBone*>(item);
		if (boneItem)
		{
			QString sKey = boneItem->getName();
			aBoneNameLookup.insert(sKey, false);
		}
	}
	// add additional follower bones if any
	// 1. Walk through entire scene
	for (auto node : dzScene->getNodeList())
	{
		if (!node)
			continue;

		// 2. if inherits Skeleton, Check to see if it follows pSkeleton
		DzSkeleton* skeletonNode = qobject_cast<DzSkeleton*>(node);
		if (node->inherits("DzSkeleton") && skeletonNode)
		{
			// 3. if 2, Compare bones to pSkeleton to see if it is not in pSkeleton
			DzSkeleton* followTarget = skeletonNode->getFollowTarget();
			if (followTarget == pSkeleton)
			{
				// 4. If 3, Add any bones that are not already in aBoneNameLookup
				for (auto oFollowerBone : skeletonNode->getAllBones())
				{
					DzBone* boneFollowerBone = qobject_cast<DzBone*>(oFollowerBone);
					if (boneFollowerBone)
					{
						QString sFollowerBoneName = boneFollowerBone->getName();
						if (!aBoneNameLookup.contains(sFollowerBoneName))
						{
							aBoneList.append(boneFollowerBone);
							aBoneNameLookup.insert(sFollowerBoneName, false);
						}
					}
				}
			}
		}

	}

	// Calculate Bone Offset (aka ERC y-translate offset)
	DzVec3 vecBoneOffset(0, 0, 0);
	if (aBoneList.length() <= 0) {
		dzApp->log("DazBridge: ERROR: DzSkeleton for Node: " + Node->getName() + ", has no bones.  Using (0,0,0) for bone offset in HeadTailData of DTU.");
	} else {
		vecBoneOffset = calculateBoneOffset(qobject_cast<DzBone*>(aBoneList[0]));
	}

	double nSkeletonScale = pSkeleton->getScaleControl()->getValue();

	writer.startMemberObject("HeadTailData", true);

	for (auto pObject : aBoneList)
	{
		DzBone* pBone = qobject_cast<DzBone*>(pObject);
		if (pBone)
		{
			// Assign Head
			DzVec3 vecHead = pBone->getOrigin(true);
			if (Node->className() == "DzFigure")
			{
				DzFigure* figure = qobject_cast<DzFigure*>(pSkeleton);
				DzSkinBinding* skinBinding = figure->getSkinBinding();
				if (skinBinding)
				{
					DzBoneBinding* boneBinding = skinBinding->findBoneBinding(pBone);
					if (boneBinding) {
						vecHead = boneBinding->getScaledOrigin();
					}
				}
			}
			// Calculate Bone Length
			// DB 2023-July-9: bugfix for restpose
			DzVec3 vecEndVector = pBone->getEndPoint(true) - pBone->getOrigin(true);
			double nBoneLength = vecEndVector.length();
			// Calculate Primary Axis
			DzVec3 vecPrimaryAxis = calculatePrimaryAxis(pBone, vecEndVector, nBoneLength);
			// Calculate Secondary Axis
			DzVec3 vecSecondAxis = calculateSecondaryAxis(pBone, vecEndVector);
			// Calculate Tail
			DzVec3 vecTail = vecHead + vecPrimaryAxis;

			QString sBoneName = pBone->getName();
			writer.startMemberArray(sBoneName, true);
			for (int axis = 0; axis <= 2; axis++) {
				writer.addItem((vecHead[axis] + vecBoneOffset[axis]) * nSkeletonScale);
			}
			for (int axis = 0; axis <= 2; axis++) {
				writer.addItem((vecTail[axis] + vecBoneOffset[axis]) * nSkeletonScale);
			}
			for (int axis = 0; axis <= 2; axis++) {
				writer.addItem(vecSecondAxis[axis]);
			}

			// Bone Transform Values
			DzVec3 vecBonePosition(0.0f, 0.0f, 0.0f);
			DzVec3 vecBoneRotation(0.0f, 0.0f, 0.0f);
			DzVec3 vecBoneScale(0.0f, 0.0f, 0.0f);
			// get properties list
			DzPropertyList aPropertyList;
			aPropertyList = getAllProperties(pBone);

			for (auto propertyItem : aPropertyList)
			{
				auto pOwner = propertyItem->getOwner();
				if (!pOwner->inherits("DzBone"))
					continue;

				QStringList aAxisString;
				aAxisString.append("X");
				aAxisString.append("Y");
				aAxisString.append("Z");

				// Position
				for (int i = 0; i < 3; i++)
				{
					QString searchLabel = QString(aAxisString[i] + QString("Translate"));
					if (propertyItem->getName() == searchLabel && propertyItem->isHidden() == false)
					{
						QString sPropertyLabel = propertyItem->getLabel().toLower().replace(" ", "");
						searchLabel = searchLabel.toLower().replace(" ", "");
						// Only set to 1 if label doesn't not match name (except whitespace)
						// Example: name=xrotate but label=bend, twist, etc.
						if (searchLabel.startsWith(sPropertyLabel) == false)
						{
							vecBonePosition[i] = 1;
						}
					}
				}
				// Rotation
				for (int i = 0; i < 3; i++)
				{
					QString searchLabel = QString(aAxisString[i] + QString("Rotate"));
					if (propertyItem->getName() == searchLabel && propertyItem->isHidden() == false)
					{
						QString sPropertyLabel = propertyItem->getLabel().toLower().replace(" ", "");
						searchLabel = searchLabel.toLower().replace(" ", "");
						// Only set to 1 if label doesn't not match name (except whitespace)
						// Example: name=xrotate but label=bend, twist, etc.
						if (searchLabel.startsWith(sPropertyLabel) == false)
						{
							vecBoneRotation[i] = 1;
						}
					}
				}
				// Scale
			}
			for (int i = 0; i < 3; i++) {
				writer.addItem(vecBonePosition[i]);
			}
			for (int i = 0; i < 3; i++) {
				writer.addItem(vecBoneRotation[i]);
			}
			for (int i = 0; i < 3; i++) {
				writer.addItem(vecBoneScale[i]);
			}
			writer.finishArray();
		}
	}

	writer.finishObject();

	return;
}

void DzBridgeAction::writeJointOrientation(DzBoneList& aBoneList, DzJsonWriter& writer)
{
	writer.startMemberObject("JointOrientation", true);

	for (DzBone* pBone : aBoneList)
	{
		QString sBoneName = pBone->getName();
		// DB 2023-July-9: bugfix for restpose
		QString sRotationOrder = pBone->getRotationOrder().toString();
		double nXOrientation = pBone->getOrientXControl()->getDoubleDefaultValue();
		double nYOrientation = pBone->getOrientYControl()->getDoubleDefaultValue();
		double nZOrientation = pBone->getOrientZControl()->getDoubleDefaultValue();
		DzQuat quatOrientation = pBone->getOrientation(true);

		writer.startMemberArray(sBoneName, true);
		writer.addItem(sRotationOrder);
		writer.addItem(nXOrientation);
		writer.addItem(nYOrientation);
		writer.addItem(nZOrientation);
		writer.addItem(quatOrientation.m_w);
		writer.addItem(quatOrientation.m_x);
		writer.addItem(quatOrientation.m_y);
		writer.addItem(quatOrientation.m_z);
		writer.finishArray();
	}

	writer.finishObject();

	return;
}

DzBoneList DzBridgeAction::getAllBones(DzNode* Node)
{
	DzBoneList aBoneList;

	if (Node == nullptr)
		return aBoneList;

	// get skeleton and initial bone list
	DzSkeleton* pSkeleton = Node->getSkeleton();
	if (pSkeleton == nullptr)
		return aBoneList;
	QObjectList oBoneList = pSkeleton->getAllBones();
	// Create boneName Lookup
	QMap<QString, bool> aBoneNameLookup;
	for (auto item : oBoneList)
	{
		DzBone* boneItem = qobject_cast<DzBone*>(item);
		if (boneItem)
		{
			QString sKey = boneItem->getName();
			aBoneNameLookup.insert(sKey, false);
			aBoneList.append(boneItem);
		}
	}
	// add additional follower bones if any
	// 1. Walk through entire scene
	for (auto node : dzScene->getNodeList())
	{
		if (!node)
			continue;

		// 2. if inherits Skeleton, Check to see if it follows pSkeleton
		DzSkeleton* skeletonNode = qobject_cast<DzSkeleton*>(node);
		if (node->inherits("DzSkeleton") && skeletonNode)
		{
			// 3. if 2, Compare bones to pSkeleton to see if it is not in pSkeleton
			DzSkeleton* followTarget = skeletonNode->getFollowTarget();
			if (followTarget == pSkeleton)
			{
				// 4. If 3, Add any bones that are not already in aBoneNameLookup
				for (auto oFollowerBone : skeletonNode->getAllBones())
				{
					DzBone* boneFollowerBone = qobject_cast<DzBone*>(oFollowerBone);
					if (boneFollowerBone)
					{
						QString sFollowerBoneName = boneFollowerBone->getName();
						if (!aBoneNameLookup.contains(sFollowerBoneName))
						{
							aBoneList.append(boneFollowerBone);
							aBoneNameLookup.insert(sFollowerBoneName, false);
						}
					}
				}
			}
		}
	}

	return aBoneList;
}

void DzBridgeAction::writeLimitData(DzBoneList& aBoneList, DzJsonWriter& writer)
{
	writer.startMemberObject("LimitData");

	for (DzBone* pBone : aBoneList)
	{
		QString sBoneName = pBone->getName();
		QString sRotationOrder = pBone->getRotationOrder().toString();
		double nXRotationMin = pBone->getXRotControl()->getMin();
		double nXRotationMax = pBone->getXRotControl()->getMax();
		double nYRotationMin = pBone->getYRotControl()->getMin();
		double nYRotationMax = pBone->getYRotControl()->getMax();
		double nZRotationMin = pBone->getZRotControl()->getMin();
		double nZRotationMax = pBone->getZRotControl()->getMax();

		writer.startMemberArray(sBoneName, true);
		writer.addItem(sBoneName);
		writer.addItem(sRotationOrder);

		writer.addItem(nXRotationMin);
		writer.addItem(nXRotationMax);

		writer.addItem(nYRotationMin);
		writer.addItem(nYRotationMax);

		writer.addItem(nZRotationMin);
		writer.addItem(nZRotationMax);
		writer.finishArray();
	}

	writer.finishObject();

	return;
}

QString getObjectTypeAsString(DzNode* Node)
{
	if (Node == nullptr)
		return QString("EMPTY");

	if (Node->inherits("DzBone"))
		return QString("BONE");

	if (Node->inherits("DzLight"))
		return QString("LIGHT");

	if (Node->inherits("DzCamera"))
		return QString("CAMERA");

	DzObject* oObject = Node->getObject();
	if (!oObject)
		return QString("EMPTY");

	DzShape* oShape = oObject->getCurrentShape();
	if (!oShape)
		return QString("EMPTY");

	DzGeometry* oMesh = oShape->getGeometry();
	if (!oMesh)
		return QString("EMPTY");

	return QString("MESH");
}

void DzBridgeAction::writePoseData(DzNode* Node, DzJsonWriter& writer, bool bIsFigure)
{
	if (Node == nullptr)
		return;

	// Top Nodes List
	auto aTopNodes = Node->getNodeChildren(false);
	if (aTopNodes.length() == 0 && !bIsFigure)
	{
		aTopNodes.push_back(Node);
	}

	// Create Node List
	DzNodeList aNodeList;

	aNodeList.append(Node);
	for (auto item : aTopNodes)
	{
		DzNode* nodeItem = qobject_cast<DzNode*>(item);
		if (bIsFigure && !nodeItem->inherits("DzBone"))
		{
			continue;
		}
		aNodeList.append(nodeItem);
		for (auto child : nodeItem->getNodeChildren(true))
		{
			DzNode* nodeChild = qobject_cast<DzNode*>(child);
			aNodeList.append(nodeChild);
		}
	}

	writer.startMemberObject("PoseData");

	// iterate through each node in Node list
	for (DzNode* node : aNodeList)
	{
		QString sNodeName = node->getName();
		QString sLabel = node->getLabel();
		QString sObjectType = getObjectTypeAsString(node);
		QString sObjectName = "EMPTY";
		if (sObjectType == "MESH")
			sObjectName = node->getObject()->getName();
		DzTime currentTime = dzScene->getTime();
		DzVec3 vecPosition = node->getLocalPos(currentTime, false);
		DzMatrix3 matrixScale = node->getLocalScale(currentTime, false);

		writer.startMemberObject(sNodeName);
		writer.addMember("Name", sNodeName);
		writer.addMember("Label", sLabel);
		writer.addMember("Object Type", sObjectType);
		writer.addMember("Object", sObjectName);

		writer.startMemberArray("Position", true);
		writer.addItem(vecPosition.m_x);
		writer.addItem(vecPosition.m_y);
		writer.addItem(vecPosition.m_z);
		writer.finishArray();

		writer.startMemberArray("Rotation", true);
		writer.addItem(node->getXRotControl()->getLocalValue());
		writer.addItem(node->getYRotControl()->getLocalValue());
		writer.addItem(node->getZRotControl()->getLocalValue());
		writer.finishArray();

		writer.startMemberArray("Scale", true);
		writer.addItem(matrixScale[0][0]);
		writer.addItem(matrixScale[1][1]);
		writer.addItem(matrixScale[2][2]);
		writer.finishArray();

		writer.finishObject();
	}

	writer.finishObject();

	return;
}

////////////////////////////////////////
//// Additional Utility Functions translated from Blender.dsa
////////////////////////////////////////
void DzBridgeAction::ReparentFigure(DzNode* figure)
{
	QMap<DzNode*, DzNode*> reparentMapChildToParent;
	if (reparentMapChildToParent.contains(figure))
	{
		auto parent = reparentMapChildToParent[figure];
		parent->addNodeChild(figure, true);
	}

}

// WARNING: bugged execution logic, multiple redundant return paths
// If given a DzGroupNode, returns list of all child Nodes that are type Actor/Character or Actor
DzNodeList DzBridgeAction::FindRootNodes(DzNode* pNode)
{
	DzNodeList figureList;
	DzNodeList propList;

	QString sClassName = pNode->className();
	QString sContentType = dzApp->getAssetMgr()->getTypeForNode(pNode);
	if (sClassName == "DzFigure" || sClassName == "DzLegacyFigure")
	{
		if (sContentType == "Actor/Character" || sContentType == "Actor")
		{
			figureList.append(pNode);
			return figureList + propList;
		}
		else
		{
			propList.append(pNode);
			return figureList + propList;
		}
	}
	else if (sClassName == "DzGroupNode")
	{
		DzNodeList childFigureList;

		auto children = pNode->getNodeChildren(true);
		foreach (auto childrenElement, children)
		{
			DzNode* child = qobject_cast<DzNode*>(childrenElement);
			if (child)
			{
				QString sChildType = dzApp->getAssetMgr()->getTypeForNode(pNode);
				if (sChildType == "Actor/Character" || sChildType == "Actor")
				{
					childFigureList.append(child);
				}
			}
		}
/*
		foreach(auto child, childFigureList)
		{
			figureList.append(child);
			DzNode *parent = child->getNodeParent();
			if (parent == pNode)
			{
				pNode->removeNodeChild( child, true);
			}
			else
			{
				DzNode* origParent = parent;
				while (parent != nullptr)
				{
					parent->removeNodeChild(child, true);
					parent = child->getNodeParent();
				}
				parent = origParent;
			}
			QMap<DzNode*, DzNode*> reparentMapChildToParent;
			reparentMapChildToParent.insert(child, parent);
		}
*/
		if (childFigureList.length() == 0)
		{
			propList.append(pNode);
			return figureList + propList;
		}
		figureList += childFigureList;
	}
	else
	{
		propList.append(pNode);
		return figureList + propList;
	}

	return figureList + propList;
}

// WARNING:bUnhideNodes is a destructive pathway that permanently sets invisible root nodes and all its children to visible
DzNodeList DzBridgeAction::BuildRootNodeList(bool bUnhideNodes)
{
	DzNodeList rootNodeList;

	QObjectList nodeList = dzScene->getNodeList();
	foreach(auto nodeListElement, nodeList)
	{
		DzNode* node = qobject_cast<DzNode*>(nodeListElement);
		if (node)
		{
			if (bUnhideNodes)
			{
				if (!node->isVisible() && node->isRootNode())
				{
					node->setVisible(true);
					auto children = node->getNodeChildren(true);
					foreach(auto childrenElement, children)
					{
						DzNode* child = qobject_cast<DzNode*>(childrenElement);
						child->setVisible(true);
					}
				}
			}

			if (node->isVisible() && node->isRootNode())
			{
				rootNodeList += FindRootNodes( node );
			}

		}
	}

	return rootNodeList;
}

void DzBridgeAction::resetArray_ControllersToDisconnect()
{
	m_ControllersToDisconnect.clear();
	// DB 2023-11-02: Uncertain why this is hardcoded to be here, it is breaking face exports
//	m_ControllersToDisconnect.append("facs_bs_MouthClose_div2");
	m_undoTable_ControllersToDisconnect.clear();
}

bool DzBridgeAction::exportObj(QString filepath)
{
	DzExportMgr* ExportManager = dzApp->getExportMgr();
	DzExporter* Exporter = ExportManager->findExporterByClassName("DzObjExporter");

	if (Exporter)
	{
		DzFileIOSettings ExportOptions;
		ExportOptions.setStringValue("Custom", "Custom");
		ExportOptions.setFloatValue("Scale", 1.0);
		ExportOptions.setStringValue("LatAxis", "X");
		ExportOptions.setStringValue("VertAxis", "Y");
		ExportOptions.setStringValue("DepthAxis", "Z");
		ExportOptions.setBoolValue("InvertLat", false);
		ExportOptions.setBoolValue("InvertVert", false);
		ExportOptions.setBoolValue("InvertDepth", false);
		ExportOptions.setBoolValue("IgnoreInvisible", true);
		ExportOptions.setBoolValue("WeldSeams", false);
		ExportOptions.setBoolValue("RemoveUnusedVerts", true);
		ExportOptions.setBoolValue("WriteVT", false);
		ExportOptions.setBoolValue("WriteVN", false);
		ExportOptions.setBoolValue("WriteO", false);
		ExportOptions.setBoolValue("WriteG", false);
		ExportOptions.setBoolValue("GroupGeom", false);
		ExportOptions.setBoolValue("GroupNodes", false);
		ExportOptions.setBoolValue("GroupSurfaces", false);
		ExportOptions.setBoolValue("GroupSingle", false);
		ExportOptions.setBoolValue("WriteUsemtl", false);
		ExportOptions.setBoolValue("WriteMtllib", false);
		ExportOptions.setBoolValue("OriginalMaps", false);
		ExportOptions.setBoolValue("CollectMaps", false);
		ExportOptions.setBoolValue("ConvertMaps", false);
		ExportOptions.setBoolValue("SelectedOnly", false);
		ExportOptions.setBoolValue("SelectedRootsOnly", false);
		ExportOptions.setBoolValue("PrimaryRootOnly", false);
		ExportOptions.setBoolValue("IncludeParented", false);
		ExportOptions.setBoolValue("TriangulateNgons", false);
		ExportOptions.setBoolValue("CollapseUVTiles", false);
		ExportOptions.setBoolValue("ShowIndividualSettings", false);
		ExportOptions.setIntValue("FloatPrecision", 6);
		ExportOptions.setBoolValue("WriteF", true);
		ExportOptions.setIntValue("RunSilent", 1);

		Exporter->writeFile(filepath, &ExportOptions);
//		Exporter->writeFile(filepath);
	}
	Exporter->deleteLater();
	return true;
}

bool is_faced_mesh_single(DzNode* pNode)
{
	if (pNode == nullptr)
		return false;

	if (pNode->inherits("DzBone"))
		return false;

	DzObject* pObject = pNode->getObject();
	if (pObject == nullptr)
		return false;

	DzShape* pShape = pObject->getCurrentShape();
	if (pShape == nullptr)
		return false;

	DzGeometry* pMesh = pShape->getGeometry();
	if (pMesh == nullptr)
		return false;

	//pMesh->getNumFacets();
	DzFacetMesh* pMesh2 = qobject_cast<DzFacetMesh*>(pMesh);
	int num_facets = pMesh2->getNumFacets();
	if (num_facets < 1 && pMesh2->getName().toLower().contains("eyebrow"))
		return false;

	if (num_facets > 13000)
		return false;

	if (pNode->isRootNode())
		return false;

	return true;
}

bool DzBridgeAction::prepareGeograftMorphsToExport(DzNode* Node, bool bZeroMorphForExport)
{
	bool bGeograftMorphsFoundToExport = false;
	QList<DzNode*> aGeograftNodes;
	DzNode* pParentNode = Node;
	// 1. find geograft/genital
	DzNodeListIterator oNodeChildrenIter = Node->nodeChildrenIterator();
	while (oNodeChildrenIter.hasNext())
	{
		DzNode* pChild = oNodeChildrenIter.next();
		if (isGeograft(pChild))
		{
//			pGeograftNode = pChild;
			aGeograftNodes.append(pChild);
//			break;
		}
	}
	if (aGeograftNodes.count() == 0)
	{
		return false;
	}
	DzNumericProperty* previousProperty = nullptr;
	for (DzNode* pGeograftNode : aGeograftNodes)
	{
		for (int index = 0; index < pGeograftNode->getNumProperties(); index++)
		{
			DzProperty* property = pGeograftNode->getProperty(index);
			DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
			if (numericProperty && !numericProperty->isOverridingControllers())
			{
				QString propName = property->getName();
//				if (m_mMorphNameToLabel.contains(propName))
				if (m_MorphNamesToExport.contains(propName))
				{
					if (bZeroMorphForExport)
					{
						numericProperty->setDoubleValue(0);
					}
					bGeograftMorphsFoundToExport = true;
				}
			}
		}
		DzObject* Object = pGeograftNode->getObject();
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
						DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
						if (numericProperty && !numericProperty->isOverridingControllers())
						{
							QString propName = MorphTools::GetMorphPropertyName(property);
//							if (m_mMorphNameToLabel.contains(modifier->getName()))
							if (m_MorphNamesToExport.contains(modifier->getName()))
							{
								if (bZeroMorphForExport)
								{
									numericProperty->setDoubleValue(0);
								}
								bGeograftMorphsFoundToExport = true;
							}
						}
					}
				}
			}
		}

	}

	return bGeograftMorphsFoundToExport;
}

bool DzBridgeAction::exportGeograftMorphs(DzNode *Node, QString sDestinationFolder)
{
	DzNode *pGeograftNode = nullptr;
	DzNode *pParentNode = Node;
	// 1. find geograft/genital
	DzNodeListIterator oNodeChildrenIter = Node->nodeChildrenIterator();
	while (oNodeChildrenIter.hasNext())
	{
		DzNode* pChild = oNodeChildrenIter.next();
		if (isGeograft(pChild))
		{
			pGeograftNode = pChild;
			break;
		}
	}
	if (pGeograftNode == nullptr)
	{
		return false;
	}
	// Hide everything
	QList<QPair<DzNode*, bool>> oUndoList_for_NodeVisibility;
	DzNodeListIterator oSceneNodeIterator = dzScene->nodeListIterator();
	while (oSceneNodeIterator.hasNext())
	{
		DzNode* pNode = oSceneNodeIterator.next();
		oUndoList_for_NodeVisibility.append(QPair<DzNode*, bool>(pNode, pNode->isVisible()));
		if (pNode == pParentNode || pNode == pGeograftNode)
		{
			pNode->setVisible(true);
		}
		else if (pNode->getNodeParent() == pParentNode)
		{
			pNode->setVisible(false);
		}
		else
		{
			if (is_faced_mesh_single(pNode) != false)
				pNode->setVisible(false);
		}
	}
//	pGeograftNode->setVisible(true);
//	pParentNode->setVisible(true);

	// set all morphs to zero
	// add morphs to todo list for exporting
	QList<QPair<QString, DzNumericProperty*>> oGeograftMorphsToExport;
	DzNumericProperty* previousProperty = nullptr;
	for (int index = 0; index < pGeograftNode->getNumProperties(); index++)
	{
		DzProperty* property = pGeograftNode->getProperty(index);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
		if (numericProperty && !numericProperty->isOverridingControllers())
		{
			QString propName = property->getName();
			if (m_MorphNamesToExport.contains(propName))
			{
				oGeograftMorphsToExport.append(QPair<QString, DzNumericProperty*>(propName, numericProperty));
				double propValue = numericProperty->getDoubleValue();
				numericProperty->setDoubleValue(0);
			}
		}
	}
	DzObject* Object = pGeograftNode->getObject();
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
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					if (numericProperty && !numericProperty->isOverridingControllers())
					{
						QString propName = MorphTools::GetMorphPropertyName(property);
						if (m_MorphNamesToExport.contains(modifier->getName()) )
						{
							oGeograftMorphsToExport.append(QPair<QString, DzNumericProperty*>(modifier->getName(), numericProperty));
							double propValue = numericProperty->getDoubleValue();
							numericProperty->setDoubleValue(0);
						}
					}
				}
			}
		}
	}
	// for each morph,
	foreach(auto morphPair, oGeograftMorphsToExport)
	{
		QString sMorphName = morphPair.first;
		DzNumericProperty* pNumericProperty = morphPair.second;
		QString sMorphObjFileName = sMorphName + ".obj";
		QString sObjFullPath = sDestinationFolder + "/" + sMorphObjFileName;
		pNumericProperty->setDoubleValue(1.0);
		exportObj(sObjFullPath);
		pNumericProperty->setDoubleValue(0);
	}

	// Reverse Visibility changes
	foreach(auto UndoPair, oUndoList_for_NodeVisibility)
	{
		DzNode *pNode = UndoPair.first;
		bool bIsVisible = UndoPair.second;
		pNode->setVisible(bIsVisible);
	}

	return true;
}

bool DzBridgeAction::isGeograft(const DzNode* pNode)
{
	if (pNode->inherits("DzFigure"))
	{
		const DzFigure* figure = dynamic_cast<const DzFigure*>(pNode);
		if (figure->isGraftFollowing())
		{
			DzSkeleton* target = figure->getFollowTarget();
			if (target && target->isVisible() && target->isVisibileInRender())
			{
//				dzApp->log("DazBridge: DEBUG: skipping geograft node: " + pNode->getName());
				return true;
			}
		}
	}
	return false;
}

void DzBridgeAction::writeAllLodSettings(DzJsonWriter& writer)
{
	// Start LOD subsection
	writer.startMemberObject("LOD Settings");

	// Global LOD settings
	writer.addMember("Generate LODs", m_bEnableLodGeneration);
	int lod_method = (int)getLodMethod();
	writer.addMember("LOD Method", lod_method);
	writer.addMember("Number of LODs", m_nNumberOfLods);
	writer.addMember("Use LODGroup", m_bCreateLodGroup);

	// LOD Info array
	writer.startMemberArray("LOD Info Array", true);
	foreach(LodInfo * lodinfo, m_aLodInfo)
	{
		if (lodinfo)
		{
			writer.startObject(true);
			writer.addMember("Quality Vertex", lodinfo->quality_vertex);
			writer.addMember("Quality Percent", lodinfo->quality_percent);
			writer.addMember("Threshold Screen Height", lodinfo->threshold_screen_height);
			writer.finishObject();
		}
	}
	writer.finishArray();

	writer.finishObject();

}

void DzBridgeAction::setLodMethod(int arg)
{
	if (
		(arg >= getELodMethodMin() ) && 
		(arg <= getELodMethodMax() )
		)
	{
		m_eLodMethod = (ELodMethod) arg;
	}
	else
	{
		dzApp->warning("ERROR: DzBridgeAction::setLodMethod(): index out of range.");
	}
}

QString DzBridgeAction::getLodMethodString()
{
	switch (m_eLodMethod)
	{
	case ELodMethod::PreGenerated:
		return QString("Pregenerated");
	case ELodMethod::Decimator:
		return QString("Decimator");

	default:
	case ELodMethod::Undefined:
		return QString("Undefined");
	}
}

void DzBridgeAction::setLodMethod(QString arg)
{
	if (
		(arg.toLower() == "pregenerated") ||
		(arg.toLower() == "pre-generated")
		)
	{
		m_eLodMethod = ELodMethod::PreGenerated;
	}
	else if (arg.toLower().contains("decimator"))
	{
		m_eLodMethod = ELodMethod::Decimator;
	}
	else if (arg.toLower() == "undefined")
	{
		m_eLodMethod = ELodMethod::Undefined;
	}

	return;
}

bool DzBridgeAction::combineDiffuseAndAlphaMaps(DzMaterial* Material)
{
	DiffuseAndAlphaMapsUndoData undoData;

	if (!Material) return false;

	if (Material)
	{
		// DB 2023-Oct-5: Analyze Material, combine diffuse and alpha (cutout)
		bool bHasCutout = false;
		DzProperty* cutoutProp = Material->findProperty("Cutout Opacity");
		DzProperty* opacityStrengthProp = Material->findProperty("Opacity Strength");
		DzImageProperty* imageProp = qobject_cast<DzImageProperty*>(cutoutProp);
		DzNumericProperty* numericProp = qobject_cast<DzNumericProperty*>(cutoutProp);
		if (!cutoutProp && opacityStrengthProp)
		{
			imageProp = qobject_cast<DzImageProperty*>(opacityStrengthProp);
			numericProp = qobject_cast<DzNumericProperty*>(opacityStrengthProp);
		}
		QString sAlphaFilename = "";
		if (imageProp)
		{
			sAlphaFilename = imageProp->getValue()->getFilename();
			if (sAlphaFilename != "")
			{
				bHasCutout = true;
			}
		}
		else if (numericProp)
		{
			if (numericProp->getMapValue())
			{
				sAlphaFilename = numericProp->getMapValue()->getFilename();
				if (sAlphaFilename != "")
				{
					bHasCutout = true;
				}
			}
		}
		if (bHasCutout == false)
		{
			return false;
		}

		// get diffuse
		DzProperty* diffuseProp = Material->findProperty("Diffuse Color");
		DzColorProperty* colorProp = qobject_cast<DzColorProperty*>(diffuseProp);
		QString sDiffuseFilename = "";
		if (colorProp && colorProp->getMapValue())
		{
			sDiffuseFilename = colorProp->getMapValue()->getFilename();
		}
		QString sOriginalDiffuseFilename = sDiffuseFilename;

		if (bHasCutout)
		{
			QString stemDiffuse = QFileInfo(sDiffuseFilename).fileName();
			QString stemAlpha = QFileInfo(sAlphaFilename).fileName();

			// load diffuse
			QImage diffuseImage;
			if (sDiffuseFilename == "")
			{
				stemDiffuse = "EMPTY";
				diffuseImage = dzApp->getImageMgr()->loadImage(sAlphaFilename);
				ImageTools::multiplyImageByColorMultithreaded(diffuseImage, colorProp->getColorValue());
			}
			else
			{
				diffuseImage = dzApp->getImageMgr()->loadImage(sDiffuseFilename);
			}

			// load alpha
			QImage alphaImage = dzApp->getImageMgr()->loadImage(sAlphaFilename);

			// prepare output image
			QImage outputImage;
			outputImage = diffuseImage;
			outputImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

			// DB 2024-09-23: Change alpha raw to alpha srgb colorspace
			ImageTools::ConvertImageLinearToRgbMultithreaded(alphaImage);

			// Resize Diffuse and Alpha
			if (outputImage.height() != alphaImage.height() ||
				outputImage.width() != alphaImage.width())
			{
				// if outputImage is too small (<64 pixels) then resize to alphaImage dim (if > 64)
				int dw = outputImage.width();
				int dh = outputImage.height();
				int aw = alphaImage.width();
				int ah = alphaImage.height();
				if ((dw < 64 && dh < 64) && (aw > 64 || ah > 64)) {
					outputImage = outputImage.scaled(aw, ah, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				}

				// scale diffuse to power of 2
				if (!ImageTools::isPowerOfTwo(outputImage.height()) || !ImageTools::isPowerOfTwo(outputImage.width()))
				{
                    int w = ImageTools::nearestPowerOfTwo(outputImage.width());
                    int h = ImageTools::nearestPowerOfTwo(outputImage.height());
					outputImage = outputImage.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				}

				// scale alpha to size of diffuse
				int w = outputImage.width();
				int h = outputImage.height();
				alphaImage = alphaImage.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

				if (alphaImage.isGrayscale() == false)
				{
					alphaImage.convertToFormat(QImage::Format_Mono);
				}
			}
			// combine diffuse and alpha
			outputImage.setAlphaChannel(alphaImage);
			outputImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

			// save out file
			QString tempPath = dzApp->getTempPath().replace("\\", "/");
			QString outfile = tempPath + "/" + stemDiffuse + "+alpha_"+ stemAlpha + ".png";
			//dzApp->getImageMgr()->saveImage(outfile, outputImage);
			// DB, 2023-10-27: Save with fastest settings
			outputImage.save(outfile, 0, 100);

			// create Undo Data
			DiffuseAndAlphaMapsUndoData undoData;
			// DB 2024-09-18 crashfix: avoid invalidated material properties
			undoData.materialIndex = Material->getIndex();
			undoData.materialLabel = Material->getLabel();
			undoData.diffuseProperty = colorProp;
			undoData.colorMapName = sOriginalDiffuseFilename;
			undoData.cutoutProperty = numericProp;
			undoData.cutoutMapName = sAlphaFilename;
			m_undoList_CombineDiffuseAndAlphaMaps.append(undoData);

			// assign to diffuse property and cutout property
			//colorProp->setMap(outfile);
			//numericProp->setMap(outfile);
			// DB, 2023-11-10: Do not modify internal daz data, but just add to DTU overrides
			//m_overrideTable_MaterialImageMaps.insert(colorProp, outfile);
			//m_overrideTable_MaterialImageMaps.insert(numericProp, outfile);
			if (colorProp) m_overrideTable_MaterialProperties.insert(colorProp, MaterialOverride(outfile, QColor(255,255,255)));
			if (numericProp) m_overrideTable_MaterialProperties.insert(numericProp, MaterialOverride(outfile, 1.0));
			if (imageProp) m_overrideTable_MaterialProperties.insert(imageProp, MaterialOverride(outfile));


		}
	}

	return true;
}

bool DzBridgeAction::undoCombineDiffuseAndAlphaMaps()
{
    DzProgress::setCurrentInfo("Daz Bridge: Undoing Combine Diffuse and Alpha Maps...");
                
	foreach(DiffuseAndAlphaMapsUndoData undoData, m_undoList_CombineDiffuseAndAlphaMaps)
	{
		// DB 2024-09-18 crashfix: check for material list and scene integrity
		int undoMatIndex = undoData.materialIndex;
		QString undoMatLabel = undoData.materialLabel;
		DzMaterial* pMaterial = DzMaterial::getMaterial(undoMatIndex);
		if (!pMaterial) {
			dzApp->log("DzBridge ERROR: undoCombineDiffuseAndAlphaMaps(): Scene Integrity Failure: material index integrity failure: index=" + QString("%1").arg(undoMatIndex) + ", label=" + undoMatLabel);
			continue;
		}

		if (undoData.diffuseProperty && !undoData.colorMapName.isEmpty() && undoData.colorMapName != "")
			undoData.diffuseProperty->setMap(undoData.colorMapName);
		if (undoData.cutoutProperty && !undoData.cutoutMapName.isEmpty() && undoData.cutoutMapName != "")
			undoData.cutoutProperty->setMap(undoData.cutoutMapName);
	}
	m_undoList_CombineDiffuseAndAlphaMaps.clear();

    DzProgress::setCurrentInfo("Daz Bridge: Undo Combine Diffuse and Alpha Maps Complete.");

    return true;
}

bool DzBridgeAction::multiplyTextureValues(DzMaterial* material)
{
	if (!material) return false;

	foreach(QObject *propertyObject, material->getPropertyList())
	{
		DzColorProperty* colorProperty = qobject_cast<DzColorProperty*>(propertyObject);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(propertyObject);


		if (colorProperty && colorProperty->getMapValue())
		{
			// multiply value
			QColor colorValue = colorProperty->getColorValue();
			QString textureFilename = colorProperty->getMapValue()->getFilename();

			if (colorValue != QColor(255, 255, 255) && textureFilename != "")
			{
				QImage image = dzApp->getImageMgr()->loadImage(textureFilename);
				QString tempPath = dzApp->getTempPath().replace("\\", "/");
				QString stem = QFileInfo(textureFilename).fileName();
				QString outfile = tempPath + "/" + stem + QString("_%1.png").arg(ImageTools::colorToHexString(colorValue));

				// multiply image
				dzApp->log(QString("DazBridge: Baking material strength into image: %1").arg(outfile));
				ImageTools::multiplyImageByColorMultithreaded(image, colorValue);

				// save out file
//				dzApp->getImageMgr()->saveImage(outfile, image);
				// DB, 2023-10-27: Save with fastest settings
				image.save(outfile, 0, 100);

				// create undo record
				MultiplyTextureValuesUndoData undoData;
				undoData.materialIndex = material->getIndex();
				undoData.materialLabel = material->getLabel();
				undoData.textureProperty = colorProperty;
				undoData.textureValue = colorValue.rgba();
				undoData.textureMapName = textureFilename;

				m_undoList_MultilpyTextureValues.append(undoData);

				// modify property
				colorProperty->setColorValue(QColor(255,255,255));
				colorProperty->setMap(outfile);
			}

		}
		else if (numericProperty && numericProperty->getMapValue())
		{
			// multiply value
			double numericValue = numericProperty->getDoubleValue();
			QString textureFilename = numericProperty->getMapValue()->getFilename();

			if (numericValue != 1.0 && textureFilename != "")
			{
				QImage image = dzApp->getImageMgr()->loadImage(textureFilename);
				QString tempPath = dzApp->getTempPath().replace("\\", "/");
				QString stem = QFileInfo(textureFilename).fileName();
				QString outfile = tempPath + "/" + stem + QString("_%1.png").arg(numericValue);

				// multiply image
				dzApp->log(QString("DazBridge: Baking material strength into image: %1").arg(outfile));
				ImageTools::multiplyImageByStrengthMultithreaded(image, numericValue);

				// save out file
//				dzApp->getImageMgr()->saveImage(outfile, image);
				// DB, 2023-10-27: Save with fastest settings
				image.save(outfile, 0, 100);

				// create undo record
				MultiplyTextureValuesUndoData undoData;
				undoData.materialIndex = material->getIndex();
				undoData.materialLabel = material->getLabel();
				undoData.textureProperty = numericProperty;
				undoData.textureValue = numericValue;
				undoData.textureMapName = textureFilename;

				m_undoList_MultilpyTextureValues.append(undoData);

			}
		}
	}

	return true;
}

bool DzBridgeAction::undoMultiplyTextureValues()
{

	foreach(MultiplyTextureValuesUndoData undoData, m_undoList_MultilpyTextureValues)
	{
		// DB 2024-09-18 crashfix: check for material list and scene integrity
		int undoMatIndex = undoData.materialIndex;
		QString undoMatLabel = undoData.materialLabel;
		DzMaterial* pMaterial = DzMaterial::getMaterial(undoMatIndex);
		if (!pMaterial) {
			dzApp->log("DzBridge ERROR: undoMultiplyTextureValues(): Scene Integrity Failure: material index integrity failure: index=" + QString("%1").arg(undoMatIndex) + ", label=" + undoMatLabel);
			continue;
		}

		DzColorProperty* colorProperty = qobject_cast<DzColorProperty*>(undoData.textureProperty);
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(undoData.textureProperty);

		if (colorProperty)
		{
			// undo color multiply
			QColor colorValue( undoData.textureValue.toInt() );
			colorProperty->setColorValue(colorValue);
			colorProperty->setMap(undoData.textureMapName);
		}
		else if (numericProperty)
		{
			// undo numeric multiply
			double numericValue = undoData.textureValue.toDouble();
			numericProperty->setDoubleValue(numericValue);
			numericProperty->setMap(undoData.textureMapName);
		}
	}
	m_undoList_MultilpyTextureValues.clear();

	return true;
}

/** DB 2024-12-9
*   Removing deleteDir from API due to danger of operation.
*	Needing to use this function within a bridge plugin probably represents
*	a design flaw in the solution -- consider alternative solutions such as
*	deleting known hardcoded folders (without using recursion) or deleting
*	known files.
*/
//// recursively delete directory and all its contents
//bool DzBridgeAction::deleteDir(QString folderPath)
//{
// ....
//}

bool DzBridgeAction::isAssetMorphCompatible(QString sAssetType)
{
	if (sAssetType == "Animation" || sAssetType == "SkeletalMesh")
		return true;

	return false;
}

bool DzBridgeAction::isAssetMeshCompatible(QString sAssetType)
{
	if (sAssetType == "SkeletalMesh" || sAssetType == "StaticMesh" || sAssetType == "Environment")
		return true;

	return false;
}

bool DzBridgeAction::isAssetAnimationCompatible(QString sAssetType)
{
	if (sAssetType == "Animation")
		return true;

	return false;
}

bool DzBridgeAction::isAssetEnvironmentCompatible(QString sAssetType)
{
	if (sAssetType == "Environment")
		return true;

	return false;
}

bool DzBridgeAction::isAssetPoseCompatible(QString sAssetType)
{
	if (sAssetType == "Pose")
		return true;

	return false;
}

bool DzBridgeAction::isInteractiveMode()
{
	bool isInteractive = false;

	switch (m_nNonInteractiveMode) {

	case eNonInteractiveMode::FullInteractiveMode:
	case eNonInteractiveMode::ReducedPopup:
	case eNonInteractiveMode::DzExporterMode:
		isInteractive = true;
		break;

	case eNonInteractiveMode::ScriptMode:
	case eNonInteractiveMode::DzExporterModeRunSilent:
	default:
		isInteractive = false;
		break;
	}

	return isInteractive;
}

// DB 2024-08-29: Convenience function for exporting full scene using Raw Environment Export
void DzBridgeAction::writeSceneMaterials(DzJsonWriter& Writer, QTextStream* pCSVstream)
{
	if (m_nNonInteractiveMode != eNonInteractiveMode::ScriptMode) {
		m_mapProcessedFiles.clear();
	}
	Writer.startMemberArray("Materials", true);

	DzNodeList rootNodes = BuildRootNodeList();
//	auto aObjectList = dzScene->getNodeList();
//	foreach(auto el, aObjectList) {
	foreach(auto el, rootNodes) {
		DzNode* pNode = qobject_cast<DzNode*>(el);
		writeAllMaterials(pNode, Writer, pCSVstream, true);
	}

	Writer.finishArray();
}

bool DzBridgeAction::renameDuplicateNodeName(DzNode* node, QStringList& existingNodeNameList)
{
	// 2024-SEP-10, DB: Bugfix boneconverter incompatibility
	if (node->inherits("DzBone")) {
		return true;
	}

	QString sNodeName = node->getName();
	if (existingNodeNameList.contains(sNodeName) == true) {
		int nDuplicateCount = 1;
		QString sNewName = QString("%1__DUP_%2").arg(sNodeName).arg(nDuplicateCount, 2, 10, QChar('0'));
		while (existingNodeNameList.contains(sNewName) == true)
		{
			nDuplicateCount++;
			sNewName = QString("%1__DUP_%2").arg(sNodeName).arg(nDuplicateCount, 2, 10, QChar('0'));
			if (nDuplicateCount > 999) {
				// error, abort
				dzApp->log("DzBridge: renameDuplicateNodeName(): CRITICAL ERROR: Over 999 duplicates for node name: " + sNodeName + ", aborting rename.");
				return false;
			}
		}
		node->setName(sNewName);
		// Sanity Check before creating Undo Entry
		if (node->getName() == sNewName) {
			m_undoTable_DuplicateNodeRename.insert(sNewName, sNodeName);
		}
	}
	existingNodeNameList.append(node->getName());

	return true;
}

bool DzBridgeAction::undoDuplicateNodeRename()
{
	foreach(QString sNewName, m_undoTable_DuplicateNodeRename.keys())
	{
		DzNode* pNode = dzScene->findNode(sNewName);
		if (pNode) {
			QString sOldName = m_undoTable_DuplicateNodeRename[sNewName];
			pNode->setName(sOldName);
		}
		else {
			dzApp->log("DzBridge WARNING: undoDuplicateNodeRename() unable to find pNode named: " + sNewName);
		}
	}
	m_undoTable_DuplicateNodeRename.clear();
	return true;
}

DzNode* DzBridgeAction::ChooseBestSelectedNode(const DzNodeList aNodeList)
{
	foreach(DzNode * pNode, aNodeList) {
		if (pNode->inherits("DzGroupNode") ||
			pNode->inherits("DzSkeleton")) {
			return pNode;
		}
	}
	foreach(DzNode* pNode, aNodeList) {
		if (!pNode->inherits("DzCamera")) {
			return pNode;
		}
	}
	return aNodeList[0];
}


// Find DzNode that matches "sNodeName", depth-first recursive search of all descendents of "pRootNode"
DzNode* DzBridgeAction::FindNodeByName(DzNode* pRootNode, QString sNodeName)
{
	if (pRootNode == NULL)
		pRootNode = dzScene->getPrimarySelection();

	DzObject* pObject = pRootNode->getObject();
	if (pObject && pObject->getCurrentShape())
	{
		if (pRootNode->getName() == sNodeName)
			return pRootNode;
	}

	for (int ChildIndex = 0; ChildIndex < pRootNode->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* pChildNode = pRootNode->getNodeChild(ChildIndex);
		DzNode* pFoundNode = FindNodeByName(pChildNode, sNodeName);
		if (pFoundNode)
			return pFoundNode;
	}
	return NULL;
}

bool DzBridgeAction::bakeMakeup(DzMaterial* pMaterial)
{
		if (!pMaterial) return false;
	
		bool bHasMakeup = false;
		DzProperty* pDiffusePropertySearchResult = pMaterial->findProperty("Diffuse Color");
		DzProperty* pMakeupEnablePropertySearchResult = pMaterial->findProperty("Makeup Enable");
		DzProperty* pMakeupWeightPropertySearchResult = pMaterial->findProperty("Makeup Weight");
		DzProperty* pMakeupBasePropertySearchResult = pMaterial->findProperty("Makeup Base Color");
	
		DzColorProperty* pDiffuseColorProperty = qobject_cast<DzColorProperty*>(pDiffusePropertySearchResult);
		DzBoolProperty* pMakeupEnableBoolProperty = qobject_cast<DzBoolProperty*>(pMakeupEnablePropertySearchResult);
		DzNumericProperty* pMakeupWeightNumericProperty = qobject_cast<DzNumericProperty*>(pMakeupWeightPropertySearchResult);
		DzColorProperty* pMakeupBaseColorProperty = qobject_cast<DzColorProperty*>(pMakeupBasePropertySearchResult);
	
		// update with more advanced logic in the future if needed
		if (pDiffuseColorProperty && pMakeupEnableBoolProperty && pMakeupWeightNumericProperty && pMakeupBaseColorProperty) {
			bHasMakeup = true;
		}
	
		// return false if the code block above fails to find valid makeup
		if (!bHasMakeup) return false;
	
		bool bMakeupEnabled = pMakeupEnableBoolProperty->getBoolValue();
		if (!bMakeupEnabled)
			return false;

	bool bResult = bakeOverlayProperty(pMaterial, "Diffuse Color", "Makeup Base Color", "Makeup Weight");

	return bResult;
}


bool DzBridgeAction::bakeTranslucency(DzMaterial* pMaterial)
{
	DzProperty* pEnableTranslucencySearchResult = pMaterial->findProperty("Translucency Enable");
	DzProperty* pEnableTransmissionSearchResult = pMaterial->findProperty("Transmission Enable");
	DzProperty* pTransmissionColorSearchResult = pMaterial->findProperty("Transmitted Color");
	DzProperty* pTranslucencyColorSearchResult = pMaterial->findProperty("Translucency Color");
	DzProperty* pTranslucencyWeightSearchResult = pMaterial->findProperty("Translucency Weight");
	DzBoolProperty* pEnableTranslucencyBoolProperty = qobject_cast<DzBoolProperty*>(pEnableTranslucencySearchResult);
	DzBoolProperty* pEnableTransmissionBoolProperty = qobject_cast<DzBoolProperty*>(pEnableTransmissionSearchResult);
	DzColorProperty* pTransmissionColorProperty = qobject_cast<DzColorProperty*>(pTransmissionColorSearchResult);
	DzColorProperty* pTranslucencyColorProperty = qobject_cast<DzColorProperty*>(pTranslucencyColorSearchResult);
	DzNumericProperty* pTranslucencyWeightProperty = qobject_cast<DzNumericProperty*>(pTranslucencyWeightSearchResult);
	if (!pEnableTranslucencyBoolProperty || !pEnableTransmissionBoolProperty || !pTransmissionColorProperty || !pTranslucencyColorProperty || !pTranslucencyWeightProperty) {
		return false;
	}
	if (!pEnableTranslucencyBoolProperty->getBoolValue()) {
		return false;
	}
	if (!pEnableTransmissionBoolProperty->getBoolValue()) {
		return false;
	}
	if (!pTranslucencyColorProperty->getMapValue()) {
		return false;
	}

	// bake transmission color into translucency map
	QColor colorValue = pTransmissionColorProperty->getColorValue();
	QString sTranslucencyFilename = pTranslucencyColorProperty->getMapValue()->getFilename();
	QImage image = dzApp->getImageMgr()->loadImage(sTranslucencyFilename);
	QString tempPath = dzApp->getTempPath().replace("\\", "/");
	QString stem = QFileInfo(sTranslucencyFilename).fileName();
	QString outfile = tempPath + "/" + stem + QString("_%1.png").arg(ImageTools::colorToHexString(colorValue));
	dzApp->log(QString("DazBridge: Baking transmission color into translucency map: %1").arg(outfile));
	ImageTools::multiplyImageByColorMultithreaded(image, colorValue);
	image.save(outfile, 0, 100);

	// create undo record
	MultiplyTextureValuesUndoData undoData;
	undoData.materialIndex = pMaterial->getIndex();
	undoData.materialLabel = pMaterial->getLabel();
	undoData.textureProperty = pTranslucencyColorProperty;
	undoData.textureValue = pTranslucencyColorProperty->getColorValue().rgba();
	undoData.textureMapName = sTranslucencyFilename;
	m_undoList_MultilpyTextureValues.append(undoData);
	//
	MultiplyTextureValuesUndoData undoData2;
	undoData2.materialIndex = pMaterial->getIndex();
	undoData2.materialLabel = pMaterial->getLabel();
	undoData2.textureProperty = pTransmissionColorProperty;
	undoData2.textureValue = pTransmissionColorProperty->getColorValue().rgba();
	if (pTransmissionColorProperty->getMapValue()) {
		undoData2.textureMapName = pTransmissionColorProperty->getMapValue()->getFilename();
	}
	m_undoList_MultilpyTextureValues.append(undoData2);
	//
	MultiplyTextureValuesUndoData undoData3;
	undoData3.materialIndex = pMaterial->getIndex();
	undoData3.materialLabel = pMaterial->getLabel();
	undoData3.textureProperty = pTranslucencyWeightProperty;
	double fOriginalWeight = pTranslucencyWeightProperty->getDoubleValue();
	undoData3.textureValue = fOriginalWeight;
	if (pTranslucencyWeightProperty->getMapValue()) {
		undoData3.textureMapName = pTranslucencyWeightProperty->getMapValue()->getFilename();
	}
	m_undoList_MultilpyTextureValues.append(undoData3);

	// modify property
	pTransmissionColorProperty->setColorValue(QColor(255, 255, 255));
	pTransmissionColorProperty->setMap("");
	pTranslucencyColorProperty->setMap(outfile);
	double newWeight = fOriginalWeight;
	newWeight *= 0.5;
	pTranslucencyWeightProperty->setDoubleValue(newWeight);

	bool bResult = bakeOverlayProperty(pMaterial, "Diffuse Color", "Translucency Color", "Translucency Weight");
	if (bResult) 
	{
		// set overrides or undo, etc.
	}

	return bResult;
}


bool DzBridgeAction::bakeOverlayProperty(DzMaterial* pMaterial, QString sPropertyA, QString sPropertyB, QString sAlphaMaskProperty)
{
	if (!pMaterial) return false;

	bool bValidBlend = false;
	DzProperty* pColorPropertyASearchResult = pMaterial->findProperty(sPropertyA);
	DzProperty* pColorPropertyBSearchResult = pMaterial->findProperty(sPropertyB);
	DzProperty* pAlphaMaskPropertySearchResult = pMaterial->findProperty(sAlphaMaskProperty);

	DzColorProperty* pColorPropertyA = qobject_cast<DzColorProperty*>(pColorPropertyASearchResult);
	DzColorProperty* pColorPropertyB = qobject_cast<DzColorProperty*>(pColorPropertyBSearchResult);
	DzNumericProperty* pNumericPropertyAlphaMask = qobject_cast<DzNumericProperty*>(pAlphaMaskPropertySearchResult);

	// update with more advanced logic in the future if needed
	if (pColorPropertyA && pColorPropertyB && pNumericPropertyAlphaMask) {
		bValidBlend = true;
	}

	// return false if the code block above fails to find valid makeup
	if (!bValidBlend) return false;

	// get file for propertyA
	QString sFilenameA = "";
	// look for override
	if (m_overrideTable_MaterialProperties.contains(pColorPropertyA)) {
		sFilenameA = m_overrideTable_MaterialProperties[pColorPropertyA].filename;
	}
	//if (m_overrideTable_MaterialImageMaps.contains(pColorPropertyA)) {
	//	sFilenameA = m_overrideTable_MaterialImageMaps[pColorPropertyA];
	//}
	else
	{
		DzTexture* pTextureA = pColorPropertyA->getMapValue();
		DzLayeredTexture* pLayeredTextureA = qobject_cast<DzLayeredTexture*>(pTextureA);
		if (pTextureA)
		{
			if (pLayeredTextureA) {
				pLayeredTextureA->getPreviewPixmap(1, 1);
			}
			sFilenameA = pTextureA->getFilename();
		}
	}
	if (sFilenameA == "")
	{
		return false;
	}

	// get file for propertyA
	QString sFilenameB = "";
	// look for override
	if (m_overrideTable_MaterialProperties.contains(pColorPropertyB)) {
		sFilenameA = m_overrideTable_MaterialProperties[pColorPropertyB].filename;
	}
	//if (m_overrideTable_MaterialImageMaps.contains(pColorPropertyB)) {
	//	sFilenameB = m_overrideTable_MaterialImageMaps[pColorPropertyB];
	//}
	else
	{
		DzTexture* pTextureB = pColorPropertyB->getMapValue();
		DzLayeredTexture* pLayeredTextureB = qobject_cast<DzLayeredTexture*>(pTextureB);
		if (pTextureB)
		{
			if (pLayeredTextureB) {
				pLayeredTextureB->getPreviewPixmap(1, 1);
			}
			sFilenameB = pTextureB->getFilename();
		}
	}
	if (sFilenameB == "")
	{
		return false;
	}

	// get file for Alpha mask
	QString sFilenameAlphaMask = "";
	// look for override
	if (m_overrideTable_MaterialProperties.contains(pNumericPropertyAlphaMask)) {
		sFilenameA = m_overrideTable_MaterialProperties[pNumericPropertyAlphaMask].filename;
	}
	//if (m_overrideTable_MaterialImageMaps.contains(pNumericPropertyAlphaMask)) {
	//	sFilenameAlphaMask = m_overrideTable_MaterialImageMaps[pNumericPropertyAlphaMask];
	//}
	else
	{
		DzTexture* pTextureAlphaMask = pNumericPropertyAlphaMask->getMapValue();
		DzLayeredTexture* pLayeredTextureAlphaMask = qobject_cast<DzLayeredTexture*>(pTextureAlphaMask);
		if (pTextureAlphaMask)
		{
			if (pLayeredTextureAlphaMask) {
				pLayeredTextureAlphaMask->getPreviewPixmap(1, 1);
			}
			sFilenameAlphaMask = pTextureAlphaMask->getFilename();
		}
	}

	// Load images
	QImage imageA;
	QImage imageB;
	QImage alphaMask;
	if (dzApp->getImageMgr()->loadImage(sFilenameA, imageA) != DZ_NO_ERROR) {
		return false;
	}
	if (dzApp->getImageMgr()->loadImage(sFilenameB, imageB) != DZ_NO_ERROR) {
		return false;
	}
	if (sFilenameAlphaMask == "" ||
		dzApp->getImageMgr()->loadImage(sFilenameAlphaMask, alphaMask) != DZ_NO_ERROR)
	{
		int imageWidth = 32; // Small size since it's a solid color
		int imageHeight = 32;
		alphaMask = QImage(imageWidth, imageHeight, QImage::Format_ARGB32);
		double nMaskWeight = pNumericPropertyAlphaMask->getDoubleValue();
		int alphaValue = qBound(0, static_cast<int>(nMaskWeight * 255), 255);
		QColor alphaColor(alphaValue, alphaValue, alphaValue);
		alphaMask.fill(alphaColor);
	}


	// Check if images are valid
	if (imageA.isNull() || imageB.isNull() || alphaMask.isNull())
	{
		dzApp->log("DazBridge: ERROR: bakeOverlayProperty(): One or more images failed to load: " + pMaterial->getLabel());
		return false;
	}

	// Ensure images have same dimensions
	int width = imageA.width();
	int height = imageA.height();

	if (imageB.width() != width || imageB.height() != height)
	{
		imageB = imageB.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}

	if (alphaMask.width() != width || alphaMask.height() != height)
	{
		alphaMask = alphaMask.scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}

	// Perform blending using the static utility function
	ImageTools::BlendImagesWithAlphaMultithreaded(imageA, imageB, alphaMask);

	// Save output image
	QString tempPath = dzApp->getTempPath().replace("\\", "/");
	QString stemA = QFileInfo(sFilenameA).fileName();
	QString stemB = QFileInfo(sFilenameB).fileName();
	QString outputFilename = tempPath + "/" + stemA + "+" + stemB + ".png"; // TODO: detect if alpha channel present, if not then use jpg (aka, after alpha bake stage)

	if (!imageA.save(outputFilename, 0, 100))
	{
		dzApp->log("DazBridge: ERROR: bakeOverlayProperty() Failed to save output image:" + outputFilename + " for material: " + pMaterial->getLabel());
		return false;
	}

	// WARNING!!! IF YOU DO NOT APPLY TO DAZ PROPERTY, FURTHER SCENE PROCESSING INVOLVING THIS MATERIAL MAY BE CORRUPTED!!!
	// Assign the output image to propertyA (or use an override)
	//m_overrideTable_MaterialImageMaps.insert(pColorPropertyA, outputFilename);
	//m_overrideTable_MaterialImageMaps.insert(pColorPropertyB, "");
	//m_overrideTable_MaterialImageMaps.insert(pNumericPropertyAlphaMask, "");
	m_overrideTable_MaterialProperties.insert(pColorPropertyA, MaterialOverride(outputFilename, QColor(255,255,255)));
	m_overrideTable_MaterialProperties.insert(pColorPropertyB, MaterialOverride("", QColor(0,0,0)));
	m_overrideTable_MaterialProperties.insert(pNumericPropertyAlphaMask, MaterialOverride("", 0.0));
	return true;
}

DzSkeleton* DzBridgeAction::GetNonFollowerParent(DzSkeleton* pSkeleton)
{
	DzSkeleton* pFollowTarget = pSkeleton->getFollowTarget();

	if (!pFollowTarget) return pSkeleton;

	// redirect to parent of follower
	while (pFollowTarget->getFollowTarget())
	{
		pFollowTarget = pFollowTarget->getFollowTarget();
	}

	return pFollowTarget;
}

EAssetType DzBridgeAction::SelectBestRootNodeForTransfer(bool bAvoidFollowers)
{
	EAssetType eBestAssetType = EAssetType::None;

	// Build a list of all selected nodes and select the best among them, potentially selecting no node
	if (dzScene->getNumSelectedNodes() > 1)
	{
		DzNodeList selectedNodes;
		int numMeshes = 0;
		QObjectList objectList = dzScene->getSelectedNodeList();
		foreach(auto el, objectList) {
			DzNode* pNode = qobject_cast<DzNode*>(el);
			if (pNode) {
				selectedNodes.append(pNode);
				if (pNode->getObject()) {
					numMeshes++;
				}
			}
		}
		dzScene->selectAllNodes(false);
		DzNode* pSelection = ChooseBestSelectedNode(selectedNodes);
		if (pSelection) {
			pSelection->select(true);
			dzScene->setPrimarySelection(pSelection);
			if (numMeshes > 0) {
				eBestAssetType = EAssetType::Scene;
			}

		}
	}

	// Build List of Root Nodes in Scene and choose the best among them
	if (dzScene->getNumSelectedNodes() == 0)
	{
		DzNodeList rootNodes = DZ_BRIDGE_NAMESPACE::DzBridgeAction::BuildRootNodeList();
		if (rootNodes.length() == 1) {
			dzScene->setPrimarySelection(rootNodes[0]);
		}
		else if (rootNodes.length() > 1)
		{
			// Switch to default Environment mode
			DzNode* pSelection = ChooseBestSelectedNode(rootNodes);
			dzScene->setPrimarySelection(pSelection);
			eBestAssetType = EAssetType::Scene;
		}
	}

	DzNode* pSelection = dzScene->getPrimarySelection();
	if (pSelection && eBestAssetType == EAssetType::None)
	{
		DzSkeleton* pSkeleton = qobject_cast<DzSkeleton*>(pSelection);
		DzBone* pBone = qobject_cast<DzBone*>(pSelection);
		DzObject* pObject = pSelection->getObject();

		// if bone, redirect to skeleton
		if (pBone) {
			pSkeleton = pBone->getSkeleton();
		}
		// if skeleton, assume a figure or legacy figure
		if (pSkeleton) 
		{
			if (bAvoidFollowers) {
				pSkeleton = GetNonFollowerParent(pSkeleton);
			}
			dzScene->selectAllNodes(false);
			pSkeleton->select(true);
			dzScene->setPrimarySelection(pSkeleton);
			eBestAssetType = EAssetType::SkeletalMesh;
		}
		else if (pObject) // has geometry
		{
			if (pSelection->isRootNode()) return EAssetType::StaticMesh;

			// else if object (geometry) exists, assume static
			// 1. First go up ancestor chain to see if pObject is parented to a figure
			DzNode* pAncestor = pSelection->getNodeParent();
			DzSkeleton* pSkeleton = qobject_cast<DzSkeleton*>(pAncestor);
			DzObject* pObject2 = pAncestor->getObject();
			while (!pSkeleton && pAncestor->getNodeParent()) {
				pAncestor = pAncestor->getNodeParent();
				pSkeleton = qobject_cast<DzSkeleton*>(pAncestor);
			}
			if (pSkeleton) 
			{
				if (bAvoidFollowers) {
					pSkeleton = GetNonFollowerParent(pSkeleton);
				}
				dzScene->selectAllNodes(false);
				pSkeleton->select(true);
				dzScene->setPrimarySelection(pSkeleton);
				eBestAssetType = EAssetType::SkeletalMesh;
			}
			else if (pObject2)
			{
				eBestAssetType = EAssetType::StaticMesh;
			}
			else
			{
				eBestAssetType = EAssetType::Scene;
			}
		}
		else
		{
			eBestAssetType = EAssetType::Scene;
		}
	}

	return eBestAssetType;
}

bool DzBridgeAction::forceLieUpdate(DzMaterial* pMaterial)
{
	if (!pMaterial) return false;

    auto propertyListIterator = pMaterial->propertyListIterator();
  
    while (propertyListIterator.hasNext())
    {
        DzProperty* property = propertyListIterator.next();

        DzNumericProperty* pNumericProperty = qobject_cast<DzNumericProperty*>(property);
        DzImageProperty* pImageProperty = qobject_cast<DzImageProperty*>(property);

        DzTexture* pTexture = NULL;
        if (pNumericProperty) {
            pTexture = pNumericProperty->getMapValue();
        }
        else if (pImageProperty) {
            pTexture = pImageProperty->getValue();
        }

        DzLayeredTexture* pLayeredTexture = NULL;
        if (pTexture && pTexture->inherits("DzLayeredTexture")) {
            pLayeredTexture = qobject_cast<DzLayeredTexture*>(pTexture);
            if (pLayeredTexture) {
                pLayeredTexture->getPreviewPixmap(1,1);
            }
        }
    }

	return true;
}

bool DzBridgeAction::DetectInstancesInScene()
{
	auto nodeListIterator = dzScene->nodeListIterator();

	while (nodeListIterator.hasNext())
	{
		DzNode* pNode = nodeListIterator.next();
		DzInstanceNode* pInstance = qobject_cast<DzInstanceNode*>(pNode);
		if (pInstance && pInstance->getTarget() != NULL) 
		{
			return true;
		}
	}

	return false;
}

bool DzBridgeAction::DetectCustomPivotsInScene()
{
	auto nodeListIterator = dzScene->nodeListIterator();

	while (nodeListIterator.hasNext())
	{
		DzNode* pNode = nodeListIterator.next();
		if (pNode->getObject() && pNode->getObject()->getCurrentShape() && pNode->inherits("DzSkeleton")==false)
		{
			DzVec3 pivotPoint = pNode->getOrigin(false);
			if (pivotPoint.m_x != 0 || pivotPoint.m_y != 0 || pivotPoint.m_z != 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool DzBridgeAction::BakePivots(QScopedPointer<DzScript>& Script, QString sScriptPath)
{
	bool bResult;
	bool bReplace = false;
	QString sScriptFilename = "bake_all_pivots_nogui.dsa";
	QString sEmbeddedFolderPath = ":/DazBridge";
	QString sEmbeddedFilepath = sEmbeddedFolderPath + "/" + sScriptFilename;
	if (sScriptPath != "") {
		sEmbeddedFilepath = sScriptPath;
	}
	QFile srcFile(sEmbeddedFilepath);
	if (srcFile.exists() == false) {
		dzApp->log(tr("DzBridge: ERROR: BakePivots() Invalid Script Path: ") + sScriptFilename);
		return false;
	}
	QString sTempFilepath = dzApp->getTempPath() + "/" + sScriptFilename;
	bResult = DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFile, &sTempFilepath, bReplace);
	srcFile.close();
	if (!bResult)
	{
		dzApp->log(tr("DzBridge: ERROR: BakePivots() Error occured while trying to copy script to temp folder: ") + sTempFilepath);
	}

//	DzScript* Script = new DzScript();
	Script.reset(new DzScript());

	bResult = Script->loadFromFile(sTempFilepath);
	if (!bResult) {
		dzApp->log(tr("DzBridge: CRITICAL ERROR: BakePivots() Error occured while trying to load script file: ") + sTempFilepath + ", aborting script.");
		return false;
	}

	bResult = Script->execute();

	// this may cause instability or crashes
//	Script->deleteLater();

	return bResult;
}

bool DzBridgeAction::BakeInstances(QScopedPointer<DzScript>& Script, QString sScriptPath)
{
	bool bResult;
	bool bReplace = false;
	QString sScriptFilename = "bake_all_instances_nogui.dsa";
	QString sEmbeddedFolderPath = ":/DazBridge";
	QString sEmbeddedFilepath = sEmbeddedFolderPath + "/" + sScriptFilename;
	if (sScriptPath != "") {
		sEmbeddedFilepath = sScriptPath;
	}
	QFile srcFile(sEmbeddedFilepath);
	if (srcFile.exists() == false) {
		dzApp->log(tr("DzBridge: ERROR: BakeInstances() Invalid Script Path: ") + sScriptFilename);
		return false;
	}
	QString sTempFilepath = dzApp->getTempPath() + "/" + sScriptFilename;
	bResult = DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFile, &sTempFilepath, bReplace);
	srcFile.close();
	if (!bResult)
	{
		dzApp->log(tr("DzBridge: ERROR: BakeInstances() Error occured while trying to copy script to temp folder: ") + sTempFilepath);
	}

	//	DzScript* Script = new DzScript();
	Script.reset(new DzScript());

	bResult = Script->loadFromFile(sTempFilepath);
	if (!bResult) {
		dzApp->log(tr("DzBridge: CRITICAL ERROR: BakeInstances() Error occured while trying to load script file: ") + sTempFilepath + ", aborting script.");
		return false;
	}

	bResult = Script->execute();

	// this may cause instability or crashes
//	Script->deleteLater();

	return bResult;
}

bool DzBridgeAction::DetectRigidFollowNodes()
{
	auto nodeListIterator = dzScene->nodeListIterator();

	while (nodeListIterator.hasNext())
	{
		DzNode* pNode = nodeListIterator.next();
		if (pNode->inherits("DzRigidFollowNode"))
		{
			return true;
		}
	}

	return false;

}

bool DzBridgeAction::BakeRigidFollowNodes(QScopedPointer<DzScript> &Script, QString sScriptPath)
{
	bool bResult;
	bool bReplace = false;
	QString sScriptFilename = "bake_rfn_nogui.dsa";
	QString sEmbeddedFolderPath = ":/DazBridge";
	QString sEmbeddedFilepath = sEmbeddedFolderPath + "/" + sScriptFilename;
	if (sScriptPath != "") {
		sEmbeddedFilepath = sScriptPath;
	}
	QFile srcFile(sEmbeddedFilepath);
	if (srcFile.exists() == false) {
		dzApp->log(tr("DzBridge: ERROR: BakeRigidFollowNodes() Invalid Script Path: ") + sScriptFilename);
		return false;
	}
	QString sTempFilepath = dzApp->getTempPath() + "/" + sScriptFilename;
	bResult = DZ_BRIDGE_NAMESPACE::DzBridgeAction::copyFile(&srcFile, &sTempFilepath, bReplace);
	srcFile.close();
	if (!bResult)
	{
		dzApp->log(tr("DzBridge: ERROR: BakeRigidFollowNodes() Error occured while trying to copy script to temp folder: ") + sTempFilepath);
	}

//	DzScript* Script = new DzScript();
	Script.reset(new DzScript());

	bResult = Script->loadFromFile(sTempFilepath);
	if (!bResult) {
		dzApp->log(tr("DzBridge: CRITICAL ERROR: BakeRigidFollowNodes() Error occured while trying to load script file: ") + sTempFilepath + ", aborting script.");
		return false;
	}

	bResult = Script->execute();

	// this may cause instability or crashes
//	Script->deleteLater();

	return bResult;

}

DzError DzBridgeAction::doPromptableObjectBaking()
{
	DzNode* pOriginalSelection = dzScene->getPrimarySelection();

	bool bInstancesDetected = DetectInstancesInScene();
	bool bCustomPivotsDetected = DetectCustomPivotsInScene();
	bool bRigidFollowNodesDetected = DetectRigidFollowNodes();

	int userChoice = QMessageBox::Yes;
	if (isInteractiveMode())
	{
		bool bDoPrompt = false;
		QString sDetected = "";

		if (bInstancesDetected && m_eBakeInstancesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask)
		{
			bDoPrompt = true;
			if (sDetected != "") sDetected += ", ";
			sDetected += "instances";

		}
		if (bCustomPivotsDetected && m_eBakePivotPointsMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask)
		{
			bDoPrompt = true;
			if (sDetected != "") sDetected += ", ";
			sDetected += "custom pivot points";
		}
		if (bRigidFollowNodesDetected && m_eBakeRigidFollowNodesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask)
		{
			bDoPrompt = true;
			if (sDetected != "") sDetected += ", ";
			sDetected += "rigid follow nodes";
		}
		int offset = sDetected.indexOf(", ");
		while (sDetected.indexOf(", ", offset + 1) != -1)
		{
			offset = sDetected.indexOf(", ", offset + 1);
		}
		if (offset != -1) {
			sDetected.replace(offset, (int) strlen(", "), " and ");
		}

		if (bDoPrompt)
		{
			QString sMessageCustom = tr("\
The current scene contains %1 which should be replaced and baked out to avoid conversion errors. \
These changes can not be undone. Make sure you Abort and save your scene \
if needed.\n\
\n\
Do you want to proceed with these changes?\n\
Or do you want to Ignore and continue the transfer without making changes?\n\
You may also Abort the transfer operation.").arg(sDetected);

			QString sBakeObjectsPrompt;
			sBakeObjectsPrompt = sMessageCustom;
			userChoice = QMessageBox::information(0,
				tr("Object Baking Recommended"),
				sBakeObjectsPrompt,
				QMessageBox::Yes,
				QMessageBox::Ignore,
				QMessageBox::Abort);
			if (userChoice == QMessageBox::Abort) {
				return DZ_USER_CANCELLED_OPERATION;
			}
		}
	}

	QScopedPointer<DzScript> Script(new DzScript());
	if (bInstancesDetected && m_eBakeInstancesMode != DZ_BRIDGE_NAMESPACE::EBakeMode::NeverBake)
	{
		if ((m_eBakeInstancesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::AlwaysBake) ||
			(m_eBakeInstancesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask && userChoice == QMessageBox::Yes))
		{
			QString sScriptFilename = "bake_all_instances_nogui.dsa";
			QString sEmbeddedFilepath = m_sEmbeddedFolderPath + "/" + sScriptFilename;
			BakeInstances(Script, sEmbeddedFilepath);
		}
	}
	if (bCustomPivotsDetected && m_eBakePivotPointsMode != DZ_BRIDGE_NAMESPACE::EBakeMode::NeverBake)
	{
		if ((m_eBakePivotPointsMode == DZ_BRIDGE_NAMESPACE::EBakeMode::AlwaysBake) ||
			(m_eBakePivotPointsMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask && userChoice == QMessageBox::Yes))
		{
			QString sScriptFilename = "bake_all_pivots_nogui.dsa";
			QString sEmbeddedFilepath = m_sEmbeddedFolderPath + "/" + sScriptFilename;
			BakePivots(Script, sEmbeddedFilepath);
		}
	}
	if (bRigidFollowNodesDetected && m_eBakeRigidFollowNodesMode != DZ_BRIDGE_NAMESPACE::EBakeMode::NeverBake)
	{
		if ((m_eBakeRigidFollowNodesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::AlwaysBake) ||
			(m_eBakeRigidFollowNodesMode == DZ_BRIDGE_NAMESPACE::EBakeMode::Ask && userChoice == QMessageBox::Yes))
		{
			QString sScriptFilename = "bake_rfn_nogui.dsa";
			QString sEmbeddedFilepath = m_sEmbeddedFolderPath + "/" + sScriptFilename;
			BakeRigidFollowNodes(Script, sEmbeddedFilepath);
		}
	}

	dzScene->setPrimarySelection(pOriginalSelection);

	return DZ_NO_ERROR;
}

bool DzBridgeAction::InstallEmbeddedArchive(QString sEmbeddedArchivePath, QString sDestinationPath)
{
	bool bInstallSuccessful = false;

	QString sArchiveFilename = QFileInfo(sEmbeddedArchivePath).fileName();
	// copy zip plugin to temp
	bool replace = true;
	QFile srcFile(sEmbeddedArchivePath);
	QString tempPathArchive = dzApp->getTempPath() + "/" + sArchiveFilename;
	DzBridgeAction::copyFile(&srcFile, &tempPathArchive, replace);
	srcFile.close();

	// extract to destionation
	int zip_result = ::zip_extract(tempPathArchive.toUtf8().data(), sDestinationPath.toUtf8().data(), nullptr, nullptr);
	if (zip_result != 0) {
		dzApp->log("DzBridge: ::zip_extract() error=" + QString("%1").arg(zip_result));
		return bInstallSuccessful;
	}

	// verify extraction was successfull
	// 1. get filename from archive
	// 2. test to see if destination path contains filename
	QStringList archiveFileNames;
	struct zip_t* zip = ::zip_open(tempPathArchive.toUtf8().data(), 0, 'r');
	int i, n = ::zip_entries_total(zip);
	for (i = 0; i < n; ++i) {
		::zip_entry_openbyindex(zip, i);
		{
			const char* name = ::zip_entry_name(zip);
			archiveFileNames.append(QString(name));
			//int isdir = ::zip_entry_isdir(zip);
			//unsigned long long size = ::zip_entry_size(zip);
			//unsigned int crc32 = ::zip_entry_crc32(zip);
		}
		::zip_entry_close(zip);
	}
	::zip_close(zip);
	bInstallSuccessful = true;
	for (QString filename : archiveFileNames)
	{
		QString filePath = sDestinationPath + "/" + filename;
		if (QFile(filePath).exists() == false)
		{
			bInstallSuccessful = false;
			break;
		}
	}

	// remove if succcessful, else leave intermediate files for debugging
	if (bInstallSuccessful)
		QFile(tempPathArchive).remove();

	return bInstallSuccessful;
}

void DzBridgeAction::setAssetType(EAssetType arg_eAssetType)
{
	switch (arg_eAssetType) {
	case EAssetType::Scene:
		m_sAssetType = "Environment";
		break;
		
	case EAssetType::SkeletalMesh:
		m_sAssetType = "SkeletalMesh";
		break;

	case EAssetType::StaticMesh:
		m_sAssetType = "StaticMesh";
		break;

	case EAssetType::Animation:
		m_sAssetType = "Animation";
		break;

	case EAssetType::Pose:
		m_sAssetType = "Pose";
		break;

	}
}

// DB 2024-12-09: new CleanIntermediateFolder method to replace the more dangerous deleteDir method
// This method will delete specific files of a specified type that are contained with an explicitly specified subfolder string
// that can be resolved into an existing subfolder found within the specified root intermediate folder.  Several safety checks
// are performed to minimize risk of unintentionally deleting large amounts of data.
// Pass "*.*" to clean entire Intermediate Folder
bool DzBridgeAction::cleanIntermediateSubFolder(QString sSubFolder)
{
	bool bResult = false;

	// sanity check
	if (sSubFolder == "") return false;

	if (m_sRootFolder == "") {
		if (isInteractiveMode()) {
			return false;
		}
		m_sRootFolder = this->readGuiRootFolder();
	}

	if (sSubFolder == "*.*") {
		// clean entire Intermediate Folder
		// perform sanity check that root folder is not global root and not user documents or other important folder
		if (DzBridgeTools::IsDangerousPath(m_sRootFolder))
		{
			dzApp->log("CRITICAL ERROR: cleanIntermediateSubFolder() can not run because target folder is set to dangerous sPath: " + m_sRootFolder);
			return false;
		}
		dzApp->log("DEBUG: WARNING: cleanIntermediateSubFolder() attempting to clean Intermediate folder: " + m_sRootFolder);
		// clean each intermediate subfolder
		QDir dirRootIntermediateFolder(m_sRootFolder);
		if (dirRootIntermediateFolder.exists()) {
			foreach(QFileInfo fi, dirRootIntermediateFolder.entryInfoList())
			{
				if (fi.isDir()) {
					bResult = this->cleanIntermediateSubFolder(fi.baseName());
				}
			}
		}
		return bResult;
	}
	else
	{
		QString sSubFolderPath = m_sRootFolder + "/" + sSubFolder;
		if (DzBridgeTools::IsDangerousPath(sSubFolderPath))
		{
			dzApp->log("CRITICAL ERROR: cleanIntermediateSubFolder() can not run because target folder is set to dangerous sPath: " + sSubFolderPath);
			return false;
		}

		// clean asset subfolder
		QDir qdirSubFolder(sSubFolderPath);
		if (qdirSubFolder.exists() == false)
		{
			return false;
		}
		dzApp->log("DEBUG: cleanIntermediateSubFolder() attempting to clean Intermedediate subfolder: " + sSubFolderPath);
		// clean hardcoded folders and known file
		// delete ExportTextures subfolder
		bResult = DzBridgeTools::SafeCleanIntermediateSubFolder(
			sSubFolderPath + "/ExportTextures",
			m_aKnownIntermediateFileExtensionsList);
		//// delete scripts subfolder
		bResult = DzBridgeTools::SafeCleanIntermediateSubFolder(
			sSubFolderPath + "/scripts/__pycache__",
			m_aKnownIntermediateFileExtensionsList
		);
		bResult = DzBridgeTools::SafeCleanIntermediateSubFolder(
			sSubFolderPath + "/scripts",
			m_aKnownIntermediateFileExtensionsList
		);
		// clean main asset subfolder
		bResult = DzBridgeTools::SafeCleanIntermediateSubFolder(
			sSubFolderPath,
			m_aKnownIntermediateFileExtensionsList
		);

	}

	return bResult;
}

// DB 2024-11-080: Used for Work around to correct Character Morphs using non - standard Object ERC Offset
bool DzBridgeTools::CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset)
{
	if (pNode == NULL) return false;

	auto pPropX = pNode->getXPosControl();
	auto pPropY = pNode->getYPosControl();
	auto pPropZ = pNode->getZPosControl();

	if (pPropX == NULL || pPropY == NULL || pPropZ == NULL) return false;

	double baseX = pPropX->getValue();
	double baseY = pPropY->getValue();
	double baseZ = pPropZ->getValue();

	double rawX = pPropX->getRawValue();
	double rawY = pPropY->getRawValue();
	double rawZ = pPropZ->getRawValue();

	vOffset.m_x = rawX - baseX;
	vOffset.m_y = rawY - baseY;
	vOffset.m_z = rawZ - baseZ;

	return true;
}

bool DzBridgeTools::IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList) {


	QString sExtension = fi.suffix().toLower();
	if (sExtension.isEmpty() || sExtension == "") {
		return false;
	}

	foreach(QString sKnownExtension, aExtensionsList) {
		if (sExtension == sKnownExtension) {
			return true;
		}
	}

	return false;
}

// aExtensionsList - QStringList of extensions to DELETE
// sSubFolderPath - QString of full path to subfolder to clean contents and RMDIR
bool DzBridgeTools::SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete)
{
	if (IsDangerousPath(sSubFolderPath)) {
		return false;
	}

	QDir qdirScripts(sSubFolderPath);
	if (qdirScripts.exists()) {
		// remove all known intermediate file types within it (nonrecursively)
		foreach(QFileInfo fi, qdirScripts.entryInfoList())
		{
			if (fi.isFile() && DzBridgeTools::IsFileTypeInList(fi, aExtensionsToDelete)) {
				qdirScripts.remove(fi.filePath());
			}
		}
	}
	bool bResult = qdirScripts.rmdir(qdirScripts.absolutePath());

	return bResult;
}

QString DzBridgeTools::CleanTrailingSeparator(QString sFolderPath)
{
	QString sResult = sFolderPath;
	sResult = sResult.replace("\\", "/").toLower();

	while (sResult.endsWith("/") && sResult.length() > 2)
	{
		sResult.chop(1);
	}

	if (sResult == "/") {
		return "";
	}

	return sResult;
}

bool DzBridgeTools::IsDangerousPath(const QString& sPath)
{
	if (sPath.isEmpty()) {
		return true;
	}

	QDir dir(sPath);
	// Normalize path
	QString sNormalizedPath = dir.canonicalPath(); // Removes symbolic links, redundant "." and ".."
	sNormalizedPath = sNormalizedPath.replace("\\", "/").toLower(); // Normalize directory separators

	// Check common dangerous paths
	if (sNormalizedPath.isEmpty() ||
		sNormalizedPath == "/" ||
		sNormalizedPath == "~" ||
		sNormalizedPath == QDir::homePath() ||
		sNormalizedPath == QDir::rootPath() ||
		sNormalizedPath == QDir::tempPath()) // system temporary directory
	{
		return true;
	}

	// Optionally add platform-specific checks
#ifdef Q_OS_WIN
	if (sNormalizedPath == "c:/" || sNormalizedPath == "d:/") {
		return true;
	}
	QStringList sSplit = sNormalizedPath.split(":");
	if (sSplit.length() == 2) {
		QString cleanedString = DzBridgeTools::CleanTrailingSeparator(sSplit[1]);
		if (cleanedString == "/windows") {
			return true;
		}
	}
#endif

	return false;
}

QString DzBridgeAction::scaleAndReEncodeMaterialProperties(DzNode* Node, DzMaterial* Material, DzProperty* Property)
{
	if (Node == nullptr || Material == nullptr || Property == nullptr)
		return "";

	QString Name = Property->getName();
	QString sLabel = Property->getLabel();
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
		return TextureName;
	}
	// Check for Override Image Map
	//if (m_overrideTable_MaterialImageMaps.contains(Property))
	//{
	//	TextureName = m_overrideTable_MaterialImageMaps[Property];
	//}
	if (m_overrideTable_MaterialProperties.contains(Property))
	{
		MaterialOverride overrideData = m_overrideTable_MaterialProperties[Property];
		if (overrideData.bHasFilename) TextureName = overrideData.filename;
		if (overrideData.bHasColor) dtuPropValue = overrideData.color.name();
		if (overrideData.bHasNumericValue) dtuPropNumericValue = overrideData.numericValue;
	}

	QString dtuTextureName = TextureName;
	//	if ( !TextureName.isEmpty() && m_aProcessedFiles.contains(TextureName, Qt::CaseInsensitive)==false)
	if (!TextureName.isEmpty() && m_mapProcessedFiles.contains(TextureName.toLower()) == false)
	{

		// DB, 2023-10-27: New integrated resizing/re-encoding integrated save pathway
		// Execution Flow:
		// 1. Check if any re-encoding operation is enabled (m_bResizeTextures || m_bRecompressIfFileSizeTooBig || m_bConverToPng || m_bConverToJpg)
		// 2. If so then use the following order of operations:
		// 3. ResizeTextures takes highest priority
		// 4. RecompressIfFileSizeTooBig is second
		// 5. Convert non-PNG/JPG to PNG/JPG takes third
		QString sReEncodedFilename = "";
		// bUseReEncodedFilename is the flag used below to use the ReEncodedFilename string without doing any other operations upon it.
		// This is necessary for deferred operations where the path is invalid until later in the pipeline, so nothing else can be done
		// until the path becomes valid.
		bool bUseReEncodedFilename = false;
		if (m_bForceReEncoding || m_bResizeTextures || m_bRecompressIfFileSizeTooBig || m_bConvertToPng || m_bConvertToJpg)
		{
			DzImageMgr* imageMgr = dzApp->getImageMgr();
			QImage image = imageMgr->loadImage(TextureName);
			QString cleanedTempPath = dzApp->getTempPath().toLower().replace("\\", "/");
			QFileInfo fileInfo(TextureName);
			QString filestem = fileInfo.fileName();
			QString fileTypeExtension = fileInfo.suffix().toLower();
			int customEncodingQuality = 95;
			QSize customImageSize = m_qTargetTextureSize;
			bool bHasAlpha = image.hasAlphaChannel();
			bool bSkip = true;

			if (m_bForceReEncoding) bSkip = false;
			if (fileTypeExtension == "jpeg") fileTypeExtension = "jpg";
			if (fileTypeExtension == "jpg") customEncodingQuality = 95;
			if (fileTypeExtension == "png") customEncodingQuality = 75;

			// adjust on target quality/compression level if needed
			if (m_bRecompressIfFileSizeTooBig &&
				(QFileInfo(TextureName).size() > m_nFileSizeThresholdToInitiateRecompression)
				)
			{
				fileTypeExtension = "jpg";
				if (bHasAlpha && m_bConvertToPng) // default to jpg, unless png=true and jpg=false
					fileTypeExtension = "png";
				// if image.size is already <= m_qTargetTextureSize, then reduce even further
				if (image.size().width() <= m_qTargetTextureSize.width() &&
					image.size().height() <= m_qTargetTextureSize.height())
				{
					customImageSize = m_qTargetTextureSize / 2;
				}
				// decrease encoding quality if file was originally JPG, note: check against original name/ext
				customEncodingQuality -= 25;
				bSkip = false;
			}

			// resize image if needed
			if (customImageSize != m_qTargetTextureSize ||
				m_bResizeTextures &&
				(image.size().width() > m_qTargetTextureSize.width() ||
					image.size().height() > m_qTargetTextureSize.height())
				)
			{
				//QString sImageOperationMessage = QString(tr("Scaling: ") + filestem + " to " + QString("(%1x%2)").arg(customImageSize.width()).arg(customImageSize.height()) );
				//dzApp->log(sImageOperationMessage);
				//DzProgress::setCurrentInfo(sImageOperationMessage);
				//image = image.scaled(customImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
				bSkip = false;
			}

			// decide on target encoding
			if (m_bConvertToPng &&
				(fileTypeExtension.compare("png", Qt::CaseInsensitive) != 0)
				)
			{
				fileTypeExtension = "png";
				bSkip = false;
			}

			// decide on target encoding (JPG will override PNG above)
			if (m_bConvertToJpg &&
				(fileTypeExtension.compare("jpg", Qt::CaseInsensitive) != 0)
				)
			{
				fileTypeExtension = "jpg";
				if (m_bConvertToPng && bHasAlpha) // png+alpha takes priority over jpg
					fileTypeExtension = "png";
				bSkip = false;
			}

			if (!bSkip)
			{
				// finally, save out file with final resized image, quality level and encoding format
				if (m_bExportAllTextures) {
					sReEncodedFilename = generateExportAssetFilename(TextureName, Node->getLabel() + "_" + Material->getName());
					sReEncodedFilename += "." + fileTypeExtension;
					bUseReEncodedFilename = true;
				}
				else
				{
					sReEncodedFilename = cleanedTempPath + "/" + filestem + "." + fileTypeExtension;
				}
				if (m_bDeferProcessingImageToolsJobs) {
					// create unique only if using deferred image jobs, copy original to use as reference placeholder
					sReEncodedFilename = makeUniqueFilename(sReEncodedFilename, TextureName);
					QFile placeHolder(TextureName);
					placeHolder.copy(sReEncodedFilename);
				}
				else
				{
					// if performing jobs immediately, unable to check uniqueness since will be modifying from original
					// so do nothing and assume filename collisions at this point in the execution are the same file
				}

				if (sReEncodedFilename.split(".").count() > 3)
				{
					dzApp->log("WARNING: multiple file extensions possibly detected: " + sReEncodedFilename);
				}

				JobReEncodeImage* job = new JobReEncodeImage("myJob" + TextureName, TextureName, sReEncodedFilename,
					customEncodingQuality, customImageSize, m_bRecompressIfFileSizeTooBig, m_nFileSizeThresholdToInitiateRecompression,
					m_bResizeTextures, m_qTargetTextureSize);
				if (m_bDeferProcessingImageToolsJobs)
				{
					m_ImageToolsJobsManager->addJob(job);
					// prepare re-encoded filename for deferred use
					bUseReEncodedFilename = true;
					dtuTextureName = sReEncodedFilename;
				}
				else
				{
					job->performJob();
					if (job->m_bSuccessful) {
						dtuTextureName = sReEncodedFilename;
					}
					else {
						dzApp->log("ERROR: saving file failed: " + sReEncodedFilename);
					}

				}

			}
			///////////////////////
		}

		// DB 2024-12-16: NB: dtuTexture is initialized to be used as a proxy for TextureName above.  it may be redirected
		// from TextureNme by above rescaling/re-encoding operations.  Therefore, dtueTextureName defines the current active
		// instance of TextureName after any image processing operation.
		if (bUseReEncodedFilename == false)
		{
			if (m_bExportAllTextures)
			{
				// use dtuTexture as source instead of TextureName since TextureName may already be redirected by above operations
				dtuTextureName = exportAssetWithDtu(dtuTextureName, Node->getLabel() + "_" + Material->getName());
			}
			else if (m_bUseRelativePaths)
			{
				// use dtuTexture as source instead of TextureName since TextureName may already be redirected by above operations
				dtuTextureName = dzApp->getContentMgr()->getRelativePath(dtuTextureName, true);
			}
			else if (isTemporaryFile(dtuTextureName))
			{
				// use dtuTexture as source instead of TextureName since TextureName may already be redirected by above operations
				dtuTextureName = exportAssetWithDtu(dtuTextureName, Node->getLabel() + "_" + Material->getName());
			}
		}
		// This command maps the original TextureName to the current active redirected image filepath
		m_mapProcessedFiles.insert(TextureName.toLower(), dtuTextureName);

	}
	else if (!TextureName.isEmpty())
	{
		dtuTextureName = m_mapProcessedFiles.value(TextureName.toLower());
	}

	return dtuTextureName;

}

bool DzBridgeAction::writeSceneDefinition(DzJsonWriter& Writer, DzNode* RootNode)
{
	Writer.startMemberArray("SceneDefinition", true);
	QMap<QString, DzMatrix3> WritingInstances;
	QList<DzGeometry*> ExportedGeometry;

	DzNodeList aNodeStack;
	if (RootNode == NULL) {
		aNodeStack = BuildRootNodeList();
	} else {
		aNodeStack.append(RootNode);
	}

	while (aNodeStack.isEmpty() == false)
	{
		// depth first
		DzNode* Node = aNodeStack.last();
		aNodeStack.removeLast();

		writeSceneDefinitionNode(Node, Writer);
		for (int nChildIndex = 0; nChildIndex < Node->getNumNodeChildren(); nChildIndex++)
		{
			DzNode* pChildNode = Node->getNodeChild(nChildIndex);
			aNodeStack.append(pChildNode);
		}
	}
	Writer.finishArray();

	return true;
}

bool DzBridgeAction::writeSceneDefinitionNode(DzNode* Node, DzJsonWriter& Writer)
{
	QString SceneAssetUri = Node->getAssetUri().toString();
	DzNode* pNodeParent = Node->getNodeParent();
	QString sParentAssetUri = "";
	if (pNodeParent) {
		sParentAssetUri = pNodeParent->getAssetUri().toString();
	}

	Writer.startObject(true);
	Writer.addMember("Version", 4);
	Writer.addMember("StudioNodeName", Node->getName());
	Writer.addMember("StudioNodeLabel", Node->getLabel());
	Writer.addMember("StudioSceneID", SceneAssetUri);
	Writer.addMember("ClassName", Node->className());
	Writer.addMember("ParentNodeSceneID", sParentAssetUri);

	//Writer.addMember("TranslationX", Node->getWSPos().m_x);
	//Writer.addMember("TranslationY", Node->getWSPos().m_y);
	//Writer.addMember("TranslationZ", Node->getWSPos().m_z);
	//DzQuat RotationQuat = Node->getWSRot();
	//DzVec3 Rotation;
	//RotationQuat.getValue(Node->getRotationOrder(), Rotation);
	//Writer.addMember("RotationX", Rotation.m_x);
	//Writer.addMember("RotationY", Rotation.m_y);
	//Writer.addMember("RotationZ", Rotation.m_z);
	//DzMatrix3 Scale = Node->getWSScale();
	//Writer.addMember("ScaleX", Scale.row(0).length());
	//Writer.addMember("ScaleY", Scale.row(1).length());
	//Writer.addMember("ScaleZ", Scale.row(2).length());

	if (Node->inherits("DzInstanceNode"))
	{
		QString sTargetSceneAssetUri = "";
		DzNode* pNodeTarget = qobject_cast<DzInstanceNode*>(Node)->getTarget();
		if (pNodeTarget != NULL)
		{
			sTargetSceneAssetUri = pNodeTarget->getAssetUri().toString();
		}
		Writer.addMember("TargetSceneID", sTargetSceneAssetUri);
	}
	Writer.finishObject();

	return true;
}



#include "moc_DzBridgeAction.cpp"
