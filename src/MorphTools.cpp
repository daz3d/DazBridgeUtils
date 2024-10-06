#define USE_DAZ_LOG 1

#include "MorphTools.h"

#include <QtCore/qobject.h>
#include "dznode.h"
#include "dzobject.h"
#include "dzpresentation.h"
#include "dzmodifier.h"
#include "dzmorph.h"
#include "dznumericproperty.h"
#include "dzerclink.h"
#include "dzfacetmesh.h"
#include "dzvertexmesh.h"
#include "dzfigure.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzscene.h"
#include "dzaction.h"
#include "dzfloatproperty.h"
#include "dzscript.h"
#include "dzshape.h"
#include "dzenumproperty.h"
#include "dzbone.h"

#if USE_DAZ_LOG
#include <dzapp.h>	
#endif

void MorphInfo::log(QString message)
{
#if USE_DAZ_LOG
	dzApp->log(message);
#else
	printf(message.toUtf8().constData());
#endif
}

QString MorphInfo::getMorphPropertyName(DzProperty* pMorphProperty)
{
	if (pMorphProperty == nullptr)
	{
		// issue error message or alternatively: throw exception
		printf("ERROR: DazBridge: DzBridgeMorphSelectionDialog.cpp, getPropertyName(): nullptr passed as argument.");
		return "";
	}
	QString sPropertyName = pMorphProperty->getName();
	auto owner = pMorphProperty->getOwner();
	if (owner && owner->inherits("DzMorph"))
	{
		sPropertyName = owner->getName();
	}
	return sPropertyName;
}

bool MorphInfo::isValidMorph(DzProperty* pMorphProperty)
{
	if (pMorphProperty == nullptr)
	{
		// issue error message or alternatively: throw exception
		dzApp->log("ERROR: DazBridge: DzBridgeMorphSelectionDialog.cpp, isValidMorph(): nullptr passed as argument.");
		return false;
	}
	QString sMorphName = getMorphPropertyName(pMorphProperty);
	QStringList ignoreConditionList;
	ignoreConditionList += "x"; ignoreConditionList += "y"; ignoreConditionList += "z";
	for (auto ignoreCondition : ignoreConditionList)
	{
		if (sMorphName.toLower()[0] == ignoreCondition[0])
			return false;
	}
	for (auto iterator = pMorphProperty->controllerListIterator(); iterator.hasNext(); )
	{
		DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
		if (ercLink == nullptr)
			continue;
		if (ercLink->getType() == 3) // Multiply
		{
			auto controllerProperty = ercLink->getProperty();
			if (controllerProperty && controllerProperty->getDoubleValue() == 0)
				return false;
		}
	}
	return true;
}

bool MorphInfo::hasErcLink()
{
	DzProperty* morphProperty = this->Property;
	// DB (2022-Sept-26): crashfix
	if (morphProperty == nullptr)
		return false;
	
	// DB, 2022-June-07: NOTE: using iterator may be more efficient due to potentially large number of controllers
	for (auto iterator = morphProperty->controllerListIterator(); iterator.hasNext(); )
	{
		DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
		if (ercLink == nullptr)
			continue;

		auto controllerProperty = ercLink->getProperty();
		if (controllerProperty)
		{
			QString sControllerName = MorphInfo::getMorphPropertyName(controllerProperty);
			if (sControllerName.isEmpty() == false)
			{
				return true;
			}
		}
	}

	return false;
}

int MorphInfo::getNumErcLinks()
{
	int numERCLinks = 0;

	DzProperty* morphProperty = this->Property;
	if (morphProperty == nullptr)
		return -1;

	if (m_ErcList == nullptr)
	{
		m_ErcList = getErcList();
	}
	numERCLinks = m_ErcList->count();

	return numERCLinks;
}

