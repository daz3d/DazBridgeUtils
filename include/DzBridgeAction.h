#pragma once
#include <dzaction.h>
#include <dznode.h>
#include <DzFileIOSettings.h>
#include <dzjsonwriter.h>
#include <dzimageproperty.h>
#include <dzweightmap.h>
#include "QtCore/qfile.h"
#include "QtCore/qtextstream.h"

#include "DzBridgeMorphSelectionDialog.h"

class DzProgress;
class DzGeometry;

class UnitTest_DzBridgeAction;

#include "dzbridge.h"
namespace DzBridgeNameSpace
{
	class DzBridgeDialog;
	class DzBridgeMorphSelectionDialog;
	class DzBridgeSubdivisionDialog;

	/// <summary>
	/// Abstract base class that manages exporting of assets to Target Software via FBX/DTU
	/// intermediate files.  Manages destination filepaths, morphs, subdivisions, animations, etc.
	/// 
	/// Usage: 
	/// Subclass and implement executeAction() to open m_bridgeDialog. Implement readGuiRootFolder()
	/// to read and return any custom UI widget containing destination root folder. Implement
	/// writeConfiguration() to manage DTU file generation.  Implement setExportOptions() to override
	/// FBX generation options.  
	/// 
	/// See also: 
	/// DzBridgeScriptableAction.h for Daz Script usage.
	/// </summary>
	class CPP_Export DzBridgeAction : public DzAction {
		Q_OBJECT
		Q_PROPERTY(int nNonInteractiveMode READ getNonInteractiveMode WRITE setNonInteractiveMode)
		Q_PROPERTY(QString sAssetType READ getAssetType WRITE setAssetType)
		Q_PROPERTY(QString sExportFilename READ getExportFilename WRITE setExportFilename)
		Q_PROPERTY(QString sExportFolder READ getExportFolder WRITE setExportFolder)
		Q_PROPERTY(QString sRootFolder READ getRootFolder WRITE setRootFolder)
		Q_PROPERTY(QString sProductName READ getProductName WRITE setProductName)
		Q_PROPERTY(QString sProductComponentName READ getProductComponentName WRITE setProductComponentName)
		Q_PROPERTY(QStringList aMorphList READ getMorphList WRITE setMorphList)
		Q_PROPERTY(bool bUseRelativePaths READ getUseRelativePaths WRITE setUseRelativePaths)
		Q_PROPERTY(bool bUndoNormalMaps READ getUndoNormalMaps WRITE setUndoNormalMaps)
		Q_PROPERTY(QString sExportFbx READ getExportFbx WRITE setExportFbx)
		Q_PROPERTY(DzBasicDialog* wBridgeDialog READ getBridgeDialog WRITE setBridgeDialog)
		Q_PROPERTY(DzBasicDialog* wSubdivisionDialog READ getSubdivisionDialog WRITE setSubdivisionDialog)
		Q_PROPERTY(DzBasicDialog* wMorphSelectionDialog READ getMorphSelectionDialog WRITE setMorphSelectionDialog)

	public:

		DzBridgeAction(const QString& text = QString::null, const QString& desc = QString::null);
		virtual ~DzBridgeAction();

		Q_INVOKABLE void resetToDefaults();
		Q_INVOKABLE QString cleanString(QString argString) { return argString.remove(QRegExp("[^A-Za-z0-9_]")); };
		Q_INVOKABLE QStringList getAvailableMorphs(DzNode* Node);
		Q_INVOKABLE QStringList getActiveMorphs(DzNode* Node);

		// Normal Map Handling
		Q_INVOKABLE QImage makeNormalMapFromHeightMap(QString heightMapFilename, double normalStrength);
		// Pre-Process Scene data to workaround FbxExporter issues, called by Export() before FbxExport operation.
		virtual bool preProcessScene(DzNode* parentNode = nullptr);
		// Undo changes made by preProcessScene(), called by Export() after FbxExport operation.
		virtual bool undoPreProcessScene();
		bool renameDuplicateMaterial(DzMaterial* material, QList<QString>* existingMaterialNameList);
		bool undoRenameDuplicateMaterials();
		bool generateMissingNormalMap(DzMaterial* material);
		bool undoGenerateMissingNormalMaps();

