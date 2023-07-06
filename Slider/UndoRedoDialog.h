#ifndef UndoRedoDialog_h
#define UndoRedoDialog_h
#pragma once

#include "AutoDrop.h"
#include "utl/UI/LayoutDialog.h"


class CUndoRedoDialog : public CLayoutDialog
{
public:
	CUndoRedoDialog( auto_drop::COpStack& rFromStack, auto_drop::COpStack& rToStack, bool isUndoOp, CWnd* pParent = nullptr );
	virtual ~CUndoRedoDialog();
private:
	auto_drop::COpStack& m_rFromStack;
	auto_drop::COpStack& m_rToStack;
	bool m_isUndoOp;
public:
	// enum { IDD = IDD_UNDO_REDO_DIALOG };

	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};


#endif // UndoRedoDialog_h
