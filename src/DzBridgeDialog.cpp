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
#include "dzmenubutton.h"

#include "DzBridgeAction.h"
#include "DzBridgeDialog.h"
#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeSubdivisionDialog.h"
#include "DzBridgeLodSettingsDialog.h"
#include "common_version.h"

#include "zip.h"
#include "MorphTools.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"
#define ADVANCED_OPTIONS_TITLE "Advanced Options : "

using namespace DzBridgeNameSpace;

DzBridgeBrowseEdit::DzBridgeBrowseEdit(const QString& text, QWidget* parent)
	: QWidget(parent)
{
	// create horizontal layout
	// add edit, add browser
}

#include "dzstyledbutton.h"
DzBridgeThinButton::DzBridgeThinButton(const QString& text, QWidget* parent)
//	: QPushButton(text, parent)
	: DzStyledButton(text, parent)
{
	setText(text);
	// set up default styles
	m_eDefaultElementStyle = getPrimitive();
	m_eDefaultTextStyle = getTextStyle();
}

void DzBridgeDialogTools::SetThinButtonText(QPushButton* widget, const QString& text)
{
	widget->setText(text);
	int labelSize = widget->fontMetrics().width("  " + text + "  ");
	int minSize = widget->style()->pixelMetric(DZ_PM_ButtonMinWidth);
	int buttonSize = labelSize;
	if (labelSize < minSize)
		buttonSize = minSize;
	widget->setFixedWidth(buttonSize);
	widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void DzBridgeThinButton::setText(const QString& text)
{
	DzBridgeDialogTools::SetThinButtonText(this, text);
}

void DzBridgeThinButton::setHighlightStyle(bool bHighlight)
{
	if (bHighlight) {
		setCustomPrimitive(DZ_PE_SpecialInterestButton);
		setCustomTextStyle(DZ_TS_SpecialInterestButton);
	}
	else {
		setCustomPrimitive(m_eDefaultElementStyle);
		setCustomTextStyle(m_eDefaultTextStyle);
	}
}

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
	DzOptionsDialog(parent, DAZ_BRIDGE_LIBRARY_NAME),  m_eAssetType(EAssetType::None)
{
	this->setObjectName("DzBridge_Base_Dialog");

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
	int nStyleMargin = style()->pixelMetric(DZ_PM_GeneralMargin);
	int nStyleButtonHeight = style()->pixelMetric(DZ_PM_ButtonHeight);
	int nStyleButtonMinWidth = style()->pixelMetric(DZ_PM_ButtonMinWidth);

	this->setOptionsMargin(nStyleMargin);

	// Set the dialog title
	int revision = COMMON_REV % 1000;
	QString workingTitle;
	if (windowTitle != "")
		workingTitle = windowTitle + QString(tr(" %1 v%2.%3.%4")).arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD);
	else
		workingTitle = QString(tr("DazBridge %1 v%2.%3.%4").arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD));
	setWindowTitle(workingTitle);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
	layout()->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);

	mainLayout = new QFormLayout();
	mainLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	mainLayout->setSpacing(nStyleMargin);
	mainLayout->setMargin(nStyleMargin);
	mainLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

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

	advancedLayout = new QFormLayout();
	advancedLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	advancedLayout->setSpacing(nStyleMargin);
	advancedLayout->setMargin(nStyleMargin);
	advancedLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

	// Asset Name
	assetNameEdit = new QLineEdit(this);
	assetNameEdit->setValidator(new QRegExpValidator(QRegExp("[A-Za-z0-9_]*"), this));
	assetNameEdit->setFixedHeight(nStyleButtonHeight);

	// Asset Transfer Type
	assetTypeCombo = new QComboBox(this);
	assetTypeCombo->setFixedHeight(nStyleButtonHeight);
	assetTypeCombo->addItem("Skeletal Mesh", EAssetType::SkeletalMesh);
	assetTypeCombo->addItem("Static Mesh", EAssetType::StaticMesh);
	assetTypeCombo->addItem("Animation", EAssetType::Animation);
	assetTypeCombo->addItem("Environment", EAssetType::Scene);
	assetTypeCombo->addItem("Pose", EAssetType::Pose);
	connect(assetTypeCombo, SIGNAL(activated(int)), this, SLOT(HandleAssetTypeComboChange(int)));

	// Animation Settings
#ifdef VODSVERSION
    animationSettingsGroupBox = new QGroupBox(tr("Animation Options : "), this);
#else
    animationSettingsGroupBox = new QGroupBox(tr("Experimental Animation Options : "), this);
#endif
	QFormLayout* animationSettingsLayout = new QFormLayout();
	animationSettingsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	animationSettingsGroupBox->setLayout(animationSettingsLayout);

#ifdef VODSVERSION
	QString sExperimentalAnimationExport = tr("Use new Export");
#else
	QString sExperimentalAnimationExport = tr("Export Bone Positions Only");
#endif
	experimentalAnimationExportCheckBox = new QCheckBox(sExperimentalAnimationExport, animationSettingsGroupBox);
	experimentalAnimationExportCheckBox->setChecked(true);
	animationSettingsLayout->addRow(experimentalAnimationExportCheckBox);

    // DB 2023-Aug-09: bake animation does not appear to be hooked up to anything, disabling for now
	QString sBakeAnimationExport = tr("Bake Animation");
	bakeAnimationExportCheckBox = new QCheckBox(sBakeAnimationExport, animationSettingsGroupBox);
