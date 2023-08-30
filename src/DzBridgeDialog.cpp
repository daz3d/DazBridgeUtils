#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>
#include <QtGui/QToolTip>
#include <QtGui/QWhatsThis>
#include <QtGui/qlineedit.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qfiledialog.h>
#include <QtCore/qsettings.h>
#include <QtGui/qformlayout.h>
#include <QtGui/qcombobox.h>
#include <QtGui/qdesktopservices.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/qlistwidget.h>
#include <QtGui/qgroupbox.h>

#include "qmessagebox.h"

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"

#include "DzBridgeAction.h"
#include "DzBridgeDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"
#include "DzBridgeLodSettingsDialog.h"
#include "common_version.h"

#include "zip.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"

using namespace DzBridgeNameSpace;

bool DzBridgeDialog::setBridgeActionObject(QObject* arg) {
	DzBridgeAction* action = qobject_cast<DzBridgeAction*>(arg);
	if (action)
	{
		m_BridgeAction = (DzBridgeAction*)arg;
		return true;
	}
	return false;
}

DzBridgeDialog::DzBridgeDialog(QWidget *parent, const QString &windowTitle) :
	DzBasicDialog(parent, DAZ_BRIDGE_LIBRARY_NAME)
{
	if (dzScene->getPrimarySelection() == nullptr)
	{
		m_bSetupMode = true;
	}

#ifdef VODSVERSION
	m_bSetupMode = false;
#endif

	 assetNameEdit = nullptr;
//	 projectEdit = nullptr;
//	 projectButton = nullptr;
	 assetTypeCombo = nullptr;
	 morphsButton = nullptr;
	 morphsEnabledCheckBox = nullptr;
	 subdivisionButton = nullptr;
	 subdivisionEnabledCheckBox = nullptr;
	 advancedSettingsGroupBox = nullptr;
	 fbxVersionCombo = nullptr;
	 showFbxDialogCheckBox = nullptr;
	 animationSettingsGroupBox = nullptr;
	 m_wLodSettingsButton = nullptr;
	 m_wEnableLodCheckBox = nullptr;

	 settings = nullptr;
	 m_wTargetPluginInstaller = nullptr;

	// Declarations
	int margin = style()->pixelMetric(DZ_PM_GeneralMargin);
	int wgtHeight = style()->pixelMetric(DZ_PM_ButtonHeight);
	int btnMinWidth = style()->pixelMetric(DZ_PM_ButtonMinWidth);

	// Set the dialog title
	int revision = COMMON_REV % 1000;
	QString workingTitle;
	if (windowTitle != "")
		workingTitle = windowTitle + QString(tr(" %1 v%2.%3.%4")).arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD);
	else
		workingTitle = QString(tr("DazBridge %1 v%2.%3.%4").arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD));
	setWindowTitle(workingTitle);
	layout()->setSizeConstraint(QLayout::SetFixedSize);
	mainLayout = new QFormLayout();
	mainLayout->setMargin(5);

	QString sSetupModeString = tr("<h4>\
If this is your first time using this bridge, please be sure to read or watch \
any provided tutorials or videos to fully install and configure the bridge.<br><br>\
Once configured, please add a Character or Prop to the Scene to transfer assets using the Daz Bridge.</h4><br>\
To find out more about Daz Bridges, go to <a href=\"https://www.daz3d.com/daz-bridges\">https://www.daz3d.com/daz-bridges</a><br>\
");
	m_WelcomeLabel = new QLabel();
	m_WelcomeLabel->setTextFormat(Qt::RichText);
	m_WelcomeLabel->setWordWrap(true);
	m_WelcomeLabel->setText(sSetupModeString);
	m_WelcomeLabel->setOpenExternalLinks(true);
#ifdef VODSVERSION
	m_WelcomeLabel->setHidden(true);
#else
	mainLayout->addRow(m_WelcomeLabel);
#endif

	advancedWidget = new QWidget();
	QHBoxLayout* advancedLayoutOuter = new QHBoxLayout();
	advancedLayoutOuter->addWidget(advancedWidget);
	advancedLayout = new QFormLayout();
	advancedWidget->setLayout(advancedLayout);

	// Asset Name
	assetNameEdit = new QLineEdit(this);
	assetNameEdit->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]*"), this));

	// Asset Transfer Type
	assetTypeCombo = new QComboBox(this);
	assetTypeCombo->addItem("Skeletal Mesh");
	assetTypeCombo->addItem("Static Mesh");
	assetTypeCombo->addItem("Animation");
	assetTypeCombo->addItem("Environment");
	assetTypeCombo->addItem("Pose");
	//assetTypeCombo->addItem("MLDeformer");
	connect(assetTypeCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(HandleAssetTypeComboChange(const QString&)));
	// Connect new asset type handler
	connect(assetTypeCombo, SIGNAL(activated(int)), this, SLOT(HandleAssetTypeComboChange(int)));

	// Animation Settings
