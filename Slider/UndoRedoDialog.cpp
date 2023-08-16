
#include "pch.h"
#include "UndoRedoDialog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDOK, MoveX },
		{ IDCANCEL, MoveX }
	};
}

CUndoRedoDialog::CUndoRedoDialog( auto_drop::COpStack& rFromStack, auto_drop::COpStack& rToStack, bool isUndoOp, CWnd* pParent /*= nullptr*/ )
	: CLayoutDialog( IDD_UNDO_REDO_DIALOG, pParent )
	, m_rFromStack( rFromStack )
	, m_rToStack( rToStack )
	, m_isUndoOp( isUndoOp )
{
	m_regSection = _T("UndoRedoDialog");
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );
}

CUndoRedoDialog::~CUndoRedoDialog()
{
}

void CUndoRedoDialog::DoDataExchange( CDataExchange* pDX )
{
	CLayoutDialog::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CUndoRedoDialog, CLayoutDialog )
END_MESSAGE_MAP()

BOOL CUndoRedoDialog::OnInitDialog( void )
{
	CLayoutDialog::OnInitDialog();
	return TRUE;
}
