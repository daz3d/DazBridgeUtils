#pragma once

#include <qstring.h>
#include <qmap.h>

class DzProperty;
class DzNode;
class DzVertexMesh;
class DzFloatProperty;

// Data structure containing DzProperty-DzNode information
class MorphInfo 
{
public:
	QString Name;
	QString Label;
	QString Type;
	QString Path;
	DzProperty* Property;
	DzNode* Node;

	QList<DzProperty*>* m_ErcList;

	MorphInfo()
	{
		Name = QString();
		Label = QString();
		Type = QString();
		Path = QString();
		Property = nullptr;
		Node = nullptr;
		m_ErcList = nullptr;
	}

	static QString getMorphPropertyName(DzProperty* pMorphProperty);
	static bool isValidMorph(DzProperty* pMorphProperty);
	static void log(QString message);
	bool hasErcLink();
	int getNumErcLinks();
	DzProperty* getErcController(int ercIndex);
	QList<DzProperty*>* getErcList(bool bGetMorphs=true, bool bGetPoses=true);
	bool hasMorphErc();
	bool hasPoseErc();

	inline bool operator==(MorphInfo other)
	{
		if (Name == other.Name)
		{
			return true;
		}
		return false;
	}
};

QMap<QString, MorphInfo> enumerateMorphInfoMap(DzNode* Node);
void createMorph(const QString NewMorphName, DzVertexMesh* Mesh, DzNode* Node);
QString bakePoseMorph(DzFloatProperty* morphProperty);
int setMeshResolution(DzNode* node, int desiredResolutionIndex);
void bakePoseMorphPerNode(DzFloatProperty* morphProperty, DzNode* node);
