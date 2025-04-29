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
#include <QtGui/qlistwidget.h>
#include <QtGui/qtreewidget.h>
#include <QtGui/qcheckbox.h>
#include <QtGui/qdesktopservices.h>
#include <QtCore/qdiriterator.h>

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"
#include "dzfigure.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzmodifier.h"
#include "dzpresentation.h"
#include "dzassetmgr.h"
#include "dzproperty.h"
#include "dzsettings.h"
#include "dzmorph.h"
#include "dzcontroller.h"
#include "dznumericnodeproperty.h"
#include "dzerclink.h"
#include "dzbone.h"

#include "QtGui/qlayout.h"
#include "QtGui/qlineedit.h"

#include <QtCore/QDebug.h>

#include "DzBridgeMorphSelectionDialog.h"
#include "DzBridgeAction.h"
#include "DzBridgeDialog.h"

#include "MorphTools.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"

using namespace DzBridgeNameSpace;

CPP_Export DzBridgeMorphSelectionDialog* DzBridgeMorphSelectionDialog::singleton = nullptr;

DzBridgeMorphSelectionDialog* DzBridgeMorphSelectionDialog::Get(QWidget* Parent)
{
	if (singleton == nullptr)
	{
		// Crash fix
		if (dzApp->isClosing())
		{
			dzApp->log("WARNING: DzBridgeMorphSelectionDialog::Get() called during Daz Studio shutdown.");
			return nullptr;
		}
		singleton = new DzBridgeMorphSelectionDialog(Parent);
	}
	else
	{
		singleton->PrepareDialog();
	}
	return singleton;
}

// Subclass of QListWidgetItem for sorting the lists
class SortingListItem : public QListWidgetItem {

public:
	virtual bool operator< (const QListWidgetItem &otherItem) const
	{
		if (this->checkState() != otherItem.checkState())
		{
			return (this->checkState() == Qt::Checked);
		}
		return QListWidgetItem::operator<(otherItem);
	}
};

DzBridgeMorphSelectionDialog::DzBridgeMorphSelectionDialog(QWidget *parent) :
	DzBasicDialog(parent, DAZ_BRIDGE_LIBRARY_NAME)
{

	int nStyleMargin = this->style()->pixelMetric(DZ_PM_GeneralMargin);

	connect(this, SIGNAL(accepted()), this, SLOT(HandleDialogAccepted()));

	// Try to retrieve settings from parent dialog
	DzBridgeDialog* bridgeDialog = qobject_cast<DzBridgeDialog*>(parent);
	if (bridgeDialog != nullptr)
	{
		settings = bridgeDialog->getSettings();
	}
	else
	{
		settings = new QSettings("Daz 3D", "Morph Selection Dialog");
	}

	presetsFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DAZ 3D" + QDir::separator() + "Bridges" + QDir::separator() + "Morph Selection Presets";

	 m_morphListWidget = NULL;
	 m_morphExportListWidget = NULL;
	 m_morphTreeWidget = NULL;
	 filterEdit = NULL;
	 presetCombo = NULL;
	 fullBodyMorphTreeItem = NULL;
	 charactersTreeItem = NULL;

	// Set the dialog title
	setWindowTitle(tr("Select Morphs"));

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	mainLayout->setSpacing(nStyleMargin);

	// Left tree with morph structure
	m_morphTreeWidget = new QTreeWidget(this);
	m_morphTreeWidget->setHeaderHidden(true);

	// Center list showing morhps for selected tree items
	m_morphListWidget = new QListWidget(this);
	m_morphListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

	// Right list showing morphs that will export
	m_morphExportListWidget = new QListWidget(this);
	m_morphExportListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

	// Quick filter box
	QHBoxLayout* filterLayout = new QHBoxLayout();
	filterLayout->addWidget(new QLabel("filter"));
	filterEdit = new QLineEdit();
	connect(filterEdit, SIGNAL(textChanged(const QString &)), this, SLOT(FilterChanged(const QString &)));
	filterLayout->addWidget(filterEdit);

	// Presets
	QHBoxLayout* settingsLayout = new QHBoxLayout();
	settingsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	settingsLayout->setSpacing(nStyleMargin);
	presetCombo = new QComboBox(this);
	QPushButton* savePresetButton = new QPushButton(tr("Save Preset..."), this);
	connect(savePresetButton, SIGNAL(released()), this, SLOT(HandleSavePreset()));
	settingsLayout->addWidget(new QLabel("Choose Preset"));
	settingsLayout->addWidget(presetCombo);
	settingsLayout->addWidget(savePresetButton);
	settingsLayout->addStretch();

	// All Morphs
	QHBoxLayout* morphsLayout = new QHBoxLayout();
	morphsLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	morphsLayout->setSpacing(nStyleMargin);

	// Left Tree
	QVBoxLayout* treeLayout = new QVBoxLayout();
	treeLayout->addWidget(new QLabel("Morph Groups"));
	treeLayout->addWidget(new QLabel("Select to see available morphs"));
	treeLayout->addWidget(m_morphTreeWidget);

	// Buttons for quickly adding certain JCMs
	QGroupBox* MorphGroupBox = new QGroupBox("Morph Utilities", this);
	QVBoxLayout* pMorphsGroupBoxLayout = new QVBoxLayout();
	pMorphsGroupBoxLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	pMorphsGroupBoxLayout->setSpacing(nStyleMargin);
	MorphGroupBox->setLayout(pMorphsGroupBoxLayout);
	QGroupBox* JCMGroupBox = new QGroupBox("Add JCMs", this);
	QGridLayout* pJCMGroupBoxGridLayout = new QGridLayout();
	pJCMGroupBoxGridLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	pJCMGroupBoxGridLayout->setSpacing(nStyleMargin);
	JCMGroupBox->setLayout(pJCMGroupBoxGridLayout);
	QGroupBox* FaceGroupBox = new QGroupBox("Add Expressions", this);
	QGridLayout* pFaceGroupBoxGridLayout = new QGridLayout();
	pFaceGroupBoxGridLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	pFaceGroupBoxGridLayout->setSpacing(nStyleMargin);
	FaceGroupBox->setLayout(pFaceGroupBoxGridLayout);

	QPushButton* ArmsJCMButton = new QPushButton("Arms");
	QPushButton* LegsJCMButton = new QPushButton("Legs");
	QPushButton* TorsoJCMButton = new QPushButton("Torso");
	QPushButton* ARKit81Button = new QPushButton("ARKit/FACS (Genesis8.1+9)");
	QPushButton* FaceFX8Button = new QPushButton("FaceFX (Genesis8)");

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 2025-04-25, DB: Moved JCM and other Morph options to main options dialog (DzBridgeDialog)
	/////////////////////////////////////////////////////////////////////////////////////////////
	addConnectedMorphsButton = new QPushButton("Add Connected Morphs");
	addConnectedMorphsButton->setVisible(true);
	QString sAddConnectedMorphsHelpText = QString(tr("Add any morphs or property sliders which can contribute to strength of exported morphs."));
	addConnectedMorphsButton->setWhatsThis(sAddConnectedMorphsHelpText);
	addConnectedMorphsButton->setToolTip(sAddConnectedMorphsHelpText);

	m_wAddConnectedJcmsButton = new QPushButton("Add Connected JCMs");
	m_wAddConnectedJcmsButton->setVisible(true);
	QString sAddConnectedJcmsHelpText = QString(tr("Add any JCMs which can contribute to strength of exported morphs."));
	m_wAddConnectedJcmsButton->setWhatsThis(sAddConnectedJcmsHelpText);
	m_wAddConnectedJcmsButton->setToolTip(sAddConnectedJcmsHelpText);

	((QGridLayout*)JCMGroupBox->layout())->addWidget(ArmsJCMButton, 0, 0);
	((QGridLayout*)JCMGroupBox->layout())->addWidget(LegsJCMButton, 0, 1);
	((QGridLayout*)JCMGroupBox->layout())->addWidget(TorsoJCMButton, 0, 2);
	((QGridLayout*)FaceGroupBox->layout())->addWidget(ARKit81Button, 0, 1);
	((QGridLayout*)FaceGroupBox->layout())->addWidget(FaceFX8Button, 0, 2);

	MorphGroupBox->layout()->addWidget(JCMGroupBox);
	MorphGroupBox->layout()->addWidget(FaceGroupBox);
	MorphGroupBox->layout()->addWidget(addConnectedMorphsButton);
	MorphGroupBox->layout()->addWidget(m_wAddConnectedJcmsButton);

	connect(ArmsJCMButton, SIGNAL(released()), this, SLOT(HandleArmJCMMorphsButton()));
	connect(LegsJCMButton, SIGNAL(released()), this, SLOT(HandleLegJCMMorphsButton()));
	connect(TorsoJCMButton, SIGNAL(released()), this, SLOT(HandleTorsoJCMMorphsButton()));
	connect(ARKit81Button, SIGNAL(released()), this, SLOT(HandleARKitGenesis81MorphsButton()));
	connect(FaceFX8Button, SIGNAL(released()), this, SLOT(HandleFaceFXGenesis8Button()));
	connect(addConnectedMorphsButton, SIGNAL(clicked(bool)), this, SLOT(HandleAddConnectedMorphs()));
	connect(m_wAddConnectedJcmsButton, SIGNAL(clicked(bool)), this, SLOT(HandleAddConnectedJcms()));

	treeLayout->addWidget(MorphGroupBox);
	morphsLayout->addLayout(treeLayout);

	// Center List of morphs based on tree selection
	QVBoxLayout* morphListLayout = new QVBoxLayout();
	morphListLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	morphListLayout->setSpacing(nStyleMargin);
	morphListLayout->addWidget(new QLabel("Morphs in Group"));
	morphListLayout->addWidget(new QLabel("Select and click Add for Export"));
	morphListLayout->addLayout(filterLayout);
	morphListLayout->addWidget(m_morphListWidget);

	// Button for adding morphs
	QPushButton* addMorphsButton = new QPushButton("Add For Export", this);
	connect(addMorphsButton, SIGNAL(released()), this, SLOT(HandleAddMorphsButton()));
	morphListLayout->addWidget(addMorphsButton);
	morphsLayout->addLayout(morphListLayout);

	// Right List of morphs that will export
	QVBoxLayout* selectedListLayout = new QVBoxLayout();
	selectedListLayout->setContentsMargins(nStyleMargin, nStyleMargin, nStyleMargin, nStyleMargin);
	selectedListLayout->setSpacing(nStyleMargin);
	selectedListLayout->addWidget(new QLabel("Morphs to Export"));
	selectedListLayout->addWidget(m_morphExportListWidget);

	// Button for clearing morphs from export
	QPushButton* removeMorphsButton = new QPushButton("Remove From Export", this);
	connect(removeMorphsButton, SIGNAL(released()), this, SLOT(HandleRemoveMorphsButton()));
	selectedListLayout->addWidget(removeMorphsButton);
	morphsLayout->addLayout(selectedListLayout);

	mainLayout->addLayout(settingsLayout);
	mainLayout->addLayout(morphsLayout);

	this->addLayout(mainLayout);
	resize(QSize(800, 750));//.expandedTo(minimumSizeHint()));
	setFixedWidth(width());
	setFixedHeight(height());
	RefreshPresetsCombo();

//	connect(morphListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(ItemChanged(QListWidgetItem*)));

	connect(m_morphTreeWidget, SIGNAL(itemSelectionChanged()),
		this, SLOT(ItemSelectionChanged()));

	PrepareDialog();
}

