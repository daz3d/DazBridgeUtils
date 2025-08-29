#pragma once

class DzNode;
class DzVec3;
class QFileInfo;
class QString;
class QStringList;

class BridgeTools
{
public:
	static bool CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset);
	static bool IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList);
	static bool SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete);
	static bool IsDangerousPath(const QString& sPath);
	static QString CleanTrailingSeparator(QString sFolderPath);

	static void SetExportOptionsAllOff(DzFileIOSettings &ExportOptions);
	static void SetExportOptionsBridgeDefaults(DzFileIOSettings &ExportOptions);
	static void SetExportOptionsMvcProxyMesh(DzFileIOSettings &ExportOptions);
	
};