#ifdef VODSVERSION
    animationSettingsGroupBox = new QGroupBox("Animation Settings", this);
#else
    animationSettingsGroupBox = new QGroupBox("Experimental Animation Settings", this);
#endif
	QFormLayout* animationSettingsLayout = new QFormLayout();
	animationSettingsGroupBox->setLayout(animationSettingsLayout);
	experimentalAnimationExportCheckBox = new QCheckBox("", animationSettingsGroupBox);
	experimentalAnimationExportCheckBox->setChecked(true);
	animationSettingsLayout->addRow("Use New Export", experimentalAnimationExportCheckBox);

    // DB 2023-Aug-09: bake animation does not appear to be hooked up to anything, disabling for now
	bakeAnimationExportCheckBox = new QCheckBox("", animationSettingsGroupBox);
//	animationSettingsLayout->addRow("Bake", bakeAnimationExportCheckBox);
    bakeAnimationExportCheckBox->setVisible(false);
    bakeAnimationExportCheckBox->setDisabled(true);

	faceAnimationExportCheckBox = new QCheckBox("", animationSettingsGroupBox);
	animationSettingsLayout->addRow("Transfer Face Bones", faceAnimationExportCheckBox);
	animationExportActiveCurvesCheckBox = new QCheckBox("", animationSettingsGroupBox);
	animationSettingsLayout->addRow("Transfer Active Curves", animationExportActiveCurvesCheckBox);
	animationApplyBoneScaleCheckBox = new QCheckBox("", animationSettingsGroupBox);
	animationSettingsLayout->addRow("Apply Bone Scale", animationApplyBoneScaleCheckBox);
	animationSettingsGroupBox->setVisible(false);
                                  
    // Animation Help Text
    const char* AnimationExportHelpText = "New custom animation export pathway which can produce\n\
better quality.  **DOES NOT EXPORT MESH**";
    experimentalAnimationExportCheckBox->setWhatsThis(tr(AnimationExportHelpText));
    experimentalAnimationExportCheckBox->setToolTip(tr(AnimationExportHelpText));
    const char* BakeAnimationHelpText ="Bake complex animations to their base componenents.";
    bakeAnimationExportCheckBox->setWhatsThis(tr(BakeAnimationHelpText));
    bakeAnimationExportCheckBox->setToolTip(tr(BakeAnimationHelpText));
	const char* FaceAnimationHelpText = "Export animated face bones.";
    faceAnimationExportCheckBox->setWhatsThis(tr(FaceAnimationHelpText));
	faceAnimationExportCheckBox->setToolTip(tr(FaceAnimationHelpText));
	const char* ActiveCurvesHelpText = "Export animated properties.";
    animationExportActiveCurvesCheckBox->setWhatsThis(tr(ActiveCurvesHelpText));
	animationExportActiveCurvesCheckBox->setToolTip(tr(ActiveCurvesHelpText));
	const char* ApplyBoneScaleHelpText = "Apply bone scale values to animations.";
    animationApplyBoneScaleCheckBox->setWhatsThis(tr(ApplyBoneScaleHelpText));
	animationApplyBoneScaleCheckBox->setToolTip(tr(ApplyBoneScaleHelpText));

	// Morphs
	QHBoxLayout* morphsLayout = new QHBoxLayout();
	morphsButton = new QPushButton("Choose Morphs", this);
	connect(morphsButton, SIGNAL(released()), this, SLOT(HandleChooseMorphsButton()));
	morphsEnabledCheckBox = new QCheckBox("", this);
	morphsEnabledCheckBox->setMaximumWidth(25);
	morphsLayout->addWidget(morphsEnabledCheckBox);
	morphsLayout->addWidget(morphsButton);
	connect(morphsEnabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleMorphsCheckBoxChange(int)));

	// Morph Settings
	morphSettingsGroupBox = new QGroupBox("Morph Settings", this);
	QFormLayout* morphSettingsLayout = new QFormLayout();
	morphSettingsGroupBox->setLayout(morphSettingsLayout);
	morphLockBoneTranslationCheckBox = new QCheckBox("", morphSettingsGroupBox);
	morphLockBoneTranslationCheckBox->setChecked(false);
	morphSettingsLayout->addRow("Lock Bone Translation for Morphs", morphLockBoneTranslationCheckBox);
	morphSettingsGroupBox->setVisible(false);

	// Subdivision
	QHBoxLayout* subdivisionLayout = new QHBoxLayout();
	subdivisionButton = new QPushButton(tr("Bake Subdivision Levels"), this);
	connect(subdivisionButton, SIGNAL(released()), this, SLOT(HandleChooseSubdivisionsButton()));
	subdivisionEnabledCheckBox = new QCheckBox("", this);
	subdivisionEnabledCheckBox->setMaximumWidth(25);
	subdivisionLayout->addWidget(subdivisionEnabledCheckBox);
	subdivisionLayout->addWidget(subdivisionButton);
	connect(subdivisionEnabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleSubdivisionCheckBoxChange(int)));

	// LOD Settings
	QHBoxLayout* lodSettingsLayout = new QHBoxLayout();
	lodSettingsLayout->setContentsMargins(0,0,0,0);
	m_wLodSettingsButton = new QPushButton(tr("Configure LOD Settings"), this);
	connect(m_wLodSettingsButton, SIGNAL(released()), this, SLOT(HandleLodSettingsButton()));
	m_wEnableLodCheckBox = new QCheckBox("", this);
	m_wEnableLodCheckBox->setMaximumWidth(25);
	lodSettingsLayout->addWidget(m_wEnableLodCheckBox);
	lodSettingsLayout->addWidget(m_wLodSettingsButton);
	connect(m_wEnableLodCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleEnableLodCheckBoxChange(int)));

	/////////////////// Advanced Settings Section /////////////////////

	// FBX Version
	fbxVersionCombo = new QComboBox(this);
	fbxVersionCombo->addItem("FBX 2014 -- Binary");
	fbxVersionCombo->addItem("FBX 2014 -- Ascii");
	fbxVersionCombo->addItem("FBX 2013 -- Binary");
	fbxVersionCombo->addItem("FBX 2013 -- Ascii");
	fbxVersionCombo->addItem("FBX 2012 -- Binary");
	fbxVersionCombo->addItem("FBX 2012 -- Ascii");
	fbxVersionCombo->addItem("FBX 2011 -- Binary");
	fbxVersionCombo->addItem("FBX 2011 -- Ascii");
	fbxVersionCombo->addItem("FBX 2010 -- Binary");
	fbxVersionCombo->addItem("FBX 2010 -- Ascii");
	fbxVersionCombo->addItem("FBX 2009 -- Binary");
	fbxVersionCombo->addItem("FBX 2009 -- Ascii");
	connect(fbxVersionCombo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(HandleFBXVersionChange(const QString &)));

	// Show FBX Dialog option
	showFbxDialogCheckBox = new QCheckBox("", this);
	connect(showFbxDialogCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleShowFbxDialogCheckBoxChange(int)));

	// Enable Normal Map Generation checkbox
	enableNormalMapGenerationCheckBox = new QCheckBox("", this);
	connect(enableNormalMapGenerationCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleEnableNormalMapGenerationCheckBoxChange(int)));

	// Export Material Property CSV option
	exportMaterialPropertyCSVCheckBox = new QCheckBox("", this);
	connect(exportMaterialPropertyCSVCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleExportMaterialPropertyCSVCheckBoxChange(int)));

	// Install Destination Software Bridge