	protected:
		// Struct to remember attachment info
		struct AttachmentInfo
		{
			DzNode* Parent;
			DzNode* Child;
		};

		DzBridgeDialog* m_bridgeDialog;
		DzBridgeSubdivisionDialog* m_subdivisionDialog;
		DzBridgeMorphSelectionDialog* m_morphSelectionDialog;

		int m_nNonInteractiveMode;
		QString m_sAssetName; // Exported filename without extension
		QString m_sRootFolder; // The destination Root Folder
		QString m_sDestinationPath; // Path to destination files: <m_sRootFolder> + "/" + <m_sAssetName (folder)> + "/"
		QString m_sDestinationFBX; // Path to destination fbx file: <m_sDestinationPath> + <m_sExportFbx> + ".fbx";
		QString m_sAssetType; // Asset Types: "SkeletalMesh", "StaticMesh", "Animation", "Pose", "Environment"
		QString m_sMorphSelectionRule; // Selection Rule used by FbxExporter to choose morphs to export
		QString m_sFbxVersion; // FBX file format version to export
		QMap<QString, QString> m_mMorphNameToLabel; // Internal name to Friendly label
		QList<QString> m_aPoseList; // Control Pose names
		QMap<DzImageProperty*, double> m_imgPropertyTable_NormalMapStrength; // Image Property to Normal Map Strength

		// Used only by script system
		QString m_sExportSubfolder; // Destination subfolder within Root Folder for exporting. [Default is <m_sAssetName>]
		QString m_sProductName; // Daz Store Product Name, can contain spaces and special characters
		QString m_sProductComponentName; // Friendly name of Component of Daz Store Product, can contain spaces and special characters
		QStringList m_aMorphListOverride; // overrides Morph Selection Dialog
		bool m_bUseRelativePaths; // use relative paths in DTU instead of absolute paths
		bool m_bGenerateNormalMaps; // generate normal maps from height maps
		bool m_bUndoNormalMaps;  // remove generated normal maps after export
		QString m_sExportFbx; // override filename of exported fbx

		bool m_bEnableMorphs; // enable morph export
		bool m_EnableSubdivisions; // enable subdivision baking
		bool m_bExportingBaseMesh;
		bool m_bShowFbxOptions;
		bool m_bExportMaterialPropertiesCSV;
		DzNode* m_pSelectedNode;

		virtual QString getActionGroup() const { return tr("Bridges"); }
		virtual QString getDefaultMenuPath() const { return tr("&File/Send To"); }

		virtual void exportAsset();
		virtual void exportNode(DzNode* Node);

		virtual void writeConfiguration() = 0;
		virtual void setExportOptions(DzFileIOSettings& ExportOptions) = 0;
		virtual QString readGuiRootFolder() = 0;

		Q_INVOKABLE virtual void writeDTUHeader(DzJsonWriter& writer);

		Q_INVOKABLE virtual void writeAllMaterials(DzNode* Node, DzJsonWriter& Writer, QTextStream* CVSStream = nullptr, bool bRecursive = false);
		Q_INVOKABLE virtual void startMaterialBlock(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, DzMaterial* Material);
		Q_INVOKABLE virtual void finishMaterialBlock(DzJsonWriter& Writer);
		Q_INVOKABLE virtual void writeMaterialProperty(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream, DzMaterial* Material, DzProperty* Property);

		Q_INVOKABLE virtual void writeAllMorphs(DzJsonWriter& Writer);
		Q_INVOKABLE virtual void writeMorphProperties(DzJsonWriter& writer, const QString& key, const QString& value);
		Q_INVOKABLE virtual void writeMorphJointLinkInfo(DzJsonWriter& writer, const JointLinkInfo& linkInfo);

		Q_INVOKABLE virtual void writeAllSubdivisions(DzJsonWriter& Writer);
		Q_INVOKABLE virtual void writeSubdivisionProperties(DzJsonWriter& writer, const QString& Name, int targetValue);

