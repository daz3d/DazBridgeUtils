#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qcombobox.h>
#include <QtCore/qsettings.h>
#include <QtGui/qcheckbox.h>

class QPushButton;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QFormLayout;
class QLabel;

class UnitTest_DzBridgeDialog;

#include "dzbridge.h"

namespace DzBridgeNameSpace
{
	class DzBridgeAction;

	class CPP_Export DzBridgeDialog : public DzBasicDialog {
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
		Q_INVOKABLE QCheckBox* getEnableNormalMapGenerationCheckBox() { return enableNormalMapGenerationCheckBox; }
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

		/** Constructor **/
		DzBridgeDialog(QWidget* parent = nullptr, const QString& windowTitle = "");

		/** Destructor **/
		virtual ~DzBridgeDialog() {}

		// Pass so the DazToUnrealAction can access it from the morph dialog
		Q_INVOKABLE QString GetMorphString();
		// Pass so the DazToUnrealAction can access it from the morph dialog
		Q_INVOKABLE QMap<QString, QString> GetMorphMappingFromMorphSelectionDialog();
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

		void accept();

	protected slots:
		virtual void handleSceneSelectionChanged();
		virtual int  HandleChooseMorphsButton();
		virtual void HandleMorphsCheckBoxChange(int state);
		virtual void HandleChooseSubdivisionsButton();
		virtual void HandleSubdivisionCheckBoxChange(int state);
		virtual void HandleFBXVersionChange(const QString& fbxVersion);
		virtual void HandleShowFbxDialogCheckBoxChange(int state);
		virtual void HandleExportMaterialPropertyCSVCheckBoxChange(int state);
		virtual void HandleShowAdvancedSettingsCheckBoxChange(bool checked);
		virtual void HandleEnableNormalMapGenerationCheckBoxChange(int state);
		virtual void HandleTargetPluginInstallerButton();
		virtual void HandleOpenIntermediateFolderButton(QString sFolderPath="");
		virtual void HandleAssetTypeComboChange(const QString& assetType);
		virtual void HandleAssetTypeComboChange(int state);
        virtual void HandleExperimentalOptionsCheckBoxClicked();
        virtual void HandleLodSettingsButton();
		virtual void HandleEnableLodCheckBoxChange(int state);

	protected:
		DzBridgeAction* m_BridgeAction = nullptr;
		QSettings* settings = nullptr;

		virtual void refreshAsset();

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

        // Advanced settings
		QGroupBox* advancedSettingsGroupBox = nullptr;
		QWidget* advancedWidget = nullptr;
		QComboBox* fbxVersionCombo = nullptr;
		QCheckBox* showFbxDialogCheckBox = nullptr;
		QCheckBox* enableNormalMapGenerationCheckBox = nullptr;
		QCheckBox* exportMaterialPropertyCSVCheckBox = nullptr;
		QWidget* m_wTargetPluginInstaller = nullptr;
		QPushButton* m_TargetPluginInstallerButton = nullptr;
		QComboBox* m_TargetSoftwareVersionCombo = nullptr;
		QLabel* m_BridgeVersionLabel = nullptr;
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

		// LOD settings
		QPushButton* m_wLodSettingsButton = nullptr;
		QCheckBox* m_wEnableLodCheckBox = nullptr;
		QWidget* m_wLodRowLabelWidget = nullptr;

		QString m_sEmbeddedFilesPath = ":/DazBridge";
		bool m_bDontSaveSettings = false;
		bool m_bSetupMode = false;

#ifdef UNITTEST_DZBRIDGE
		friend class ::UnitTest_DzBridgeDialog;
#endif

	};

}
