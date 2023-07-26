#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once


class CMFCPopupMenu;
class CMFCToolBar;
class CMFCToolBarButton;

class CMFCColorPopupMenu;
class CMFCColorBar;

class CColorEntry;
class CColorTable;


namespace ui
{
	typedef CArray<COLORREF, COLORREF> TMFCColorArray;
	typedef CList<COLORREF, COLORREF> TMFCColorList;
}


namespace ui
{
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
	// CMFCColorBar protected access:
	//
	int ColorBar_InitColors( ui::TMFCColorArray& colors, CPalette* pPalette = nullptr );
	bool ColorBar_FindColorName( COLORREF realColor, OUT OPTIONAL std::tstring* pColorName = nullptr );
	inline bool ColorBar_ContainsColorName( COLORREF realColor ) { return ColorBar_FindColorName( realColor ); }
	void ColorBar_RegisterColorName( COLORREF realColor, const std::tstring& colorName );

	void* GetItemData( const CMFCToolBarButton* pButton );
	void* GetButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu );
	CMFCToolBarButton* FindToolBarButton( const CMFCToolBar* pToolBar, UINT btnId );
	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu );
}


#endif // PopupMenus_fwd_h
