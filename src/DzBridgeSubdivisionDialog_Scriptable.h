#pragma once

#include "DzBridgeSubdivisionDialog.h"
#include "dzbridge.h"

class CPP_Export DzBridgeSubdivisionDialog : public DzBridgeNameSpace::DzBridgeSubdivisionDialog {
	Q_OBJECT
public:
	DzBridgeSubdivisionDialog(QWidget* parent = nullptr) :
		DzBridgeNameSpace::DzBridgeSubdivisionDialog(parent) {};
};
