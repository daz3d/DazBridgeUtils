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

	QList<DzProperty*>* m_PrimaryErcList;
	QList<DzProperty*>* m_SecondaryErcList;

	MorphInfo()
	{
		ExportString = QString();
		Name = QString();
		Label = QString();
		Type = QString();
		Path = QString();
		Property = nullptr;
		Node = nullptr;
		m_PrimaryErcList = nullptr;
		m_SecondaryErcList = nullptr;
	}

	static void log(QString message);
	bool hasErcLink();
	int getNumErcLinks();
	DzProperty* getErcController(int ercIndex);
	QList<DzProperty*>* getErcList(bool bGetMorphs=true, bool bGetPoses=true, bool bPrimaryErcOnly=false, bool bSecondaryErcOnly=false, bool bAllNonPrimaryDescendants=false);
	QList<DzProperty*>* getPrimaryErcList() { return getErcList(true, true, true); };
	QList<DzProperty*>* getSecondaryErcList() { return getErcList(true, true, false, true); };
	bool hasMorphErc();
	bool hasPoseErc();
	bool hasPoseData();

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
	static QString GetModifierName(DzProperty* pModifierProperty);
	static QString GetMorphPropertyName(DzProperty* pMorphProperty);
	static bool IsValidMorph(DzProperty* pMorphProperty);

	static void createMorph(const QString NewMorphName, DzVertexMesh* Mesh, DzNode* Node);
	static QString bakePoseMorph(DzFloatProperty* morphProperty, QString);
	static int setMeshResolution(DzNode* node, int desiredResolutionIndex);
	static void bakePoseMorphPerNode(DzFloatProperty* morphProperty, DzNode* node, QString);

	static QMap<QString, MorphInfo> enumerateMorphInfoTable(DzNode* Node);
	static QString getMorphString(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);
	static QString GetMorphString(QList<QString> aMorphsToExport, DzNode* pNode, bool bAutoJCMEnabled=false);
	static QStringList getAvailableMorphNames(DzNode* Node);
	static QStringList getFinalizedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);
	static QStringList getCombinedMorphList(QList<QString> m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled);

	// DB 2024-06-07: get available morphs independent of GUI
	static QMap<QString, MorphInfo>* getAvailableMorphs(DzNode* Node);
	static void safeDeleteMorphInfoTable(QMap<QString, MorphInfo>*);
	// 2025-04-24, DB: hassle-free version of getAavailableMorphs()
	static QMap<QString, MorphInfo> GetAvailableMorphs(DzNode* Node, bool bRecursive=false);

	static QList<QString> GetMorphNamesToDisconnectList(QList<MorphInfo> aMorphInfosToExport);
	static QList<QString> GetMorphNamesToDisconnectList(QList<QString> aMorphNamesToExport, DzNode* pNode);
	static bool CheckForIrreversibleOperations_in_disconnectOverrideControllers(DzNode* Selection, QList<QString> aMorphNamesToExport);
	static QString GetMorphLabelFromName(QString sMorphName, DzNode* pNode);
	static QList<JointLinkInfo> GetActiveJointControlledMorphs( DzNode* pNode = nullptr );
	static QList<DzProperty*> MorphTools::GetDownstreamErcList(DzProperty* pProperty, bool bPrimaryErcOnly, bool bSecondaryErcOnly, bool bAllNonPrimaryDescendants);
	static QList<DzProperty*> MorphTools::GetUpstreamErcList(DzProperty* pProperty, bool bPrimaryErcOnly, bool bSecondaryErcOnly, bool bAllNonPrimaryAncestors);

private:
	static void AddActiveJointControlledMorphs(QList<QString> &m_morphsToExport, QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node = nullptr);
	static QList<JointLinkInfo> GetActiveJointControlledMorphs(QMap<QString, MorphInfo> availableMorphsTable, bool bAutoJCMEnabled, DzNode* Node = nullptr);
	static QList<JointLinkInfo> GetJointControlledMorphInfo(DzProperty* property, QMap<QString, MorphInfo> availableMorphsTable);

};
