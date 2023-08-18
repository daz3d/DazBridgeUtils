#include <QtGui/QLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>

#include "dzapp.h"

#include "DzBridgeLodSettingsDialog.h"
#include "DzBridgeAction.h"

/*****************************
Local definitions
*****************************/
#define DAZ_BRIDGE_LIBRARY_NAME "Daz Bridge"

using namespace DzBridgeNameSpace;

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

#include "moc_DzBridgeLodSettingsDialog.cpp"
