#pragma once
//#include "dzbasicdialog.h"
#include <QtGui/qdialog.h>
#include "dzoptionsdialog.h"
#include <QtGui/qcheckbox.h>
#include <QtGui/qpushbutton.h>
#include <QtGui/qgroupbox.h>
#include <QtGui/qcombobox.h>
#include "dzmenubutton.h"
#include "dzstyledefs.h"

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QFormLayout;
class QLabel;
class QSettings;
class DzMenuButton;

class UnitTest_DzBridgeDialog;

#include "dzbridge.h"

class DzBridgeDialogTools
{
public:
	static void SetThinButtonText(QPushButton* widget, const QString& text);
};

class DzBridgeBrowseEdit : public QWidget {
	Q_OBJECT
public:
	DzBridgeBrowseEdit(const QString& text, QWidget* parent);
};

#include "dzstyledbutton.h"
class DzBridgeThinButton : public DzStyledButton {
	Q_OBJECT
public:
	DzBridgeThinButton(const QString& text, QWidget* parent = nullptr);
	void setText(const QString& text);
	void setHighlightStyle(bool bHighlight);

	QStyle::PrimitiveElement m_eDefaultElementStyle;
	DzStyle::TextStyle m_eDefaultTextStyle;

};

class DzBridgeBrowseButton : public DzBridgeThinButton {
	Q_OBJECT
public:
	DzBridgeBrowseButton(QWidget* parent = nullptr) : DzBridgeThinButton("...", parent) {
	}
};

namespace DzBridgeNameSpace
{
	class DzBridgeAction;

	class CPP_Export DzBridgeDialog : public DzOptionsDialog {
		Q_OBJECT
		Q_PROPERTY(QWidget* wAssetNameEdit READ getAssetNameEdit)
		Q_PROPERTY(QWidget* wAssetTypeCombo READ getAssetTypeCombo)
		Q_PROPERTY(QWidget* wMorphsEnabledCheckBox READ getMorphsEnabledCheckBox)
		Q_PROPERTY(QWidget* wSubdivisionEnabledCheckBox READ getSubdivisionEnabledCheckBox)
		Q_PROPERTY(QWidget* wAdvancedSettingsGroupBox READ getAdvancedSettingsGroupBox)
		Q_PROPERTY(QWidget* wFbxVersionCombo READ getFbxVersionCombo)
		Q_PROPERTY(QWidget* wShowFbxDialogCheckBox READ getShowFbxDialogCheckBox)
		Q_PROPERTY(QWidget* wEnableNormalMapGenerationCheckBox READ getEnableNormalMapGenerationCheckBox)
		Q_PROPERTY(QWidget* wExportMaterialPropertyCSVCheckBox READ getExportMaterialPropertyCSVCheckBox)
		Q_PROPERTY(QWidget* wTargetPluginInstallerButton READ getTargetPluginInstallerButton)
		Q_PROPERTY(QWidget* wTargetSoftwareVersionCombo READ getTargetSoftwareVersionCombo)
        Q_PROPERTY(bool bEnableExperimentalOptions READ getEnableExperimentalOptions)
		Q_PROPERTY(QWidget* wEnableLodCheckBox READ getEnableLodCheckBox)
	public:
		Q_INVOKABLE bool setBridgeActionObject(QObject* arg);
		Q_INVOKABLE QLineEdit* getAssetNameEdit() { return assetNameEdit; }
		Q_INVOKABLE QComboBox* getAssetTypeCombo() { return assetTypeCombo; }
		Q_INVOKABLE QCheckBox* getMorphsEnabledCheckBox() { return morphsEnabledCheckBox; }
		Q_INVOKABLE QCheckBox* getSubdivisionEnabledCheckBox() { return subdivisionEnabledCheckBox; }
		Q_INVOKABLE QGroupBox* getAdvancedSettingsGroupBox() { return advancedSettingsGroupBox; }
		Q_INVOKABLE QComboBox* getFbxVersionCombo() { return fbxVersionCombo; }
		Q_INVOKABLE QCheckBox* getShowFbxDialogCheckBox() { return showFbxDialogCheckBox; }
		Q_INVOKABLE QCheckBox* getEnableNormalMapGenerationCheckBox() { return m_wConvertBumpToNormalCheckBox; }
		Q_INVOKABLE QCheckBox* getExportMaterialPropertyCSVCheckBox() { return exportMaterialPropertyCSVCheckBox; }
		Q_INVOKABLE QPushButton* getTargetPluginInstallerButton() { return m_TargetPluginInstallerButton; }
		Q_INVOKABLE QComboBox* getTargetSoftwareVersionCombo() { return m_TargetSoftwareVersionCombo; }
		Q_INVOKABLE QCheckBox* getExperimentalAnimationExportCheckBox() { return experimentalAnimationExportCheckBox; }
		Q_INVOKABLE QCheckBox* getBakeAnimationExportCheckBox () { return  bakeAnimationExportCheckBox; }
		Q_INVOKABLE QCheckBox* getFaceAnimationExportCheckBox() { return faceAnimationExportCheckBox; }
		Q_INVOKABLE QSettings* getSettings() { return settings; }
		Q_INVOKABLE QCheckBox* getAnimationExportActiveCurvesCheckBox() { return animationExportActiveCurvesCheckBox; }
		Q_INVOKABLE QCheckBox* getAnimationApplyBoneScaleCheckBox() { return animationApplyBoneScaleCheckBox; }
		Q_INVOKABLE QCheckBox* getMorphLockBoneTranslationCheckBox() { return morphLockBoneTranslationCheckBox; }
        Q_INVOKABLE bool getEnableExperimentalOptions() { return m_enableExperimentalOptionsCheckBox->isChecked(); }
		Q_INVOKABLE QCheckBox* getEnableLodCheckBox() { return m_wEnableLodCheckBox; }