QSize DzBridgeMorphSelectionDialog::minimumSizeHint() const
{
	return QSize(800, 800);
}

// Build out the Left morphs tree based on the current selection
void DzBridgeMorphSelectionDialog::PrepareDialog()
{
	DzNode* Selection = dzScene->getPrimarySelection();
	if (Selection == nullptr)
	{
		return;
	}

	// For items like clothing, create the morph list from the character
	DzNode* ParentFigureNode = Selection;
	while (ParentFigureNode->getNodeParent())
	{
		ParentFigureNode = ParentFigureNode->getNodeParent();
		if (DzSkeleton* Skeleton = ParentFigureNode->getSkeleton())
		{
			if (DzFigure* Figure = qobject_cast<DzFigure*>(Skeleton))
			{
				Selection = ParentFigureNode;
				break;
			}
		}
	}

	// clear and repopulate m_morphInfoMap / left-most pane
	m_morphInfoMap.clear();
	GetAvailableMorphs(Selection);
	for (int ChildIndex = 0; ChildIndex < Selection->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Selection->getNodeChild(ChildIndex);
		GetAvailableMorphs(ChildNode);
	}

	UpdateMorphsTree();
	RefreshPresetsCombo();
	HandlePresetChanged("LastUsed.csv");
	// DB (2022-Sept-26): crashfix for changed selection and export without opening morph selection dialog
	HandleDialogAccepted(false);
}

// add icons, tooltips, whatsthis, font changes to items in the center and right morph list columns
bool DzBridgeMorphSelectionDialog::decorateMorphListItem(SortingListItem* item, MorphInfo morphInfo, bool bAnalyzeErc)
{
	if (item == NULL)
	{
		return false;
	}

	// colorize item based on presentation type
	QFont normalFont = this->font();
	int normalFontSize = normalFont.pointSize() == -1 ? 8 : normalFont.pointSize();
	QString normalFontFamily = normalFont.family();

	bool bIsMorph = false;
//	if (morphInfo.Type.contains("shape", Qt::CaseInsensitive))
	if (morphInfo.Property->getOwner()->inherits("DzMorph"))
	{
		bIsMorph = true;
	}
	bool bIsPose = false;
	if (morphInfo.Type.contains("pose", Qt::CaseInsensitive))
//	if (morphInfo.Property->getOwner()->inherits("DzBone"))
	{
		bIsPose = true;
	}
	bool bHasMorphs = false;
	if (bAnalyzeErc && morphInfo.hasMorphErc())
	{
		bHasMorphs = true;
	}
	bool bHasPoses = false;
	if (bAnalyzeErc && morphInfo.hasPoseErc())
	{
		bHasPoses = true;
	}
	bool bHasErc = false;
	if (morphInfo.m_ErcList && morphInfo.m_ErcList->count() > 0)
	{
		bHasErc = true;
	}


//	if (morphInfo.Type.contains("pose", Qt::CaseInsensitive))
	if (bHasPoses || !bIsMorph && bIsPose)
	{
		//item->setBackground(QBrush(Qt::red, Qt::SolidPattern));
		//item->setForeground(QBrush(Qt::white));
		item->setFont(QFont(normalFontFamily, -1, -1, true));
		item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
	}
//	else if (morphInfo.Type.contains("shape", Qt::CaseInsensitive))
	else if (bHasMorphs || bIsMorph)
	{
		//item->setBackground(QBrush(Qt::green, Qt::SolidPattern));
		item->setFont(QFont(normalFontFamily, -1, QFont::Bold, false));
		item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
	}
	else
	{
		//item->setBackground(QBrush(Qt::red, Qt::SolidPattern));
		//item->setForeground(QBrush(Qt::white));
		item->setFont(QFont(normalFontFamily, -1, -1, true));
		item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));
	}
	QString flagString = "";
	if (bHasErc)
	{
		flagString += " +ERC";
	}
	if (bHasMorphs)
	{
		flagString += " +Morphs";
	}
	if (bHasPoses)
	{
		flagString += " +Poses";
	}
	QString sToolTip = QString("<b>%2</b><br><i>\"%1\"</i><br>(%3)").arg(morphInfo.Name).arg(morphInfo.Type).arg(morphInfo.Node->getLabel());
	if (flagString.isEmpty() == false)
	{
		sToolTip += "<br>" + flagString;
	}
	item->setToolTip(sToolTip);
	QString sWhatsThis = QString("<b>%1</b><br>").arg(morphInfo.Label);
	QString sNoChange = sWhatsThis;
	QString whatsThisErc = "<b>+ERC:</b> ERC Links allow this element to modify the value of other controls. An example is Victoria 9 controlling Victoria 9 Head and Victoria 9 Body.<br>";
	QString whatsThisMorphs = "<b>+Morphs:</b> This element has ERC Links to control other Morph controls.  This may cause double-dipping in programs outside Daz Studio, where the final morph effect is applied multiple times (once for each linked morph).<br>";
	QString whatsThisPoses = "<b>+Poses:</b> This element has ERC Links to control Pose controls.  This will require the bone poses to be baked into the exported morph.<br>";
	if (bHasErc)
	{
		sWhatsThis += "<br>" + whatsThisErc;
	}
	if (bHasMorphs)
	{
		sWhatsThis += "<br>" + whatsThisMorphs;
	}
	if (bHasPoses)
	{
		sWhatsThis += "<br>" + whatsThisPoses;
	}
	if (sWhatsThis == sNoChange)
	{
		sWhatsThis += "<br>" + sToolTip;
	}
	item->setWhatsThis(sWhatsThis);


	return true;

}


// When the filter text is changed, update the center list
void DzBridgeMorphSelectionDialog::FilterChanged(const QString& filter)
{
	m_morphListWidget->clear();
	QString newFilter = filter;
	m_morphListWidget->clear();
	m_morphListWidget->setIconSize(QSize(16, 16));
	foreach(MorphInfo morphInfo, m_selectedInTree)
	{
		if (newFilter == NULL || newFilter.isEmpty() || morphInfo.Label.contains(newFilter, Qt::CaseInsensitive))
		{
			SortingListItem* item = new SortingListItem();// modLabel, morphListWidget);
			item->setText(morphInfo.Label);
			item->setData(Qt::UserRole, morphInfo.Name);

			decorateMorphListItem(item, morphInfo);

			m_morphListWidget->addItem(item);
		}
	}

	m_morphListWidget->sortItems();
}

