#pragma once

#include "DzBridgeMorphSelectionDialog.h"
#include "dzbridge.h"

class CPP_Export DzBridgeMorphSelectionDialog : public DzBridgeNameSpace::DzBridgeMorphSelectionDialog {
	Q_OBJECT
public:
	DzBridgeMorphSelectionDialog(QWidget* parent = nullptr) :
		DzBridgeNameSpace::DzBridgeMorphSelectionDialog(parent) {};

};
