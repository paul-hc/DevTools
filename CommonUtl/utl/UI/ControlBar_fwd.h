#ifndef ControlBar_fwd_h
#define ControlBar_fwd_h
#pragma once

#include "utl/Functional.h"
#include <afxtempl.h>
#include <afxtoolbar.h>


class CBasePane;
class CMFCToolBar;
class CMFCStatusBar;
class CMFCStatusBarPaneInfo;
class CMFCToolBarButton;
class CToolTipCtrl;
class CDockingManager;

namespace ui { struct CCmdAlias; }


namespace mfc
{
	typedef CArray<COLORREF, COLORREF> TColorArray;
	typedef CList<COLORREF, COLORREF> TColorList;


	struct CColorLabels
	{
		static const TCHAR s_autoLabel[];
		static const TCHAR s_moreLabel[];
	};


	template< typename CListT, typename ObjectT >
	bool CList_Remove( CListT& rObjectList, ObjectT* pTargetObject )
	{
		for ( POSITION pos = rObjectList.GetHeadPosition(); pos != nullptr; )
		{
			POSITION removePos = pos;
			ObjectT* pObject = checked_static_cast<ObjectT*>( rObjectList.GetNext( pos ) );

			if ( pObject == pTargetObject )
			{
				rObjectList.RemoveAt( removePos );
				return true;
			}
		}

		return false;
	}
}


namespace mfc
{
	// CCommandManager utils:
	int FindButtonImageIndex( UINT btnId, bool userImage = false );

	bool RegisterCmdImageAlias( UINT aliasCmdId, UINT imageCmdId );
	void RegisterCmdImageAliases( const ui::CCmdAlias cmdAliases[], size_t count );


	class CScopedCmdImageAliases
	{
	public:
		CScopedCmdImageAliases( UINT aliasCmdId, UINT imageCmdId );
		CScopedCmdImageAliases( const ui::CCmdAlias cmdAliases[], size_t count );
		~CScopedCmdImageAliases();
	private:
		typedef std::pair<UINT, int> TCmdImagePair;		// <cmdId, imageIndex>

		std::vector<TCmdImagePair> m_oldCmdImages;
	};


	bool AssignTooltipText( OUT TOOLINFO* pToolInfo, const std::tstring& text );

}


namespace mfc
{
	// CMFCToolBarImages protected access:
	bool ToolBarImages_DrawStretch( CMFCToolBarImages* pImages, CDC* pDC, const CRect& destRect, int imageIndex,
									bool hilite = false, bool disabled = false, bool indeterminate = false, bool shadow = false, bool inactive = false,
									BYTE alphaSrc = 255 );


	// CBasePane protected access:
	void BasePane_SetIsDialogControl( OUT CBasePane* pBasePane, bool isDlgControl = true );		// getter IsDialogControl() is public


	// CMFCToolBar access:
	const CMap<UINT, UINT, int, int>& ToolBar_GetDefaultImages( void );
	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar );
	CMFCToolBarButton* ToolBar_FindButton( const CMFCToolBar* pToolBar, UINT btnId );
	inline int ToolBar_ReplaceButton( OUT CMFCToolBar* pToolBar, const CMFCToolBarButton& srcButton ) { return safe_ptr( pToolBar )->ReplaceButton( srcButton.m_nID, srcButton ); }
	void ToolBar_SetBtnText( OUT CMFCToolBar* pToolBar, UINT btnId, const std::tstring& text, bool showText = true, bool showImage = true );
	bool ToolBar_RestoreOriginalState( OUT CMFCToolBar* pToolBar );
	CMFCToolBarButton* ToolBar_ButtonHitTest( const CMFCToolBar* pToolBar, const CPoint& clientPos, OUT int* pBtnIndex = nullptr );

	template< typename ButtonT >
	inline ButtonT* ToolBar_LookupButton( const CMFCToolBar& toolBar, UINT btnId ) { return safe_ptr( checked_static_cast<ButtonT*>( ToolBar_FindButton( &toolBar, btnId ) ) ); }


	// CMFCStatusBar protected access:
	CMFCStatusBarPaneInfo* StatusBar_GetPaneInfo( const CMFCStatusBar* pStatusBar, int index );
	int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, const CMFCStatusBarPaneInfo* pPaneInfo );
	inline int StatusBar_CalcPaneTextWidth( const CMFCStatusBar* pStatusBar, int index ) { return StatusBar_CalcPaneTextWidth( pStatusBar, StatusBar_GetPaneInfo( pStatusBar, index ) ); }
	int StatusBar_ResizePaneToFitText( OUT CMFCStatusBar* pStatusBar, int index );


	// CMFCToolBarButton protected access:
	bool ToolBarButton_SetStyleFlag( CMFCToolBarButton* pButton, UINT styleFlag, bool on = true );
	void* ToolBarButton_GetItemData( const CMFCToolBarButton* pButton );
	void ToolBarButton_SetItemData( CMFCToolBarButton* pButton, const void* pItemData );
	void ToolBarButton_SetImageById( CMFCToolBarButton* pButton, UINT btnId, bool userImage = false );

	template< typename ObjectT >
	ObjectT* ToolBarButton_GetItemPtr( const CMFCToolBarButton* pButton ) { return reinterpret_cast<ObjectT*>( ToolBarButton_GetItemData( pButton ) ); }

	CRect ToolBarButton_GetImageRect( const CMFCToolBarButton* pButton, bool bounds = true );

	// CMFCToolBarButton utils:
	inline HWND ToolBarButton_GetHwnd( const CMFCToolBarButton* pButton ) { return const_cast<CMFCToolBarButton*>( pButton )->GetHwnd(); }
	bool ToolBarButton_EditSelectAll( CMFCToolBarButton* pButton );
	void ToolBarButton_Redraw( CMFCToolBarButton* pButton );
	void ToolBarButton_RedrawImage( CMFCToolBarButton* pButton );


	// CMFCColorBar protected access:
	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette = nullptr );		// protected access
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
	FuncT ForEach_MatchingButton( UINT btnId, FuncT func, const ButtonT* pExceptBtn = nullptr )
	{
		CObList buttonList;
		CMFCToolBar::GetCommandButtons( btnId, buttonList );

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != nullptr; )
		{
			ButtonT* pButton = checked_static_cast<ButtonT*>( buttonList.GetNext( pos ) );

			if ( pExceptBtn != nullptr ? ( pButton != pExceptBtn ) : !CMFCToolBar::IsLastCommandFromButton( pButton ) )		// exclude pExceptBtn or the one handling the message
				func( pButton );
		}

		return func;
	}

	template< typename ButtonT, typename UnaryPred >
	ButtonT* FindMatchingButtonThat( UINT btnId, UnaryPred pred )		// find first ID-matching button satisfying the predicate
	{
		CObList buttonList;
		CMFCToolBar::GetCommandButtons( btnId, buttonList );

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != nullptr; )
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

		for ( POSITION pos = buttonList.GetHeadPosition(); pos != nullptr; )
			rButtons.push_back( checked_static_cast<ButtonT*>( buttonList.GetNext( pos ) ) );
	}


	inline void RedrawMatchingButtons( UINT btnId ) { ForEach_MatchingButton<CMFCToolBarButton>( btnId, &mfc::ToolBarButton_Redraw ); }


	template< typename ButtonT >
	inline ButtonT* FindFirstMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, pred::True() ); }

	template< typename ButtonT >
	inline ButtonT* FindNotifyingMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, &CMFCToolBar::IsLastCommandFromButton ); }

	template< typename ButtonT >
	inline ButtonT* FindFocusedMatchingButton( UINT btnId ) { return FindMatchingButtonThat<ButtonT>( btnId, std::mem_fun( &CMFCToolBarButton::HasFocus ) ); }
}


