#pragma once

#include "DzBridgeDialog.h"

#include "dzbridge.h"

class CPP_Export DzBridgeDialog : public DzBridgeNameSpace::DzBridgeDialog {
	Q_OBJECT
public:
	DzBridgeDialog(QWidget* parent = nullptr, const QString& windowTitle = "") :
		DzBridgeNameSpace::DzBridgeDialog(parent, windowTitle) {};

};
