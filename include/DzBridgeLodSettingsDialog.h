#pragma once
#include <dzbasicdialog.h>
#include "dznode.h"

#include "dzbridge.h"
namespace DzBridgeNameSpace
{
	class DzBridgeAction;

    class CPP_Export DzBridgeLodSettingsDialog : public DzBasicDialog {
        Q_OBJECT
    public:

		/** Constructor **/
		DzBridgeLodSettingsDialog(DzBridgeAction* action, QWidget* parent = nullptr);

		/** Destructor **/
		virtual ~DzBridgeLodSettingsDialog() {}

		Q_INVOKABLE void PrepareDialog();

		Q_INVOKABLE static DzBridgeLodSettingsDialog* Get(DzBridgeAction* action, QWidget* Parent)
		{
			if (singleton == nullptr)
			{
				singleton = new DzBridgeLodSettingsDialog(action, Parent);
			}
			else
			{
				singleton->PrepareDialog();
			}
			return singleton;
		}

		void showEvent(QShowEvent *event) override;
		void accept() override;
		void reject() override;

    public slots:

	protected:
		DzBridgeAction* m_BridgeAction = nullptr;
		QComboBox* lodMethodComboBox;
		QComboBox* numberOfLodComboBox;

    private:
		static DzBridgeLodSettingsDialog* singleton;

	};

}