		Q_INVOKABLE bool getSkipResizeTextures() { return (m_wResizeTexturesGroupBox->isEnabled() == false); }
		Q_INVOKABLE bool getResizeTextures() { return m_wResizeTexturesGroupBox->isChecked(); }
		Q_INVOKABLE int getMaxTextureFileSize() { return m_wMaxTextureFileSizeCombo->itemData(m_wMaxTextureFileSizeCombo->currentIndex()).toInt(); }
		Q_INVOKABLE int getMaxTextureResolution() { return m_wMaxTextureResolutionCombo->itemData(m_wMaxTextureResolutionCombo->currentIndex()).toInt(); }
		Q_INVOKABLE QString getExportTextureFileFormat() { return m_wExportTextureFileFormatCombo->itemData(m_wExportTextureFileFormatCombo->currentIndex()).toString(); }

		Q_INVOKABLE bool getIsBakeEnabled() { return m_wBakeAlphaChannelCheckBox->isEnabled(); }
		Q_INVOKABLE bool getBakeAlpha() { return m_wBakeAlphaChannelCheckBox->isChecked(); }
		Q_INVOKABLE bool getBakeMakeup() { return m_wBakeMakeupOverlayCheckBox->isChecked(); }
		Q_INVOKABLE bool getBakeColorTint() { return m_wBakeColorTintCheckBox->isChecked(); }
		Q_INVOKABLE bool getBakeTranslucency() { return m_wBakeTranslucencyTintCheckBox->isChecked(); }
		Q_INVOKABLE bool getBakeSpecularToMetallic();
		Q_INVOKABLE bool getBakeRefractionWeight();

		Q_INVOKABLE QString getBakeInstances() { return m_wBakeInstancesComboBox->itemData(m_wBakeInstancesComboBox->currentIndex()).toString(); }
		Q_INVOKABLE QString getBakePivotPoints() { return m_wBakeCustomPivotsComboBox->itemData(m_wBakeCustomPivotsComboBox->currentIndex()).toString(); }
		Q_INVOKABLE QString getBakeRigidFollowNodes() { return m_wBakeRigidFollowNodesComboBox->itemData(m_wBakeRigidFollowNodesComboBox->currentIndex()).toString(); }

		Q_INVOKABLE void setEAssetType(int assetType) { m_eAssetType = assetType; }

		Q_INVOKABLE bool getAllowMorphDoubleDipping() { return m_wAllowMorphDoubleDippingCheckBox->isChecked(); }
		Q_INVOKABLE bool getAutoJCM() { return m_wAutoJCMCheckBox->isChecked(); }
		Q_INVOKABLE bool getFakeDualQuat() { return m_wFakeDualQuatCheckBox->isChecked(); }
		Q_INVOKABLE bool getMorphsEnabled() { return morphsEnabledCheckBox->isChecked(); }

		/** Constructor **/
		DzBridgeDialog(QWidget* parent = nullptr, const QString& windowTitle = "");

		/** Destructor **/
		virtual ~DzBridgeDialog() {}

