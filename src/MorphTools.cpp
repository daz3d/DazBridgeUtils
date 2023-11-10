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

#if USE_DAZ_LOG
#include <dzapp.h>	
#endif

void MorphInfo::log(QString message)
{
#if USE_DAZ_LOG
	dzApp->log(message);
#else
	printf(message.toLocal8Bit().constData());
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


QMap<QString, MorphInfo> enumerateMorphInfoMap(DzNode* Node)
{
	QMap<QString, MorphInfo> m_morphInfoMap;

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

void bakePoseMorphPerNode(DzFloatProperty* morphProperty, DzNode* node)
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
		bakePoseMorphPerNode(morphProperty, childNode);
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

	QString newMorphName = MorphInfo::getMorphPropertyName(morphProperty) + "_baked";
	createMorph(newMorphName, CachedDualQuaternionMesh, node);

	setMeshResolution(node, origResolution);

}


QString bakePoseMorph(DzFloatProperty* morphProperty)
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

	bakePoseMorphPerNode(morphProperty, Selection);
	QString newMorphName = MorphInfo::getMorphPropertyName(morphProperty) + "_baked";
	return newMorphName;
}

void createMorph(const QString NewMorphName, DzVertexMesh* Mesh, DzNode* Node)
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

int setMeshResolution(DzNode* node, int desiredResolutionIndex)
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

// Get the morph string (aka m_morphsToExport_finalized) in the format for the Daz FBX Export
QString getMorphString(QList<MorphInfo> m_morphsToExport)
{

	//if (IsAutoJCMEnabled())
	//{
	//	AddActiveJointControlledMorphs();
	//}

//	QList<JointLinkInfo> jointLinks = GetActiveJointControlledMorphs();
	QList<int> jointLinks;

	if (m_morphsToExport.length() == 0 && jointLinks.length() == 0)
	{
		return "";
	}
	QStringList morphNamesToExport;
	foreach(MorphInfo exportMorph, m_morphsToExport)
	{
		QString sExportName = exportMorph.Name;
		if (exportMorph.ExportString.isEmpty() == false)
		{
			sExportName = exportMorph.ExportString;
		}
		morphNamesToExport.append(sExportName);
	}
	//foreach(JointLinkInfo jointLink, jointLinks)
	//{
	//	morphNamesToExport.append(jointLink.Morph);
	//	morphNamesToExport.append(jointLink.Morph + "_dq2lb");
	//}
	QString morphString = morphNamesToExport.join("\n1\n");
	morphString += "\n1\n.CTRLVS\n2\nAnything\n0";
	return morphString;
}