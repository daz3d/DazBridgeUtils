#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/qmessagebox.h>

#include "dzapp.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzobject.h"
#include "dzshape.h"
#include "dzgeometry.h"

#include "DzBridgeLodSettingsDialog.h"
#include "DzBridgeAction.h"
#include "DzBridgeDialog.h"
#include "DzBridgeSubdivisionDialog.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"

using namespace DZ_BRIDGE_NAMESPACE;

CPP_Export DzBridgeLodSettingsDialog* DzBridgeLodSettingsDialog::singleton = nullptr;

DzBridgeLodSettingsDialog::DzBridgeLodSettingsDialog(DzBridgeAction* action, QWidget* parent) :
	DzBasicDialog(parent, DAZ_BRIDGE_LIBRARY_NAME)
{
	m_BridgeAction = action;

	// Set the dialog title 
	setWindowTitle(tr("Configure LOD Settings"));

	QVBoxLayout* mainLayout = new QVBoxLayout();
	mainLayout->setMargin(5);

	// Level of Detail (LOD) Explanation
	QLabel* helpBox = new QLabel();
	helpBox->setTextFormat(Qt::RichText);
	helpBox->setWordWrap(true);
	helpBox->setText(
		tr("The following settings will help you reduce the mesh complexity of your exported Daz Studio content.") +
		"<p>" +
		tr("Reducing mesh complexity is an excellent way of optimizing both memory requirements and performance in games and \
interactive realtime 3D applications.") +
		"<p>"
	);
	mainLayout->addWidget(helpBox);

	QLabel* helpText_LodMethod = new QLabel();
	helpText_LodMethod->setTextFormat(Qt::RichText);
	helpText_LodMethod->setWordWrap(true);
	helpText_LodMethod->setText(
		"<p>" + 
//		tr("<b>STEP 1</b>: Select the method for generating LODs.")
		tr("Method for generating LODs:")
);
	mainLayout->addWidget(helpText_LodMethod);

	// Create Drop-Down to choose LOD generation method
	m_wLodMethodComboBox = new QComboBox(this);
	//m_wLodMethodComboBox->addItem("Use the currently active Resolution level", QVariant(0));
	//m_wLodMethodComboBox->addItem("Use pregenerated LOD mesh", QVariant(0));
	//m_wLodMethodComboBox->addItem("Use Daz Decimator plugin", QVariant(1));
	m_wLodMethodComboBox->addItem("Unreal Engine Built-in LOD generator", QVariant(2));
	connect(m_wLodMethodComboBox, SIGNAL(activated(int)), this, SLOT(HandleLodMethodComboChange(int)));
	mainLayout->addWidget(m_wLodMethodComboBox);

	QLabel* spacerWidget1 = new QLabel();
	spacerWidget1->setTextFormat(Qt::RichText);
	spacerWidget1->setText("<p>");
	mainLayout->addWidget(spacerWidget1);

	QLabel* helpText_NumberOfLod = new QLabel();
	helpText_NumberOfLod->setTextFormat(Qt::RichText);
	helpText_NumberOfLod->setWordWrap(true);
	helpText_NumberOfLod->setText(
		"<p>" +
		tr("<b>STEP 1</b>: Specify the number of detail levels to generate.  The base mesh resolution counts \
as the first level of detail.") 
	);
	mainLayout->addWidget(helpText_NumberOfLod);

	// Choose Number of LODs to generate
	m_wNumberOfLodComboBox = new QComboBox(this);
	//m_wNumberOfLodComboBox->addItem("1", QVariant(1));
	m_wNumberOfLodComboBox->addItem("2", QVariant(2));
	m_wNumberOfLodComboBox->addItem("3", QVariant(3));
	m_wNumberOfLodComboBox->addItem("4", QVariant(4));
	m_wNumberOfLodComboBox->addItem("5", QVariant(5));
	m_wNumberOfLodComboBox->addItem("6", QVariant(6));
	m_wNumberOfLodComboBox->addItem("7", QVariant(7));
	m_wNumberOfLodComboBox->setCurrentIndex(0);
	connect(m_wNumberOfLodComboBox, SIGNAL(activated(int)), this, SLOT(HandleNumberOfLodComboChange(int)));
	mainLayout->addWidget(m_wNumberOfLodComboBox);

	QLabel* spacerWidget2 = new QLabel();
	spacerWidget2->setTextFormat(Qt::RichText);
	spacerWidget2->setText("<p>");
	mainLayout->addWidget(spacerWidget2);

	QLabel* helpText_LodPreset = new QLabel();
	helpText_LodPreset->setTextFormat(Qt::RichText);
	helpText_LodPreset->setWordWrap(true);
	helpText_LodPreset->setText(
		"<p>" +
		tr("<b>STEP 2</b>: Specify the LOD preset.<br><br>\
The <b>Default</b> preset will use base resolution for closeups and gradually decrease the percent mesh resolution with distance.<br><br>\
The <b>Competitive Multiplayer</b> preset will use base resolution for closeups, then 5000 vertex resolution for full body and gradually decrease further with distance.")
);
	mainLayout->addWidget(helpText_LodPreset);

	// LOD settings preset dropdown
	m_wLodSettingPresetComboBox = new QComboBox(this);
	m_wLodSettingPresetComboBox->addItem("Default", "#default");
	m_wLodSettingPresetComboBox->addItem("Competitive Multiplayer", "#high_performance");
	mainLayout->addWidget(m_wLodSettingPresetComboBox);

	this->addLayout(mainLayout);

	setMinimumWidth(400);
	setMinimumHeight(400);
                                
	PrepareDialog();
}

