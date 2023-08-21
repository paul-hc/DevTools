
#include "pch.h"
#include "ControlBar_fwd.h"
#include "utl/Algorithms_fwd.h"
#include <afxbasepane.h>
#include <afxtoolbar.h>
#include <afxstatusbar.h>
#include <afxdockingmanager.h>
#include <afxdropdowntoolbar.h>		// for is_a()

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CBasePane_ : public CBasePane
	{
		// public access
		void SetIsDialogControl( bool isDlgControl = true ) { m_bIsDlgControl = isDlgControl; }
	};


	struct CToolBar_ : public CMFCToolBar
	{
		// public access
		using CMFCToolBar::m_bMasked;
		using CMFCToolBar::AllowShowOnList;

		CToolTipCtrl* GetToolTip( void ) const { return m_pToolTip; }
	};


	struct CStatusBar_ : public CMFCStatusBar
	{
		// public access
		using CMFCStatusBar::_GetPanePtr;
		using CMFCStatusBar::GetCurrentFont;
	};
}


namespace mfc
{
	// CBasePane access

	void BasePane_SetIsDialogControl( CBasePane* pBasePane, bool isDlgControl /*= true*/ )
	{
		mfc::nosy_cast<nosy::CBasePane_>( pBasePane )->SetIsDialogControl( isDlgControl );
	}


	// CMFCToolBar access

	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar )
	{
		return mfc::nosy_cast<nosy::CToolBar_>( pToolBar )->GetToolTip();
	}

	CMFCToolBarButton* ToolBar_FindButton( const CMFCToolBar* pToolBar, UINT btnId )
	{
		ASSERT_PTR( pToolBar );
		int btnIndex = pToolBar->CommandToIndex( btnId );

		return btnIndex != -1 ? pToolBar->GetButton( btnIndex ) : nullptr;
	}

	bool ToolBar_RestoreOriginalState( CMFCToolBar* pToolBar )
	{
	#if _MFC_VER > 0x0900		// newer MFC version?
		return pToolBar->RestoreOriginalState() != FALSE;
	#else
		return pToolBar->RestoreOriginalstate() != FALSE;		// workaround typo in older MFC
	#endif
	}


	// CMFCStatusBar access

	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index )
	{
		return mfc::nosy_cast<nosy::CStatusBar_>( pStatusBar )->_GetPanePtr( index );
	}

	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo )
	{
		ASSERT( pStatusBar != nullptr && pPaneInfo != nullptr );

		CClientDC screenDC( nullptr );
		HGDIOBJ hOldFont = screenDC.SelectObject( mfc::nosy_cast<nosy::CStatusBar_>( pStatusBar )->GetCurrentFont() );

		int textWidth = screenDC.GetTextExtent( pPaneInfo->lpszText, str::GetLength( pPaneInfo->lpszText ) ).cx;

		screenDC.SelectObject( hOldFont );
		return textWidth;
	}

	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index )
	{
		CMFCStatusBarPaneInfo* pPaneInfo = StatusBar_GetPaneInfo( pStatusBar, index );

		pPaneInfo->cxText = mfc::StatusBar_CalcPaneTextWidth( pStatusBar, pPaneInfo );
		pStatusBar->SetPaneWidth( index, pPaneInfo->cxText );

		return pPaneInfo->cxText;
	}


	// CMFCToolBarButton access:

	void ToolBarButton_Redraw( CMFCToolBarButton* pButton )
	{
		ASSERT_PTR( pButton );

		if ( pButton->GetParentWnd()->GetSafeHwnd() != nullptr )
		{
			CRect rect = pButton->GetInvalidateRect();

			rect.InflateRect( 2, 2 );
			pButton->GetParentWnd()->RedrawWindow( &rect, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME );
		}

		if ( HWND hCtrl = pButton->GetHwnd() )
			::RedrawWindow( hCtrl, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME );
	}
}


namespace mfc
{
	void QueryAllCustomizableControlBars( OUT std::vector<CMFCToolBar*>& rToolbars, CWnd* pFrameWnd /*= AfxGetMainWnd()*/ )
	{
		const CObList& allToolbars = CMFCToolBar::GetAllToolbars();

		for ( POSITION pos = allToolbars.GetHeadPosition(); pos != nullptr; )
		{
			nosy::CToolBar_* pToolBar = mfc::nosy_cast<nosy::CToolBar_>( (CMFCToolBar*)allToolbars.GetNext( pos ) );
			ASSERT_PTR( pToolBar );

			if ( CWnd::FromHandlePermanent( pToolBar->m_hWnd ) != nullptr )
				if ( !is_a<CMFCDropDownToolBar>( pToolBar ) )										// skip dropdown toolbars!
					if ( ( nullptr == pFrameWnd || pFrameWnd == pToolBar->GetTopLevelFrame() ) &&	// check if toolbar belongs to the frame window filter?
						 pToolBar->AllowShowOnList() && !pToolBar->m_bMasked )
					{
						rToolbars.push_back( pToolBar );
					}
		}
	}

	void ResetAllControlBars( CWnd* pFrameWnd /*= AfxGetMainWnd()*/ )
	{
		std::vector<CMFCToolBar*> toolbars;

		mfc::QueryAllCustomizableControlBars( toolbars, pFrameWnd );

		for ( std::vector<CMFCToolBar*>::const_iterator itToolbar = toolbars.begin(); itToolbar != toolbars.end(); ++itToolbar )
			if ( (*itToolbar)->CanBeRestored() )
				mfc::ToolBar_RestoreOriginalState( *itToolbar );

		// TODO: check whether we need to re-assign the command aliases after this!
	}


	// FrameWnd utils:

	void DockPanesOnRow( CDockingManager* pFrameDocManager, size_t barCount, CMFCToolBar* pFirstToolBar, ... )
	{
		REQUIRE( barCount != 0 );
		ASSERT_PTR( pFirstToolBar->GetSafeHwnd() );

		std::vector<CMFCToolBar*> toolbars;
		toolbars.push_back( pFirstToolBar );

		{	// push subsequent toolbars
			va_list argList;

			va_start( argList, pFirstToolBar );
			utl::QueryArgumentList( toolbars, argList, utl::npos == barCount ? barCount : ( barCount - 1 ) );		// pFirstToolBar already pushed
			va_end( argList );
		}

		// dock toolbars in reverse order: DockPane() last, and DockPaneLeftOf() subsequently
		size_t i = toolbars.size();
		CMFCToolBar* pRightToolbar = toolbars[ --i ];

		pFrameDocManager->DockPane( pRightToolbar );		// dock last toolbar on a new row

		for ( ; i-- != 0; pRightToolbar = toolbars[i] )
			pFrameDocManager->DockPaneLeftOf( toolbars[i] /*LEFT*/, pRightToolbar );
	}
}