		Q_INVOKABLE QList<QString> GetPoseList();
		Q_INVOKABLE virtual void resetToDefaults();
		Q_INVOKABLE virtual bool loadSavedSettings();
		Q_INVOKABLE virtual void saveSettings();
		void Accepted();

		Q_INVOKABLE virtual void showTargetPluginInstaller(bool bShowWidget = true);
		Q_INVOKABLE virtual void renameTargetPluginInstaller(QString sNewLabelName);
		Q_INVOKABLE bool installEmbeddedArchive(QString sArchiveFilename, QString sDestinationPath);
		Q_INVOKABLE virtual void setBridgeVersionStringAndLabel(QString sVersionString, QString sLabel="");
		Q_INVOKABLE virtual void setDisabled(bool bDisable);
		Q_INVOKABLE virtual void showLodRow(bool bShowWidget = true);
		Q_INVOKABLE virtual bool sanityChecksAndWarnUser();

		void accept() override;

		// Used to set universal width for all row labels.  Append rowlabel widgets that are added using Layout::addRow() method
		QList<QLabel*> m_aRowLabels;
		virtual void fixRowLabelWidths();
		virtual void fixRowLabelStyle();

		virtual void toggleOptions() override { DzOptionsDialog::toggleOptions(); fixRowLabelWidths(); };

	protected:
		virtual void showEvent(QShowEvent* event) override { handleSceneSelectionChanged(); fixRowLabelWidths(); QDialog::showEvent(event); }

	protected slots:
		virtual void handleSceneSelectionChanged();
		virtual int  HandleChooseMorphsButton();
		virtual void HandleMorphsCheckBoxChange(int state);
		virtual void HandleChooseSubdivisionsButton();
		virtual void HandleSubdivisionCheckBoxChange(int state);
		virtual void HandleFBXVersionChange(const QString& fbxVersion);
		virtual void HandleShowFbxDialogCheckBoxChange(int state);
		virtual void HandleExportMaterialPropertyCSVCheckBoxChange(int state);
		virtual void HandleConvertBumpToNormalCheckBoxChange(int state);
		virtual void HandleTargetPluginInstallerButton();
		virtual void HandleOpenIntermediateFolderButton(QString sFolderPath="");
		virtual void HandleAssetTypeComboChange(int state);
        virtual void HandleExperimentalOptionsCheckBoxClicked();
        virtual void HandleLodSettingsButton();
		virtual void HandleEnableLodCheckBoxChange(int state);

		// Extra Bottom Row Button Handlers
		virtual void HandlePdfButton();
		virtual void HandleYoutubeButton();
		virtual void HandleSupportButton();
		virtual void HandleHelpMenuButton(int);

	protected:
		// Extra Bottom Row Buttons
		DzMenuButton* wHelpMenuButton = nullptr;
#define BRIDGE_HELP_ID_PDF 0
#define BRIDGE_HELP_ID_YOUTUBE 1
#define BRIDGE_HELP_ID_SUPPORT 2

		DzBridgeAction* m_BridgeAction = nullptr;
		QSettings* settings = nullptr;

		virtual void refreshAsset();

		QGroupBox* m_wMainGroupBox = nullptr;
		QFormLayout* mainLayout = nullptr;
		QFormLayout* advancedLayout = nullptr;
		QLineEdit* assetNameEdit = nullptr;
		//	QLineEdit* projectEdit = nullptr;
		//	QPushButton* projectButton = nullptr;
		QComboBox* assetTypeCombo = nullptr;
		QPushButton* morphsButton = nullptr;
		QCheckBox* morphsEnabledCheckBox = nullptr;
		QPushButton* subdivisionButton = nullptr;
		QCheckBox* subdivisionEnabledCheckBox = nullptr;
        QLabel* m_WelcomeLabel = nullptr;

		// Row Widgets
		QLabel* m_wAssetNameRowLabelWidget = nullptr;
		QLabel* m_wAssetTypeRowLabelWidget = nullptr;
		QLabel* m_wMorphsRowLabelWidget = nullptr;
		QLabel* m_wSubDRowLabelWidget = nullptr;
		QLabel* m_wFbxVersionRowLabelWidget = nullptr;
		QLabel* m_wShowFbxRowLabelWidget = nullptr;
		QLabel* m_wExportCsvRowLabelWidget = nullptr;
		QLabel* m_wEnableExperimentalRowLabelWidget = nullptr;