void DzBridgeLodSettingsDialog::PrepareDialog()
{
	if (m_BridgeAction)
	{
		// Set ComboBox based on ItemData value, not ComboBox Index
		int comboIndex = m_wLodMethodComboBox->findData(m_BridgeAction->getLodMethodIndex());
		m_wLodMethodComboBox->setCurrentIndex(comboIndex);
		comboIndex = m_wNumberOfLodComboBox->findData(m_BridgeAction->getNumberOfLods());
		m_wNumberOfLodComboBox->setCurrentIndex(comboIndex);
	}
	return;
}

void DzBridgeLodSettingsDialog::showEvent(QShowEvent* event)
{
    resize(sizeHint());

	DzBasicDialog::showEvent(event);
	bWarningShown = false;
}

void DzBridgeLodSettingsDialog::accept()
{
	if (m_BridgeAction)
	{
		//m_BridgeAction->setEnableLodGeneration(true);
		int comboIndex = m_wLodMethodComboBox->currentIndex();
		int lodMethodIndex = m_wLodMethodComboBox->itemData(comboIndex).toInt();
		m_BridgeAction->setLodMethod(lodMethodIndex);

		comboIndex = m_wNumberOfLodComboBox->currentIndex();
		int numLODs = m_wNumberOfLodComboBox->itemData(comboIndex).toInt();
		m_BridgeAction->setNumberOfLods(numLODs);

		//applyLodPresetDefault();
		comboIndex = m_wLodSettingPresetComboBox->currentIndex();
		QString sLodPreset = m_wLodSettingPresetComboBox->itemData(comboIndex).toString();
		if (sLodPreset.toLower().contains("#high_performance"))
		{
			applyLodPresetHighPerformance();
		}
		else if (sLodPreset.toLower().contains("#default"))
		{
			applyLodPresetDefault();
		}
		//applyLodPresetHighPerformance();
	}

	float fEstimatedLodGenerationTime = calculateLodGenerationTime();

	if (fEstimatedLodGenerationTime > 5.0 && !bWarningShown)
	{
		QString sWarningString = QString(tr("The estimated LOD generation time may be more than %1 minutes.  Times will vary depending on your CPU.")).arg((int)fEstimatedLodGenerationTime);
		// Warn User with Popup that estimated LOD generation will be more than 5 minutes
		QMessageBox::warning(0, "Daz Bridge",
			sWarningString, QMessageBox::Ok);
		bWarningShown = true;
	}

	DzBasicDialog::accept();
}

void DzBridgeLodSettingsDialog::reject()
{
	DzBasicDialog::reject();
}

