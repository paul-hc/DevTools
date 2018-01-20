
#include "stdafx.h"
#include "ChildFrm.h"
#include "ExplorerBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( CChildFrame, CMDIChildWnd )

CChildFrame::CChildFrame( void )
{
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if ( g_theApp.m_maximizeFirst )
		cs.style |= ( WS_VISIBLE | WS_MAXIMIZE );		// show maximized

	return CMDIChildWnd::PreCreateWindow( cs );
}


// message handlers

BEGIN_MESSAGE_MAP( CChildFrame, CMDIChildWnd )
END_MESSAGE_MAP()
