#pragma once

class DzNode;
class DzVec3;
class QFileInfo;
class QString;
class QStringList;

class DzBridgeTools
{
public:
    static bool CalculateRawOffset(const DzNode* pNode, DzVec3 &vOffset);
    static bool IsFileTypeInList(QFileInfo fi, QStringList aExtensionsList);
    static bool SafeCleanIntermediateSubFolder(QString sSubFolderPath, QStringList aExtensionsToDelete);
    static bool IsDangerousPath(const QString& sPath);
    static QString CleanTrailingSeparator(QString sFolderPath);
};

