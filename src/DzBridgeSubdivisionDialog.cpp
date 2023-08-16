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
#include "QtCore/qfile.h"
#include "QtCore/qtextstream.h"

#include "dzapp.h"
#include "dzscene.h"
#include "dzstyle.h"
#include "dzmainwindow.h"
#include "dzactionmgr.h"
#include "dzaction.h"
#include "dzskeleton.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzmodifier.h"
#include "dzpresentation.h"
#include "dzassetmgr.h"
#include "dzproperty.h"
#include "dznumericnodeproperty.h"
#include "dzsettings.h"
#include "dzmorph.h"
#include "dzgeometry.h"
#include "dzenumproperty.h"

#include "DzBridgeSubdivisionDialog.h"

#include "QtGui/qlayout.h"
#include "QtGui/qlineedit.h"

#include <QtCore/QDebug.h>

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"

using namespace DzBridgeNameSpace;

CPP_Export DzBridgeSubdivisionDialog* DzBridgeSubdivisionDialog::singleton = nullptr;

DzBridgeSubdivisionDialog::DzBridgeSubdivisionDialog(QWidget *parent) :
	DzBasicDialog(parent, DAZ_BRIDGE_LIBRARY_NAME)
{
	 subdivisionItemsGrid = NULL;
	//settings = new QSettings("Code Wizards", "DazToUnreal");

	// Set the dialog title 
	setWindowTitle(tr("Bake Subdivision Levels"));

	// Setup folder
	presetsFolder = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + QDir::separator() + "DazBridge" + QDir::separator() + "Presets";

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setMargin(5);

	// Bake Subdivision Levels Explanation
	QLabel* helpBox = new QLabel();
	helpBox->setTextFormat(Qt::RichText);
	helpBox->setWordWrap(true);
	helpBox->setText(
		tr("<div>Daz Studio uses Catmull-Clark Subdivision Surface technology which is a mathematical way to describe an infinitely smooth surface in a very \
efficient manner. Similar to how an infinitely smooth circle can be described with just the equation: <i>Radius R=x</i>, the base resolution mesh of a \
Daz Figure is actually the mathematical data in an equation to describe an infinitely smooth surface.<br></div>") + \
tr("<div>For Software which supports Catmull-Clark Subdivision and subdivision surface-based morphs (also known as HD Morphs), there is no loss in quality \
or detail by exporting the base resolution mesh (subdivision level 0).<br></div>") + \
tr("<div>For Software which does not fully support Catmull-Clark Subdivision or HD Morphs, we can \"Bake\" additional subdivision detail levels into the mesh to \
more closely approximate the detail of the original surface. However, baking each additional subdivision level requires exponentially more CPU time, memory, \
and storage space.<br></div>") + \
tr("<div><h3><i>Only</i> bake subdivision levels when the destination software does not support Catmull-Clark Subdivision Surface technology. \
Due to this exponentially increasing resource requirement, it is not recommended to bake more than two subdivision levels.</h3></div>") + \
	"<p>"
);
	mainLayout->addWidget(helpBox);

	subdivisionItemsGrid = new QGridLayout();
	subdivisionItemsGrid->addWidget(new QLabel("Object Name"), 0, 0);
	subdivisionItemsGrid->addWidget(new QLabel("Subdivision Level"), 0, 1);
	subdivisionItemsGrid->addWidget(new QLabel("Base Vert Count"), 0, 2);
	subdivisionItemsGrid->addWidget(new QLabel("Estimated Subdivided Vert Count"), 0, 3);
	mainLayout->addLayout(subdivisionItemsGrid);
	mainLayout->addStretch();

	this->addLayout(mainLayout);
	resize(QSize(800, 750));//.expandedTo(minimumSizeHint()));
	setFixedWidth(width());
	setFixedHeight(height());

	SubdivisionCombos.clear();

	PrepareDialog();
}

QSize DzBridgeSubdivisionDialog::minimumSizeHint() const
{
	return QSize(800, 800);
}