//	animationSettingsLayout->addRow(bakeAnimationExportCheckBox);
    bakeAnimationExportCheckBox->setVisible(false);
    bakeAnimationExportCheckBox->setDisabled(true);

	QString sFaceAnimationExport = tr("Transfer Face Bones");
	faceAnimationExportCheckBox = new QCheckBox(sFaceAnimationExport, animationSettingsGroupBox);
	animationSettingsLayout->addRow(faceAnimationExportCheckBox);
	QString sAnimationExportActiveCurves = tr("Transfer Active Curves");
	animationExportActiveCurvesCheckBox = new QCheckBox(sAnimationExportActiveCurves, animationSettingsGroupBox);
	animationSettingsLayout->addRow(animationExportActiveCurvesCheckBox);
	QString sAnimationApplyBoneScale = tr("Apply Bone Scale");
	animationApplyBoneScaleCheckBox = new QCheckBox(sAnimationApplyBoneScale, animationSettingsGroupBox);
	animationSettingsLayout->addRow(animationApplyBoneScaleCheckBox);
	animationSettingsGroupBox->setVisible(false);

	animationSettingsLayout->invalidate();

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
//	morphsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	morphsLayout->setContentsMargins(0,0,0,0);
	morphsLayout->setSpacing(0);
	morphsButton = new QPushButton(tr("Choose Morphs..."), this);
	morphsButton->setObjectName("DzBridge_ChoooseMorphs_Button");
	connect(morphsButton, SIGNAL(released()), this, SLOT(HandleChooseMorphsButton()));
	morphsEnabledCheckBox = new QCheckBox("", this);
	morphsEnabledCheckBox->setObjectName("DzBridge_EnableMorphExport_Checkbox");
	morphsEnabledCheckBox->setMaximumWidth(25);
	morphsLayout->addWidget(morphsEnabledCheckBox);
	morphsLayout->addWidget(morphsButton);
	connect(morphsEnabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleMorphsCheckBoxChange(int)));

	// Morph Settings
	morphSettingsGroupBox = new QGroupBox(tr("Morph Options : "), this);
	QFormLayout* morphSettingsLayout = new QFormLayout();
	morphSettingsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	morphSettingsLayout->setSpacing(nStyleMargin);
	morphSettingsGroupBox->setLayout(morphSettingsLayout);
	morphSettingsGroupBox->setVisible(false);

	QString sMorphLockBoneTranslationOption = tr("Lock Bone Translations");
	morphLockBoneTranslationCheckBox = new QCheckBox(sMorphLockBoneTranslationOption, morphSettingsGroupBox);
	morphLockBoneTranslationCheckBox->setChecked(false);
	morphSettingsLayout->addRow(morphLockBoneTranslationCheckBox);

	// 2025-04-14, DB: Refactor MorphSelectionDialog Settings (move to main export options dialog)
	m_wAutoJCMCheckBox = new QCheckBox("Auto JCM");
	m_wAutoJCMCheckBox->setChecked(false);
	m_wAutoJCMCheckBox->setVisible(false);
	morphSettingsLayout->addRow(m_wAutoJCMCheckBox);

	m_wFakeDualQuatCheckBox = new QCheckBox("Fake Dual Quat");
	m_wFakeDualQuatCheckBox->setChecked(false);
	m_wFakeDualQuatCheckBox->setVisible(false);
	m_wFakeDualQuatCheckBox->setWhatsThis("Adds additional JCMs that fake the difference between Linear Blending and Dual Quaternion Skinning.");
	morphSettingsLayout->addRow(m_wFakeDualQuatCheckBox);

	m_wAllowMorphDoubleDippingCheckBox = new QCheckBox(tr("Allow Morph Double-Dipping"));
	m_wAllowMorphDoubleDippingCheckBox->setChecked(false);
	m_wAllowMorphDoubleDippingCheckBox->setVisible(true);
	QString sAllowDoubleDippingHelpText = QString(tr("\
Allow Connected Morphs such as Victoria 9 and Victoria 9 Head and Victoria 9 Body to all fully contribute \n\
to the exported blendshape when they are exported simultaneously. \n\n\
WARNING: this will cause 200% or similar morph distortion when they are all applied together and may break \n\
functionality for some Morph and JCM products.\
"));
	m_wAllowMorphDoubleDippingCheckBox->setWhatsThis(sAllowDoubleDippingHelpText);
	m_wAllowMorphDoubleDippingCheckBox->setToolTip(sAllowDoubleDippingHelpText);
	morphSettingsLayout->addRow(m_wAllowMorphDoubleDippingCheckBox);

	// Subdivision
	QHBoxLayout* subdivisionLayout = new QHBoxLayout();
//	subdivisionLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	subdivisionLayout->setContentsMargins(0,0,0,0);
	subdivisionLayout->setSpacing(0);
	subdivisionButton = new QPushButton(tr("Bake Subdivision Levels..."), this);
	subdivisionButton->setObjectName("DzBridge_ConfigureSubdivisionLevels_Button");
	connect(subdivisionButton, SIGNAL(released()), this, SLOT(HandleChooseSubdivisionsButton()));
	subdivisionEnabledCheckBox = new QCheckBox("", this);
	subdivisionEnabledCheckBox->setObjectName("DzBridge_EnableSubdivisionExport_Checkbox");
	subdivisionEnabledCheckBox->setMaximumWidth(25);
	subdivisionLayout->addWidget(subdivisionEnabledCheckBox);
	subdivisionLayout->addWidget(subdivisionButton);
	connect(subdivisionEnabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleSubdivisionCheckBoxChange(int)));

	// LOD Settings
	QHBoxLayout* lodSettingsLayout = new QHBoxLayout();
//	lodSettingsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	lodSettingsLayout->setContentsMargins(0,0,0,0);
	lodSettingsLayout->setSpacing(0);
	m_wLodSettingsButton = new QPushButton(tr("Configure LOD Settings..."), this);
	m_wLodSettingsButton->setObjectName("DzBridge_ConfigureLodSettings_Button");
	connect(m_wLodSettingsButton, SIGNAL(released()), this, SLOT(HandleLodSettingsButton()));
	m_wEnableLodCheckBox = new QCheckBox("", this);
	m_wEnableLodCheckBox->setObjectName("DzBridge_EnableLodExport_Checkbox");
	m_wEnableLodCheckBox->setMaximumWidth(25);
	lodSettingsLayout->addWidget(m_wEnableLodCheckBox);
	lodSettingsLayout->addWidget(m_wLodSettingsButton);
	connect(m_wEnableLodCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleEnableLodCheckBoxChange(int)));

	/////////////////// Advanced Settings Section /////////////////////

	// FBX Version
	fbxVersionCombo = new QComboBox(this);
	fbxVersionCombo->setObjectName("DzBridge_FbxVersion_Combobox");
	fbxVersionCombo->setFixedHeight(nStyleButtonHeight);
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
	showFbxDialogCheckBox->setObjectName("DzBridge_ShowFbxDialog_Checkbox");
	connect(showFbxDialogCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleShowFbxDialogCheckBoxChange(int)));

	// Export Material Property CSV option
	exportMaterialPropertyCSVCheckBox = new QCheckBox("", this);
	exportMaterialPropertyCSVCheckBox->setObjectName("DzBridge_ExportMaterialPropertyCsv_Checkbox");
	connect(exportMaterialPropertyCSVCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleExportMaterialPropertyCSVCheckBoxChange(int)));

	// Use this->getEnableExperimentalOptions() to query state, see HandleAssetTypeComboChange() for example
	// Enable Experimental Settings
	m_enableExperimentalOptionsCheckBox = new QCheckBox("", this);
	m_enableExperimentalOptionsCheckBox->setObjectName("DzBridge_EnableExperimentalOptions_Checkbox");
	connect(m_enableExperimentalOptionsCheckBox, SIGNAL(clicked(bool)), this, SLOT(HandleExperimentalOptionsCheckBoxClicked()));

	// Install Destination Software Bridge
#ifndef VODSVERSION
	m_wTargetPluginInstaller = new QWidget();
	QHBoxLayout* targetPluginInstallerLayout = new QHBoxLayout();