#ifndef VODSVERSION
	m_wTargetPluginInstaller = new QWidget();
	QHBoxLayout* targetPluginInstallerLayout = new QHBoxLayout();
	m_TargetSoftwareVersionCombo = new QComboBox(m_wTargetPluginInstaller);
	m_TargetSoftwareVersionCombo->addItem("Software Version");
	m_TargetPluginInstallerButton = new QPushButton("Install Plugin", m_wTargetPluginInstaller);
	connect(m_TargetPluginInstallerButton, SIGNAL(clicked(bool)), this, SLOT(HandleTargetPluginInstallerButton()));
	targetPluginInstallerLayout->addWidget(m_TargetSoftwareVersionCombo, 2);
	targetPluginInstallerLayout->addWidget(m_TargetPluginInstallerButton, 1);
	m_wTargetPluginInstaller->setLayout(targetPluginInstallerLayout);
#endif

	// Bridge Software Version Label
	QString sBridgeVersionString = QString(tr("Daz Bridge Library %1 v%2.%3.%4")).arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD);
	m_BridgeVersionLabel = new QLabel(sBridgeVersionString);

	// Go To Intermediate Folder
	m_OpenIntermediateFolderButton = new QPushButton(tr("Open Intermediate Folder"));
	connect(m_OpenIntermediateFolderButton, SIGNAL(clicked(bool)), this, SLOT(HandleOpenIntermediateFolderButton()));

    // Use this->getEnableExperimentalOptions() to query state, see HandleAssetTypeComboChange() for example
    // Enable Experimental Settings
    m_enableExperimentalOptionsCheckBox = new QCheckBox("", this);
	connect(m_enableExperimentalOptionsCheckBox, SIGNAL(clicked(bool)), this, SLOT(HandleExperimentalOptionsCheckBoxClicked()));
                                  
	// Add the widget to the basic dialog
	mainLayout->addRow("Asset Name", assetNameEdit);
	mainLayout->addRow("Asset Type", assetTypeCombo);
	mainLayout->addRow("Export Morphs", morphsLayout);
	mainLayout->addRow("Bake Subdivision", subdivisionLayout);

	// Create LOD Row, then store lod row widget, then hide row as default state
	mainLayout->addRow("Enable LOD", lodSettingsLayout);
	m_wLodRowLabelWidget = mainLayout->itemAt(mainLayout->rowCount()-1, QFormLayout::LabelRole)->widget();
	this->showLodRow(false);

	// Advanced Settings Layout
	advancedLayout->addRow("", m_BridgeVersionLabel);