void DzBridgeSubdivisionDialog::PrepareDialog()
{
	/*foreach(QObject* object, this->children())
	{
		delete object;
	}*/

	/*if (this->layout())
	{
		delete this->layout();
	}


	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(new QLabel("Subdivision can greatly increase transfer time."));*/

	int itemCount = subdivisionItemsGrid->count();
	while(QLayoutItem* item = subdivisionItemsGrid->takeAt(0))
	{
		if (QWidget* widget = item->widget())
		{
			delete widget;
			//delete item;
		}
	}

	//subdivisionItemsGrid = new QGridLayout(this);
	subdivisionItemsGrid->addWidget(new QLabel("Object Name"), 0, 0);
	subdivisionItemsGrid->addWidget(new QLabel("Subdivision Level"), 0, 1);
	subdivisionItemsGrid->addWidget(new QLabel("Base Vert Count"), 0, 2);
	subdivisionItemsGrid->addWidget(new QLabel("Estimated Subdivided Vert Count"), 0, 3);
	//mainLayout->addLayout(subdivisionItemsGrid);
	//mainLayout->addStretch();

	//this->addLayout(mainLayout);
	//resize(QSize(800, 800));//.expandedTo(minimumSizeHint()));
	//setFixedWidth(width());
	//setFixedHeight(height());

	SubdivisionCombos.clear();
	DzNode* Selection = dzScene->getPrimarySelection();
	CreateList(Selection);
}

void DzBridgeSubdivisionDialog::CreateList(DzNode* Node)
{
	DzObject* Object = Node->getObject();
	if (Object)
	{
		DzShape* Shape = Object ? Object->getCurrentShape() : NULL;
		DzGeometry* Geo = Shape ? Shape->getGeometry() : NULL;
		
		int row = subdivisionItemsGrid->rowCount();
		subdivisionItemsGrid->addWidget(new QLabel(Node->getLabel()), row, 0);
		QComboBox* subdivisionLevelCombo = new QComboBox(this);
		subdivisionLevelCombo->setProperty("Object", QVariant(Node->getName()));
		subdivisionLevelCombo->addItem("0");
		subdivisionLevelCombo->addItem("1");
		subdivisionLevelCombo->addItem("2");
		subdivisionLevelCombo->addItem("3");
		subdivisionLevelCombo->addItem("4");
		SubdivisionCombos.append(subdivisionLevelCombo);
		subdivisionItemsGrid->addWidget(subdivisionLevelCombo, row, 1);
		if (SubdivisionLevels.contains(Node->getName()))
		{
			subdivisionLevelCombo->setCurrentIndex(SubdivisionLevels[Node->getName()]);
		}
		connect(subdivisionLevelCombo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(HandleSubdivisionLevelChanged(const QString &)));

		if (Geo)
		{
			int VertCount = Geo->getNumVertices();
			// 2022-July-04: Fix fo Geoshell crash on export
			if (VertCount > 0)
			{
				QLabel* baseVertCountLabel = new QLabel(QString::number(VertCount));
				baseVertCountLabel->setAlignment(Qt::AlignCenter);
				subdivisionItemsGrid->addWidget(baseVertCountLabel, row, 2);

				// estimated subdivided vert count
				int currentSubDLevel = subdivisionLevelCombo->currentText().toInt();
				int subdVertCount = VertCount;
				for (int i = 0; i < currentSubDLevel; i++) subdVertCount = subdVertCount * 4;
				float scale = subdVertCount / VertCount;
				QLabel* subdVertCountLabel = new QLabel(QString::number(subdVertCount) + " (" + QString::number(scale) + "x)");
				subdVertCountLabel->setAlignment(Qt::AlignCenter);
				subdivisionItemsGrid->addWidget(subdVertCountLabel, row, 3);

				/*for (int index = 0; index < Shape->getNumProperties(); index++)
				{
					DzProperty* property = Shape->getProperty(index);
					QString propName = property->getName();//property->getName();
					QString propLabel = property->getLabel();
					qDebug() << propName << " " << propLabel;
				}*/

			}
			else
			{
				//auto debugGeoName = Geo->getName();
				//auto debugShapeName = Shape->getName();
				//auto debugNodeName = Node->getName();
				//printf("DEBUG: DazBridgeLibrary, DzBridgeSubdivisionDialog.h: Geo VertCount is <= 0: %s", debugNodeName.toAscii().constData());
			}
		}
	}

	for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		CreateList(ChildNode);
	}
}