DzProperty* MorphInfo::getErcController(int ercIndex)
{
	DzProperty* pErcControllerProperty = nullptr;

	DzProperty* morphProperty = this->Property;
	if (morphProperty == nullptr)
		return nullptr;

	int ercCount = 0;

	if (m_ErcList == nullptr)
	{
		m_ErcList = getErcList();
	}
	pErcControllerProperty = m_ErcList->at(ercIndex);

	return pErcControllerProperty;
}

QList<DzProperty*>* MorphInfo::getErcList(bool bGetMorphs, bool bGetPoses)
{
	QList<DzProperty*> *ercList = new QList<DzProperty*>();

	DzProperty* morphProperty = this->Property;
	if (morphProperty == nullptr)
		return ercList;

	QList<DzProperty*> toDoList;

//	for (auto iterator = morphProperty->controllerListIterator(); iterator.hasNext(); )
	for (auto iterator = morphProperty->slaveControllerListIterator(); iterator.hasNext(); )
	{
		DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
		if (ercLink == nullptr)
			continue;

//		auto controllerProperty = ercLink->getProperty();
		auto controllerProperty = ercLink->getOwner();
		if (controllerProperty)
		{
			DzElement* owner = controllerProperty->getOwner();
			if (owner && ercList->contains(controllerProperty) == false)
			{
				ercList->append(controllerProperty);
			//	if (
			//		(bGetMorphs && owner->inherits("DzMorph")) ||
			//		(bGetPoses && owner->inherits("DzBone"))
			//		)
			//		{
			//			log("DEBUG: MorphInfo::getErcList(): Adding controller: " + controllerProperty->getName() + ", class=" + owner->className() + "\n");
			//			ercList->append(controllerProperty);
			//		}
			//	else
			//	{
			//		log("DEBUG: MorphInfo::getErcList(): skipping controller: " + controllerProperty->getName() + ", class=" + owner->className() + "\n");
			//	}

				if (controllerProperty->hasSlaveControllers() && toDoList.contains(controllerProperty) == false)
				{
					QString propertyName = controllerProperty->getOwner()->getName();
					MorphInfo::log("adding controller to todolist: " + propertyName);
					toDoList.append(controllerProperty);
				}
			}

		}
	}

	while (toDoList.isEmpty() == false)
	{
		DzProperty* morphProperty = toDoList.takeFirst();
		for (auto iterator = morphProperty->slaveControllerListIterator(); iterator.hasNext(); )
		{
			DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
			if (ercLink == nullptr)
				continue;

			auto controllerProperty = ercLink->getOwner();
			if (controllerProperty)
			{
				DzElement* owner = controllerProperty->getOwner();
				if (owner && ercList->contains(controllerProperty) == false)
				{
					ercList->append(controllerProperty);

					if (controllerProperty->hasSlaveControllers() && toDoList.contains(controllerProperty) == false)
					{
						QString propertyName = controllerProperty->getOwner()->getName();
						MorphInfo::log("adding controller to todolist: " + propertyName);
						toDoList.append(controllerProperty);
					}
				}
			}
		}
	}

	return ercList;
}

bool MorphInfo::hasMorphErc()
{
	if (m_ErcList == nullptr)
	{
		m_ErcList = getErcList();
	}

	for (DzProperty* erc : *m_ErcList)
	{
		DzElement *owner = erc->getOwner();
		if (owner && owner->inherits("DzMorph"))
			return true;
	}

	return false;
}

bool MorphInfo::hasPoseErc()
{
	if (m_ErcList == nullptr)
	{
		m_ErcList = getErcList();
	}

	for (DzProperty* erc : *m_ErcList)
	{
		QString propName = erc->getName();
		DzElement *owner = erc->getOwner();
//		if (owner && owner->inherits("DzBone") && propName.contains("rotate", Qt::CaseInsensitive))
		if (owner && owner->inherits("DzBone") )
		{
			return true;
		}
	}

	return false;
}