void DzBridgeLodSettingsDialog::HandleLodMethodComboChange(int state)
{
	float fEstimatedLodGenerationTime = calculateLodGenerationTime();

	if (fEstimatedLodGenerationTime > 5.0 && !bWarningShown)
	{
		QString sWarningString = QString(tr("The estimated LOD generation time may be more than %1 minutes.  Times will vary depending on your CPU.")).arg((int)fEstimatedLodGenerationTime);
		// Warn User with Popup that estimated LOD generation will be more than 5 minutes
		QMessageBox::warning(0, "Daz Bridge",
			sWarningString, QMessageBox::Ok);
		bWarningShown = true;
	}

}

void DzBridgeLodSettingsDialog::HandleNumberOfLodComboChange(int state)
{
	float fEstimatedLodGenerationTime = calculateLodGenerationTime();

	if (fEstimatedLodGenerationTime > 5.0 && !bWarningShown)
	{
		QString sWarningString = QString(tr("The estimated LOD generation time may be more than %1 minutes.  Times will vary depending on your CPU.")).arg((int) fEstimatedLodGenerationTime);
		// Warn User with Popup that estimated LOD generation will be more than 5 minutes
		QMessageBox::warning(0, "Daz Bridge",
			sWarningString, QMessageBox::Ok);
		bWarningShown = true;
	}

}

int DzBridgeLodSettingsDialog::getSourceVertexCount(DzNode* pNode)
{
	int numVerts = 0;

	DzObject* object = pNode->getObject();
	if (object)
	{
		//int numShapes = object->getNumShapes();
		//for (int i = 0; i < numShapes; i++)
		//{
		//	DzShape* shape = object->getShape(i);
		//	numVerts = shape->ge()->getNumVertices();
		//}
		DzShape* shape = object->getCurrentShape();
		numVerts = shape->getGeometry()->getNumVertices();

		// multiply by subd factor
		// 1. Check if Bake SubD enabled
		bool bBakeSubDEnabled = false;
		if (m_BridgeAction && m_BridgeAction->getBridgeDialog())
		{
			bBakeSubDEnabled = m_BridgeAction->getBridgeDialog()->getSubdivisionEnabledCheckBox()->isChecked();
		}
		if (bBakeSubDEnabled)
		{
			// 2. Obtain pointer to SubD window
			DzBridgeSubdivisionDialog* pSubDDialog = m_BridgeAction->getSubdivisionDialog();
			if (pSubDDialog)
			{
				QObjectList pComboBoxList = pSubDDialog->getSubdivisionCombos();
				foreach(QObject* pObject, pComboBoxList)
				{
					QComboBox *pComboBox = qobject_cast<QComboBox*>(pObject);
					// 3. Use pNode to lookup correct combobox pointer in SubD window
					QString sComboName = pComboBox->property("Object").toString();
					if (pNode->getName() == sComboName)
					{
						// 4. Retrieve SubD value from combobox pointer
						int nSubDLevel = pComboBox->currentText().toInt();
						// 5. Lookup correct scale factor from SubD value
						int nSubDMultiplier = 1;
						for (int i = 0; i < nSubDLevel; i++) nSubDMultiplier *= 4;
						// 6. Multiply
						numVerts *= nSubDMultiplier;
						break;
					}
				}
			}
		}
	}

	// call recursively and add up all vertex estimates
	int numChildren = pNode->getNumNodeChildren();
	for (int i = 0; i < numChildren; i++)
	{
		DzNode* pChildNode = pNode->getNodeChild(i);
		int childNumVerts = getSourceVertexCount(pChildNode);
		numVerts += childNumVerts;
	}

	return numVerts;
}

int DzBridgeLodSettingsDialog::getSourceVertexCount()
{
	int numVerts = -1;

	DzNode* pSelectedNode = m_BridgeAction->getSelectedNode();
	if (pSelectedNode == nullptr)
	{
		pSelectedNode = dzScene->getPrimarySelection();
	}

	// count all child nodes, find vertex count for the lodlevel or subd level to be exported
	if (pSelectedNode)
	{
		numVerts = getSourceVertexCount(pSelectedNode);
	}
	
	return numVerts;
}