// Build a list of availaboe morphs for the node
// TODO: This function evolved a lot as I figured out where to find the morphs.
// There may be dead code in here.
QMap<QString, MorphInfo> DzBridgeMorphSelectionDialog::GetAvailableMorphs(DzNode* Node)
{
	DzObject* Object = Node->getObject();
	DzShape* Shape = Object ? Object->getCurrentShape() : NULL;

	for (int index = 0; index < Node->getNumProperties(); index++)
	{
		DzProperty* property = Node->getProperty(index);
		QString propName = property->getName();
		QString propLabel = property->getLabel();
		DzPresentation* presentation = property->getPresentation();
		if (presentation)
		{
			MorphInfo morphInfo;
			morphInfo.Name = propName;
			morphInfo.Label = propLabel;
			morphInfo.Path = Node->getLabel() + "/" + property->getPath();
			morphInfo.Type = presentation->getType();
			morphInfo.Property = property;
			morphInfo.Node = Node;
			if (!m_morphInfoMap.contains(morphInfo.Name))
			{
				m_morphInfoMap.insert(morphInfo.Name, morphInfo);
			}
			//qDebug() << "Property Name " << propName << " Label " << propLabel << " Presentation Type:" << presentation->getType() << "Path: " << property->getPath();
			//qDebug() << "Path " << property->getGroupOnlyPath();
		}
		// DB (2022-Sept-24): This appears to be dead code.  All active data now stored in m_morphInfoMap, commenting out.
		//if (presentation && presentation->getType() == "Modifier/Shape")
		//{
		//	SortingListItem* item = new SortingListItem();// modLabel, morphListWidget);
		//	item->setText(propLabel);
		//	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		//	if (morphList.contains(propLabel))
		//	{
		//		item->setCheckState(Qt::Checked);
		//		newMorphList.append(propName);
		//	}
		//	else
		//	{
		//		item->setCheckState(Qt::Unchecked);
		//	}
		//	item->setData(Qt::UserRole, propName);
		//	morphNameMapping.insert(propName, propLabel);
		//}
	}

	if (Object)
	{
		for (int index = 0; index < Object->getNumModifiers(); index++)
		{
			DzModifier* modifier = Object->getModifier(index);
			QString modName = modifier->getName();
			QString modLabel = modifier->getLabel();
			DzMorph* mod = qobject_cast<DzMorph*>(modifier);
			if (mod)
			{
				for (int propindex = 0; propindex < modifier->getNumProperties(); propindex++)
				{
					DzProperty* property = modifier->getProperty(propindex);
					QString propName = property->getName();
					QString propLabel = property->getLabel();
					DzPresentation* presentation = property->getPresentation();
					if (presentation)
					{
						MorphInfo morphInfoProp;
						morphInfoProp.Name = modName;
						morphInfoProp.Label = propLabel;
						morphInfoProp.Path = Node->getLabel() + "/" + property->getPath();
						morphInfoProp.Type = presentation->getType();
						morphInfoProp.Property = property;
						morphInfoProp.Node = Node;
						if (!m_morphInfoMap.contains(morphInfoProp.Name))
						{
							m_morphInfoMap.insert(morphInfoProp.Name, morphInfoProp);
						}
						//qDebug() << "Modifier Name " << modName << " Label " << propLabel << " Presentation Type:" << presentation->getType() << " Path: " << property->getPath();
						//qDebug() << "Path " << property->getGroupOnlyPath();
					}
				}

			}

		}
	}

	return m_morphInfoMap;
}

// Build out the left tree
void DzBridgeMorphSelectionDialog::UpdateMorphsTree()
{
	m_morphTreeWidget->clear();
	m_morphsForNode.clear();
	foreach(QString morphName, m_morphInfoMap.keys())
	{
		QString path = m_morphInfoMap[morphName].Path;
		QTreeWidgetItem* parentItem = nullptr;
		foreach(QString pathPart, path.split("/"))
		{
			if (pathPart == "") continue;
			parentItem = FindTreeItem(parentItem, pathPart);

			if (!m_morphsForNode.keys().contains(parentItem))
			{
				m_morphsForNode.insert(parentItem, QList<MorphInfo>());
			}
			m_morphsForNode[parentItem].append(m_morphInfoMap[morphName]);
		}
	}
}

// This function could be better named.  It will find the node matching the property path
// but it will also create the structure of that path in the tree as needed as it searches
QTreeWidgetItem* DzBridgeMorphSelectionDialog::FindTreeItem(QTreeWidgetItem* parent, QString name)
{
	if (parent == nullptr)
	{
		for(int i = 0; i < m_morphTreeWidget->topLevelItemCount(); i++)
		{
			QTreeWidgetItem* item = m_morphTreeWidget->topLevelItem(i);
			if (item->text(0) == name)
			{
				return item;
			}
		}

		QTreeWidgetItem* newItem = new QTreeWidgetItem(m_morphTreeWidget);
		newItem->setText(0, name);
		newItem->setExpanded(true);
		m_morphTreeWidget->addTopLevelItem(newItem);
		return newItem;
	}
	else
	{
		for (int i = 0; i < parent->childCount(); i++)
		{
			QTreeWidgetItem* item = parent->child(i);
			if (item->text(0) == name)
			{
				return item;
			}
		}

		QTreeWidgetItem* newItem = new QTreeWidgetItem(parent);
		newItem->setText(0, name);
		newItem->setExpanded(true);
		parent->addChild(newItem);
		return newItem;
	}
}

// For selection changes in the Left Tree
void DzBridgeMorphSelectionDialog::ItemSelectionChanged()
{
	m_selectedInTree.clear();
	foreach(QTreeWidgetItem* selectedItem, m_morphTreeWidget->selectedItems())
	{
		SelectMorphsInNode(selectedItem);
	}

	FilterChanged(filterEdit->text());
}

// Updates the list of selected morphs in the Left Tree
// including any children
void DzBridgeMorphSelectionDialog::SelectMorphsInNode(QTreeWidgetItem* item)
{
	if (m_morphsForNode.keys().contains(item))
	{
		m_selectedInTree.append(m_morphsForNode[item]);
	}
}

// Add Morphs for export
void DzBridgeMorphSelectionDialog::HandleAddMorphsButton()
{
	foreach(QListWidgetItem* selectedItem, m_morphListWidget->selectedItems())
	{
		QString morphName = selectedItem->data(Qt::UserRole).toString();
		if (m_morphInfoMap.contains(morphName) && !m_morphsToExport.contains(m_morphInfoMap[morphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[morphName]);
		}
	}
	RefreshExportMorphList();
}

// Remove morph from export list
void DzBridgeMorphSelectionDialog::HandleRemoveMorphsButton()
{
	foreach(QListWidgetItem* selectedItem, m_morphExportListWidget->selectedItems())
	{
		QString morphName = selectedItem->data(Qt::UserRole).toString();
		if (m_morphInfoMap.keys().contains(morphName))
		{
			m_morphsToExport.removeAll(m_morphInfoMap[morphName]);
		}
	}
	RefreshExportMorphList();
}

// Brings up a dialgo for choosing a preset name
void DzBridgeMorphSelectionDialog::HandleSavePreset()
{
	QString filters("CSV Files (*.csv)");
	QString defaultFilter("CSV Files (*.csv)");
	QDir dir;
	dir.mkpath(presetsFolder);

	QString presetName = QFileDialog::getSaveFileName(this, QString("Save Preset"),
		presetsFolder,
		filters,
		&defaultFilter);

	if (presetName != NULL)
	{
		SavePresetFile(presetName);
	}
}

// Saves out a preset.  If the path isn't supplied, it's saved as the last selection
void DzBridgeMorphSelectionDialog::SavePresetFile(QString filePath)
{
	QDir dir;
	dir.mkpath(presetsFolder);
	if (filePath == NULL)
	{
		filePath = presetsFolder + QDir::separator() + "LastUsed.csv";
	}

	QFile file(filePath);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);
	out << GetMorphCSVString(false);

	// optional, as QFile destructor will already do it:
	file.close();
	RefreshPresetsCombo();

}

