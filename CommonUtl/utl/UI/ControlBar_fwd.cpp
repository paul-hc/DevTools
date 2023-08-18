
#include "pch.h"
#include "ControlBar_fwd.h"
#include <afxbasepane.h>
#include <afxstatusbar.h>

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


	struct CMFCStatusBar_ : public CMFCStatusBar
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


	// CMFCStatusBar access

	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index )
	{
		return mfc::nosy_cast<nosy::CMFCStatusBar_>( pStatusBar )->_GetPanePtr( index );
	}

	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo )
	{
		ASSERT( pStatusBar != nullptr && pPaneInfo != nullptr );

		CClientDC screenDC( nullptr );
		HGDIOBJ hOldFont = screenDC.SelectObject( mfc::nosy_cast<nosy::CMFCStatusBar_>( pStatusBar )->GetCurrentFont() );

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
}