//	targetPluginInstallerLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	targetPluginInstallerLayout->setContentsMargins(0,0,0,0);
	targetPluginInstallerLayout->setSpacing(nStyleMargin);
	m_TargetSoftwareVersionCombo = new QComboBox(m_wTargetPluginInstaller);
	m_TargetSoftwareVersionCombo->setFixedHeight(nStyleButtonHeight);
	m_TargetSoftwareVersionCombo->addItem(tr("Software Version"));
	m_TargetPluginInstallerButton = new QPushButton(tr("Install Plugin..."), m_wTargetPluginInstaller);
	m_TargetPluginInstallerButton->setObjectName("DzBridge_InstallTargetPlugin_Button");
	connect(m_TargetPluginInstallerButton, SIGNAL(clicked(bool)), this, SLOT(HandleTargetPluginInstallerButton()));
	targetPluginInstallerLayout->addWidget(m_TargetSoftwareVersionCombo, 2);
	targetPluginInstallerLayout->addWidget(m_TargetPluginInstallerButton, 1);
	m_wTargetPluginInstaller->setLayout(targetPluginInstallerLayout);
#endif

	// Bridge Software Version Label
//	QString sBridgeVersionString = QString(tr("Daz Bridge Library %1 v%2.%3.%4")).arg(COMMON_MAJOR).arg(COMMON_MINOR).arg(revision).arg(COMMON_BUILD);
//	m_BridgeVersionLabel = new QLabel(sBridgeVersionString);

	// Go To Intermediate Folder
	m_OpenIntermediateFolderButton = new QPushButton(tr("Open Intermediate Folder..."));
	m_OpenIntermediateFolderButton->setObjectName("DzBridge_OpenIntermediateFolder_Button");
	connect(m_OpenIntermediateFolderButton, SIGNAL(clicked(bool)), this, SLOT(HandleOpenIntermediateFolderButton()));

	///////////////////////////////////////
	// Add Widgets to Main Layout
	///////////////////////////////////////

	// Add the widget to the basic dialog
	m_wAssetNameRowLabelWidget = new QLabel(tr("Asset Name"));
	mainLayout->addRow(m_wAssetNameRowLabelWidget, assetNameEdit);
	m_aRowLabels.append(m_wAssetNameRowLabelWidget);
	m_wAssetTypeRowLabelWidget = new QLabel(tr("Asset Type"));
	mainLayout->addRow(m_wAssetTypeRowLabelWidget, assetTypeCombo);
	m_aRowLabels.append(m_wAssetTypeRowLabelWidget);

	// Add Animation settings to the main layout as a new row without header
	mainLayout->addRow("", animationSettingsGroupBox);

	m_wMorphsRowLabelWidget = new QLabel(tr("Export Morphs"));
	mainLayout->addRow(m_wMorphsRowLabelWidget, morphsLayout);
	m_aRowLabels.append(m_wMorphsRowLabelWidget);

	// Add Morph settings to the main layout as a new row without header
	mainLayout->addRow("", morphSettingsGroupBox);

	m_wSubDRowLabelWidget = new QLabel(tr("Bake Subdivision"));
	mainLayout->addRow(m_wSubDRowLabelWidget, subdivisionLayout);
	m_aRowLabels.append(m_wSubDRowLabelWidget);

	// Create LOD Row, then store lod row widget, then hide row as default state
	m_wLodRowLabelWidget = new QLabel(tr("Enable LOD"));
	mainLayout->addRow(m_wLodRowLabelWidget, lodSettingsLayout);
	m_aRowLabels.append(m_wLodRowLabelWidget);
	this->showLodRow(false);

	// DB 2024-09-17: Texture Resizing options
	m_wResizeTexturesGroupBox = new QGroupBox(tr("Texture Resizing Options : "));
	m_wResizeTexturesGroupBox->setCheckable(true);
	m_wResizeTexturesGroupBox->setChecked(false);
	QFormLayout* textureResizingOptionsLayout = new QFormLayout(m_wResizeTexturesGroupBox);
	textureResizingOptionsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	textureResizingOptionsLayout->setSpacing(nStyleMargin);
	textureResizingOptionsLayout->setMargin(nStyleMargin);
	textureResizingOptionsLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

	m_wMaxTextureFileSizeCombo = new QComboBox(0);
	m_wMaxTextureFileSizeCombo->setObjectName("DzBridge_MaxTextureFileSize_Combobox");
	m_wMaxTextureFileSizeCombo->addItem(tr("No Maximum"), QVariant(-1));
	m_wMaxTextureFileSizeCombo->addItem(tr("1 MB"), QVariant(1024));
	m_wMaxTextureFileSizeCombo->addItem(tr("2 MB"), QVariant(1024 * 2));
	m_wMaxTextureFileSizeCombo->addItem(tr("5 MB"), QVariant(1024 * 5));
	m_wMaxTextureFileSizeCombo->addItem(tr("10 MB"), QVariant(1024 * 10));
	m_wMaxTextureFileSizeCombo->addItem(tr("25 MB"), QVariant(1024 * 25));
//	m_wMaxTextureFileSizeCombo->addItem(tr("Custom File Size in kilobytes..."), QVariant("custom"));
	QString sMaximumTextureFileSizeHelp = tr("The exporter will try to resize and/or re-encode the texture in order to reduce any files above the maximum file size.");
	m_wMaxTextureFileSizeCombo->setWhatsThis(sMaximumTextureFileSizeHelp);
	m_wMaxTextureFileSizeCombo->setToolTip(sMaximumTextureFileSizeHelp);
	m_wMaxTextureFileSizeRowLabelWidget = new QLabel(tr("Maximum Texture File Size"));
	textureResizingOptionsLayout->addRow(m_wMaxTextureFileSizeRowLabelWidget, m_wMaxTextureFileSizeCombo);
//	m_aTextureResizingLabels.append(m_wMaxTextureFileSizeRowLabelWidget);
	m_aRowLabels.append(m_wMaxTextureFileSizeRowLabelWidget);

	m_wMaxTextureResolutionCombo = new QComboBox(0);
	m_wMaxTextureResolutionCombo->setObjectName("DzBridge_MaxTextureResolution_Combobox");
	m_wMaxTextureResolutionCombo->addItem(tr("No Maximum"), QVariant(-1));
	m_wMaxTextureResolutionCombo->addItem(tr("512x512"), QVariant(512));
	m_wMaxTextureResolutionCombo->addItem(tr("1K (1024x1024)"), QVariant(1024));
	m_wMaxTextureResolutionCombo->addItem(tr("2K (2028x2048)"), QVariant(2048));
	m_wMaxTextureResolutionCombo->addItem(tr("4K (4096x4096)"), QVariant(4096));
//	m_wMaxTextureResolutionCombo->addItem(tr("Custom Resolution in pixels..."), QVariant("custom"));
	QString sMaximumTextureResolutionHelp = tr("The exporter will try to resize any texture greater than the specified maximum resolution to the maximum.");
	m_wMaxTextureResolutionCombo->setWhatsThis(sMaximumTextureResolutionHelp);
	m_wMaxTextureResolutionCombo->setToolTip(sMaximumTextureResolutionHelp);
	m_wMaxTextureResolutionRowLabelWidget = new QLabel(tr("Maximum Texture Resolution"));
	textureResizingOptionsLayout->addRow(m_wMaxTextureResolutionRowLabelWidget, m_wMaxTextureResolutionCombo);