		Q_INVOKABLE virtual void writeAllDforceInfo(DzNode* Node, DzJsonWriter& Writer, QTextStream* pCVSStream = nullptr, bool bRecursive = false);
		Q_INVOKABLE virtual void writeDforceMaterialProperties(DzJsonWriter& Writer, DzMaterial* Material, DzShape* Shape);
		Q_INVOKABLE virtual void writeDforceModifiers(const QList<DzModifier*>& dforceModifierList, DzJsonWriter& Writer, DzShape* Shape);

		Q_INVOKABLE virtual void writeEnvironment(DzJsonWriter& writer);
		Q_INVOKABLE virtual void writeInstances(DzNode* Node, DzJsonWriter& Writer, QMap<QString, DzMatrix3>& WritenInstances, QList<DzGeometry*>& ExportedGeometry, QUuid ParentID = QUuid());
		Q_INVOKABLE virtual QUuid writeInstance(DzNode* Node, DzJsonWriter& Writer, QUuid ParentID);

		Q_INVOKABLE virtual void writeAllPoses(DzJsonWriter& writer);

		// Used to find all the unique props in a scene for Environment export
		void getScenePropList(DzNode* Node, QMap<QString, DzNode*>& Types);

		// During Environment export props need to get disconnected as they are exported.
		void disconnectNode(DzNode* Node, QList<AttachmentInfo>& AttachmentList);
		void reconnectNodes(QList<AttachmentInfo>& AttachmentList);

		// During Skeletal Mesh Export Disconnect Override Controllers
		QList<QString> disconnectOverrideControllers();
		void reconnectOverrideControllers(QList<QString>& DisconnetedControllers);
		QList<QString> m_ControllersToDisconnect;

		// For Pose exports check if writing to the timeline will alter existing keys
		bool checkIfPoseExportIsDestructive();

		// Need to be able to move asset instances to origin during environment export
		void unlockTranform(DzNode* NodeToUnlock);

		// Getter/Setter methods
		Q_INVOKABLE DzBridgeDialog* getBridgeDialog() { return m_bridgeDialog; }
		Q_INVOKABLE bool setBridgeDialog(DzBasicDialog* arg_dlg);
		Q_INVOKABLE DzBridgeSubdivisionDialog* getSubdivisionDialog() { return m_subdivisionDialog; }
		Q_INVOKABLE bool setSubdivisionDialog(DzBasicDialog* arg_dlg);
		Q_INVOKABLE DzBridgeMorphSelectionDialog* getMorphSelectionDialog() { return m_morphSelectionDialog; }
		Q_INVOKABLE bool setMorphSelectionDialog(DzBasicDialog* arg_dlg);

		Q_INVOKABLE QString getAssetType() { return this->m_sAssetType; };
		Q_INVOKABLE void setAssetType(QString arg_AssetType) { this->m_sAssetType = arg_AssetType; };
		Q_INVOKABLE QString getExportFilename() { return this->m_sAssetName; };
		Q_INVOKABLE void setExportFilename(QString arg_Filename) { this->m_sAssetName = arg_Filename; };

		Q_INVOKABLE QString getExportFolder() { return this->m_sExportSubfolder; };
		Q_INVOKABLE void setExportFolder(QString arg_Folder) { this->m_sExportSubfolder = arg_Folder; };
		Q_INVOKABLE QString getRootFolder() { return this->m_sRootFolder; };
		Q_INVOKABLE void setRootFolder(QString arg_Root) { this->m_sRootFolder = arg_Root; };

		Q_INVOKABLE QString getProductName() { return this->m_sProductName; };
		Q_INVOKABLE void setProductName(QString arg_ProductName) { this->m_sProductName = arg_ProductName; };
		Q_INVOKABLE QString getProductComponentName() { return this->m_sProductComponentName; };
		Q_INVOKABLE void setProductComponentName(QString arg_ProductComponentName) { this->m_sProductComponentName = arg_ProductComponentName; };

		Q_INVOKABLE QStringList getMorphList() { return m_aMorphListOverride; };
		Q_INVOKABLE void setMorphList(QStringList arg_MorphList) { this->m_aMorphListOverride = arg_MorphList; };

