#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once

#include <afxtempl.h>


class CBasePane;
class CMFCPopupMenu;
class CMFCToolBar;
class CMFCToolBarButton;

class CMFCColorPopupMenu;
class CMFCColorBar;

class CMFCButton;

class CColorEntry;
class CColorTable;


namespace mfc
{
	typedef CArray<COLORREF, COLORREF> TColorArray;
	typedef CList<COLORREF, COLORREF> TColorList;
}


namespace ui
{
	bool IsUndefinedColor( COLORREF rawColor );		// defined in Color.h


	interface ICustomPopupMenu
	{
		virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup, int trackingMode ) = 0;
	};


	interface IColorHost
	{
		virtual COLORREF GetColor( void ) const = 0;
		virtual const CColorEntry* GetRawColor( void ) const = 0;
		virtual COLORREF GetAutoColor( void ) const = 0;

		virtual const CColorTable* GetSelColorTable( void ) const = 0;
		virtual const CColorTable* GetDocColorTable( void ) const = 0;
		virtual bool UseUserColors( void ) const = 0;

		// implemented
		COLORREF GetActualColor( void ) const
		{
			COLORREF actualColor = GetColor();
			return ui::IsUndefinedColor( actualColor ) ? GetAutoColor() : actualColor;		// the color current selection amounts to
		}
	};

	interface IColorEditorHost : public IColorHost
	{
		virtual CWnd* GetHostWindow( void ) const = 0;

		virtual void SetColor( COLORREF rawColor, bool notify ) = 0;
		virtual void SetSelColorTable( const CColorTable* pSelColorTable ) = 0;

		// implemented
		bool EditColorDialog( void );
		bool SwitchSelColorTable( const CColorTable* pSelColorTable );
	};
}


namespace mfc
{
	struct CColorLabels
	{
		static const TCHAR s_autoLabel[];
		static const TCHAR s_moreLabel[];
	};


	void BasePane_SetIsDialogControl( CBasePane* pBasePane, bool isDlgControl = true );		// getter IsDialogControl() is public


	// CMFCToolBar protected access:
	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar );
	CMFCToolBarButton* ToolBar_ButtonHitTest( const CMFCToolBar* pToolBar, const CPoint& clientPos, OUT int* pBtnIndex = nullptr );

	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette = nullptr );		// CMFCColorBar protected access


	// CMFCToolBarButton protected access:
	void* Button_GetItemData( const CMFCToolBarButton* pButton );
	void Button_SetItemData( CMFCToolBarButton* pButton, const void* pItemData );
	void* Button_GetItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );
	void Button_SetImageById( CMFCToolBarButton* pButton, UINT btnId, bool userImage = false );
	int FindImageIndex( UINT btnId, bool userImage = false );

	CRect Button_GetImageRect( const CMFCToolBarButton* pButton, bool bounds = true );
	void Button_RedrawImage( CMFCToolBarButton* pButton );

	// CMFCButton protected access:
	void MfcButton_SetCaptured( CMFCButton* pButton, bool captured );


	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu );
	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId );
	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu );
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

		COLORREF GetAutoColor( void ) const { return m_ColorAutomatic; }
		void SetAutoColor( COLORREF autoColor ) { m_ColorAutomatic = autoColor; }

		void SetInternal( bool bInternal = true ) { m_bInternal = bInternal; }		// for customization mode

		bool IsAutoBtn( const CMFCToolBarButton* pButton ) const { return HasAutoBtn() && pButton->m_strText == m_strAutoColor; }
		bool IsMoreBtn( const CMFCToolBarButton* pButton ) const { return HasMoreBtn() && pButton->m_strText == m_strOtherColor; }
		bool IsMoreColorSampleBtn( const CMFCToolBarButton* pButton ) const { return pButton->m_bImage && HasMoreBtn() && pButton == GetButton( GetCount() - 1 ); }
	};
}


#endif // PopupMenus_fwd_h