float DzBridgeLodSettingsDialog::calculateLodGenerationTime()
{
	int numLODs = m_wNumberOfLodComboBox->itemData(m_wNumberOfLodComboBox->currentIndex()).toInt();
	int numVerts = getSourceVertexCount();
	int lodMethod = m_wLodMethodComboBox->itemData(m_wLodMethodComboBox->currentIndex()).toInt();
	float fTimeScaleFactor = 0;
	
	// hardcoded estimate for Unreal Builtin LOD generator and Zen 3 AMD processor.
	if (lodMethod == 2)
	{
		fTimeScaleFactor = 1.0f / 500000.0f; // 1 minute for every 500,000 vertices
	}

	float fEstimatedTime = numVerts * numLODs * fTimeScaleFactor;

	return fEstimatedTime;
}

void DzBridgeLodSettingsDialog::generateLodLerp(LodInfo start, LodInfo end, int numberOfPoints)
{
	for (int i = 0; i < numberOfPoints; i++)
	{
		// add new LOD info object
		struct LodInfo* newLodInfo = new LodInfo;
		// interpolate between start and finish values
		double interpolation;
		if (numberOfPoints > 1)
			interpolation = (double)i / (numberOfPoints-1);
		else
			interpolation = 1.0;
		if (end.quality_vertex != -1)
		{
			newLodInfo->quality_vertex = (start.quality_vertex * (1.0-interpolation)) + (end.quality_vertex * interpolation);
		}
		if (end.quality_percent != -1)
		{
			newLodInfo->quality_percent = (start.quality_percent * (1.0 - interpolation)) + (end.quality_percent * interpolation);
		}
		if (end.threshold_screen_height != -1)
		{
			newLodInfo->threshold_screen_height = (start.threshold_screen_height * (1.0 - interpolation)) + (end.threshold_screen_height * interpolation);
		}
		m_BridgeAction->m_aLodInfo.append(newLodInfo);
	}
}

void DzBridgeLodSettingsDialog::applyLodPresetHighPerformance()
{
	m_BridgeAction->m_aLodInfo.clear();
	int comboIndex = m_wNumberOfLodComboBox->currentIndex();
	int numLODs = m_wNumberOfLodComboBox->itemData(comboIndex).toInt();

	LodInfo lod0, lod1;
	lod0.quality_vertex = getSourceVertexCount();
	lod0.threshold_screen_height = 3.0f; // full-screen view of head is approximately screen height = 2.0-3.0
	lod1.quality_vertex = 5000;
	lod1.threshold_screen_height = 2.0f; // full-screen view of head is approximately screen height = 2.0-3.0
	LodInfo* newLodInfo = new LodInfo;
	*newLodInfo = lod0;
	m_BridgeAction->m_aLodInfo.append(newLodInfo);
	newLodInfo = new LodInfo;
	*newLodInfo = lod1;
	m_BridgeAction->m_aLodInfo.append(newLodInfo);

	// set first and last lod quality and screen height targets
	LodInfo start, end;
	start.quality_vertex = 4000;
	start.threshold_screen_height = 1.0f;
	end.quality_vertex = 500;
	end.threshold_screen_height = 0.05f; 

	// numLODs-2 because two already added to array above
	generateLodLerp(start, end, numLODs-2);
}

void DzBridgeLodSettingsDialog::applyLodPresetDefault()
{
	m_BridgeAction->m_aLodInfo.clear();
	int comboIndex = m_wNumberOfLodComboBox->currentIndex();
	int numLODs = m_wNumberOfLodComboBox->itemData(comboIndex).toInt();

	// set first and last lod quality and screen height targets
	LodInfo start, end;
	start.quality_percent = 1.0f; // 100% quality
	start.threshold_screen_height = 2.0f; // full-screen view of head is approximately screen height = 2.0-3.0
	end.quality_percent = 0.05f; // 10% quality
	end.threshold_screen_height = 0.10f; 
	
	generateLodLerp(start, end, numLODs);
}

#include "moc_DzBridgeLodSettingsDialog.cpp"