		Q_INVOKABLE bool getUseRelativePaths() { return this->m_bUseRelativePaths; };
		Q_INVOKABLE void setUseRelativePaths(bool arg_UseRelativePaths) { this->m_bUseRelativePaths = arg_UseRelativePaths; };

		bool isTemporaryFile(QString sFilename);
		QString exportAssetWithDtu(QString sFilename, QString sAssetMaterialName = "");
		void writePropertyTexture(DzJsonWriter& Writer, QString sName, QString sValue, QString sType, QString sTexture);
		void writePropertyTexture(DzJsonWriter& Writer, QString sName, double dValue, QString sType, QString sTexture);
		QString makeUniqueFilename(QString sFilename);

		Q_INVOKABLE bool getUndoNormalMaps() { return this->m_bUndoNormalMaps; };
		Q_INVOKABLE void setUndoNormalMaps(bool arg_UndoNormalMaps) { this->m_bUndoNormalMaps = arg_UndoNormalMaps; };

		Q_INVOKABLE int getNonInteractiveMode() { return this->m_nNonInteractiveMode; };
		Q_INVOKABLE void setNonInteractiveMode(int arg_Mode) { this->m_nNonInteractiveMode = arg_Mode; };

		Q_INVOKABLE QString getExportFbx() { return this->m_sExportFbx; };
		Q_INVOKABLE void setExportFbx(QString arg_FbxName) { this->m_sExportFbx = arg_FbxName; };

		Q_INVOKABLE void readGui(DzBridgeDialog*);
		Q_INVOKABLE void exportHD(DzProgress* exportProgress = nullptr);
		Q_INVOKABLE bool upgradeToHD(QString baseFilePath, QString hdFilePath, QString outFilePath, std::map<std::string, int>* pLookupTable);
		Q_INVOKABLE void writeWeightMaps(DzNode* Node, DzJsonWriter& Stream);

		Q_INVOKABLE bool metaInvokeMethod(QObject* object, const char* methodSig, void** returnPtr);
		Q_INVOKABLE bool copyFile(QFile* file, QString* dst, bool replace = true, bool compareFiles = true);
		Q_INVOKABLE QString getMD5(const QString& path);

	private:
		class MaterialGroupExportOrderMetaData
		{
		public:
			int materialIndex;
			int vertex_offset;
			int vertex_count;

			MaterialGroupExportOrderMetaData(int a_index, int a_offset)
			{
				materialIndex = a_index;
				vertex_offset = a_offset;
				vertex_count = -1;
			}

			bool operator< (MaterialGroupExportOrderMetaData b) const
			{
				if (vertex_offset < b.vertex_offset)
				{
					return true;
				}
				else
				{
					return false;
				}
			}

		};

		// Undo data structures
		QMap<DzMaterial*, QString> m_undoTable_DuplicateMaterialRename;
		QMap<DzMaterial*, DzProperty*> m_undoTable_GenerateMissingNormalMap;

		// NormalMap utility methods
		double getPixelIntensity(const  QRgb& pixel);
		uint8_t getNormalMapComponent(double pX);
		int getIntClamp(int x, int low, int high);
		QRgb getSafePixel(const QImage& img, int x, int y);
		bool isNormalMapMissing(DzMaterial* material);
		bool isHeightMapPresent(DzMaterial* material);
		QString getHeightMapFilename(DzMaterial* material);
		double getHeightMapStrength(DzMaterial* material);

		DzWeightMapPtr getWeightMapPtr(DzNode* Node);

		// Need to temporarily rename surfaces if there is a name collision
		void renameDuplicateMaterials(DzNode* Node, QList<QString>& MaterialNames, QMap<DzMaterial*, QString>& OriginalMaterialNames);
		void undoRenameDuplicateMaterials(DzNode* Node, QList<QString>& MaterialNames, QMap<DzMaterial*, QString>& OriginalMaterialNames);

#ifdef UNITTEST_DZBRIDGE
		friend class ::UnitTest_DzBridgeAction;
#endif

	};

}