//	m_aTextureResizingLabels.append(m_wMaxTextureResolutionRowLabelWidget);
	m_aRowLabels.append(m_wMaxTextureResolutionRowLabelWidget);

	m_wExportTextureFileFormatCombo = new QComboBox(0);
	m_wExportTextureFileFormatCombo->setObjectName("DzBridge_ExportTextureFileFormat_Combobox");
	m_wExportTextureFileFormatCombo->addItem(tr("Keep Original File Format(s)"), QVariant("any"));
	m_wExportTextureFileFormatCombo->addItem(tr("Convert Everything to JPG Only"), QVariant("jpg"));
	m_wExportTextureFileFormatCombo->addItem(tr("Convert Everything to PNG Only"), QVariant("png"));
	m_wExportTextureFileFormatCombo->addItem(tr("Convert Everything to PNG or JPG"), QVariant("png+jpg"));
	m_wExportTextureFormatRowLabelWidget = new QLabel(tr("Export Texture File Format"));
	textureResizingOptionsLayout->addRow(m_wExportTextureFormatRowLabelWidget, m_wExportTextureFileFormatCombo);
//	m_aTextureResizingLabels.append(m_wExportTextureFormatRowLabelWidget);
	m_aRowLabels.append(m_wExportTextureFormatRowLabelWidget);

	// DB 2024-09-21: Texture Baking options
	m_wTextureBakingGroupBox = new QGroupBox(tr("Texture Baking Options : "));
	m_wTextureBakingGroupBox->setCheckable(true);
	m_wTextureBakingGroupBox->setChecked(false);
	QFormLayout* textureBakingOptionsLayout = new QFormLayout(m_wTextureBakingGroupBox);
	textureBakingOptionsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	textureBakingOptionsLayout->setSpacing(nStyleMargin);
	textureBakingOptionsLayout->setMargin(nStyleMargin);

	// Enable Normal Map Generation checkbox
	QString sBumpMapToNormal = tr("Convert Bump Maps to Normal Maps");
	m_wNormalMapsRowLabelWidget = new QLabel(tr("Bump to Normal"));
	m_wConvertBumpToNormalCheckBox = new QCheckBox(sBumpMapToNormal, this);
	m_wConvertBumpToNormalCheckBox->setObjectName("DzBridge_ConvertBumpToNormal_Checkbox");
	connect(m_wConvertBumpToNormalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(HandleConvertBumpToNormalCheckBoxChange(int)));
	textureBakingOptionsLayout->addRow(m_wNormalMapsRowLabelWidget, m_wConvertBumpToNormalCheckBox);
	//	advancedLayout->addRow(m_wNormalMapsRowLabelWidget, m_wConvertBumpToNormalCheckBox);
	m_aRowLabels.append(m_wNormalMapsRowLabelWidget);

	QString sBakeAlphaChannel = tr("Bake Cutout/Opacity to Diffuse Alpha Channel");
	m_wBakeAlphaChannelRowLabel = new QLabel(tr("Opacity Cutout"));
	m_wBakeAlphaChannelCheckBox = new QCheckBox(sBakeAlphaChannel);
	m_wBakeAlphaChannelCheckBox->setObjectName("DzBridge_BakeAlphaChannel_Checkbox");
	textureBakingOptionsLayout->addRow(m_wBakeAlphaChannelRowLabel, m_wBakeAlphaChannelCheckBox);
	m_aRowLabels.append(m_wBakeAlphaChannelRowLabel);

	QString sBakeColorTint = tr("Bake Color Tints (and Strengths) to Image Maps");
	m_wBakeColorTintRowLabel = new QLabel(tr("Tint / Strength"));
	m_wBakeColorTintCheckBox = new QCheckBox(sBakeColorTint);
	m_wBakeColorTintCheckBox->setObjectName("DzBridge_BakeColorTint_Checkbox");
	textureBakingOptionsLayout->addRow(m_wBakeColorTintRowLabel, m_wBakeColorTintCheckBox);
	m_aRowLabels.append(m_wBakeColorTintRowLabel);

	QString sBakeMakeupOverlay = tr("Bake HD Makeup Materials to Diffuse Maps");
	m_wBakeMakeupOverlayRowLabel = new QLabel(tr("Make-up"));
	m_wBakeMakeupOverlayCheckBox = new QCheckBox(sBakeMakeupOverlay);
	m_wBakeMakeupOverlayCheckBox->setObjectName("DzBridge_BakeMakeupOverlay_Checkbox");
	textureBakingOptionsLayout->addRow(m_wBakeMakeupOverlayRowLabel, m_wBakeMakeupOverlayCheckBox);
	m_aRowLabels.append(m_wBakeMakeupOverlayRowLabel);

	QString sBakeTranslucencyTint = tr("Bake Translucency Maps to Diffuse Maps");
	m_wBakeTranslucencyTintRowLabel = new QLabel(tr("Translucency"));
	m_wBakeTranslucencyTintCheckBox = new QCheckBox(sBakeTranslucencyTint);
	m_wBakeTranslucencyTintCheckBox->setObjectName("DzBridge_BakeTranslucencyTint_Checkbox");
	textureBakingOptionsLayout->addRow(m_wBakeTranslucencyTintRowLabel, m_wBakeTranslucencyTintCheckBox);
	m_aRowLabels.append(m_wBakeTranslucencyTintRowLabel);

	QString sBakeRefractionWeight = tr("Bake Refraction Weight Simulation to Textures");
//	m_wBakeRefractionWeightRowLabel = new QLabel(tr("Refraction Weight"));
//	m_wBakeRefractionWeightCheckBox = new QCheckBox(sBakeRefractionWeight);
//	textureBakingOptionsLayout->addRow(m_wBakeRefractionWeightRowLabel, m_wBakeRefractionWeightCheckBox);
//	m_aRowLabels.append(m_wBakeRefractionWeightRowLabel);

	QString sBakeSpecularToMetallic = tr("Bake Specular/Glossy to Metallic/Roughness");
