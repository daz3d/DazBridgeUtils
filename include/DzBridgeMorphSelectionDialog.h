#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qlineedit.h>
#include <QtGui/qcombobox.h>
#include <QtGui/qcheckbox.h>
#include <QtCore/qsettings.h>
#include "dznode.h"

class QListWidget;
class QListWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;
class QCheckBox;

#include "dzbridge.h"

class SortingListItem;

#include "MorphTools.h"

namespace DzBridgeNameSpace
{
	class DzBridgeAction;

	class CPP_Export DzBridgeMorphSelectionDialog : public DzBasicDialog {
		Q_OBJECT
	public:

		DzBridgeMorphSelectionDialog(QWidget* parent = nullptr);
		virtual ~DzBridgeMorphSelectionDialog() {}

		// Setup the dialog
		Q_INVOKABLE void PrepareDialog();

		// Singleton access
		Q_INVOKABLE static DzBridgeMorphSelectionDialog* Get(QWidget* Parent);

		// Get the morph string (aka m_morphsToExport) in the format used for presets
		Q_INVOKABLE QString GetMorphCSVString(bool bUseFinalizedList = true);

		// Used to rename them to the friendly name in Unreal
		Q_INVOKABLE QMap<QString, MorphInfo> GetAvailableMorphsTable();
		// Morph Selection Overhaul
		Q_INVOKABLE QList<QString> GetMorphNamesToExport();

		// Retrieve label based on morph name
		// DB Dec-21-2021, Created for scripting.
		Q_INVOKABLE QString GetMorphLabelFromName(QString morphName);

		// Get MorphInfo from morph name
		// DB June-01-2022, Created for MorphLinks Generation for Blender Bridge Morphs Support
		Q_INVOKABLE MorphInfo GetMorphInfoFromName(QString morphName);

		// get morph property name
		Q_INVOKABLE static QString getMorphPropertyName(DzProperty* pMorphProperty);

		// Get Pose list.  Similart to morphs, but without AutoJCM or FakeDualQuat items
		Q_INVOKABLE QList<QString> GetPoseList();

	public slots:
		void FilterChanged(const QString& filter);
		void ItemSelectionChanged();
		void HandleAddMorphsButton();
		void HandleRemoveMorphsButton();
		void HandleSavePreset();
		void HandlePresetChanged(const QString& presetName);
		void HandleArmJCMMorphsButton();
		void HandleLegJCMMorphsButton();
		void HandleTorsoJCMMorphsButton();
		void HandleARKitGenesis81MorphsButton();
		void HandleFaceFXGenesis8Button();
		void HandleAddConnectedMorphs();
		void HandleAddConnectedJcms();
		void HandleDialogAccepted(bool bSavePreset = true);

	protected:
		virtual void addGenesis9FACS(QStringList& MorphsToAdd); // similar to ARKit G81 method, but adds a set of FACS specifically for WonderStudio
		virtual void addGenesis81FACS(QStringList& MorphsToAdd); // similar to ARKit G81 method, but adds a set of FACS specifically for WonderStudio
		virtual bool decorateMorphListItem(SortingListItem* item, MorphInfo morphInfo, bool bAnalyzeErc=false);
		virtual MorphExportSettings getMorphExportSettings();

	private:
		// check if Morph is Valid
		bool isValidMorph(DzProperty* pMorphProperty);

		// Refresh the list of possible presets from disk
		void RefreshPresetsCombo();

		// function for finding all morphs for a node
		QMap<QString, MorphInfo> GetAvailableMorphs(DzNode* Node);

		void UpdateMorphsTree();

		// Returns the tree node for the morph name (with path)
		// Builds out the structure of the tree as needed.
		QTreeWidgetItem* FindTreeItem(QTreeWidgetItem* parent, QString name);

		void SavePresetFile(QString filePath);

		// Updates selectedInTree to have all the morphs for the nodes
		// selected in the left tree
		void SelectMorphsInNode(QTreeWidgetItem* item);

		// Rebuild the right box that lists all the morphs that will export.
		void RefreshExportMorphList();

		// Morphs currently selected in the left tree box
		QList<MorphInfo> m_selectedInTree;

		// List of morphs moved to the export box
		QList<MorphInfo> m_morphsToExport;
		QList<MorphInfo> m_morphsToExport_finalized;

		// Store off the presetsFolder path at dialog setup
		QString presetsFolder;

		static DzBridgeMorphSelectionDialog* singleton;

		// Mapping of morph name to MorphInfo
		QMap<QString, MorphInfo> m_morphInfoMap;

		// List of morphs (recursive) under each tree node
		// For convenience populating the middle box.
		QMap<QTreeWidgetItem*, QList<MorphInfo>> m_morphsForNode;

		// Force the size of the dialog
		QSize minimumSizeHint() const override;

		// Widgets the dialog will access after construction
		QListWidget* m_morphListWidget; // Center Column (Morphs in Group)
		QListWidget* m_morphExportListWidget; // Right Column (Morphs to Export)
		QTreeWidget* m_morphTreeWidget;  // Left Column (Morph Groups)
		QLineEdit* filterEdit;
		QComboBox* presetCombo;

		QTreeWidgetItem* fullBodyMorphTreeItem;
		QTreeWidgetItem* charactersTreeItem;
		QPushButton* addConnectedMorphsButton;
		QPushButton* m_wAddConnectedJcmsButton;

		QSettings* settings;

		friend DzBridgeAction;
	};

}