#ifndef VODSVERSION
	advancedLayout->addRow("Install Destination Plugin", m_wTargetPluginInstaller);
#endif
	advancedLayout->addRow("", m_OpenIntermediateFolderButton);
	showTargetPluginInstaller(false);
	advancedLayout->addRow("FBX Version", fbxVersionCombo);
	advancedLayout->addRow("Show FBX Dialog", showFbxDialogCheckBox);
	advancedLayout->addRow("Generate Normal Maps", enableNormalMapGenerationCheckBox);
	advancedLayout->addRow("Export Material CSV", exportMaterialPropertyCSVCheckBox);
    advancedLayout->addRow("Enable Experimental Options", m_enableExperimentalOptionsCheckBox);

	addLayout(mainLayout);

	// Add Animation settings to the main layout as a new row without header
	mainLayout->addRow(animationSettingsGroupBox);

	// Add Morph settings to the main layout as a new row without header
	mainLayout->addRow(morphSettingsGroupBox);
	
	// Advanced
	advancedSettingsGroupBox = new QGroupBox("Advanced Settings", this);
	advancedSettingsGroupBox->setLayout(advancedLayoutOuter);
	advancedSettingsGroupBox->setCheckable(true);
	advancedSettingsGroupBox->setChecked(false);
	advancedSettingsGroupBox->setFixedWidth(500); // This is what forces the whole forms width
	addWidget(advancedSettingsGroupBox);
	advancedWidget->setHidden(true);
	connect(advancedSettingsGroupBox, SIGNAL(clicked(bool)), this, SLOT(HandleShowAdvancedSettingsCheckBoxChange(bool)));

	// Help
	assetNameEdit->setWhatsThis("This is the name the asset will use in the destination software.");
	assetTypeCombo->setWhatsThis("Skeletal Mesh for something with moving parts, like a character\nStatic Mesh for things like props\nAnimation for a character animation.");
	subdivisionButton->setWhatsThis("Select Subdivision Detail Level to Bake into each exported mesh.");
	morphsButton->setWhatsThis("Select Morphs to export with asset.");
	fbxVersionCombo->setWhatsThis("The version of FBX to use when exporting assets.");
	showFbxDialogCheckBox->setWhatsThis("Checking this will show the FBX Dialog for adjustments before export.");
	exportMaterialPropertyCSVCheckBox->setWhatsThis("Checking this will write out a CSV of all the material properties.  Useful for reference when changing materials.");
	enableNormalMapGenerationCheckBox->setWhatsThis("Checking this will enable generation of Normal Maps for any surfaces that only have Bump Height Maps.");
	//m_wTargetPluginInstaller->setWhatsThis("Install a plugin to use Daz Bridge with the destination software.");
	QString sEnableLodHelp = tr("Enable Level of Detail (LOD) mesh.  Specific features depend on the destination software.");
	m_wLodRowLabelWidget->setWhatsThis(sEnableLodHelp);
	m_wLodRowLabelWidget->setToolTip(sEnableLodHelp);
	m_wEnableLodCheckBox->setWhatsThis(sEnableLodHelp);
	m_wEnableLodCheckBox->setToolTip(sEnableLodHelp);
	QString sLodSettingsHelp = tr("Configure how Level of Detail (LOD) meshes are set up or generated in the destination software.");
	m_wLodSettingsButton->setWhatsThis(sLodSettingsHelp);
	m_wLodSettingsButton->setToolTip(sLodSettingsHelp);


	// detect scene change
	connect(dzScene, SIGNAL(nodeSelectionListChanged()), this, SLOT(handleSceneSelectionChanged()));

	// Set Defaults
	resetToDefaults();

	if (m_bSetupMode)
	{
		setDisabled(true);
	}

#ifndef VODSVERSION
	m_WelcomeLabel->setVisible(true);
#endif
}

void DzBridgeDialog::renameTargetPluginInstaller(QString sNewLabelName)
{
	if (m_wTargetPluginInstaller == nullptr)
		return;

	auto wTargetPluginInstallerLabel = advancedLayout->labelForField(m_wTargetPluginInstaller);
	QLabel* rowLabel = qobject_cast<QLabel*>(wTargetPluginInstallerLabel);
	if (rowLabel != nullptr)
	{
		rowLabel->setText(sNewLabelName);
	}

	return;
}