//	m_wBakeSpecularToMetallicRowLabel = new QLabel(tr("Specular to Metallic"));
//	m_wBakeSpecularToMetallicCheckBox = new QCheckBox(sBakeSpecularToMetallic);
//	textureBakingOptionsLayout->addRow(m_wBakeSpecularToMetallicRowLabel, m_wBakeSpecularToMetallicCheckBox);
//	m_aRowLabels.append(m_wBakeSpecularToMetallicRowLabel);

	// Object Baking Options
	m_wObjectBakingGroupBox = new QGroupBox(tr("Object Baking Options : "), 0);
	QFormLayout* objectBakingOptionsLayout = new QFormLayout(m_wObjectBakingGroupBox);
	objectBakingOptionsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	objectBakingOptionsLayout->setSpacing(nStyleMargin);
	objectBakingOptionsLayout->setMargin(nStyleMargin);

	QString sBakeInstances = tr("Replace Daz Studio Instance Nodes with original objects");
	m_wBakeInstancesComboBox = new QComboBox(0);
	m_wBakeInstancesComboBox->addItem(tr("Ask for my decision"), "ask");
	m_wBakeInstancesComboBox->addItem(tr("Disable this step"), "never");
	m_wBakeInstancesComboBox->addItem(tr("Always replace with objects"), "always");
	m_wBakeInstancesRowLabel = new QLabel(tr("Daz Instance Nodes"));
	m_aRowLabels.append(m_wBakeInstancesRowLabel);
	objectBakingOptionsLayout->addRow(m_wBakeInstancesRowLabel, m_wBakeInstancesComboBox);
	m_wBakeInstancesComboBox->setCurrentIndex(m_wBakeInstancesComboBox->findData("ask"));

	m_wBakeCustomPivotsComboBox = new QComboBox(0);
	m_wBakeCustomPivotsComboBox->addItem(tr("Ask for my decision"), "ask");
	m_wBakeCustomPivotsComboBox->addItem(tr("Disable this step"), "never");
	m_wBakeCustomPivotsComboBox->addItem(tr("Always bake pivots"), "always");
	m_wBakeCustomPivotsRowLabel = new QLabel(tr("Pivot Points"));
	m_aRowLabels.append(m_wBakeCustomPivotsRowLabel);
	objectBakingOptionsLayout->addRow(m_wBakeCustomPivotsRowLabel, m_wBakeCustomPivotsComboBox);
	m_wBakeCustomPivotsComboBox->setCurrentIndex(m_wBakeInstancesComboBox->findData("ask"));

	m_wBakeRigidFollowNodesComboBox = new QComboBox(0);
	m_wBakeRigidFollowNodesComboBox->addItem(tr("Ask for my decision"), "ask");
	m_wBakeRigidFollowNodesComboBox->addItem(tr("Disable this step"), "never");
	m_wBakeRigidFollowNodesComboBox->addItem(tr("Always convert to rigged mesh"), "always");
	m_wBakeRigidFollowNodesRowLabel = new QLabel(tr("Rigid Follow Nodes"));
	m_aRowLabels.append(m_wBakeRigidFollowNodesRowLabel);
	objectBakingOptionsLayout->addRow(m_wBakeRigidFollowNodesRowLabel, m_wBakeRigidFollowNodesComboBox);
	m_wBakeRigidFollowNodesComboBox->setCurrentIndex(m_wBakeInstancesComboBox->findData("ask"));

	///////////////////////////////////////
	// Add Widgets to Advanced Layout
	///////////////////////////////////////

#ifndef VODSVERSION
	QLabel* wTargetPluginInstallerRowLabel = new QLabel(tr("Install Destination Plugin"));
	advancedLayout->addRow(wTargetPluginInstallerRowLabel, m_wTargetPluginInstaller);
	m_aRowLabels.append(wTargetPluginInstallerRowLabel);
#endif
	advancedLayout->addRow("", m_OpenIntermediateFolderButton);
	showTargetPluginInstaller(false);
	m_wFbxVersionRowLabelWidget = new QLabel(tr("FBX Version"));
	advancedLayout->addRow(m_wFbxVersionRowLabelWidget, fbxVersionCombo);
	m_aRowLabels.append(m_wFbxVersionRowLabelWidget);
	m_wShowFbxRowLabelWidget = new QLabel(tr("Show FBX Dialog"));
	advancedLayout->addRow(m_wShowFbxRowLabelWidget, showFbxDialogCheckBox);
	m_aRowLabels.append(m_wShowFbxRowLabelWidget);

	m_wExportCsvRowLabelWidget = new QLabel(tr("Export Material CSV"));
	advancedLayout->addRow(m_wExportCsvRowLabelWidget, exportMaterialPropertyCSVCheckBox);
	m_aRowLabels.append(m_wExportCsvRowLabelWidget);
	m_wEnableExperimentalRowLabelWidget = new QLabel(tr("Experimental Options"));
	advancedLayout->addRow(m_wEnableExperimentalRowLabelWidget, m_enableExperimentalOptionsCheckBox);
	m_aRowLabels.append(m_wEnableExperimentalRowLabelWidget);

	// Texture Resizing Options
	advancedLayout->addRow(m_wResizeTexturesGroupBox);
	// Texture Baking
	advancedLayout->addRow(m_wTextureBakingGroupBox);

	// Object Baking
	advancedLayout->addRow(m_wObjectBakingGroupBox);

	// add main layout to dialog window
	m_wMainGroupBox = new QGroupBox(tr("Main Export Options : "));
	m_wMainGroupBox->setMinimumWidth(500);
	m_wMainGroupBox->setLayout(mainLayout);
	addWidget(m_wMainGroupBox);

	// add advanced layout to options section of dialog window
	QString sAdvancedOptionsTitle = tr(ADVANCED_OPTIONS_TITLE);
	advancedSettingsGroupBox = new QGroupBox(sAdvancedOptionsTitle);
	advancedSettingsGroupBox->setObjectName("DzBridge_AdvancedSettings_Groupbox");
	advancedSettingsGroupBox->setLayout(advancedLayout);
	advancedSettingsGroupBox->setCheckable(false);
	advancedSettingsGroupBox->setChecked(false);
	advancedSettingsGroupBox->setMinimumWidth(500); // This is what forces the whole forms width	
	this->addOptionsWidget(advancedSettingsGroupBox);

	// add help text
	assetNameEdit->setWhatsThis(tr("This is the name the asset will use in the destination software."));
	assetTypeCombo->setWhatsThis(tr("Skeletal Mesh for something with moving parts, like a character\nStatic Mesh for things like props\nAnimation for a character animation."));
	subdivisionButton->setWhatsThis(tr("Select Subdivision Detail Level to Bake into each exported mesh."));
	morphsButton->setWhatsThis(tr("Select Morphs to export with asset."));
	fbxVersionCombo->setWhatsThis(tr("The version of FBX to use when exporting assets."));
	showFbxDialogCheckBox->setWhatsThis(tr("Checking this will show the FBX Dialog for adjustments before export."));
	exportMaterialPropertyCSVCheckBox->setWhatsThis(tr("Checking this will write out a CSV of all the material properties.  Useful for reference when changing materials."));
	m_wConvertBumpToNormalCheckBox->setWhatsThis(tr("Checking this will enable generation of Normal Maps for any surfaces that only have Bump Height Maps."));
	//m_wTargetPluginInstaller->setWhatsThis("Install a plugin to use Daz Bridge with the destination software.");
	QString sEnableLodHelp = tr("Enable Level of Detail (LOD) mesh.  Specific features depend on the destination software.");
	m_wLodRowLabelWidget->setWhatsThis(sEnableLodHelp);
	m_wLodRowLabelWidget->setToolTip(sEnableLodHelp);
	m_wEnableLodCheckBox->setWhatsThis(sEnableLodHelp);
	m_wEnableLodCheckBox->setToolTip(sEnableLodHelp);
	QString sLodSettingsHelp = tr("Configure how Level of Detail (LOD) meshes are set up or generated in the destination software.");
	m_wLodSettingsButton->setWhatsThis(sLodSettingsHelp);
	m_wLodSettingsButton->setToolTip(sLodSettingsHelp);

	// Set Universal Width for all Row Labels
	fixRowLabelStyle();
	fixRowLabelWidths();

	// detect scene change
