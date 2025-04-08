#pragma once

#define MORPH_EXPORT_PREFIX "export____"

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
	QString ExportString;
	QString Name;
	QString Label;
	QString Type;
	QString Path;
	DzProperty* Property;
	DzNode* Node;

	QList<DzProperty*>* m_ErcList;

	MorphInfo()
	{
		ExportString = QString();
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

struct JointLinkKey
{
	int Angle;
	int Value;
};

struct JointLinkInfo
{
	QString Bone;
	QString Axis;
	QString MorphName;
	double Scalar;
	double Alpha;
	bool IsBaseJCM = false;
	MorphInfo LinkMorphInfo;
	QList<JointLinkKey> Keys;
};

class MorphExportSettings
{
public:
	QList<MorphInfo> m_MorphNamesToExport;
	bool m_bDoubleDipping;
	bool m_bAutoJCM;
	bool m_bBakeFaceMorphs;
};

class MorphTools
{
public:
	static void createMorph(const QString NewMorphName, DzVertexMesh* Mesh, DzNode* Node);
	static QString bakePoseMorph(DzFloatProperty* morphProperty, QString);
	static int setMeshResolution(DzNode* node, int desiredResolutionIndex);
	static void bakePoseMorphPerNode(DzFloatProperty* morphProperty, DzNode* node, QString);

	static QMap<QString, MorphInfo> enumerateMorphInfoTable(DzNode* Node);
	static QString getMorphString(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);
	static QStringList getAvailableMorphNames(DzNode* Node);
	static QStringList getFinalizedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);
	static QStringList getCombinedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);

	// DB 2024-06-07: get available morphs independent of GUI
	static QMap<QString, MorphInfo>* getAvailableMorphs(DzNode* Node);
	static void safeDeleteMorphInfoTable(QMap<QString, MorphInfo>*);

private:
	static void AddActiveJointControlledMorphs(QList<QString> &m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node = nullptr);
	static QList<JointLinkInfo> GetActiveJointControlledMorphs(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node = nullptr);
	static QList<JointLinkInfo> GetJointControlledMorphInfo(DzProperty* property, QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable);

};
