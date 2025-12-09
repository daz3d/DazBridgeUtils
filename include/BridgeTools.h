#pragma once

class DzNode;
class DzVec3;
class QFileInfo;
class QString;
class QStringList;
class DzExporter;

class BridgeTools
{
public:
	static bool CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset);
	static bool IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList);
	static bool SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete);
	static bool IsDangerousPath(const QString& sPath);
	static QString CleanTrailingSeparator(QString sFolderPath);

	static bool LogDefaultExportOptions(DzExporter* Exporter);
	static void SetFbxExportOptionsAllOff(DzFileIOSettings &ExportOptions);
	static void SetFbxExportOptionsBridgeDefaults(DzFileIOSettings &ExportOptions);
	static void SetFbxExportOptionsMvcProxyMesh(DzFileIOSettings &ExportOptions);
	
	static bool ExpandClothingFit(DzNode* pNode);
	static bool BakeClothingFits(DzNode* pNode);

};