//	connect(dzScene, SIGNAL(nodeSelectionListChanged()), this, SLOT(handleSceneSelectionChanged()));

	// Set Defaults
	resetToDefaults();

	if (m_bSetupMode)
	{
		this->setDisabled(true);
	}

#ifndef VODSVERSION
	m_WelcomeLabel->setVisible(true);
#endif

	wHelpMenuButton = new DzMenuButton(0, tr("BridgeHelpMenu"));
	wHelpMenuButton->setObjectName("DzBridge_BridgeHelpMenu_Button");
	wHelpMenuButton->setIndeterminateText(tr("Help"), true);
	this->addButton(wHelpMenuButton);
	wHelpMenuButton->style()->pixelMetric(DZ_PM_ButtonMinWidth);
	DzBridgeDialogTools::SetThinButtonText(wHelpMenuButton, tr("Help") + "       ");
	wHelpMenuButton->insertItem(tr("PDF..."), BRIDGE_HELP_ID_PDF);
	wHelpMenuButton->insertItem(tr("Youtube Tutorials..."), BRIDGE_HELP_ID_YOUTUBE);
	wHelpMenuButton->insertItem(tr("Request Support..."), BRIDGE_HELP_ID_SUPPORT);
	connect(wHelpMenuButton, SIGNAL(menuIndexSelected(int)), this, SLOT(HandleHelpMenuButton(int)));
	wHelpMenuButton->hide();

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
#define LOAD_CHECKED(name,widget) if (!settings->value(name).isNull() && widget) widget->setChecked(settings->value(name).toBool());
#define LOAD_ITEMDATA(name,widget) if (!settings->value(name).isNull() && widget) widget->setCurrentIndex(widget->findData(settings->value(name)));

	if (settings == nullptr) return false;

	LOAD_CHECKED("MorphsEnabled", morphsEnabledCheckBox);
	LOAD_CHECKED("SubdivisionEnabled", subdivisionEnabledCheckBox);
	LOAD_CHECKED("ShowFBXDialog", showFbxDialogCheckBox);

	// 2025-04-14, DB: MorphSelectionDialog Settings Refactor
	LOAD_CHECKED("AutoJCMEnabled", m_wAutoJCMCheckBox);
	LOAD_CHECKED("FakeDualQuatEnabled", m_wFakeDualQuatCheckBox);
	LOAD_CHECKED("AllowMorphDoubleDipping", m_wAllowMorphDoubleDippingCheckBox);

	if (m_bSetupMode)
	{
		this->showOptions();
	}
	else if (!settings->value("ShowAdvancedSettings").isNull())
	{
		bool bShowOptions = settings->value("ShowAdvancedSettings").toBool();
		if (bShowOptions)
			this->showOptions();
		else
			this->hideOptions();
	}
	else
	{
		this->hideOptions();
	}

	if (!settings->value("FBXExportVersion").isNull())
	{
		int index = fbxVersionCombo->findText(settings->value("FBXExportVersion").toString());
		if (index != -1)
		{
			fbxVersionCombo->setCurrentIndex(index);
		}
	}

	LOAD_CHECKED("EnableNormalMapGeneration", m_wConvertBumpToNormalCheckBox);
	LOAD_CHECKED("ExportMaterialPropertyCSV", exportMaterialPropertyCSVCheckBox);

	LOAD_CHECKED("ResizeTexturesEnabled", m_wResizeTexturesGroupBox);
	LOAD_ITEMDATA("MaxTextureFileSize", m_wMaxTextureFileSizeCombo);
	LOAD_ITEMDATA("MaxTextureResolution", m_wMaxTextureResolutionCombo);
	LOAD_ITEMDATA("ExportTextureFileFormat", m_wExportTextureFileFormatCombo);

	LOAD_CHECKED("BakeTexturesEnabled", m_wTextureBakingGroupBox);
	LOAD_CHECKED("BakeAlphaChannel", m_wBakeAlphaChannelCheckBox);
	LOAD_CHECKED("BakeColorTint", m_wBakeColorTintCheckBox);
	LOAD_CHECKED("BakeMakeupOverlay", m_wBakeMakeupOverlayCheckBox);
	LOAD_CHECKED("BakeTranslucencyTint", m_wBakeTranslucencyTintCheckBox);

	LOAD_ITEMDATA("BakeInstances", m_wBakeInstancesComboBox);
	LOAD_ITEMDATA("BakePivots", m_wBakeCustomPivotsComboBox);
	LOAD_ITEMDATA("BakeRigidFollowNodes", m_wBakeRigidFollowNodesComboBox);

	// Animation settings
	LOAD_CHECKED("AnimationExperminentalExport", experimentalAnimationExportCheckBox);
	LOAD_CHECKED("AnimationBake", bakeAnimationExportCheckBox);
	LOAD_CHECKED("AnimationExportFace", faceAnimationExportCheckBox);
	LOAD_CHECKED("AnimationExportActiveCurves", animationExportActiveCurvesCheckBox);
	LOAD_CHECKED("AnimationApplyBoneScale", animationApplyBoneScaleCheckBox);

	return true;
}

// Some settings will be saved when Accept is hit so we don't need a hanlder attached to all of them
void DzBridgeDialog::saveSettings()
{
#define SAVE_CHECKED(name, widget) if (widget) settings->setValue(name, widget->isChecked());
#define SAVE_ITEMDATA(name, widget) if (widget) settings->setValue(name, widget->itemData(widget->currentIndex()));

	if (settings == nullptr || m_bDontSaveSettings) return;

	SAVE_CHECKED("MorphsEnabled", morphsEnabledCheckBox);
	SAVE_CHECKED("SubdivisionEnabled", subdivisionEnabledCheckBox);
	SAVE_CHECKED("LodEnabled", m_wEnableLodCheckBox);

	// 2025-04-14, DB: MorphSelectionDialog Settings Refactor
	SAVE_CHECKED("AutoJCMEnabled", m_wAutoJCMCheckBox);
	SAVE_CHECKED("FakeDualQuatEnabled", m_wFakeDualQuatCheckBox);
	SAVE_CHECKED("AllowMorphDoubleDipping", m_wAllowMorphDoubleDippingCheckBox);

	if (!m_bSetupMode) settings->setValue("ShowAdvancedSettings", this->optionsShown());

	if (fbxVersionCombo->isEnabled()) settings->setValue("FBXExportVersion", fbxVersionCombo->itemText(fbxVersionCombo->currentIndex()));
	SAVE_CHECKED("ShowFBXDialog", showFbxDialogCheckBox);
	SAVE_CHECKED("ExportMaterialPropertyCSV", exportMaterialPropertyCSVCheckBox);

	SAVE_CHECKED("ResizeTexturesEnabled", m_wResizeTexturesGroupBox);
	SAVE_ITEMDATA("MaxTextureFileSize", m_wMaxTextureFileSizeCombo);
	SAVE_ITEMDATA("MaxTextureResolution", m_wMaxTextureResolutionCombo);
	SAVE_ITEMDATA("ExportTextureFileFormat", m_wExportTextureFileFormatCombo);

	SAVE_CHECKED("BakeTexturesEnabled", m_wTextureBakingGroupBox);
	SAVE_CHECKED("EnableNormalMapGeneration", m_wConvertBumpToNormalCheckBox);
	SAVE_CHECKED("BakeAlphaChannel", m_wBakeAlphaChannelCheckBox);
	SAVE_CHECKED("BakeColorTint", m_wBakeColorTintCheckBox);
	SAVE_CHECKED("BakeMakeupOverlay", m_wBakeMakeupOverlayCheckBox);
	SAVE_CHECKED("BakeTranslucencyTint", m_wBakeTranslucencyTintCheckBox);

	SAVE_ITEMDATA("BakeInstances", m_wBakeInstancesComboBox);
	SAVE_ITEMDATA("BakePivots", m_wBakeCustomPivotsComboBox);
	SAVE_ITEMDATA("BakeRigidFollowNodes", m_wBakeRigidFollowNodesComboBox);

	if (experimentalAnimationExportCheckBox)
	{
		SAVE_CHECKED("AnimationExperminentalExport", experimentalAnimationExportCheckBox);
		SAVE_CHECKED("AnimationBake", bakeAnimationExportCheckBox);
		SAVE_CHECKED("AnimationExportFace", faceAnimationExportCheckBox);
		SAVE_CHECKED("AnimationExportActiveCurves", animationExportActiveCurvesCheckBox);
		SAVE_CHECKED("AnimationApplyBoneScale", animationApplyBoneScaleCheckBox);
	}
}

