
#include "pch.h"
#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CToolBarButton_ : public CMFCToolBarButton
	{
		// public access
		void* GetItemData( void ) const { return reinterpret_cast<void*>( m_dwdItemData ); }
		void SetItemData( const void* pItemData ) { m_dwdItemData = reinterpret_cast<DWORD_PTR>( pItemData ); }

		CRect GetImageBoundsRect( void ) const
		{
			CRect imageBoundsRect = m_rect;

			imageBoundsRect.left += CMFCVisualManager::GetInstance()->GetMenuImageMargin();
			imageBoundsRect.right = imageBoundsRect.left + CMFCToolBar::GetMenuImageSize().cx + CMFCVisualManager::GetInstance()->GetMenuImageMargin();
			return imageBoundsRect;
		}

		CRect GetImageRect( bool bounds = true ) const
		{
			CRect imageRect = m_rect;

			if ( bounds )
			{
				int margin = CMFCVisualManager::GetInstance()->GetMenuImageMargin();
				imageRect.DeflateRect( margin, margin );
			}
			return imageRect;
		}

		void RedrawImage( void )
		{
			if ( m_pWndParent != nullptr )
			{
				CRect imageRect = GetImageRect();

				m_pWndParent->InvalidateRect( &imageRect );
				m_pWndParent->UpdateWindow();
			}
		}
	};


	struct CToolBar_ : public CMFCToolBar
	{
		// public access
		CToolTipCtrl* GetToolTip( void ) const { return m_pToolTip; }
	};
}


namespace mfc
{
	const TCHAR CColorLabels::s_autoLabel[] = _T("Automatic");
	const TCHAR CColorLabels::s_moreLabel[] = _T("More Colors...");


	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar )
	{
		return mfc::nosy_cast<nosy::CToolBar_>( pToolBar )->GetToolTip();
	}

	CMFCToolBarButton* ToolBar_ButtonHitTest( const CMFCToolBar* pToolBar, const CPoint& clientPos, OUT int* pBtnIndex /*= nullptr*/ )
	{
		int btnIndex = const_cast<CMFCToolBar*>( pToolBar )->HitTest( clientPos );

		utl::AssignPtr( pBtnIndex, btnIndex );
		if ( -1 == btnIndex )
			return nullptr;

		return pToolBar->GetButton( btnIndex );
	}


	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette /*= nullptr*/ )
	{
		return nosy::CColorBar_::InitColors( pPalette, colors );
	}


	void* Button_GetItemData( const CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetItemData();
	}

	void Button_SetItemData( CMFCToolBarButton* pButton, const void* pItemData )
	{
		mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->SetItemData( pItemData );
	}

	void* Button_GetItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{
		CMFCToolBarButton* pFoundButton = FindBarButton( pPopupMenu, btnId );

		if ( nullptr == pFoundButton )
		{
			ASSERT( false );
			return 0;
		}

		return Button_GetItemData( pFoundButton );
	}

	CRect Button_GetImageRect( const CMFCToolBarButton* pButton, bool bounds /*= true*/ )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->GetImageRect( bounds );
	}

	void Button_RedrawImage( CMFCToolBarButton* pButton )
	{
		return mfc::nosy_cast<nosy::CToolBarButton_>( pButton )->RedrawImage();
	}


	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu )
	{
		if ( pPopupMenu != nullptr && ::IsWindow( pPopupMenu->m_hWnd ) && CWnd::FromHandlePermanent( pPopupMenu->m_hWnd ) != nullptr )
			return pPopupMenu;

		return nullptr;
	}

	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId )
	{
		ASSERT_PTR( pToolBar );
		int btnIndex = pToolBar->CommandToIndex( btnId );

		return btnIndex != -1 ? pToolBar->GetButton( btnIndex ) : nullptr;
	}

	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId )
	{	// buttons reside in the embedded bar
		return pPopupMenu != nullptr ? FindToolBarButton( const_cast<CMFCPopupMenu*>( pPopupMenu )->GetMenuBar(), btnId ) : nullptr;
	}


	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu )
	{
		ASSERT( is_a<CMFCColorPopupMenu>( pColorPopupMenu ) );
		return checked_static_cast<CMFCColorBar*>( const_cast<CMFCPopupMenu*>( pColorPopupMenu )->GetMenuBar() );
	}
}
