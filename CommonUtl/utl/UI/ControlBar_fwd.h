#ifndef ControlBar_fwd_h
#define ControlBar_fwd_h
#pragma once

#include "utl/Functional.h"
#include <afxtoolbar.h>


class CBasePane;
class CMFCToolBar;
class CMFCStatusBar;
class CMFCStatusBarPaneInfo;
class CMFCToolBarButton;
class CToolTipCtrl;
class CDockingManager;


namespace mfc
{
	// CBasePane protected access:
	void BasePane_SetIsDialogControl( OUT CBasePane* pBasePane, bool isDlgControl = true );		// getter IsDialogControl() is public


	// CMFCToolBar access:
	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar );
	CMFCToolBarButton* ToolBar_FindButton( const CMFCToolBar* pToolBar, UINT btnId );
	bool ToolBar_RestoreOriginalState( OUT CMFCToolBar* pToolBar );

	template< typename ButtonT >
	inline int ToolBar_ReplaceButton( OUT CMFCToolBar* pToolBar, const ButtonT& srcButton ) { return safe_ptr( pToolBar )->ReplaceButton( srcButton.m_nID, srcButton ); }


	// CMFCStatusBar protected access:
	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index );
	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo );
	inline int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, int index ) { return StatusBar_CalcPaneTextWidth( pStatusBar, StatusBar_GetPaneInfo( pStatusBar, index ) ); }
	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index );


	// CMFCToolBarButton access:
	inline HWND ToolBarButton_GetHwnd( const CMFCToolBarButton* pButton ) { return const_cast<CMFCToolBarButton*>( pButton )->GetHwnd(); }
	bool ToolBarButton_EditSelectAll( CMFCToolBarButton* pButton );
	void ToolBarButton_Redraw( CMFCToolBarButton* pButton );
}


namespace mfc
{
	// FrameWnd:

	enum FrameToolbarStyle
	{
		FirstToolbarStyle = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
		AdditionalToolbarStyle = FirstToolbarStyle | CBRS_HIDE_INPLACE | CBRS_BORDER_3D
	};


	void DockPanesOnRow( CDockingManager* pFrameDocManager, size_t barCount, CMFCToolBar* pFirstToolBar, ... );
}


#include <afxtoolbar.h>
#include <afxtoolbarbutton.h>


namespace mfc
{
	// global control bars:

	void QueryAllCustomizableControlBars( OUT std::vector<CMFCToolBar*>& rToolbars, CWnd* pFrameWnd = AfxGetMainWnd() );
	void ResetAllControlBars( CWnd* pFrameWnd = AfxGetMainWnd() );


	// CMFCToolBar algorithms:

	template< typename ButtonT, typename FuncT >
	FuncT ForEachMatchingButton( UINT btnId, FuncT func )
	{
		CObList buttonList;
		CMFCToolBar::GetCommandButtons( btnId, buttonList );

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != NULL; )
		{
			ButtonT* pButton = checked_static_cast<ButtonT*>( buttonList.GetNext( pos ) );

			if ( !CMFCToolBar::IsLastCommandFromButton( pButton ) )		// exclude the button handling the command
				func( pButton );
		}

		return func;
	}

	template< typename ButtonT, typename UnaryPred >
	ButtonT* FindMatchingButtonThat( UINT btnId, UnaryPred pred )		// find first ID-matching button satisfying the predicate
	{
		CObList buttonList;
		CMFCToolBar::GetCommandButtons( btnId, buttonList );

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != NULL; )
		{
			ButtonT* pButton = checked_static_cast<ButtonT*>( buttonList.GetNext( pos ) );
			if ( pred( pButton ) )
				return pButton;
		}

		return nullptr;
	}

	template< typename ButtonT >
	void QueryMatchingButtons( OUT std::vector<ButtonT*>& rButtons, UINT btnId )
	{	// to sync other buttons on all toolbars that share the same btnId:
		CObList buttonList;

		CMFCToolBar::GetCommandButtons( btnId, buttonList );
		rButtons.reserve( buttonList.GetSize() );

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != NULL; )
			rButtons.push_back( checked_static_cast<ButtonT*>( buttonList.GetNext( pos ) ) );
	}


	inline void RedrawMatchingButtons( UINT btnId ) { ForEachMatchingButton<CMFCToolBarButton>( btnId, &mfc::ToolBarButton_Redraw ); }


	template< typename ButtonT >
	inline ButtonT* FindFirstMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, pred::True() ); }

	template< typename ButtonT >
	inline ButtonT* FindNotifyingMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, &CMFCToolBar::IsLastCommandFromButton ); }

	template< typename ButtonT >
	inline ButtonT* FindFocusedMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, std::mem_fun( &CMFCToolBarButton::HasFocus ) ); }
}


#endif // ControlBar_fwd_h