// Return list of Morphs to disable if the morph has a controller that is also being exported
QList<QString> getMorphNamesToDisconnectList(QList<MorphInfo> m_morphsToExport_finalized)
{
	QList<QString> morphsToDisconnect;

	foreach(MorphInfo exportMorph, m_morphsToExport_finalized)
	{
		DzProperty* morphProperty = exportMorph.Property;
		// DB (2022-Sept-26): crashfix
		if (morphProperty == nullptr)
			continue;

		// DB, 2022-June-07: NOTE: using iterator may be more efficient due to potentially large number of controllers
		for (auto iterator = morphProperty->controllerListIterator(); iterator.hasNext(); )
		{
			DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
			if (ercLink == nullptr)
				continue;
			auto controllerProperty = ercLink->getProperty();
			QString sControllerName = MorphInfo::getMorphPropertyName(controllerProperty);
			// iterate through each exported morph
			foreach(MorphInfo compareMorph, m_morphsToExport_finalized)
			{
				if (compareMorph.Name == sControllerName)
				{
					morphsToDisconnect.append(exportMorph.Name);
					break;
				}
			}
		}
	}

	return morphsToDisconnect;
}


// DB 2023-11-02: TODO: Fix method so that it is not dependent on member variables being set by readGUI() --
// in other words, it should be able to function prior to readGUI() being called or being completed.
bool checkForIrreversibleOperations_in_disconnectOverrideControllers(DzNode* Selection, QList<MorphInfo> )
{
	if (Selection == nullptr)
		return false;

	//QList<QString> m_ControllersToDisconnect = getMorphNamesToDisconnectList(m_mMorphNameToLabel.keys());

	//DzNumericProperty* previousProperty = nullptr;
	//for (int index = 0; index < Selection->getNumProperties(); index++)
	//{
	//	DzProperty* property = Selection->getProperty(index);
	//	DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
	//	if (numericProperty && !numericProperty->isOverridingControllers())
	//	{
	//		QString propName = property->getName();
	//		if (m_mMorphNameToLabel.contains(propName) && m_ControllersToDisconnect.contains(propName))
	//		{
	//			double propValue = numericProperty->getDoubleValue();
	//			if (propValue != 0)
	//			{
	//				return true;
	//			}
	//		}
	//	}
	//}

	//DzObject* Object = Selection->getObject();
	//if (Object)
	//{
	//	for (int index = 0; index < Object->getNumModifiers(); index++)
	//	{
	//		DzModifier* modifier = Object->getModifier(index);
	//		DzMorph* mod = qobject_cast<DzMorph*>(modifier);
	//		if (mod)
	//		{
	//			for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
	//			{
	//				DzProperty* property = modifier->getProperty(propindex);
	//				DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
	//				if (numericProperty && !numericProperty->isOverridingControllers())
	//				{
	//					QString propName = DzBridgeMorphSelectionDialog::getMorphPropertyName(property);
	//					if (m_mMorphNameToLabel.contains(modifier->getName()) && m_ControllersToDisconnect.contains(modifier->getName()))
	//					{
	//						double propValue = numericProperty->getDoubleValue();
	//						if (propValue != 0)
	//						{
	//							return true;
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	return false;
}


QMap<QString, MorphInfo> MorphTools::enumerateMorphInfoTable(DzNode* Node)
{
	QMap<QString, MorphInfo> m_morphInfoMap;

	DzObject* Object = Node->getObject();
//	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		QString propName = property->getName();
		QString propLabel = property->getLabel();
		DzPresentation* presentation = property->getPresentation();
		if (presentation)
		{
			MorphInfo morphInfo;
			morphInfo.Name = propName;
			morphInfo.Label = propLabel;
			morphInfo.Path = Node->getLabel() + "/" + property->getPath();
			morphInfo.Type = presentation->getType();
			morphInfo.Property = property;
			morphInfo.Node = Node;
			if (!m_morphInfoMap.contains(morphInfo.Name))
			{
				m_morphInfoMap.insert(morphInfo.Name, morphInfo);
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
						MorphInfo morphInfoProp;
						morphInfoProp.Name = modName;
						morphInfoProp.Label = propLabel;
						morphInfoProp.Path = Node->getLabel() + "/" + property->getPath();
						morphInfoProp.Type = presentation->getType();
						morphInfoProp.Property = property;
						morphInfoProp.Node = Node;
						if (!m_morphInfoMap.contains(morphInfoProp.Name))
						{
							m_morphInfoMap.insert(morphInfoProp.Name, morphInfoProp);
						}
					}
				}

			}

		}
	}

	return m_morphInfoMap;
}

