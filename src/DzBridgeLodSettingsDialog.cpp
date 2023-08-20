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

	// Bake Subdivision Levels Explanation
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
	helpText_LodMethod->setText(
		"<p>" + 
		tr("<b>STEP 1</b>: Select the method for generating LODs.")
	);
	mainLayout->addWidget(helpText_LodMethod);

	// Create Drop-Down to choose LOD generation method
	m_wLodMethodComboBox = new QComboBox(this);
	m_wLodMethodComboBox->addItem("Use the currently active Resolution level", QVariant(0));
	//m_wLodMethodComboBox->addItem("Use pregenerated LOD mesh", QVariant(0));
	m_wLodMethodComboBox->addItem("Use Daz Decimator plugin", QVariant(1));
	m_wLodMethodComboBox->addItem("Use Unreal Engine Built-in LOD generator", QVariant(2));
	connect(m_wLodMethodComboBox, SIGNAL(activated(int)), this, SLOT(HandleLodMethodComboChange(int)));
	mainLayout->addWidget(m_wLodMethodComboBox);

	QLabel* spacerWidget1 = new QLabel();
	spacerWidget1->setTextFormat(Qt::RichText);
	spacerWidget1->setText("<p>");
	mainLayout->addWidget(spacerWidget1);

	QLabel* helpText_NumberOfLod = new QLabel();
	helpText_NumberOfLod->setTextFormat(Qt::RichText);
	helpText_NumberOfLod->setText(
		"<p>" +
		tr("<b>STEP 2</b>: Specify the number of detail levels to generate.") 
	);
	mainLayout->addWidget(helpText_NumberOfLod);

	// Choose Number of LODs to generate
	m_wNumberOfLodComboBox = new QComboBox(this);
	m_wNumberOfLodComboBox->addItem("1");
	m_wNumberOfLodComboBox->addItem("2");
	m_wNumberOfLodComboBox->addItem("3");
	m_wNumberOfLodComboBox->addItem("4");
	m_wNumberOfLodComboBox->addItem("5");
	m_wNumberOfLodComboBox->addItem("6");
	m_wNumberOfLodComboBox->addItem("7");
	m_wNumberOfLodComboBox->setCurrentIndex(0);
	connect(m_wNumberOfLodComboBox, SIGNAL(activated(int)), this, SLOT(HandleNumberOfLodComboChange(int)));
	mainLayout->addWidget(m_wNumberOfLodComboBox);

	QLabel* spacerWidget2 = new QLabel();
	spacerWidget2->setTextFormat(Qt::RichText);
	spacerWidget2->setText("<p>");
	mainLayout->addWidget(spacerWidget2);

	this->addLayout(mainLayout);

	setMinimumWidth(300);
	setMinimumHeight(250);

	PrepareDialog();
}

void DzBridgeLodSettingsDialog::PrepareDialog()
{
	if (m_BridgeAction)
	{
		m_wLodMethodComboBox->setCurrentIndex(m_BridgeAction->getLodMethodIndex());
		m_wNumberOfLodComboBox->setCurrentIndex(m_BridgeAction->getNumberOfLods()-1);
	}
	return;
}

void DzBridgeLodSettingsDialog::showEvent(QShowEvent* event)
{
	resize(100, 100);

	DzBasicDialog::showEvent(event);
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
		lodMethodIndex = comboIndex + 1;
		m_BridgeAction->setNumberOfLods(lodMethodIndex);
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
}

void DzBridgeLodSettingsDialog::HandleNumberOfLodComboChange(int state)
{
	float fEstimatedLodGenerationTime = calculateLodGenerationTime();

	if (fEstimatedLodGenerationTime > 5.0)
	{
		QString sWarningString = QString(tr("The estimated LOD generation time may be more than %1 minutes.  Times will vary depending on your CPU.")).arg((int) fEstimatedLodGenerationTime);
		// Warn User with Popup that estimated LOD generation will be more than 5 minutes
		QMessageBox::warning(0, "Daz Bridge",
			sWarningString, QMessageBox::Ok);
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
	int numLODs = m_wNumberOfLodComboBox->currentIndex();
	int numVerts = getSourceVertexCount();
	int lodMethod = m_wLodMethodComboBox->currentIndex();
	float fTimeScaleFactor = 0;
	
	// hardcoded estimate for Unreal Builtin LOD generator and Zen 3 AMD processor.
	if (lodMethod == 2)
	{
		fTimeScaleFactor = 1.0f / 500000.0f; // 1 minute for every 500,000 vertices
	}

	float fEstimatedTime = numVerts * numLODs * fTimeScaleFactor;

	return fEstimatedTime;
}

#include "moc_DzBridgeLodSettingsDialog.cpp"