void DzBridgeDialog::showTargetPluginInstaller(bool bShowWidget)
{
	if (m_wTargetPluginInstaller == nullptr)
		return;

	if (bShowWidget)
	{
		m_wTargetPluginInstaller->setVisible(true);
		auto targetPluginInstallerLabel = advancedLayout->labelForField(m_wTargetPluginInstaller);
		targetPluginInstallerLabel->setVisible(true);
		this->advancedLayout->update();
	}
	else
	{
		m_wTargetPluginInstaller->setVisible(false);
		auto targetPluginInstallerLabel = advancedLayout->labelForField(m_wTargetPluginInstaller);
		targetPluginInstallerLabel->setVisible(false);
		this->advancedLayout->update();
	}
	return;
}

bool DzBridgeDialog::loadSavedSettings()
{
	if (settings == nullptr)
	{
		return false;
	}

	if (!settings->value("MorphsEnabled").isNull())
	{
		morphsEnabledCheckBox->setChecked(settings->value("MorphsEnabled").toBool());
	}
	if (!settings->value("SubdivisionEnabled").isNull())
	{
		subdivisionEnabledCheckBox->setChecked(settings->value("SubdivisionEnabled").toBool());
	}
	if (!settings->value("ShowFBXDialog").isNull())
	{
		showFbxDialogCheckBox->setChecked(settings->value("ShowFBXDialog").toBool());
	}
	if (m_bSetupMode)
	{
		advancedSettingsGroupBox->setChecked(true);
		advancedWidget->setHidden(false);
	}
	else if (!settings->value("ShowAdvancedSettings").isNull())
	{
		advancedSettingsGroupBox->setChecked(settings->value("ShowAdvancedSettings").toBool());
		advancedWidget->setHidden(!advancedSettingsGroupBox->isChecked());
	}
	else
	{
		advancedSettingsGroupBox->setChecked(false);
		advancedWidget->setHidden(true);
	}
	if (!settings->value("FBXExportVersion").isNull())
	{
		int index = fbxVersionCombo->findText(settings->value("FBXExportVersion").toString());
		if (index != -1)
		{
			fbxVersionCombo->setCurrentIndex(index);
		}
	}
	if (!settings->value("EnableNormalMapGeneration").isNull())
	{
		enableNormalMapGenerationCheckBox->setChecked(settings->value("EnableNormalMapGeneration").toBool());
	}
	if (!settings->value("ExportMaterialPropertyCSV").isNull())
	{
		exportMaterialPropertyCSVCheckBox->setChecked(settings->value("ExportMaterialPropertyCSV").toBool());
	}

	// Animation settings
	if (!settings->value("AnimationExperminentalExport").isNull())
	{
		experimentalAnimationExportCheckBox->setChecked(settings->value("AnimationExperminentalExport").toBool());
	}
	if (!settings->value("AnimationBake").isNull())
	{
		bakeAnimationExportCheckBox->setChecked(settings->value("AnimationBake").toBool());
	}
	if (!settings->value("AnimationExportFace").isNull())
	{
		faceAnimationExportCheckBox->setChecked(settings->value("AnimationExportFace").toBool());
	}
	if (!settings->value("AnimationExportActiveCurves").isNull())
	{
		animationExportActiveCurvesCheckBox->setChecked(settings->value("AnimationExportActiveCurves").toBool());
	}
	if (!settings->value("AnimationApplyBoneScale").isNull())
	{
		animationApplyBoneScaleCheckBox->setChecked(settings->value("AnimationApplyBoneScale").toBool());
	}
	//if (!settings->value("MLDeformerPoseCount").isNull())
	//{
	//	mlDeformerPoseCountEdit->setText(settings->value("MLDeformerPoseCount").toString());
	//}

	return true;
}

// Some settings will be saved when Accept is hit so we don't need a hanlder attached to all of them
void DzBridgeDialog::saveSettings()
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	if (experimentalAnimationExportCheckBox == nullptr) return;
	settings->setValue("AnimationExperminentalExport", experimentalAnimationExportCheckBox->isChecked());
	settings->setValue("AnimationBake", bakeAnimationExportCheckBox->isChecked());
	settings->setValue("AnimationExportFace", faceAnimationExportCheckBox->isChecked());
	settings->setValue("AnimationExportActiveCurves", animationExportActiveCurvesCheckBox->isChecked());
	settings->setValue("AnimationApplyBoneScale", animationApplyBoneScaleCheckBox->isChecked());
	//settings->setValue("MLDeformerPoseCount", mlDeformerPoseCountEdit->text().toInt());
}

void DzBridgeDialog::refreshAsset()
{
	DzNode* Selection = dzScene->getPrimarySelection();

	if (dzScene->getFilename().length() > 0)
	{
		QFileInfo fileInfo = QFileInfo(dzScene->getFilename());
		assetNameEdit->setText(fileInfo.baseName().remove(QRegExp("[^A-Za-z0-9_]")));
	}
	else if (dzScene->getPrimarySelection())
	{
		assetNameEdit->setText(Selection->getLabel().remove(QRegExp("[^A-Za-z0-9_]")));
	}

	if (qobject_cast<DzSkeleton*>(Selection))
	{
		assetTypeCombo->setCurrentIndex(0);
	}
	else
	{
		assetTypeCombo->setCurrentIndex(1);
	}

}