void MorphTools::bakePoseMorphPerNode(DzFloatProperty* morphProperty, DzNode* node, QString newMorphName)
{
	if (node == nullptr)
		return;
	DzObject* Object = node->getObject();
	if (Object == nullptr)
		return;

	// apply recursively to children
	for (int index = 0; index < node->getNumNodeChildren(); index++)
	{
		DzNode* childNode = node->getNodeChild(index);
		bakePoseMorphPerNode(morphProperty, childNode, newMorphName);
	}

	int origResolution = setMeshResolution(node, 0);

	float zero = morphProperty->getDefaultValue();
	float max = morphProperty->getMax();

	morphProperty->setValue(max);
	Object->forceCacheUpdate(node);

	DzVertexMesh* DualQuaternionMesh = Object->getCachedGeom();
	DzFacetMesh* CachedDualQuaternionMesh = new DzFacetMesh();
	CachedDualQuaternionMesh->copyFrom(DualQuaternionMesh, false, false);

	morphProperty->setValue(zero);
	Object->forceCacheUpdate(node);

	if (newMorphName.isEmpty())
	{
		newMorphName = MorphInfo::getMorphPropertyName(morphProperty) + "_baked";
	}
	createMorph(newMorphName, CachedDualQuaternionMesh, node);

	setMeshResolution(node, origResolution);

}

QString MorphTools::bakePoseMorph(DzFloatProperty* morphProperty, QString newMorphName)
{
	DzNode* Selection = dzScene->getPrimarySelection();
	if (Selection == nullptr)
		return "";

	// if selection inherits dzfigure, then recast to dzfigure and zero out the pose
	DzActionMgr *actionMgr = dzApp->getInterface()->getActionMgr();
	DzAction *restoreAction = actionMgr->findAction("DzRestoreFigurePoseAction");
	if (restoreAction)
	{
		restoreAction->trigger();
	}

	if (newMorphName.isEmpty())
	{
		QString newMorphName = MorphInfo::getMorphPropertyName(morphProperty) + "_baked";
	}
	bakePoseMorphPerNode(morphProperty, Selection, newMorphName);
	return newMorphName;
}

void MorphTools::createMorph(const QString NewMorphName, DzVertexMesh* Mesh, DzNode* Node)
{
	DzScript* Script = new DzScript();

	Script->addLine("function myFunction(oNode, oSavedGeom, sName) {");
	Script->addLine("var oMorphLoader = new DzMorphLoader();");
	Script->addLine("oMorphLoader.setMorphName(sName);");
	Script->addLine("oMorphLoader.setDeltaTolerance(0.01);");
//	Script->addLine("oMorphLoader.setCreateControlProperty(true);");
	Script->addLine("oMorphLoader.setPropertyGroupPath(\"Morphs/Morph Loader\");");
	Script->addLine("oMorphLoader.setReverseDeformations(true);");
//	Script->addLine("oMorphLoader.setReverseDeformations(false);");
	Script->addLine("oMorphLoader.setOverwriteExisting(DzMorphLoader.MakeUnique);");
	Script->addLine("oMorphLoader.setCleanUpOrphans(true);");
	Script->addLine("oMorphLoader.setMorphMirroring(DzMorphLoader.DoNotMirror);");
	Script->addLine("var result = oMorphLoader.createMorphFromMesh(oSavedGeom, oNode);");
	Script->addLine("App.log(\"RESULT: \" + result); ");
	Script->addLine("};");

	QVariantList Args;
	QVariant v(QMetaType::QObjectStar, &Node);
	Args.append(QVariant(QMetaType::QObjectStar, &Node));
	Args.append(QVariant(QMetaType::QObjectStar, &Mesh));
	Args.append(QVariant(NewMorphName));
	Script->call("myFunction", Args);
}

