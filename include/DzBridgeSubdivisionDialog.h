#pragma once
#include "dzbasicdialog.h"
#include <QtGui/qlineedit.h>
#include <QtGui/qcombobox.h>
#include <QtCore/qsettings.h>
#include "dznode.h"
#include <dzjsonwriter.h>

class QListWidget;
class QListWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;
class QGridLayout;

#include "dzbridge.h"
namespace DzBridgeNameSpace
{
	class CPP_Export DzBridgeSubdivisionDialog : public DzBasicDialog {
		Q_OBJECT
		Q_PROPERTY(QObjectList aSubdivisionCombos READ getSubdivisionCombos)
	public:
		Q_INVOKABLE QObjectList getSubdivisionCombos();

		/** Constructor **/
		DzBridgeSubdivisionDialog(QWidget* parent = nullptr);

		Q_INVOKABLE void PrepareDialog();

		/** Destructor **/
		virtual ~DzBridgeSubdivisionDialog() {}

		Q_INVOKABLE static DzBridgeSubdivisionDialog* Get(QWidget* Parent)
		{
			if (singleton == nullptr)
			{
				singleton = new DzBridgeSubdivisionDialog(Parent);
			}
			else
			{
				singleton->PrepareDialog();
			}
			return singleton;
		}

		QGridLayout* subdivisionItemsGrid;

		Q_INVOKABLE void LockSubdivisionProperties(bool subdivisionEnabled);
		Q_INVOKABLE void WriteSubdivisions(DzJsonWriter& Writer);
		Q_INVOKABLE DzNode* FindObject(DzNode* Node, QString Name);

		Q_INVOKABLE bool setSubdivisionLevelByNode(DzNode* Node, int level);

		Q_INVOKABLE void UnlockSubdivisionProperties();
		std::map<std::string, int>* GetLookupTable();

	public slots:
		void HandleSubdivisionLevelChanged(const QString& text);

	private:
		void CreateList(DzNode* Node);

		void SavePresetFile(QString filePath);

		QSize minimumSizeHint() const override;

		QString presetsFolder;

		QList<QComboBox*> SubdivisionCombos;

		QMap<QString, int> SubdivisionLevels;

		static DzBridgeSubdivisionDialog* singleton;

		struct UndoData
		{
			QString sNodeName; // name of DzNode
			QString sPropertyName;
			bool originalNumericLockState;
			double originalNumericValue;
		};
		QMap<DzProperty*, UndoData> UndoSubdivisionOverrides;

	};

}