void DzBridgeDialog::accept()
{
	if (m_bSetupMode)
		return  DzBasicDialog::reject();

	saveSettings();
	return DzBasicDialog::accept();
}

void DzBridgeDialog::resetToDefaults()
{
	m_bDontSaveSettings = true;
	// Set Defaults
	refreshAsset();

	subdivisionEnabledCheckBox->setChecked(false);
	morphsEnabledCheckBox->setChecked(false);
	showFbxDialogCheckBox->setChecked(false);
	exportMaterialPropertyCSVCheckBox->setChecked(false);
	m_bDontSaveSettings = false;
}

void DzBridgeDialog::handleSceneSelectionChanged()
{
	refreshAsset();

	if (dzScene->getPrimarySelection() == nullptr)
	{
		m_bSetupMode = true;
		setDisabled(true);
	}
	else
	{
		m_bSetupMode = false;
		setDisabled(false);
	}

	// DB (2022-Sept-26): Crashfix for changing selection and exporting without opening morphselectiondialog
	DzBridgeMorphSelectionDialog* morphDialog = DzBridgeMorphSelectionDialog::Get(this);
	morphDialog->PrepareDialog();

}

int DzBridgeDialog::HandleChooseMorphsButton()
{
	DzBridgeMorphSelectionDialog * morphDialog = DzBridgeMorphSelectionDialog::Get(this);
	int dialogReturnCode = morphDialog->exec();
	return dialogReturnCode;
}

void DzBridgeDialog::HandleChooseSubdivisionsButton()
{
	DzBridgeSubdivisionDialog *subdivisionDialog = DzBridgeSubdivisionDialog::Get(this);
	subdivisionDialog->exec();
}

QString DzBridgeDialog::GetMorphString()
{
	DzBridgeMorphSelectionDialog* morphDialog = DzBridgeMorphSelectionDialog::Get(this);
	return morphDialog->GetMorphString();
}

QMap<QString, QString> DzBridgeDialog::GetMorphMappingFromMorphSelectionDialog()
{
	DzBridgeMorphSelectionDialog* morphDialog = DzBridgeMorphSelectionDialog::Get(this);
	return morphDialog->GetMorphMapping();
}

QList<QString> DzBridgeDialog::GetPoseList()
{
	return DzBridgeMorphSelectionDialog::Get(this)->GetPoseList();
}

void DzBridgeDialog::HandleMorphsCheckBoxChange(int state)
{
	morphSettingsGroupBox->setHidden(state != Qt::Checked);
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("MorphsEnabled", state == Qt::Checked);
}

void DzBridgeDialog::HandleSubdivisionCheckBoxChange(int state)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("SubdivisionEnabled", state == Qt::Checked);
}

void DzBridgeDialog::HandleFBXVersionChange(const QString& fbxVersion)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("FBXExportVersion", fbxVersion);
}
void DzBridgeDialog::HandleShowFbxDialogCheckBoxChange(int state)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("ShowFBXDialog", state == Qt::Checked);
}
void DzBridgeDialog::HandleExportMaterialPropertyCSVCheckBoxChange(int state)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("ExportMaterialPropertyCSV", state == Qt::Checked);
}

void DzBridgeDialog::HandleShowAdvancedSettingsCheckBoxChange(bool checked)
{
	if (m_bSetupMode)
	{
		advancedWidget->setHidden(false);
		advancedSettingsGroupBox->setChecked(true);
		return;
	}

	advancedWidget->setHidden(!checked);

	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("ShowAdvancedSettings", checked);
}
void DzBridgeDialog::HandleEnableNormalMapGenerationCheckBoxChange(int state)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("EnableNormalMapGeneration", state == Qt::Checked);
}