// Update Default GUI Settings based on currently selected asset
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

	if (m_eAssetType == EAssetType::None)
	{
		// LEGACY CODE
		if (qobject_cast<DzSkeleton*>(Selection))
		{
			int nSkeletalIndex = assetTypeCombo->findText("Skeletal Mesh");
			if (nSkeletalIndex != -1) assetTypeCombo->setCurrentIndex(nSkeletalIndex);
		}
		else
		{
			int nStaticIndex = assetTypeCombo->findText("Static Mesh");
			if (nStaticIndex != -1) assetTypeCombo->setCurrentIndex(nStaticIndex);
		}
	}
	else
	{
		// MODERN PATHWAY
		int index = assetTypeCombo->findData(m_eAssetType);
		assetTypeCombo->setCurrentIndex(index);
	}

}

bool DzBridgeDialog::sanityChecksAndWarnUser()
{
	// 2025-04-23, DB: perform checks to allow graceful cancel back to dialog
	if (getMorphsEnabled())
	{
		DzNode* pSelection = dzScene->getPrimarySelection();
		DzBridgeMorphSelectionDialog* morphDialog = DzBridgeMorphSelectionDialog::Get(this);
		if (pSelection && morphDialog && !getAllowMorphDoubleDipping())
		{
			QList<QString> aMorphNamesToExport = morphDialog->GetMorphNamesToExport();
			if (MorphTools::CheckForIrreversibleOperations_in_disconnectOverrideControllers(pSelection, aMorphNamesToExport))
			{
				// warn user
				auto userChoice = QMessageBox::question(0, "Daz Bridge",
					tr("You are exporting morph controllers that are \"connected\" or controlling \n\
the strength of other morphs that are also being exported. \n\n\
To prevent morph values from exponential growth to 200% or more \n\
(aka \"Double-Dipping\"), we must now disconnect all linked morph \n\
controllers. This may cause irreversible changes to your scene.\n\n\
Are you ready to proceed, or do you want to Cancel to save your changes?"),
QMessageBox::Yes | QMessageBox::Cancel,
QMessageBox::Yes);
				if (userChoice == QMessageBox::StandardButton::Cancel)
					return false;
			}
		}
	}

	return true;
}

void DzBridgeDialog::accept()
{
	if (m_bSetupMode) {
		saveSettings();
		return  DzBasicDialog::reject();
	}

	if (sanityChecksAndWarnUser() == false)
		return;

	saveSettings();
	return DzBasicDialog::accept();
}

void DzBridgeDialog::resetToDefaults()
{
	m_bDontSaveSettings = true;
	// Set Defaults
	subdivisionEnabledCheckBox->setChecked(false);
	morphsEnabledCheckBox->setChecked(false);
	showFbxDialogCheckBox->setChecked(false);
	exportMaterialPropertyCSVCheckBox->setChecked(false);

	refreshAsset();
	m_bDontSaveSettings = false;
}

void DzBridgeDialog::handleSceneSelectionChanged()
{
	// crashfix
	if (dzApp->isClosing()) return;

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

	// DB, 2024-09-29: morphselectiondialog Crashfix moved to  DzBridgeAction::readGui(DzBridgeDialog* BridgeDialog) line 3873

}

int DzBridgeDialog::HandleChooseMorphsButton()
{
	DzBridgeMorphSelectionDialog * morphDialog = DzBridgeMorphSelectionDialog::Get(this);
	int dialogReturnCode = morphDialog->exec();
	if (dialogReturnCode == QDialog::Accepted)
	{
		this->morphsEnabledCheckBox->setChecked(true);
	}
	return dialogReturnCode;
}

void DzBridgeDialog::HandleChooseSubdivisionsButton()
{
	DzBridgeSubdivisionDialog *subdivisionDialog = DzBridgeSubdivisionDialog::Get(this);
	int returncode = subdivisionDialog->exec();
	if (returncode == QDialog::Accepted)
	{
		this->subdivisionEnabledCheckBox->setChecked(true);
	}
}

QList<QString> DzBridgeDialog::GetPoseList()
{
	return DzBridgeMorphSelectionDialog::Get(this)->GetPoseList();
}

void DzBridgeDialog::HandleMorphsCheckBoxChange(int state)
{
	morphSettingsGroupBox->setHidden(state != Qt::Checked);
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("MorphsEnabled", state == Qt::Checked);
}

void DzBridgeDialog::HandleSubdivisionCheckBoxChange(int state)
{
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("SubdivisionEnabled", state == Qt::Checked);
}

void DzBridgeDialog::HandleFBXVersionChange(const QString& fbxVersion)
{
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("FBXExportVersion", fbxVersion);
}
void DzBridgeDialog::HandleShowFbxDialogCheckBoxChange(int state)
{
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("ShowFBXDialog", state == Qt::Checked);
}
void DzBridgeDialog::HandleExportMaterialPropertyCSVCheckBoxChange(int state)
{
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("ExportMaterialPropertyCSV", state == Qt::Checked);
}

void DzBridgeDialog::HandleConvertBumpToNormalCheckBoxChange(int state)
{
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("EnableNormalMapGeneration", state == Qt::Checked);
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
	bInstallSuccessful = DzBridgeAction::InstallEmbeddedArchive(sPluginZipFilename, directoryName);

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
	QString sEmbeddedFilePath = m_sEmbeddedFilesPath + "/" + sArchiveFilename;
	return DzBridgeAction::InstallEmbeddedArchive(sEmbeddedFilePath, sDestinationPath);
}

