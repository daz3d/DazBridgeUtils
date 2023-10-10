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
namespace DzBridgeNameSpace
{
	struct MorphInfo {
		QString Name;
		QString Label;
		QString Type;
		QString Path;
		DzProperty *Property;
		DzNode *Node;

		inline bool operator==(MorphInfo other)
		{
			if (Name == other.Name)
			{
				return true;
			}
			return false;
		}

		MorphInfo()
		{
			Name = QString();
			Label = QString();
			Type = QString();
			Path = QString();
			Property = nullptr;
			Node = nullptr;
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
		QString Morph;
		double Scalar;
		double Alpha;
		bool IsBaseJCM = false;
		MorphInfo LinkMorphInfo;
		QList<JointLinkKey> Keys;
	};

	class CPP_Export DzBridgeMorphSelectionDialog : public DzBasicDialog {
		Q_OBJECT
	public:

		DzBridgeMorphSelectionDialog(QWidget* parent = nullptr);
		virtual ~DzBridgeMorphSelectionDialog() {}

		// Setup the dialog
		Q_INVOKABLE void PrepareDialog();

		// Singleton access
		Q_INVOKABLE static DzBridgeMorphSelectionDialog* Get(QWidget* Parent);

		// Get the morph string (aka m_morphsToExport) in the format for the Daz FBX Export
		Q_INVOKABLE QString GetMorphString();

		// Get the morph string (aka m_morphsToExport) in the format used for presets
		Q_INVOKABLE QString GetMorphCSVString(bool bUseFinalizedList = true);

		// Get the morph string (aka m_morphsToExport) in an internal name = friendly name format
		// Used to rename them to the friendly name in Unreal
		Q_INVOKABLE QMap<QString, QString> GetMorphMapping();

		Q_INVOKABLE bool IsAutoJCMEnabled() { return autoJCMCheckBox->isChecked(); }
		Q_INVOKABLE bool IsFakeDualQuatEnabled() { return fakeDualQuatCheckBox->isChecked(); }
		Q_INVOKABLE bool IsAllowMorphDoubleDippingEnabled() { return allowMorphDoubleDippingCheckBox->isChecked(); }

		// Recursive function for finding all active JCM morphs for a node
		Q_INVOKABLE QList<JointLinkInfo> GetActiveJointControlledMorphs(DzNode* Node = nullptr);
		// Find and ADD JCMs
		Q_INVOKABLE void AddActiveJointControlledMorphs(DzNode* Node = nullptr);

		// Retrieve label based on morph name
		// DB Dec-21-2021, Created for scripting.
		Q_INVOKABLE QString GetMorphLabelFromName(QString morphName);

		// Get MorphInfo from morph name
		// DB June-01-2022, Created for MorphLinks Generation for Blender Bridge Morphs Support
		Q_INVOKABLE MorphInfo GetMorphInfoFromName(QString morphName);
		Q_INVOKABLE void SetAutoJCMVisible(bool bVisible);
		Q_INVOKABLE void SetAutoJCMEnabled(bool bEnabled);
		// DB 2023-July-10
		Q_INVOKABLE void SetAllowMorphDoubleDippingVisible(bool bVisible);
		Q_INVOKABLE void SetAllowMorphDoubleDippingEnabled(bool bEnabled);

		// get morph property name
		Q_INVOKABLE static QString getMorphPropertyName(DzProperty* pMorphProperty);
		Q_INVOKABLE QList<QString> getMorphNamesToDisconnectList();

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
		void HandleAutoJCMCheckBoxChange(bool checked);
		void HandleFakeDualQuatCheckBoxChange(bool checked);
		void HandleAddConnectedMorphs();
		void HandleDialogAccepted(bool bSavePreset = true);

	private:
		// check if Morph is Valid
		bool isValidMorph(DzProperty* pMorphProperty);

		// Refresh the list of possible presets from disk
		void RefreshPresetsCombo();

		// Recursive function for finding all morphs for a node
		QStringList GetAvailableMorphs(DzNode* Node);

		//
		QList<JointLinkInfo> GetJointControlledMorphInfo(DzProperty* property);

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
		QList<MorphInfo> selectedInTree;

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
		QMap<QTreeWidgetItem*, QList<MorphInfo>> morphsForNode;

		// Force the size of the dialog
		QSize minimumSizeHint() const override;

		// Widgets the dialog will access after construction
		QListWidget* morphListWidget;
		QListWidget* morphExportListWidget;
		QTreeWidget* morphTreeWidget;
		QLineEdit* filterEdit;
		QComboBox* presetCombo;

		QTreeWidgetItem* fullBodyMorphTreeItem;
		QTreeWidgetItem* charactersTreeItem;
		QCheckBox* autoJCMCheckBox;
		QCheckBox* fakeDualQuatCheckBox;
		QPushButton* addConnectedMorphsButton;

		QCheckBox* allowMorphDoubleDippingCheckBox;

		QSettings* settings;
	};

}