namespace mfc
{
	// for toolbars hosted in child frames, which are not customizable, persistent, and its buttons don't sync with copies with same ID

	class CFixedToolBar : public CMFCToolBar
	{
	public:
		CFixedToolBar( void );
		virtual ~CFixedToolBar();

		bool IsBarVisible( void ) const { return IsVisible() != FALSE; }
		void ShowBar( bool show = true ) { ShowPane( show, false, false ); }

		// base overrides
		virtual BOOL CanBeRestored( void ) const { return false; }
		virtual BOOL AllowShowOnList( void ) const { return false; }		// don't list it on Customize > Toolbars
		virtual BOOL OnShowControlBarMenu( CPoint point ) overrides(CMFCBaseToolBar) { point; return TRUE; }	// prevent displaying the toolbar context menu
		virtual BOOL OnUserToolTip( CMFCToolBarButton* pButton, CString& rTipText ) const overrides(CMFCToolBar);

		using CMFCToolBar::InsertButton;		// both the protected and public
	public:
		enum { Style = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_HIDE_INPLACE };

		// generated stuff
	protected:
		afx_msg int OnCreate( CREATESTRUCT* pCS );		// remove this from CMFCToolBar::GetAllToolbars() to disable button value syncing

		DECLARE_MESSAGE_MAP()
	};
}


#include <afxcolorbar.h>


namespace nosy
{
	struct CColorBar_ : public CMFCColorBar
	{
		// public access
		using CMFCColorBar::m_colors;
		using CMFCColorBar::m_lstDocColors;
		using CMFCColorBar::m_ColorNames;		// CMap<COLORREF,COLORREF,CString, LPCTSTR>

		using CMFCColorBar::InitColors;
		using CMFCColorBar::InvokeMenuCommand;

		bool HasAutoBtn( void ) const { return !m_strAutoColor.IsEmpty(); }
		bool HasMoreBtn( void ) const { return !m_strOtherColor.IsEmpty(); }
		bool HasDocColorBtns( void ) const { return !m_strDocColors.IsEmpty(); }

		void SetInternal( bool bInternal = true ) { m_bInternal = bInternal; }		// for customization mode

		bool IsAutoBtn( const CMFCToolBarButton* pButton ) const { return HasAutoBtn() && pButton->m_strText == m_strAutoColor; }
		bool IsMoreBtn( const CMFCToolBarButton* pButton ) const { return HasMoreBtn() && pButton->m_strText == m_strOtherColor; }
		bool IsMoreColorSampleBtn( const CMFCToolBarButton* pButton ) const { return pButton->m_bImage && HasMoreBtn() && pButton == GetButton( GetCount() - 1 ); }

		COLORREF GetAutoColor( void ) const { return m_ColorAutomatic; }
		//void SetAutoColor( COLORREF autoColor ) { m_ColorAutomatic = autoColor; }		// not reliable after Rebuild(), since it doesn't update the Auto button
	};
}


#endif // ControlBar_fwd_h