void DzBridgeSubdivisionDialog::HandleSubdivisionLevelChanged(const QString& text)
{
	foreach(QComboBox* combo, SubdivisionCombos)
	{
		QString name = combo->property("Object").toString();
		int targetValue = combo->currentText().toInt();
		SubdivisionLevels[name] = targetValue;
	}

	// update estimated subdivided vert counts
	for (int row=0; row < subdivisionItemsGrid->rowCount(); row++)
	{
		auto item1 = subdivisionItemsGrid->itemAtPosition(row, 1);
		if (item1 == nullptr) continue;
		auto subdCombo = qobject_cast<QComboBox*>(item1->widget());
		if (subdCombo)
		{
			QString name = subdCombo->property("Object").toString();
			int currentSubDLevel = subdCombo->currentText().toInt();
			SubdivisionLevels[name] = currentSubDLevel;

			auto item2 = subdivisionItemsGrid->itemAtPosition(row, 2);
			if (item2 == nullptr) continue;
			auto vertCountLabel = qobject_cast<QLabel*>(item2->widget());
			if (vertCountLabel)
			{
				int vertCount = vertCountLabel->text().toInt();
				int subdVertCount = vertCount;
				for (int i = 0; i < currentSubDLevel; i++) subdVertCount = subdVertCount * 4;

				float scale = subdVertCount / vertCount;

				auto item3 = subdivisionItemsGrid->itemAtPosition(row, 3);
				if (item3 == nullptr) continue;
				auto subdVertLabel = qobject_cast<QLabel*>(item3->widget());
				subdVertLabel->setText(QString::number(subdVertCount) + " (" + QString::number(scale)+ "x)");

			}

		}
		
	}

}

DzNode* DzBridgeSubdivisionDialog::FindObject(DzNode* Node, QString Name)
{
	if (Node == nullptr)
		return nullptr;

	DzObject* Object = Node->getObject();
	if (Object)
	{
		if (Node->getName() == Name) return Node;
	}

	for (int ChildIndex = 0; ChildIndex < Node->getNumNodeChildren(); ChildIndex++)
	{
		DzNode* ChildNode = Node->getNodeChild(ChildIndex);
		DzNode* FoundNode = FindObject(ChildNode, Name);
		if (FoundNode) return FoundNode;
	}
	return NULL;
}

bool DzBridgeSubdivisionDialog::setSubdivisionLevelByNode(DzNode* Node, int level)
{
	if (Node == nullptr)
		return false;

	DzNode* selection = dzScene->getPrimarySelection();
	QString searchName = Node->getName();
	foreach(QComboBox * combo, SubdivisionCombos)
	{
		QString name = combo->property("Object").toString();
		if (name == searchName)
		{
			int maxLevel = combo->count() - 1;
			if (level > maxLevel)
				return false;

			combo->setCurrentIndex(level);
			return true;
		}
	}

	return false;
}

