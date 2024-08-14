#pragma once
#include <dzaction.h>
#include <dznode.h>
#include <dzgeometry.h>
#include <dzfigure.h>
#include <dzjsonwriter.h>
#include <QtCore/qfile.h>
#include <QtCore/qtextstream.h>
#include <QUuid.h>
#include <DzBridgeAction.h>

#include "dzbridge.h"

class DzBridgeAction_Scriptable : public DzBridgeNameSpace::DzBridgeAction {
	Q_OBJECT
public:
	DzBridgeAction_Scriptable();

	Q_INVOKABLE void resetToDefaults();
	QString readGuiRootFolder();

protected:
	void executeAction();
	Q_INVOKABLE void writeConfiguration();
	void setExportOptions(DzFileIOSettings& ExportOptions);

	virtual QString getDefaultMenuPath() const { return tr(""); }

};
