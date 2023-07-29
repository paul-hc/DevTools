#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once

#include <afxtempl.h>


class CMFCPopupMenu;
class CMFCToolBar;
class CMFCToolBarButton;

class CMFCColorPopupMenu;
class CMFCColorBar;

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
		virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) = 0;
	};


	interface IColorHost
	{
		virtual COLORREF GetColor( void ) const = 0;
		virtual const CColorEntry* GetRawColor( void ) const = 0;
		virtual COLORREF GetAutoColor( void ) const = 0;

		virtual const CColorTable* GetSelColorTable( void ) const = 0;
		virtual const CColorTable* GetDocColorTable( void ) const = 0;
		virtual bool UseUserColors( void ) const = 0;

		COLORREF GetActualColor( void ) const
		{
			COLORREF actualColor = GetColor();
			return ui::IsUndefinedColor( actualColor ) ? GetAutoColor() : actualColor;		// the color current selection amounts to
		}
	};

	interface IColorEditorHost : public IColorHost
	{
		virtual void SetColor( COLORREF rawColor, bool notify ) = 0;

		virtual void SetSelColorTable( const CColorTable* pSelColorTable ) = 0;
		virtual void SetDocColorTable( const CColorTable* pDocColorTable ) = 0;
	};
}


namespace mfc
{
	// CMFCToolBar protected access:
	CToolTipCtrl* ToolBar_GetToolTip( const CMFCToolBar* pToolBar );


	// CMFCColorBar protected access:
	//
	int ColorBar_InitColors( mfc::TColorArray& colors, CPalette* pPalette = nullptr );
	bool ColorBar_FindColorName( COLORREF realColor, OUT OPTIONAL std::tstring* pColorName = nullptr );
	inline bool ColorBar_ContainsColorName( COLORREF realColor ) { return ColorBar_FindColorName( realColor ); }
	void ColorBar_RegisterColorName( COLORREF realColor, const std::tstring& colorName );


	void* GetButtonItemData( const CMFCToolBarButton* pButton );
	void SetButtonItemData( CMFCToolBarButton* pButton, const void* pItemData );

	void* GetButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );


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

		bool HasAutoBtn( void ) const { return !m_strAutoColor.IsEmpty(); }
		bool HasMoreBtn( void ) const { return !m_strOtherColor.IsEmpty(); }
		bool HasDocColorBtns( void ) const { return !m_strDocColors.IsEmpty(); }

		COLORREF GetAutoColor( void ) const { return m_ColorAutomatic; }
		void SetAutoColor( COLORREF autoColor ) { m_ColorAutomatic = autoColor; }

		void SetInternal( bool bInternal = true ) { m_bInternal = bInternal; }		// for customization mode
	};
}


#endif // PopupMenus_fwd_h
