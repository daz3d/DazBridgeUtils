#pragma once
#include <dzbasicdialog.h>
#include "dznode.h"

#include "dzbridge.h"
namespace DzBridgeNameSpace
{
	class DzBridgeAction;
	struct LodInfo;

    class CPP_Export DzBridgeLodSettingsDialog : public DzBasicDialog {
        Q_OBJECT
		Q_PROPERTY(QWidget* wLodMethodComboBox READ getLodMethodComboBox)
		Q_PROPERTY(QWidget* wNumberOfLodComboBox READ getNumberOfLodComboBox)
    public:
		Q_INVOKABLE QWidget* getLodMethodComboBox() { return (QWidget*) m_wLodMethodComboBox; }
		Q_INVOKABLE QWidget* getNumberOfLodComboBox() { return (QWidget*) m_wNumberOfLodComboBox; }

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
		virtual void HandleLodMethodComboChange(int state);
		virtual void HandleNumberOfLodComboChange(int state);

	protected:
		DzBridgeAction* m_BridgeAction = nullptr;
		QComboBox* m_wLodMethodComboBox;
		QComboBox* m_wNumberOfLodComboBox;
		QComboBox* m_wLodSettingPresetComboBox;
		
		int getSourceVertexCount();
		int getSourceVertexCount(DzNode* pNode);
		float calculateLodGenerationTime();

		virtual void generateLodLerp(LodInfo start, LodInfo end, int numberOfPoints);
		virtual void applyLodPresetDefault();
		virtual void applyLodPresetHighPerformance();

    private:
		static DzBridgeLodSettingsDialog* singleton;
		bool bWarningShown = false;

	};

}