void DzBridgeSubdivisionDialog::LockSubdivisionProperties(bool subdivisionEnabled)
{
	DzNode* Selection = dzScene->getPrimarySelection();
	foreach(QComboBox* combo, SubdivisionCombos)
	{
		QString Name = combo->property("Object").toString();
		double targetValue = combo->currentText().toDouble();
		DzNode* ObjectNode = FindObject(Selection, Name);
		if (ObjectNode)
		{
			DzObject* Object = ObjectNode->getObject();
			DzShape* Shape = Object ? Object->getCurrentShape() : NULL;
			DzGeometry* Geo = Shape ? Shape->getGeometry() : NULL;
			if (Geo)
			{
				int VertCount = Geo->getNumVertices();

				for (int index = 0; index < Shape->getNumProperties(); index++)
				{
					DzProperty* property = Shape->getProperty(index);
					DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(property);
					DzEnumProperty* enumProperty = qobject_cast<DzEnumProperty*>(property);
					QString propName = property->getName();

					// DB 2023-May-26: fix for native subdivision in target software
					if (propName == "lodlevel" && enumProperty)
					{
						UndoData undo_data;
						undo_data.originalNumericLockState = enumProperty->isLocked();
						undo_data.originalNumericValue = enumProperty->getValue();
						UndoSubdivisionOverrides.insert(enumProperty, undo_data);

						// DB 2023-August-12: bugfix lodlevel, set to correct value for Base Level
						int numKeys = enumProperty->getNumItems();
						int baseValue = enumProperty->findItemString("Base");
						int hdValue = enumProperty->findItemString("High Resolution");
						if (baseValue == -1)
						{
							baseValue = 0;
						}
						//// DEBUGGING: enumerate all keys
 						//QString lodString = enumProperty->getStringValue();
						//for (int i = 0; i < numKeys; i++)
						//{
						//	lodString = enumProperty->getItem(i);
						//	dzApp->log(QString("lodlevel[%1]=[%2]").arg(i).arg(lodString));
						//	printf("DEBUG: lodlevel[%d]=[%s]", i, lodString.toLocal8Bit().constData());
						//}
						if (targetValue == 0.0)
						{
							// use base mesh resolution
							enumProperty->setValue(baseValue);
						}

					}
					if (propName == "SubDIALevel" && numericProperty)
					{
						// DB 2021-09-02: Record data to Unlock/Undo changes
						UndoData undo_data;
						undo_data.originalNumericLockState = numericProperty->isLocked();
						undo_data.originalNumericValue = numericProperty->getDoubleValue();
						UndoSubdivisionOverrides.insert(numericProperty, undo_data);

						numericProperty->lock(false);
						if (subdivisionEnabled)
						{
							numericProperty->setDoubleValue(targetValue);
						}
						else
						{
							numericProperty->setDoubleValue(0.0f);
						}
						numericProperty->lock(true);
					}
					//QString propLabel = property->getLabel();
					//qDebug() << propName << " " << propLabel;
				}
			}
		}
	}
}

// DB 2021-09-02: Unlock/Undo Subdivision Property Changes
void DzBridgeSubdivisionDialog::UnlockSubdivisionProperties()
{
	QMap<DzProperty*, UndoData>::iterator undoIterator = UndoSubdivisionOverrides.begin();
	while (undoIterator != UndoSubdivisionOverrides.end())
	{
		DzProperty* undoKey = undoIterator.key();
		DzNumericProperty* numericProperty = qobject_cast<DzNumericProperty*>(undoKey);
		if (numericProperty)
		{
			UndoData undo_data = undoIterator.value();
			numericProperty->lock(false);
			numericProperty->setDoubleValue(undo_data.originalNumericValue);
			numericProperty->lock(undo_data.originalNumericLockState);
		}
		undoIterator++;
	}

	// Clear subdivision map after processing undo
	UndoSubdivisionOverrides.clear();
}

// DEPRECATED: use DzBridgeAction::writeAllSubdivisions(DzJsonWriter& writer)
void DzBridgeSubdivisionDialog::WriteSubdivisions(DzJsonWriter& Writer)
{
	DzNode* Selection = dzScene->getPrimarySelection();

	//stream << "Version, Object, Subdivision" << endl;
	foreach(QComboBox* combo, SubdivisionCombos)
	{
		QString Name = combo->property("Object").toString() + ".Shape";
		//DzNode* ObjectNode = FindObject(Selection, Name);

		int targetValue = combo->currentText().toInt();
		Writer.startObject(true);
		Writer.addMember("Version", 1);
		Writer.addMember("Asset Name", Name);
		Writer.addMember("Value", targetValue);
		Writer.finishObject();
		//stream << "1, " << Name << ", " << targetValue << endl;
	}
}

QObjectList DzBridgeSubdivisionDialog::getSubdivisionCombos()
{
	QObjectList *returnList = new QObjectList();
	foreach(QComboBox * combo, SubdivisionCombos)
	{
		returnList->append(qobject_cast<QWidget*>(combo));
	}
	return *returnList;
}

std::map<std::string, int>* DzBridgeSubdivisionDialog::GetLookupTable()
{
	std::map<std::string, int>* pLookupTable = new std::map<std::string, int>();

	foreach(QComboBox * combo, SubdivisionCombos)
	{
		std::string name(combo->property("Object").toString().toLocal8Bit().data());
		name = name + ".Shape";
		int targetValue = combo->currentText().toInt();
		(*pLookupTable)[name] = targetValue;

	}

	return pLookupTable;
}

#include "moc_DzBridgeSubdivisionDialog.cpp"