void DzBridgeDialog::HandleTargetPluginInstallerButton()
{
	if (m_wTargetPluginInstaller == nullptr) return;

	// Validate software version and set resource zip files to extract
	QString softwareVersion = m_TargetSoftwareVersionCombo->currentText();
	if (softwareVersion == "" || softwareVersion.contains("select"))
	{
		// Warning, not a valid plugins folder path
		QMessageBox::information(0, "Daz Bridge",
			tr("Please select a software version."));
		return;
	}
	QString sPluginZipFilename = "/ThisIsExampleFilenameOnlyAndWillNotWork.zip"; // Example
	QString sEmbeddedPath = ":/DazBridge";
	QString sPluginZipPath = sEmbeddedPath + sPluginZipFilename;

	// For first run, display help / explanation popup dialog...
	// TODO

	// Get Destination Folder
	QString directoryName = QFileDialog::getExistingDirectory(this,
		tr("Choose the correct path to install plugins for your target software"),
		"/home",
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	if (directoryName == NULL)
	{
		// User hit cancel: return without addition popups
		return;
	}

	// fix path separators
	directoryName = directoryName.replace("\\", "/");

	// Validate selected Folder is valid for plugin
	bool bIsValidPluginPath = false;
	QString sPluginsPath = directoryName;

	if (bIsValidPluginPath == false)
	{
		// Warning, not a valid plugins folder path
		auto userChoice = QMessageBox::warning(0, "Daz Bridge",
			tr("The selected folder is not a valid plugins folder.  Please select a \
valid plugins folder for your sofware.\n\nYou can choose to Abort and select a new folder, \
or Ignore this error and install the plugin anyway."),
QMessageBox::Ignore | QMessageBox::Abort,
QMessageBox::Abort);
		if (userChoice == QMessageBox::StandardButton::Abort)
			return;
	}

	// create plugins folder if does not exist
	if (QDir(sPluginsPath).exists() == false)
	{
		QDir().mkdir(sPluginsPath);
	}

	bool bInstallSuccessful = false;
	bInstallSuccessful = installEmbeddedArchive(sPluginZipFilename, directoryName);

	// verify successful plugin extraction/installation
	if (bInstallSuccessful)
	{
		QMessageBox::information(0, "Daz Bridge",
			tr("Plugin successfully installed to: ") + sPluginsPath +
			tr("\n\nIf the target software is open, please quit and restart it to continue \
Bridge Export process."));
	}
	else
	{
		QMessageBox::warning(0, "Daz Bridge",
			tr("Sorry, an unknown error occured. Unable to install \
target software Plugin to: ") + sPluginsPath);
		return;
	}

	return;
}

bool DzBridgeDialog::installEmbeddedArchive(QString sArchiveFilename, QString sDestinationPath)
{
	bool bInstallSuccessful = false;

	QString sEmbeddedArchivePath = m_sEmbeddedFilesPath + sArchiveFilename;

	// copy zip plugin to temp
	bool replace = true;
	QFile srcFile(sEmbeddedArchivePath);
	QString tempPathArchive = dzApp->getTempPath() + sArchiveFilename;
	DzBridgeAction::copyFile(&srcFile, &tempPathArchive, replace);
	srcFile.close();

	// extract to destionation
	::zip_extract(tempPathArchive.toAscii().data(), sDestinationPath.toAscii().data(), nullptr, nullptr);

	// verify extraction was successfull
	// 1. get filename from archive
	// 2. test to see if destination path contains filename
	QStringList archiveFileNames;
	struct zip_t* zip = ::zip_open(tempPathArchive.toAscii().data(), 0, 'r');
	int i, n = ::zip_entries_total(zip);
	for (i = 0; i < n; ++i) {
		::zip_entry_openbyindex(zip, i);
		{
			const char* name = ::zip_entry_name(zip);
			archiveFileNames.append(QString(name));
			//int isdir = ::zip_entry_isdir(zip);
			//unsigned long long size = ::zip_entry_size(zip);
			//unsigned int crc32 = ::zip_entry_crc32(zip);
		}
		::zip_entry_close(zip);
	}
	::zip_close(zip);
	bInstallSuccessful = true;
	for (QString filename : archiveFileNames)
	{
		QString filePath = sDestinationPath + "/" + filename;
		if (QFile(filePath).exists() == false)
		{
			bInstallSuccessful = false;
			break;
		}
	}

	// remove if succcessful, else leave intermediate files for debugging
	if (bInstallSuccessful)
		QFile(tempPathArchive).remove();

	return bInstallSuccessful;
}

void DzBridgeDialog::setBridgeVersionStringAndLabel(QString sVersionString, QString sLabel)
{
	if (m_BridgeVersionLabel == nullptr)
		return;

	m_BridgeVersionLabel->setText(sVersionString);

	if (sLabel != "")
	{
		auto wBridgeVersionLabel_Label = advancedLayout->labelForField(m_BridgeVersionLabel);
		QLabel* rowLabel = qobject_cast<QLabel*>(wBridgeVersionLabel_Label);
		if (rowLabel != nullptr)
		{
			rowLabel->setText(sLabel);
		}
	}

	return;
}

void DzBridgeDialog::setDisabled(bool bDisabled)
{
	if (bDisabled)
	{
		advancedWidget->setHidden(false);
		advancedSettingsGroupBox->setChecked(true);
	}

//	m_WelcomeLabel->setVisible(bDisabled);
	assetNameEdit->setDisabled(bDisabled);
	assetTypeCombo->setDisabled(bDisabled);
	subdivisionButton->setDisabled(bDisabled);
	morphsButton->setDisabled(bDisabled);
	m_wLodSettingsButton->setDisabled(bDisabled);

}

#include <qprocess.h>
#if WIN32
#include "Windows.h"
#endif
void DzBridgeDialog::HandleOpenIntermediateFolderButton(QString sFolderPath)
{
	QString sIntermediateFolderPath = QDir().homePath() + "/Documents";
	if (sFolderPath != "")
	{
		sIntermediateFolderPath = sFolderPath;
	}

#ifdef WIN32
	ShellExecuteA(NULL, "open", sIntermediateFolderPath.toAscii().data(), NULL, NULL, SW_SHOWDEFAULT);
//// The above line does the equivalent as following lines, but has advantage of only opening 1 explorer window
//// with multiple clicks.
//
//	QStringList args;
//	args << "/select," << QDir::toNativeSeparators(sIntermediateFolderPath);
//	QProcess::startDetached("explorer", args);
//
#elif defined(__APPLE__)
	QStringList args;
	args << "-e";
	args << "tell application \"Finder\"";
	args << "-e";
	args << "activate";
	args << "-e";
	args << "select POSIX file \"" + sIntermediateFolderPath + "\"";
	args << "-e";
	args << "end tell";
	QProcess::startDetached("osascript", args);
#endif

}

void DzBridgeDialog::HandleAssetTypeComboChange(const QString& assetType)
{
#ifdef VODSVERSION
	animationSettingsGroupBox->setVisible(assetType == "Animation" || assetType == "Pose");
    //mlDeformerSettingsGroupBox->setVisible(assetType == "MLDeformer");
#else
    if (this->getEnableExperimentalOptions()) {
		animationSettingsGroupBox->setVisible(assetType == "Animation" || assetType == "Pose");
	}
	else {
		animationSettingsGroupBox->setVisible(false);
        experimentalAnimationExportCheckBox->setChecked(false);
        bakeAnimationExportCheckBox->setChecked(false);
        faceAnimationExportCheckBox->setChecked(false);
        animationExportActiveCurvesCheckBox->setChecked(false);
        animationApplyBoneScaleCheckBox->setChecked(false);
    }
#endif
}

void DzBridgeDialog::HandleAssetTypeComboChange(int state)
{
	QString assetNameString = assetNameEdit->text();

	// TODO: replace string compare with itemData system
	QString sAssetType = assetTypeCombo->currentText();

	/// Enable morphs if:
	/// skeletal mesh, static mesh, animation
	if (sAssetType == "Skeletal Mesh" ||
		sAssetType == "Static Mesh" ||
		sAssetType == "Animation" ||
		sAssetType == "Pose" )
	{
		morphsEnabledCheckBox->setDisabled(false);
		morphsButton->setDisabled(false);
	}
	else
	{
		morphsEnabledCheckBox->setChecked(false);
		morphsEnabledCheckBox->setDisabled(true);
		morphsButton->setDisabled(true);
	}

	/// Enable subdivision and lod only if:
	/// skeletal mesh, static mesh
	if (sAssetType == "Skeletal Mesh" ||
		sAssetType == "Static Mesh" )
	{
		subdivisionEnabledCheckBox->setDisabled(false);
		subdivisionButton->setDisabled(false);
		m_wEnableLodCheckBox->setDisabled(false);
		m_wLodSettingsButton->setDisabled(false);
	}
	else
	{
		subdivisionEnabledCheckBox->setChecked(false);
		subdivisionEnabledCheckBox->setDisabled(true);
		subdivisionButton->setDisabled(true);
		m_wEnableLodCheckBox->setChecked(false);
		m_wEnableLodCheckBox->setDisabled(true);
		m_wLodSettingsButton->setDisabled(true);
	}

}

void DzBridgeDialog::HandleExperimentalOptionsCheckBoxClicked()
{
	if (m_enableExperimentalOptionsCheckBox->isChecked())
	{
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, "Daz Bridge", "Enabling Experimental Options may cause unexpected results.  Are you sure you want to enable Experimental Options?",
			QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No)
		{
			m_enableExperimentalOptionsCheckBox->setChecked(false);
			return;
		}
	}

    // int state = m_enableExperimentalOptionsCheckBox->checkState();
	// if (settings == nullptr || m_bDontSaveSettings) return;
	// settings->setValue("EnableExperimentalOptions", state == Qt::Checked);

	// Intentionally saving as unchecked to force users to "opt-in" every time they use experimental features
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("EnableExperimentalOptions", false);

}

void DzBridgeDialog::HandleLodSettingsButton()
{
	DzBridgeLodSettingsDialog* lodSetttingsDialog = DzBridgeLodSettingsDialog::Get(this->m_BridgeAction, this);
	lodSetttingsDialog->exec();
}

void DzBridgeDialog::HandleEnableLodCheckBoxChange(int state)
{
	if (settings == nullptr || m_bDontSaveSettings) return;
	settings->setValue("LodEnabled", state == Qt::Checked);
}

void DzBridgeDialog::showLodRow(bool bShowWidget)
{
	if (m_wLodRowLabelWidget == nullptr) return;

	m_wEnableLodCheckBox->setVisible(bShowWidget);
	m_wLodSettingsButton->setVisible(bShowWidget);
	m_wLodRowLabelWidget->setVisible(bShowWidget);

}

#include "moc_DzBridgeDialog.cpp"