void DzBridgeDialog::setBridgeVersionStringAndLabel(QString sVersionString, QString sLabel)
{
	QString sAdvancedOptionsBoxTitle = tr(ADVANCED_OPTIONS_TITLE) + QString("(%1)").arg(sVersionString);
	advancedSettingsGroupBox->setTitle(sAdvancedOptionsBoxTitle);

	return;
}

void DzBridgeDialog::setDisabled(bool bDisabled)
{
	if (bDisabled)
	{
		this->showOptions();
	}

//	m_WelcomeLabel->setVisible(bDisabled);
	assetNameEdit->setDisabled(bDisabled);
	assetTypeCombo->setDisabled(bDisabled);
	subdivisionButton->setDisabled(bDisabled);
	morphsButton->setDisabled(bDisabled);
	m_wLodSettingsButton->setDisabled(bDisabled);

	morphsEnabledCheckBox->setDisabled(bDisabled);
	morphLockBoneTranslationCheckBox->setDisabled(bDisabled);
	// 2025-04-14, DB: MorphSelectionDialog Refactor
	m_wAutoJCMCheckBox->setDisabled(bDisabled);
	m_wFakeDualQuatCheckBox->setDisabled(bDisabled);
	m_wAllowMorphDoubleDippingCheckBox->setDisabled(bDisabled);
	subdivisionEnabledCheckBox->setDisabled(bDisabled);
	m_wLodSettingsButton->setDisabled(bDisabled);
	m_wEnableLodCheckBox->setDisabled(bDisabled);

	experimentalAnimationExportCheckBox->setDisabled(bDisabled);
	bakeAnimationExportCheckBox->setDisabled(bDisabled);
	faceAnimationExportCheckBox->setDisabled(bDisabled);
	animationExportActiveCurvesCheckBox->setDisabled(bDisabled);
	animationApplyBoneScaleCheckBox->setDisabled(bDisabled);

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
//	ShellExecuteA(NULL, "open", sIntermediateFolderPath.toUtf8().data(), NULL, NULL, SW_SHOWDEFAULT);
	std::wstring wcsIntermediateFolderPath(reinterpret_cast<const wchar_t*>(sIntermediateFolderPath.utf16()));
	ShellExecuteW(NULL, L"open", wcsIntermediateFolderPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
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
	/// skeletal mesh, static mesh, animation
	if (sAssetType == "Skeletal Mesh" ||
		sAssetType == "Static Mesh" ||
		sAssetType == "Animation" )
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

#ifdef VODSVERSION
	animationSettingsGroupBox->setVisible(sAssetType == "Animation" || sAssetType == "Pose");
#else
	if (this->getEnableExperimentalOptions()) {
		animationSettingsGroupBox->setVisible(sAssetType == "Animation" || sAssetType == "Pose");
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

void DzBridgeDialog::HandleExperimentalOptionsCheckBoxClicked()
{
	bool bIsChecked = m_enableExperimentalOptionsCheckBox->isChecked();
	if (bIsChecked)
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

	// Hide/Show experimental options for current state	
	if (assetTypeCombo->currentText()  == "Animation") {
		animationSettingsGroupBox->setVisible(bIsChecked);
		// uncheck all options if experimental options was unchecked
		if (bIsChecked == false) {
			experimentalAnimationExportCheckBox->setChecked(false);
			bakeAnimationExportCheckBox->setChecked(false);
			faceAnimationExportCheckBox->setChecked(false);
			animationExportActiveCurvesCheckBox->setChecked(false);
			animationApplyBoneScaleCheckBox->setChecked(false);
		}
	}

	//////////////////////////////////
	// Intentionally disabling saving settings and saving as unchecked to force users to "opt-in" every time they use experimental features
	//////////////////////////////////
    // int state = m_enableExperimentalOptionsCheckBox->checkState();
	// if (settings == nullptr || m_bDontSaveSettings) return;
	// settings->setValue("EnableExperimentalOptions", state == Qt::Checked);
	//////////////////////////////////
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
	//if (settings == nullptr || m_bDontSaveSettings) return;
	//settings->setValue("LodEnabled", state == Qt::Checked);
}

void DzBridgeDialog::showLodRow(bool bShowWidget)
{
	if (m_wLodRowLabelWidget == nullptr) return;

	m_wEnableLodCheckBox->setVisible(bShowWidget);
	m_wLodSettingsButton->setVisible(bShowWidget);
	m_wLodRowLabelWidget->setVisible(bShowWidget);

}

void DzBridgeDialog::HandlePdfButton()
{
	QMessageBox::information(0, "Daz Bridge",
		tr("INFO: PDF button is not configured."));
}

void DzBridgeDialog::HandleYoutubeButton()
{
	QMessageBox::information(0, "Daz Bridge",
		tr("INFO: Youtube button is not configured."));
}

void DzBridgeDialog::HandleSupportButton()
{
	QMessageBox::information(0, "Daz Bridge",
		tr("INFO: Support button is not configured."));
}

void DzBridgeDialog::HandleHelpMenuButton(int)
{
	int id = wHelpMenuButton->getSelectionID();

	switch (id) {
	case BRIDGE_HELP_ID_PDF:
		HandlePdfButton();
		break;

	case BRIDGE_HELP_ID_YOUTUBE:
		HandleYoutubeButton();
		break;

	case BRIDGE_HELP_ID_SUPPORT:
		HandleSupportButton();
		break;

	}

	wHelpMenuButton->setIndeterminate();
}

void DzBridgeDialog::fixRowLabelStyle()
{
	foreach(QLabel * pRowLabel, m_aRowLabels)
	{
		// add colon if not in string
		QString sLabel = pRowLabel->text();
		if (sLabel.endsWith(":") == false) {
			sLabel.append(" :");
		}
		pRowLabel->setText(sLabel);
		pRowLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	}

}

void DzBridgeDialog::fixRowLabelWidths()
{
	int largest_width = 0;
	foreach(QLabel* pRowLabel, m_aRowLabels)
	{
		if (pRowLabel->isHidden() || pRowLabel->isVisible() == false) continue;
#define MAX(A,B) (A>B)?A:B
		pRowLabel->adjustSize();
		int this_width = pRowLabel->sizeHint().width();
		largest_width = MAX(largest_width, this_width);
#undef MAX
	}
	foreach(QLabel* pRowLabel, m_aRowLabels)
	{
		if (pRowLabel->isHidden() || pRowLabel->isVisible() == false) continue;
		pRowLabel->setFixedWidth(largest_width);
	}

}

bool DzBridgeDialog::getBakeSpecularToMetallic()
{
	if (!m_wBakeSpecularToMetallicCheckBox) return false;
	return m_wBakeSpecularToMetallicCheckBox->isChecked();
}

bool DzBridgeDialog::getBakeRefractionWeight()
{
	if (!m_wBakeRefractionWeightCheckBox) return false;
	return m_wBakeRefractionWeightCheckBox->isChecked();
}


#include "moc_DzBridgeDialog.cpp"