// Hard coded list of morphs for Genesis 3 and 8
// It just adds them all, the other functions will ignore any that don't fit the character
void DzBridgeMorphSelectionDialog::HandleArmJCMMorphsButton()
{
	QStringList MorphsToAdd;

	// Genesis 8
	MorphsToAdd.append("pJCMCollarTwist_n30_L");
	MorphsToAdd.append("pJCMCollarTwist_n30_R");
	MorphsToAdd.append("pJCMCollarTwist_p30_L");
	MorphsToAdd.append("pJCMCollarTwist_p30_R");
	MorphsToAdd.append("pJCMCollarUp_55_L");
	MorphsToAdd.append("pJCMCollarUp_55_R");
	MorphsToAdd.append("pJCMCollarUp_50_L");
	MorphsToAdd.append("pJCMCollarUp_50_R");
	MorphsToAdd.append("pJCMForeArmFwd_135_L");
	MorphsToAdd.append("pJCMForeArmFwd_135_R");
	MorphsToAdd.append("pJCMForeArmFwd_75_L");
	MorphsToAdd.append("pJCMForeArmFwd_75_R");
	MorphsToAdd.append("pJCMHandDwn_70_L");
	MorphsToAdd.append("pJCMHandDwn_70_R");
	MorphsToAdd.append("pJCMHandUp_80_L");
	MorphsToAdd.append("pJCMHandUp_80_R");
	MorphsToAdd.append("pJCMShldrDown_40_L");
	MorphsToAdd.append("pJCMShldrDown_40_R");
	MorphsToAdd.append("pJCMShldrDown_75_L");
	MorphsToAdd.append("pJCMShldrDown_75_R");
	MorphsToAdd.append("pJCMShldrDown2_75_L");
	MorphsToAdd.append("pJCMShldrDown2_75_R");
	MorphsToAdd.append("pJCMShldrFront_n110_Bend_n40_L");
	MorphsToAdd.append("pJCMShldrFront_n110_Bend_p90_L");
	MorphsToAdd.append("pJCMShldrFront_p110_Bend_n90_R");
	MorphsToAdd.append("pJCMShldrFront_p110_Bend_p40_R");
	MorphsToAdd.append("pJCMShldrFwdDwn_110_75_L");
	MorphsToAdd.append("pJCMShldrFwdDwn_110_75_R");
	MorphsToAdd.append("pJCMShldrFwd_110_L");
	MorphsToAdd.append("pJCMShldrFwd_110_R");
	MorphsToAdd.append("pJCMShldrFwd_95_L");
	MorphsToAdd.append("pJCMShldrFwd_95_R");
	MorphsToAdd.append("pJCMShldrUp_90_L");
	MorphsToAdd.append("pJCMShldrUp_90_R");
	MorphsToAdd.append("pJCMShldrUp_35_L");
	MorphsToAdd.append("pJCMShldrUp_35_R");

	// Genesis 9
	MorphsToAdd.append("body_cbs_forearm_y135n_l");
	MorphsToAdd.append("body_cbs_forearm_y135p_r");
	MorphsToAdd.append("body_cbs_forearm_y75n_l");
	MorphsToAdd.append("body_cbs_forearm_y75p_r");
	MorphsToAdd.append("body_cbs_hand_y28n_l");
	MorphsToAdd.append("body_cbs_hand_y28p_r");
	MorphsToAdd.append("body_cbs_hand_z70n_l");
	MorphsToAdd.append("body_cbs_hand_z70p_r");
	MorphsToAdd.append("body_cbs_hand_z80n_r");
	MorphsToAdd.append("body_cbs_hand_z80p_l");
	MorphsToAdd.append("body_cbs_index1_z90n_l");
	MorphsToAdd.append("body_cbs_index1_z90p_r");
	MorphsToAdd.append("body_cbs_index2_z105n_l");
	MorphsToAdd.append("body_cbs_index2_z105p_r");
	MorphsToAdd.append("body_cbs_index3_z90n_l");
	MorphsToAdd.append("body_cbs_index3_z90p_r");
	MorphsToAdd.append("body_cbs_mid1_z95n_l");
	MorphsToAdd.append("body_cbs_mid1_z95p_r");
	MorphsToAdd.append("body_cbs_mid2_z105n_l");
	MorphsToAdd.append("body_cbs_mid2_z105p_r");
	MorphsToAdd.append("body_cbs_mid3_z90n_l");
	MorphsToAdd.append("body_cbs_mid3_z90p_r");
	MorphsToAdd.append("body_cbs_pinky1_z95n_l");
	MorphsToAdd.append("body_cbs_pinky1_z95p_r");
	MorphsToAdd.append("body_cbs_pinky2_z105n_l");
	MorphsToAdd.append("body_cbs_pinky2_z105p_r");
	MorphsToAdd.append("body_cbs_pinky3_z90n_l");
	MorphsToAdd.append("body_cbs_pinky3_z90p_r");
	MorphsToAdd.append("body_cbs_ring1_z95n_l");
	MorphsToAdd.append("body_cbs_ring1_z95p_r");
	MorphsToAdd.append("body_cbs_ring2_z105n_l");
	MorphsToAdd.append("body_cbs_ring2_z105p_r");
	MorphsToAdd.append("body_cbs_ring3_z90n_l");
	MorphsToAdd.append("body_cbs_ring3_z90p_r");
	MorphsToAdd.append("body_cbs_shoulder_x30n_l");
	MorphsToAdd.append("body_cbs_shoulder_x30n_r");
	MorphsToAdd.append("body_cbs_shoulder_x30p_l");
	MorphsToAdd.append("body_cbs_shoulder_x30p_r");
	MorphsToAdd.append("body_cbs_shoulder_z55n_r");
	MorphsToAdd.append("body_cbs_shoulder_z55n_r_COR");
	MorphsToAdd.append("body_cbs_shoulder_z55p_l");
	MorphsToAdd.append("body_cbs_shoulder_z55p_l_COR");
	MorphsToAdd.append("body_cbs_thumb1_y40n_r");
	MorphsToAdd.append("body_cbs_thumb1_y40p_l");
	MorphsToAdd.append("body_cbs_thumb1_z20n_r");
	MorphsToAdd.append("body_cbs_thumb1_z20p_l");
	MorphsToAdd.append("body_cbs_thumb2_y65n_r");
	MorphsToAdd.append("body_cbs_thumb2_y65p_l");
	MorphsToAdd.append("body_cbs_thumb3_y90n_r");
	MorphsToAdd.append("body_cbs_thumb3_y90p_l");
	MorphsToAdd.append("body_cbs_upperarm_x95n_l");
	MorphsToAdd.append("body_cbs_upperarm_x95n_r");
	MorphsToAdd.append("body_cbs_upperarm_y110n_l");
	MorphsToAdd.append("body_cbs_upperarm_y110n_z40n_l");
	MorphsToAdd.append("body_cbs_upperarm_y110n_z90p_l");
	MorphsToAdd.append("body_cbs_upperarm_y110p_r");
	MorphsToAdd.append("body_cbs_upperarm_y110p_z40p_r");
	MorphsToAdd.append("body_cbs_upperarm_y110p_z90n_r");
	MorphsToAdd.append("body_cbs_upperarm_z40n_l");
	MorphsToAdd.append("body_cbs_upperarm_z40p_r");
	MorphsToAdd.append("body_cbs_upperarm_z90n_r");
	MorphsToAdd.append("body_cbs_upperarm_z90p_l");

	// Add the list for export
	foreach(QString MorphName, MorphsToAdd)
	{
		if (m_morphInfoMap.contains(MorphName) && !m_morphsToExport.contains(m_morphInfoMap[MorphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[MorphName]);
		}
	}
	RefreshExportMorphList();
}

// Hard coded list of morphs for Genesis 3 and 8
// It just adds them all, the other functions will ignore any that don't fit the character
void DzBridgeMorphSelectionDialog::HandleLegJCMMorphsButton()
{
	QStringList MorphsToAdd;

	// Genesis 8
	MorphsToAdd.append("pJCMBigToeDown_45_L");
	MorphsToAdd.append("pJCMBigToeDown_45_R");
	MorphsToAdd.append("pJCMFootDwn_75_L");
	MorphsToAdd.append("pJCMFootDwn_75_R");
	MorphsToAdd.append("pJCMFootUp_40_L");
	MorphsToAdd.append("pJCMFootUp_40_R");
	MorphsToAdd.append("pJCMShinBend_155_L");
	MorphsToAdd.append("pJCMShinBend_155_R");
	MorphsToAdd.append("pJCMShinBend_90_L");
	MorphsToAdd.append("pJCMShinBend_90_R");
	MorphsToAdd.append("pJCMThighBack_35_L");
	MorphsToAdd.append("pJCMThighBack_35_R");
	MorphsToAdd.append("pJCMThighFwd_115_L");
	MorphsToAdd.append("pJCMThighFwd_115_R");
	MorphsToAdd.append("pJCMThighFwd_57_L");
	MorphsToAdd.append("pJCMThighFwd_57_R");
	MorphsToAdd.append("pJCMThighSide_85_L");
	MorphsToAdd.append("pJCMThighSide_85_R");
	MorphsToAdd.append("pJCMToesUp_60_L");
	MorphsToAdd.append("pJCMToesUp_60_R");

	// Genesis 9
	MorphsToAdd.append("body_cbs_foot_x45n_l");
	MorphsToAdd.append("body_cbs_foot_x45n_r");
	MorphsToAdd.append("body_cbs_foot_x65p_l");
	MorphsToAdd.append("body_cbs_foot_x65p_r");
	MorphsToAdd.append("body_cbs_foot_z45n_l");
	MorphsToAdd.append("body_cbs_foot_z45p_r");
	MorphsToAdd.append("body_cbs_shin_x155p_l");
	MorphsToAdd.append("body_cbs_shin_x155p_r");
	MorphsToAdd.append("body_cbs_shin_x90p_l");
	MorphsToAdd.append("body_cbs_shin_x90p_r");
	MorphsToAdd.append("body_cbs_thigh_x115n_l");
	MorphsToAdd.append("body_cbs_thigh_x115n_r");
	MorphsToAdd.append("body_cbs_thigh_x115n_z90n_r");
	MorphsToAdd.append("body_cbs_thigh_x115n_z90p_l");
	MorphsToAdd.append("body_cbs_thigh_x35p_l");
	MorphsToAdd.append("body_cbs_thigh_x35p_r");
	MorphsToAdd.append("body_cbs_thigh_x90n_l");
	MorphsToAdd.append("body_cbs_thigh_x90n_r");
	MorphsToAdd.append("body_cbs_thigh_z90n_r");
	MorphsToAdd.append("body_cbs_thigh_z90p_l");
	MorphsToAdd.append("body_cbs_toes_x40p_l");
	MorphsToAdd.append("body_cbs_toes_x40p_r");
	MorphsToAdd.append("body_cbs_toes_x60n_l");
	MorphsToAdd.append("body_cbs_toes_x60n_r");

	// Add the list for export
	foreach(QString MorphName, MorphsToAdd)
	{
		if (m_morphInfoMap.contains(MorphName) && !m_morphsToExport.contains(m_morphInfoMap[MorphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[MorphName]);
		}
	}
	RefreshExportMorphList();
}

// Hard coded list of morphs for Genesis 3 and 8
// It just adds them all, the other functions will ignore any that don't fit the character
void DzBridgeMorphSelectionDialog::HandleTorsoJCMMorphsButton()
{
	QStringList MorphsToAdd;

	// Genesis 8
	MorphsToAdd.append("pJCMAbdomen2Fwd_40");
	MorphsToAdd.append("pJCMAbdomen2Side_24_L");
	MorphsToAdd.append("pJCMAbdomen2Side_24_R");
	MorphsToAdd.append("pJCMAbdomenFwd_35");
	MorphsToAdd.append("pJCMAbdomenLowerFwd_Navel");
	MorphsToAdd.append("pJCMAbdomenUpperFwd_Navel");
	MorphsToAdd.append("pJCMHeadBack_27");
	MorphsToAdd.append("pJCMHeadFwd_25");
	MorphsToAdd.append("pJCMNeckBack_27");
	MorphsToAdd.append("pJCMNeckFwd_35");
	MorphsToAdd.append("pJCMNeckLowerSide_40_L");
	MorphsToAdd.append("pJCMNeckLowerSide_40_R");
	MorphsToAdd.append("pJCMNeckTwist_22_L");
	MorphsToAdd.append("pJCMNeckTwist_22_R");
	MorphsToAdd.append("pJCMNeckTwist_Reverse");
	MorphsToAdd.append("pJCMPelvisFwd_25");
	MorphsToAdd.append("pJCMChestFwd_35");
	MorphsToAdd.append("pJCMChestSide_20_L");
	MorphsToAdd.append("pJCMChestSide_20_R");

	// Genesis 9
	MorphsToAdd.append("body_cbs_head_x25p");
	MorphsToAdd.append("body_cbs_head_x30n");
	MorphsToAdd.append("body_cbs_neck1_x25n");
	MorphsToAdd.append("body_cbs_neck1_x40p");
	MorphsToAdd.append("body_cbs_neck1_x40p_COR");
	MorphsToAdd.append("body_cbs_neck1_y22n_r");
	MorphsToAdd.append("body_cbs_neck1_y22p_l");
	MorphsToAdd.append("body_cbs_neck1_z40n_l");
	MorphsToAdd.append("body_cbs_neck1_z40n_l_COR");
	MorphsToAdd.append("body_cbs_neck1_z40p_r");
	MorphsToAdd.append("body_cbs_neck1_z40p_r_COR");
	MorphsToAdd.append("body_cbs_pelvis_x25n");
	MorphsToAdd.append("body_cbs_pelvis_x25p");
	MorphsToAdd.append("body_cbs_spine1_x35p");
	MorphsToAdd.append("body_cbs_spine1_z15n_l");
	MorphsToAdd.append("body_cbs_spine1_z15p_r");
	MorphsToAdd.append("body_cbs_spine2_x40p");
	MorphsToAdd.append("body_cbs_spine2_z24n_l");
	MorphsToAdd.append("body_cbs_spine2_z24p_r");
	MorphsToAdd.append("body_cbs_spine3_x35p");
	MorphsToAdd.append("body_cbs_spine3_z20n_l");
	MorphsToAdd.append("body_cbs_spine3_z20p_r");
	MorphsToAdd.append("body_ctrl_pecmovement_l");
	MorphsToAdd.append("body_ctrl_pecmovement_r");

	// Add the list for export
	foreach(QString MorphName, MorphsToAdd)
	{
		if (m_morphInfoMap.contains(MorphName) && !m_morphsToExport.contains(m_morphInfoMap[MorphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[MorphName]);
		}
	}
	RefreshExportMorphList();
}

// Genesis 9 FACS blendshapes, blendshape selection for use with WonderStudio
void DzBridgeMorphSelectionDialog::addGenesis9FACS(QStringList& MorphsToAdd)
{
	MorphsToAdd.append("facs_bs_BrowInnerUpLeft");
	MorphsToAdd.append("facs_bs_BrowInnerUpRight");
	MorphsToAdd.append("facs_bs_BrowOuterUpLeft");
	MorphsToAdd.append("facs_bs_BrowOuterUpRight");
	MorphsToAdd.append("facs_bs_BrowSqueezeLeft");
	MorphsToAdd.append("facs_bs_BrowSqueezeRight");
	MorphsToAdd.append("facs_bs_CheekPuffLeft");
	MorphsToAdd.append("facs_bs_CheekPuffRight");
	MorphsToAdd.append("facs_bs_CheekSquintLeft");
	MorphsToAdd.append("facs_bs_CheekSquintRight");
	MorphsToAdd.append("facs_bs_EyeBlinkLeft");
	MorphsToAdd.append("facs_bs_EyeBlinkRight");
	MorphsToAdd.append("facs_bs_EyelidOpenLowerLeft");
	MorphsToAdd.append("facs_bs_EyelidOpenLowerRight");
	MorphsToAdd.append("facs_bs_EyelidOpenUpperLeft");
	MorphsToAdd.append("facs_bs_EyelidOpenUpperRight");
	MorphsToAdd.append("facs_bs_EyeLookUpLeft");
	MorphsToAdd.append("facs_bs_EyeLookUpRight");
	MorphsToAdd.append("facs_bs_EyeSquintLeft");
	MorphsToAdd.append("facs_bs_EyeSquintRight");
	MorphsToAdd.append("facs_bs_JawForward");
	MorphsToAdd.append("facs_bs_JawLeft");
	MorphsToAdd.append("facs_bs_JawOpen");
	MorphsToAdd.append("facs_bs_JawRecess");
	MorphsToAdd.append("facs_bs_JawRight");
	MorphsToAdd.append("facs_bs_MouthCornerMoveSide-SideLeft");
	MorphsToAdd.append("facs_bs_MouthCornerMoveSide-SideRight");
	MorphsToAdd.append("facs_bs_MouthDimpleLeft");
	MorphsToAdd.append("facs_bs_MouthDimpleRight");
	MorphsToAdd.append("facs_bs_MouthFrownLeft");
	MorphsToAdd.append("facs_bs_MouthFrownRight");
	MorphsToAdd.append("facs_bs_MouthLeft");
	MorphsToAdd.append("facs_bs_MouthLowerDownLeft");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight");
	MorphsToAdd.append("facs_bs_MouthRight");
	MorphsToAdd.append("facs_bs_MouthRollLowerLeft");
	MorphsToAdd.append("facs_bs_MouthRollLowerRight");
	MorphsToAdd.append("facs_bs_MouthSmileLeft");
	MorphsToAdd.append("facs_bs_MouthSmileLeft");
	MorphsToAdd.append("facs_bs_MouthSmileRight");
	MorphsToAdd.append("facs_bs_MouthSmileRight");
	MorphsToAdd.append("facs_bs_MouthUpperUpLeft");
	MorphsToAdd.append("facs_bs_MouthUpperUpRight");
	MorphsToAdd.append("facs_bs_NoseSneerLeft");
	MorphsToAdd.append("facs_bs_NoseSneerRight");
	MorphsToAdd.append("facs_cbs_EyeFullCompressionLeft");
	MorphsToAdd.append("facs_cbs_EyeFullCompressionRight");
	MorphsToAdd.append("facs_ctrl_MouthFunnel");
	MorphsToAdd.append("facs_ctrl_MouthPressLeft");
	MorphsToAdd.append("facs_ctrl_MouthPressLeft");
	MorphsToAdd.append("facs_ctrl_MouthPressRight");
	MorphsToAdd.append("facs_ctrl_MouthPressRight");
	MorphsToAdd.append("facs_ctrl_MouthPucker");
	MorphsToAdd.append("facs_ctrl_MouthRollLower");
	MorphsToAdd.append("facs_ctrl_MouthRollUpper");
	MorphsToAdd.append("facs_ctrl_MouthStickyControlLeft");
	MorphsToAdd.append("facs_ctrl_MouthStickyControlRight");
	MorphsToAdd.append("facs_ctrl_NasalCompress");
	MorphsToAdd.append("facs_ctrl_NasalFlare");
	MorphsToAdd.append("facs_bs_BrowDownLeft");
	MorphsToAdd.append("facs_bs_BrowDownLeft");
	MorphsToAdd.append("facs_bs_BrowDownRight");
	MorphsToAdd.append("facs_bs_BrowDownRight");
	MorphsToAdd.append("facs_bs_EyeLookDownLeft");
	MorphsToAdd.append("facs_bs_EyeLookDownRight");
	MorphsToAdd.append("facs_bs_EyeLookInLeft");
	MorphsToAdd.append("facs_bs_EyeLookOutRight");
	MorphsToAdd.append("facs_bs_EyeLookInRight");
	MorphsToAdd.append("facs_bs_EyeLookOutLeft");
	MorphsToAdd.append("facs_bs_JawChinCompression");
	MorphsToAdd.append("facs_bs_JawChinCompression");
	MorphsToAdd.append("facs_bs_MouthCloseLowerLeft");
	MorphsToAdd.append("facs_bs_MouthCloseLowerRight");
	MorphsToAdd.append("facs_bs_MouthCloseUpperLeft");
	MorphsToAdd.append("facs_bs_MouthCloseUpperRight");
	MorphsToAdd.append("facs_bs_MouthCloseUpperLeft");
	MorphsToAdd.append("facs_bs_MouthForward-BackMiddleUpper");
	MorphsToAdd.append("facs_bs_MouthForwardUpperLeft");
	MorphsToAdd.append("facs_bs_MouthCloseUpperRight");
	MorphsToAdd.append("facs_bs_MouthForward-BackMiddleUpper");
	MorphsToAdd.append("facs_bs_MouthForwardUpperRight");
	MorphsToAdd.append("facs_bs_MouthCornerTightnessLowerLeft");
	MorphsToAdd.append("facs_bs_MouthCornerTightnessLowerRight");
	MorphsToAdd.append("facs_bs_MouthForwardLowerLeft");
	MorphsToAdd.append("facs_bs_MouthForwardLowerRight");
	MorphsToAdd.append("facs_bs_MouthForwardUpperLeft");
	MorphsToAdd.append("facs_bs_MouthForwardUpperRight");
	MorphsToAdd.append("facs_bs_MouthFunnelLowerLeft");
	MorphsToAdd.append("facs_bs_MouthFunnelLowerRight");
	MorphsToAdd.append("facs_bs_MouthFunnelLowerLeft");
	MorphsToAdd.append("facs_bs_MouthLowerDownLeft");
	MorphsToAdd.append("facs_bs_MouthFunnelLowerRight");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight");
	MorphsToAdd.append("facs_bs_MouthFunnelUpperLeft");
	MorphsToAdd.append("facs_bs_MouthFunnelUpperRight");
	MorphsToAdd.append("facs_bs_MouthLipsSide-SideLower");
	MorphsToAdd.append("facs_bs_MouthLipsSide-SideUpper");
	MorphsToAdd.append("facs_bs_MouthLipsSide-SideLower");
	MorphsToAdd.append("facs_bs_MouthLipsSide-SideUpper");
	MorphsToAdd.append("facs_bs_MouthPurseLowerLeft");
	MorphsToAdd.append("facs_bs_MouthPurseUpperLeft");
	MorphsToAdd.append("facs_bs_MouthPurseLowerRight");
	MorphsToAdd.append("facs_bs_MouthPurseUpperRight");
	MorphsToAdd.append("facs_bs_MouthShrugUpperLeft");
	MorphsToAdd.append("facs_bs_MouthUpperUpLeft");
	MorphsToAdd.append("facs_bs_MouthShrugUpperRight");
	MorphsToAdd.append("facs_bs_MouthUpperUpRight");
	MorphsToAdd.append("facs_bs_MouthSmileLeft");
	MorphsToAdd.append("facs_bs_MouthSmileWidenLeft");
	MorphsToAdd.append("facs_bs_MouthLowerDownLeft");
	MorphsToAdd.append("facs_bs_MouthUpperUpLeft");
	MorphsToAdd.append("facs_bs_MouthSmileRight");
	MorphsToAdd.append("facs_bs_MouthSmileWidenRight");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight");
	MorphsToAdd.append("facs_bs_MouthUpperUpRight");
	MorphsToAdd.append("facs_bs_MouthSmileWidenLeft");
	MorphsToAdd.append("facs_bs_MouthCornerMoveUp-DownLeft");
	MorphsToAdd.append("facs_bs_MouthSmileWidenRight");
	MorphsToAdd.append("facs_bs_MouthCornerMoveUp-DownRight");
	MorphsToAdd.append("facs_bs_NasalFlareLeft");
	MorphsToAdd.append("facs_bs_NoseSneerLeft");
	MorphsToAdd.append("facs_bs_NasalCreaseFlexLeft");
	MorphsToAdd.append("facs_bs_NasalFlareRight");
	MorphsToAdd.append("facs_bs_NoseSneerRight");
	MorphsToAdd.append("facs_bs_NasalCreaseFlexRight");
	MorphsToAdd.append("facs_ctrl_MouthRollLower");
	MorphsToAdd.append("facs_ctrl_MouthForward-BackLowerLeft");
	MorphsToAdd.append("facs_ctrl_MouthForward-BackLowerRight");
	MorphsToAdd.append("facs_bs_MouthForward-BackMiddleLower");
	MorphsToAdd.append("facs_ctrl_MouthRollUpper");
	MorphsToAdd.append("facs_bs_MouthForward-BackMiddleUpper");
	MorphsToAdd.append("facs_ctrl_MouthForward-BackUpperLeft");
	MorphsToAdd.append("facs_ctrl_MouthForward-BackUpperRight");

	
}

// Add Genesis8 FACS controls for use by WonderStudio conversion
void DzBridgeMorphSelectionDialog::addGenesis81FACS(QStringList& MorphsToAdd)
{
	MorphsToAdd.append("facs_bs_BrowInnerUpLeft_div2");
	MorphsToAdd.append("facs_bs_BrowInnerUpRight_div2");
	MorphsToAdd.append("facs_bs_BrowOuterUpLeft_div2");
	MorphsToAdd.append("facs_bs_BrowOuterUpRight_div2");
	MorphsToAdd.append("facs_bs_CheekPuffLeft_div2");
	MorphsToAdd.append("facs_bs_CheekPuffRight_div2");
	MorphsToAdd.append("facs_bs_CheekSquintLeft_div2");
	MorphsToAdd.append("facs_bs_CheekSquintRight_div2");
	MorphsToAdd.append("facs_bs_EyeSquintLeft_div2");
	MorphsToAdd.append("facs_bs_EyeSquintRight_div2");
	MorphsToAdd.append("facs_bs_MouthDimpleLeft_div2");
	MorphsToAdd.append("facs_bs_MouthDimpleRight_div2");
	MorphsToAdd.append("facs_bs_MouthFrownLeft_div2");
	MorphsToAdd.append("facs_bs_MouthFrownRight_div2");
	MorphsToAdd.append("facs_bs_MouthLeft_div2");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight_div2");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight_div2");
	MorphsToAdd.append("facs_bs_MouthPressLeft_div2");
	MorphsToAdd.append("facs_bs_MouthPressLeft_div2");
	MorphsToAdd.append("facs_bs_MouthPressRight_div2");
	MorphsToAdd.append("facs_bs_MouthPressRight_div2");
	MorphsToAdd.append("facs_bs_MouthPucker_div2");
	MorphsToAdd.append("facs_bs_MouthRight_div2");
	MorphsToAdd.append("facs_bs_MouthRollLower_div2");
	MorphsToAdd.append("facs_bs_MouthRollUpper_div2");
	MorphsToAdd.append("facs_bs_MouthSmileLeft_div2");
	MorphsToAdd.append("facs_bs_MouthSmileLeft_div2");
	MorphsToAdd.append("facs_bs_MouthSmileRight_div2");
	MorphsToAdd.append("facs_bs_MouthSmileRight_div2");
	MorphsToAdd.append("facs_bs_MouthUpperUpLeft_div2");
	MorphsToAdd.append("facs_bs_MouthUpperUpRight_div2");
	MorphsToAdd.append("facs_bs_NasalFlare_div2");
	MorphsToAdd.append("facs_bs_NoseSneerLeft_div2");
	MorphsToAdd.append("facs_bs_NoseSneerRight_div2");
	MorphsToAdd.append("facs_cbs_EyeBlinkLeft_div2");
	MorphsToAdd.append("facs_cbs_EyeBlinkRight_div2");
	MorphsToAdd.append("facs_jnt_EyeBlinkRight");
	MorphsToAdd.append("facs_jnt_EyeLookDownLeft");
	MorphsToAdd.append("facs_jnt_JawLeft");
	MorphsToAdd.append("facs_jnt_JawOpen");
	MorphsToAdd.append("facs_jnt_JawOpen");
	MorphsToAdd.append("facs_jnt_JawRecess");
	MorphsToAdd.append("facs_jnt_JawRight");

	MorphsToAdd.append("facs_bs_BrowDownLeft_div2");
	MorphsToAdd.append("facs_bs_BrowDownRight_div2");
	MorphsToAdd.append("facs_bs_MouthClose_div2");
	MorphsToAdd.append("facs_bs_MouthFunnel_div2");
	MorphsToAdd.append("facs_bs_MouthRollLower_div2");
	MorphsToAdd.append("facs_ctrl_EyeLookDownLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookDownRight");
	MorphsToAdd.append("facs_ctrl_EyeLookInLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookOutRight");
	MorphsToAdd.append("facs_ctrl_EyeLookInRight");
	MorphsToAdd.append("facs_ctrl_EyeLookOutLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookUpLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookUpRight");
	MorphsToAdd.append("facs_jnt_EyeWideLeft");
	MorphsToAdd.append("facs_jnt_EyeWideRight");

}

// Hard coded list of morphs for Genesis 8.1 and ARKit
// It just adds them all, the other functions will ignore any that don't fit the character
void DzBridgeMorphSelectionDialog::HandleARKitGenesis81MorphsButton()
{
	QStringList MorphsToAdd;

	MorphsToAdd.append("facs_jnt_EyeWideLeft");
	MorphsToAdd.append("facs_jnt_EyeWideRight");
	MorphsToAdd.append("facs_jnt_EyeBlinkLeft");
	MorphsToAdd.append("facs_jnt_EyeBlinkRight");
	MorphsToAdd.append("facs_bs_EyeSquintLeft_div2");
	MorphsToAdd.append("facs_bs_EyeSquintRight_div2");
	MorphsToAdd.append("facs_ctrl_EyeLookUpRight");
	MorphsToAdd.append("facs_ctrl_EyeLookUpLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookOutRight");
	MorphsToAdd.append("facs_ctrl_EyeLookOutLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookInRight");
	MorphsToAdd.append("facs_ctrl_EyeLookInLeft");
	MorphsToAdd.append("facs_ctrl_EyeLookDownRight");
	MorphsToAdd.append("facs_ctrl_EyeLookDownLeft");
	MorphsToAdd.append("facs_bs_NoseSneerRight_div2");
    MorphsToAdd.append("facs_bs_NoseSneerLeft_div2");
	MorphsToAdd.append("facs_jnt_JawForward");
	MorphsToAdd.append("facs_jnt_JawLeft");
	MorphsToAdd.append("facs_jnt_JawRight");
	MorphsToAdd.append("facs_jnt_JawOpen");
	MorphsToAdd.append("facs_bs_MouthClose_div2");
	MorphsToAdd.append("facs_bs_MouthFunnel_div2");
	MorphsToAdd.append("facs_bs_MouthPucker_div2");
	MorphsToAdd.append("facs_bs_MouthLeft_div2");
	MorphsToAdd.append("facs_bs_MouthRight_div2");
	MorphsToAdd.append("facs_bs_MouthSmileLeft_div2");
	MorphsToAdd.append("facs_bs_MouthSmileRight_div2");
	MorphsToAdd.append("facs_bs_MouthFrownLeft_div2");
	MorphsToAdd.append("facs_bs_MouthFrownRight_div2");
	MorphsToAdd.append("facs_bs_MouthDimpleLeft_div2");
	MorphsToAdd.append("facs_bs_MouthDimpleRight_div2");
	MorphsToAdd.append("facs_bs_MouthStretchLeft_div2");
	MorphsToAdd.append("facs_bs_MouthStretchRight_div2");
	MorphsToAdd.append("facs_bs_MouthRollLower_div2");
	MorphsToAdd.append("facs_bs_MouthRollUpper_div2");
	MorphsToAdd.append("facs_bs_MouthShrugLower_div2");
	MorphsToAdd.append("facs_bs_MouthShrugUpper_div2");
	MorphsToAdd.append("facs_bs_MouthPressLeft_div2");
	MorphsToAdd.append("facs_bs_MouthPressRight_div2");
	MorphsToAdd.append("facs_bs_MouthLowerDownLeft_div2");
	MorphsToAdd.append("facs_bs_MouthLowerDownRight_div2");
	MorphsToAdd.append("facs_bs_MouthUpperUpLeft_div2");
	MorphsToAdd.append("facs_bs_MouthUpperUpRight_div2");
	MorphsToAdd.append("facs_bs_BrowDownLeft_div2");
	MorphsToAdd.append("facs_bs_BrowDownRight_div2");
	MorphsToAdd.append("facs_ctrl_BrowInnerUp");
	MorphsToAdd.append("facs_bs_BrowOuterUpLeft_div2");
	MorphsToAdd.append("facs_bs_BrowOuterUpRight_div2");
	MorphsToAdd.append("facs_ctrl_CheekPuff");
	MorphsToAdd.append("facs_bs_CheekSquintLeft_div2");
	MorphsToAdd.append("facs_bs_CheekSquintRight_div2");
	MorphsToAdd.append("facs_bs_NoseSneerLeft_div2");
	MorphsToAdd.append("facs_bs_NoseSneerRight_div2");
	MorphsToAdd.append("facs_bs_TongueOut");

	// add additional FACS morphs for both 8.1 and 9 to MorphsToAdd
	addGenesis81FACS(MorphsToAdd);
	addGenesis9FACS(MorphsToAdd);

	// Add the list for export
	foreach(QString MorphName, MorphsToAdd)
	{
		if (m_morphInfoMap.contains(MorphName) && !m_morphsToExport.contains(m_morphInfoMap[MorphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[MorphName]);
		}
	}
	RefreshExportMorphList();
}

void DzBridgeMorphSelectionDialog::HandleFaceFXGenesis8Button()
{
	QStringList MorphsToAdd;

	MorphsToAdd.append("eCTRLvSH");
	MorphsToAdd.append("eCTRLvW");
	MorphsToAdd.append("eCTRLvM");
	MorphsToAdd.append("eCTRLvF");
	MorphsToAdd.append("eCTRLMouthOpen");
	MorphsToAdd.append("eCTRLMouthWide-Narrow");
	MorphsToAdd.append("eCTRLTongueIn-Out");
	MorphsToAdd.append("eCTRLTongueUp-Down");

	// Add the list for export
	foreach(QString MorphName, MorphsToAdd)
	{
		if (m_morphInfoMap.contains(MorphName) && !m_morphsToExport.contains(m_morphInfoMap[MorphName]))
		{
			m_morphsToExport.append(m_morphInfoMap[MorphName]);
		}
	}
	RefreshExportMorphList();
}

// Refresh the Right export list
void DzBridgeMorphSelectionDialog::RefreshExportMorphList()
{
	m_morphExportListWidget->clear();
	m_morphExportListWidget->setIconSize(QSize(16,16));
	foreach(MorphInfo morphInfo, m_morphsToExport)
	{
		SortingListItem* item = new SortingListItem();
		item->setText(morphInfo.Label);
		item->setData(Qt::UserRole, morphInfo.Name);

		decorateMorphListItem(item, morphInfo, true);

		m_morphExportListWidget->addItem(item);
	}
}

// Refresh the list of preset csvs from the files in the folder
void DzBridgeMorphSelectionDialog::RefreshPresetsCombo()
{
	disconnect(presetCombo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(HandlePresetChanged(const QString &)));

	presetCombo->clear();
	presetCombo->addItem("None");

	QDirIterator it(presetsFolder, QStringList() << "*.csv", QDir::NoFilter, QDirIterator::NoIteratorFlags);
	while (it.hasNext())
	{
		QString Path = it.next();
		QString NewPath = Path.right(Path.length() - presetsFolder.length() - 1);
		presetCombo->addItem(NewPath);
	}
	connect(presetCombo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(HandlePresetChanged(const QString &)));
}

// Call when the preset combo is changed by the user
void DzBridgeMorphSelectionDialog::HandlePresetChanged(const QString& presetName)
{
	m_morphsToExport.clear();
	QString PresetFilePath = presetsFolder + QDir::separator() + presetName;

	QFile file(PresetFilePath);
	if (!file.open(QIODevice::ReadOnly)) {
		// TODO: should be an error dialog
		return;
	}

	// load the selected csv from disk into the export list on the right
	QTextStream InStream(&file);

	while (!InStream.atEnd()) {
		QString MorphLine = InStream.readLine();
		if (MorphLine.endsWith("\"Export\""))
		{
			QStringList Items = MorphLine.split(",");
			QString MorphName = Items[0].replace("\"", "");
			if (m_morphInfoMap.contains(MorphName))
			{
				m_morphsToExport.append(m_morphInfoMap[MorphName]);
			}
		}
	}

	RefreshExportMorphList();
	file.close();
}

// TODO: fill with data
MorphExportSettings DzBridgeMorphSelectionDialog::getMorphExportSettings()
{
	return MorphExportSettings();
}

// Get the morph string (aka m_morphsToExport_finalized) in the format used for presets
QString DzBridgeMorphSelectionDialog::GetMorphCSVString(bool bUseFinalizedList)
{
	//morphList.clear();
	QString morphString;
	QList<MorphInfo> *pMorphList = &m_morphsToExport;
	if (bUseFinalizedList)
	{
		pMorphList = &m_morphsToExport_finalized;
	}
	foreach(MorphInfo exportMorph, *pMorphList)
	{
		//morphList.append(exportMorph.Name);
		morphString += "\"" + exportMorph.Name + "\",\"Export\"\n";
	}
	morphString += "\".CTRLVS\", \"Ignore\"\n";
	morphString += "\"Anything\", \"Bake\"\n";
	return morphString;
}

// DB 2023-11-14: Morph Selection Overhaul
QList<QString> DzBridgeMorphSelectionDialog::GetMorphNamesToExport()
{
	QList<QString> morphNamesToExport;

	foreach(MorphInfo morphInfo, m_morphsToExport_finalized)
	{
		morphNamesToExport.append(morphInfo.Name);
	}

	return morphNamesToExport;
}

// DB 2023-11-14: Morph Selection Overhaul
QMap<QString, MorphInfo> DzBridgeMorphSelectionDialog::GetAvailableMorphsTable()
{
	return m_morphInfoMap;
}

// Retrieve label based on morph name
// DB Dec-21-2021, Created for scripting.
QString DzBridgeMorphSelectionDialog::GetMorphLabelFromName(QString morphName)
{
	if (m_morphInfoMap.isEmpty()) return QString();

	if (m_morphInfoMap.contains(morphName))
	{
		MorphInfo morph = m_morphInfoMap[morphName];
		return morph.Label;
	}
	else
	{
		return QString();
	}

}

// Get MorphInfo from morph name
// DB June-01-2022, Created for MorphLinks Generation for Blender Bridge Morphs Support
MorphInfo DzBridgeMorphSelectionDialog::GetMorphInfoFromName(QString morphName)
{
	if (m_morphInfoMap.isEmpty()) return MorphInfo();

	if (m_morphInfoMap.contains(morphName))
	{
		MorphInfo morph = m_morphInfoMap[morphName];
		return morph;
	}
	else
	{
		return MorphInfo();
	}

}

QString DzBridgeMorphSelectionDialog::getMorphPropertyName(DzProperty* pMorphProperty)
{
	if (pMorphProperty == nullptr)
	{
		// issue error message or alternatively: throw exception
		printf("ERROR: DazBridge: DzBridgeMorphSelectionDialog.cpp, getPropertyName(): nullptr passed as argument.");
		return "";
	}
	QString sPropertyName = pMorphProperty->getName();
	auto owner = pMorphProperty->getOwner();
	if (owner && owner->inherits("DzMorph"))
	{
		sPropertyName = owner->getName();
	}
	return sPropertyName;
}

bool DzBridgeMorphSelectionDialog::isValidMorph(DzProperty* pMorphProperty)
{
	if (pMorphProperty == nullptr)
	{
		// issue error message or alternatively: throw exception
		dzApp->warning("ERROR: DazBridge: DzBridgeMorphSelectionDialog.cpp, isValidMorph(): nullptr passed as argument.");
		return false;
	}
	QString sMorphName = getMorphPropertyName(pMorphProperty);
	QStringList ignoreConditionList;
	ignoreConditionList += "x"; ignoreConditionList += "y"; ignoreConditionList += "z";
	for (auto ignoreCondition : ignoreConditionList)
	{
		if (sMorphName.toLower()[0] == ignoreCondition[0])
			return false;
	}
	for (auto iterator = pMorphProperty->controllerListIterator(); iterator.hasNext(); )
	{
		DzERCLink* ercLink = qobject_cast<DzERCLink*>(iterator.next());
		if (ercLink == nullptr)
			continue;
		if (ercLink->getType() == 3) // Multiply
		{
			auto controllerProperty = ercLink->getProperty();
			if (controllerProperty && controllerProperty->getDoubleValue() == 0)
				return false;
		}
	}
	return true;
}

// Load morphs controlling the morphs in the export list
void DzBridgeMorphSelectionDialog::HandleAddConnectedMorphs()
{
	// sanity check
	if (m_morphsToExport.length() == 0)
	{
		return;
	}

	foreach (MorphInfo exportMorph, m_morphsToExport)
	{
	 	DzProperty *morphProperty = exportMorph.Property;
		if (morphProperty == nullptr)
		{
			// log unexpected error
			continue;
		}
		for (auto slaveControllerIterator = morphProperty->slaveControllerListIterator(); slaveControllerIterator.hasNext(); )
		{
			DzProperty *controllerProperty = slaveControllerIterator.next()->getOwner();
			if (isValidMorph(controllerProperty)==false)
				continue;
			QString sMorphName = getMorphPropertyName(controllerProperty);

			// Add the list for export
			if (m_morphInfoMap.contains(sMorphName) && !m_morphsToExport.contains(m_morphInfoMap[sMorphName]))
			{
				m_morphsToExport.append(m_morphInfoMap[sMorphName]);
			}

		}
	}
	RefreshExportMorphList();
}

// Load morphs controlling the morphs in the export list
void DzBridgeMorphSelectionDialog::HandleAddConnectedJcms()
{
	// sanity check
	if (m_morphsToExport.length() == 0)
	{
		return;
	}

	QList<JointLinkInfo> jointLinks = MorphTools::GetActiveJointControlledMorphs();
	foreach(JointLinkInfo jointLink, jointLinks)
	{
		DzProperty* morphProperty = jointLink.LinkMorphInfo.Property;
		if (morphProperty == nullptr)
		{
			// log unexpected error
			continue;
		}
		for (auto slaveControllerIterator = morphProperty->slaveControllerListIterator(); slaveControllerIterator.hasNext(); )
		{
			DzProperty* controllerProperty = slaveControllerIterator.next()->getOwner();
			if (isValidMorph(controllerProperty) == false)
				continue;
			QString sMorphName = getMorphPropertyName(controllerProperty);

			// Add the list for export
			if (m_morphInfoMap.contains(sMorphName) && !m_morphsToExport.contains(m_morphInfoMap[sMorphName]))
			{
				m_morphsToExport.append(m_morphInfoMap[sMorphName]);
			}

		}
	}
	RefreshExportMorphList();
}

QList<QString> DzBridgeMorphSelectionDialog::GetPoseList()
{
	QList<QString> poseList;
	foreach(MorphInfo exportMorph, m_morphsToExport)
	{
		poseList.append(exportMorph.Name);
	}
	return poseList;
}

void DzBridgeMorphSelectionDialog::HandleDialogAccepted(bool bSavePreset)
{
	// Commit GUI right pane listbox to m_morphsToExport
	m_morphsToExport_finalized.clear();
	for (auto morph : m_morphsToExport)
	{
		m_morphsToExport_finalized.append(morph);
	}

	if (bSavePreset)
	{
		SavePresetFile(NULL);
	}

	return;
}

#include "moc_DzBridgeMorphSelectionDialog.cpp"