        // Advanced settings
		QGroupBox* advancedSettingsGroupBox = nullptr;
		QComboBox* fbxVersionCombo = nullptr;
		QCheckBox* showFbxDialogCheckBox = nullptr;
		QCheckBox* exportMaterialPropertyCSVCheckBox = nullptr;
		QWidget* m_wTargetPluginInstaller = nullptr;
		QPushButton* m_TargetPluginInstallerButton = nullptr;
		QComboBox* m_TargetSoftwareVersionCombo = nullptr;
		QPushButton* m_OpenIntermediateFolderButton = nullptr;
        QCheckBox* m_enableExperimentalOptionsCheckBox = nullptr;

		// Animation settings
		QGroupBox* animationSettingsGroupBox = nullptr;
		QCheckBox* experimentalAnimationExportCheckBox = nullptr;
		QCheckBox* bakeAnimationExportCheckBox = nullptr;
		QCheckBox* faceAnimationExportCheckBox = nullptr;
		QCheckBox* animationExportActiveCurvesCheckBox = nullptr;
		QCheckBox* animationApplyBoneScaleCheckBox = nullptr;

		// Morph settings
		QGroupBox* morphSettingsGroupBox = nullptr;
		QCheckBox* morphLockBoneTranslationCheckBox = nullptr;
		// 2025-04-14, DB: Refactor MorphSelectionDialog Settings
		QCheckBox* m_wAutoJCMCheckBox = nullptr;
		QCheckBox* m_wFakeDualQuatCheckBox = nullptr;
		QCheckBox* m_wAllowMorphDoubleDippingCheckBox = nullptr;

		// LOD settings
		QPushButton* m_wLodSettingsButton = nullptr;
		QCheckBox* m_wEnableLodCheckBox = nullptr;
		QLabel* m_wLodRowLabelWidget = nullptr;

		// Texture Resizing options (2024-09-17, DB)
		QGroupBox* m_wResizeTexturesGroupBox = nullptr;
		QComboBox* m_wMaxTextureFileSizeCombo = nullptr;
		QComboBox* m_wMaxTextureResolutionCombo = nullptr;
		QComboBox* m_wExportTextureFileFormatCombo = nullptr;
		QList<QLabel*> m_aTextureResizingLabels;
		QLabel* m_wMaxTextureFileSizeRowLabelWidget = nullptr;
		QLabel* m_wMaxTextureResolutionRowLabelWidget = nullptr;
		QLabel* m_wExportTextureFormatRowLabelWidget = nullptr;

		// Texture Baking Options
		QGroupBox* m_wTextureBakingGroupBox = nullptr;
		QCheckBox* m_wBakeAlphaChannelCheckBox = nullptr;
		QCheckBox* m_wBakeMakeupOverlayCheckBox = nullptr;
		QCheckBox* m_wBakeColorTintCheckBox = nullptr;
		QCheckBox* m_wBakeTranslucencyTintCheckBox = nullptr;
		QCheckBox* m_wBakeSpecularToMetallicCheckBox = nullptr;
		QCheckBox* m_wBakeRefractionWeightCheckBox = nullptr;
		QCheckBox* m_wConvertBumpToNormalCheckBox = nullptr;
		QLabel* m_wBakeAlphaChannelRowLabel = nullptr;
		QLabel* m_wBakeMakeupOverlayRowLabel = nullptr;
		QLabel* m_wBakeColorTintRowLabel = nullptr;
		QLabel* m_wBakeTranslucencyTintRowLabel = nullptr;
		QLabel* m_wBakeSpecularToMetallicRowLabel = nullptr;
		QLabel* m_wBakeRefractionWeightRowLabel = nullptr;
		QLabel* m_wNormalMapsRowLabelWidget = nullptr;

		// Object Baking Options
		QGroupBox* m_wObjectBakingGroupBox = nullptr;
		QComboBox* m_wBakeInstancesComboBox = nullptr;
		QComboBox* m_wBakeCustomPivotsComboBox = nullptr;
		QComboBox* m_wBakeRigidFollowNodesComboBox = nullptr;
		QLabel* m_wBakeInstancesRowLabel = nullptr;
		QLabel* m_wBakeCustomPivotsRowLabel = nullptr;
		QLabel* m_wBakeRigidFollowNodesRowLabel = nullptr;

		QString m_sEmbeddedFilesPath = ":/DazBridge";
		bool m_bDontSaveSettings = false;
		bool m_bSetupMode = false;

		int m_eAssetType; // enum EAssetType

#ifdef UNITTEST_DZBRIDGE
		friend class ::UnitTest_DzBridgeDialog;
#endif

	};

}
