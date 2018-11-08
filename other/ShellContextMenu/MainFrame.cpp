
#include "stdafx.h"
#include "MainFrame.h"
#include "ChildFormView.h"
#include "utl/MenuUtilities.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC( CMainFrame, CFrameWnd )

CMainFrame::CMainFrame( void )
	: CFrameWnd()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::OnCreateClient( CREATESTRUCT* pCs, CCreateContext* pContext )
{
	CCreateContext ctx;
	ctx.m_pNewViewClass = RUNTIME_CLASS( CChildFormView );
	ctx.m_pCurrentFrame = this;

	return CreateView( &ctx, AFX_IDW_PANE_FIRST ) != NULL;
}


// message handlers

BEGIN_MESSAGE_MAP( CMainFrame, CFrameWnd )
END_MESSAGE_MAP()