int MorphTools::setMeshResolution(DzNode* node, int desiredResolutionIndex)
{
	if (node == nullptr)
		return -1;
	DzObject* Object = node->getObject();

	// Set the resolution level to Base
	int ResolutionLevel = 1; // High Resolution
	DzEnumProperty* ResolutionLevelProperty = NULL;
	if (Object && Object->getCurrentShape())
	{
		DzShape* Shape = Object->getCurrentShape();
		if (DzProperty* Property = Shape->findProperty("lodlevel"))
		{
			if (ResolutionLevelProperty = qobject_cast<DzEnumProperty*>(Property))
			{
				ResolutionLevel = ResolutionLevelProperty->getValue();
				ResolutionLevelProperty->setValue(0); // Base
			}
		}
	}

	return ResolutionLevel;
}

QStringList MorphTools::getCombinedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled)
{
	int debug_num_morphs = m_morphsToExport.count();

	if (bAutoJCMEnabled)
	{
		AddActiveJointControlledMorphs(m_morphsToExport, availableMorphsTable, bAutoJCMEnabled);
	}

	int debug_num_morphs_added = m_morphsToExport.count() - debug_num_morphs;

	return m_morphsToExport;
}

QStringList MorphTools::getFinalizedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled)
{
	int debug_num_morphs = m_morphsToExport.count();

	QStringList combinedList = getCombinedMorphList(m_morphsToExport, availableMorphsTable, bAutoJCMEnabled);

	int debug_num_morphs_added = combinedList.count() - debug_num_morphs;

	if (combinedList.count() == 0)
	{
		return combinedList;
	}

	QStringList morphNamesToExport;
	foreach(QString morphName, combinedList)
	{
		QString sExportName = morphName;
		QString sCorrectedKey = QString(morphName).replace("export____", "");
		if (availableMorphsTable.contains(sCorrectedKey))
		{
			MorphInfo morphInfo = availableMorphsTable[sCorrectedKey];
			if (morphInfo.ExportString.isEmpty() == false)
			{
				sExportName = morphInfo.ExportString;
			}
			morphNamesToExport.append(sExportName);
		}
		else
		{
			dzApp->log("ERROR: Morph To Export was not found in available Morphs: " + morphName);
		}
	}

	int debug_num_morphs_final = morphNamesToExport.count();

	return morphNamesToExport;
}

// Get the morph string (aka m_morphsToExport_finalized) in the format for the Daz FBX Export
QString MorphTools::getMorphString(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled)
{
	QStringList morphNamesToExport = getFinalizedMorphList(m_morphsToExport, availableMorphsTable, bAutoJCMEnabled);
	if (morphNamesToExport.count() == 0)
		return "";
	QString morphString = morphNamesToExport.join("\n1\n");
	morphString += "\n1\n.CTRLVS\n2\nAnything\n0";
	return morphString;
}

// Recursive function for finding all active JCM morphs for a node
QList<JointLinkInfo> MorphTools::GetActiveJointControlledMorphs(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node)
{
	QList<JointLinkInfo> returnMorphs;
	if (bAutoJCMEnabled)
	{
		if (Node == nullptr)
		{
			Node = dzScene->getPrimarySelection();

			// For items like clothing, create the morph list from the character
			DzNode* ParentFigureNode = Node;
			while (ParentFigureNode->getNodeParent())
			{
				ParentFigureNode = ParentFigureNode->getNodeParent();
				if (DzSkeleton* Skeleton = ParentFigureNode->getSkeleton())
				{
					if (DzFigure* Figure = qobject_cast<DzFigure*>(Skeleton))
					{
						Node = ParentFigureNode;
						break;
					}
				}
			}
		}

		DzObject* Object = Node->getObject();
//		DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

		for (int index = 0; index < Node->getNumProperties(); index++)
		{
			DzProperty* property = Node->getProperty(index);
			returnMorphs.append(GetJointControlledMorphInfo(property, m_morphsToExport, availableMorphsTable));
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
						returnMorphs.append(GetJointControlledMorphInfo(property, m_morphsToExport, availableMorphsTable));
					}

				}

			}
		}

	}

	return returnMorphs;
}

QList<JointLinkInfo> MorphTools::GetJointControlledMorphInfo(DzProperty* property, QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable)
{
	QList<JointLinkInfo> returnMorphs;

	QString propName = property->getName();
	QString propLabel = property->getLabel();
	DzPresentation* presentation = property->getPresentation();
	// DB 2023-Sep-20: 
	// This code prematurely filters out morphs based on their categorization.  However, it assumes that the categorization
	// is always correct.  Work-around to account for miscategorized DzMorphs with ERC Links: just filter specifically by
	// ERC bone link presence and ignore presentation type altogether.
	// TODO: consider filtering property Owner by DzMorph inheritance, but this may also prematurely exclude some JCMs
//	if (presentation && presentation->getType() == "Modifier/Corrective")
	if (true)
	{
		QString linkLabel;
		QString linkDescription;
		QString linkBone;
		QString linkAxis;
		QString linkBodyType;
		double bodyStrength = 0.0f;
		double currentBodyScalar = 0.0f;
		double linkScalar = 0.0f;
		bool isJCM = false;
		bool isBaseJCM = false;
		QList<double> keys;
		QList<double> keysValues;
		QList<JointLinkKey> linkKeys;

		for (int ControllerIndex = 0; ControllerIndex < property->getNumControllers(); ControllerIndex++)
		{
			DzController* controller = property->getController(ControllerIndex);

			DzERCLink* link = qobject_cast<DzERCLink*>(controller);
			if (link)
			{
				double value = link->getScalar();
				QString linkProperty = link->getProperty()->getName();
				QString linkObject = link->getProperty()->getOwner()->getName();
				double currentValue = link->getProperty()->getDoubleValue();

				DzBone* bone = qobject_cast<DzBone*>(link->getProperty()->getOwner());
				if (bone)
				{
					linkLabel = propLabel;
					linkDescription = controller->description();
					linkBone = linkObject;
					linkAxis = linkProperty;
					linkScalar = value;
					isJCM = true;

					if (link->getType() == 6)
					{
						for (int keyIndex = 0; keyIndex < link->getNumKeyValues(); keyIndex++)
						{
							JointLinkKey newKey;
							newKey.Angle = link->getKey(keyIndex);
							newKey.Value = link->getKeyValue(keyIndex);
							linkKeys.append(newKey);
							keys.append(link->getKey(keyIndex));
							keysValues.append(link->getKeyValue(keyIndex));
						}
					}
				}
				else
				{
					linkBodyType = linkObject;
					bodyStrength = value;
					currentBodyScalar = currentValue;
					if (linkProperty == "body_ctrl_basejointcorrectives" ||
						linkProperty == "BaseJointCorrectives")
					{
						isBaseJCM = true;
					}
				}
			}
		}

		if (isJCM && currentBodyScalar > 0.0f)
		{
			JointLinkInfo linkInfo;
			linkInfo.Bone = linkBone;
			linkInfo.Axis = linkAxis;
			linkInfo.MorphName = linkLabel;
			linkInfo.Scalar = linkScalar;
			linkInfo.Alpha = currentBodyScalar;
			linkInfo.Keys = linkKeys;
			linkInfo.IsBaseJCM = isBaseJCM;

			//if (m_morphInfoMap.contains(linkLabel))
			//{
			//	linkInfo.LinkMorphInfo = m_morphInfoMap[linkLabel];
			//}

			bool bMorphInfoFound = false;
			QString sCorrectedKey = QString(linkLabel).replace("export____", "");

			if (availableMorphsTable.contains(linkLabel))
			{
				linkInfo.LinkMorphInfo = availableMorphsTable[linkLabel];
				bMorphInfoFound = true;
			}
			else if (availableMorphsTable.contains(sCorrectedKey))
			{
				linkInfo.LinkMorphInfo = availableMorphsTable[sCorrectedKey];
				bMorphInfoFound = true;
			}

			if (bMorphInfoFound == false)
			{
				dzApp->log("ERROR! MorphInfo for " + linkLabel + " was not found!");
			}

			//qDebug() << "Label " << linkLabel << " Description " << linkDescription << " Bone " << linkBone << " Axis " << linkAxis << " Alpha " << currentBodyScalar << " Scalar " << linkScalar;
			if (!keys.isEmpty())
			{
				foreach(double key, keys)
				{
					//qDebug() << key;
				}

				foreach(double key, keysValues)
				{
					//qDebug() << key;
				}

			}

			returnMorphs.append(linkInfo);

		}
	}
	return returnMorphs;
}

void MorphTools::AddActiveJointControlledMorphs(QList<QString> &m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node)
{
	QList<JointLinkInfo> activeJCMs = GetActiveJointControlledMorphs(m_morphsToExport, availableMorphsTable, bAutoJCMEnabled, Node);

	for (JointLinkInfo linkInfo : activeJCMs)
	{
		QString linkLabel = linkInfo.MorphName;
		MorphInfo morphInfo = linkInfo.LinkMorphInfo;

		QString sCorrectedKey = QString(linkLabel).replace("export____", "");
		if (availableMorphsTable.contains(sCorrectedKey) && !m_morphsToExport.contains(linkLabel))
		{
			m_morphsToExport.append(linkLabel);
		}

		// repeat for dq2lb
		linkLabel += "_dq2lb";
		sCorrectedKey = QString(linkLabel).replace("export____", "");
		if (availableMorphsTable.contains(sCorrectedKey) && !m_morphsToExport.contains(linkLabel))
		{
			m_morphsToExport.append(linkLabel);
		}

	}

}

// DB, 2023-11-02: This may be an unused method.  TODO: re-evaluate the usage of this method by other projects.
QStringList MorphTools::getAvailableMorphNames(DzNode* Node)
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

// DB 2024-06-07: need available morph table independent of GUI
// NOTE: User must free table after done using
QMap<QString, MorphInfo>* MorphTools::getAvailableMorphs(DzNode* Node)
{
	// Build morphinfo table to return
	QMap<QString, MorphInfo>* pMorphInfoTable = new QMap<QString, MorphInfo>();

	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		QString propName = property->getName();
		QString propLabel = property->getLabel();
		DzPresentation* presentation = property->getPresentation();
		if (presentation)
		{
			MorphInfo morphInfo;
			morphInfo.Name = propName;
			morphInfo.Label = propLabel;
			morphInfo.Path = Node->getLabel() + "/" + property->getPath();
			morphInfo.Type = presentation->getType();
			morphInfo.Property = property;
			morphInfo.Node = Node;
			if (!pMorphInfoTable->contains(morphInfo.Name))
			{
				pMorphInfoTable->insert(morphInfo.Name, morphInfo);
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
						MorphInfo morphInfoProp;
						morphInfoProp.Name = modName;
						morphInfoProp.Label = propLabel;
						morphInfoProp.Path = Node->getLabel() + "/" + property->getPath();
						morphInfoProp.Type = presentation->getType();
						morphInfoProp.Property = property;
						morphInfoProp.Node = Node;
						if (!pMorphInfoTable->contains(morphInfoProp.Name))
						{
							pMorphInfoTable->insert(morphInfoProp.Name, morphInfoProp);
						}
					}
				}

			}

		}
	}

	return pMorphInfoTable;
}

void MorphTools::safeDeleteMorphInfoTable(QMap<QString, MorphInfo>* pMorphInfoTable)
{
	foreach(MorphInfo m, pMorphInfoTable->values())
	{
//		delete(&m);
	}
	pMorphInfoTable->clear();

	return;
}

