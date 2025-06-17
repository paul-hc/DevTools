#ifndef PopupMenus_fwd_h
#define PopupMenus_fwd_h
#pragma once

#include "ControlBar_fwd.h"


class CMFCPopupMenu;
class CMFCPopupMenuBar;
class CMFCToolBarButton;

class CMFCColorPopupMenu;
class CMFCColorBar;

class CMFCButton;

class CColorEntry;
class CColorTable;


namespace ui
{
	bool IsUndefinedColor( COLORREF rawColor );		// defined in Color.h
	TDisplayColor EvalColor( COLORREF rawColor );	// defined in Color.h


	interface ICustomPopupMenu
	{
		virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup, int trackingMode ) = 0;
	};


	interface IColorHost
	{
		virtual COLORREF GetColor( void ) const = 0;
		virtual COLORREF GetAutoColor( void ) const = 0;

		virtual const CColorTable* GetSelColorTable( void ) const = 0;
		virtual const CColorTable* GetDocColorTable( void ) const = 0;
		virtual bool UseUserColors( void ) const = 0;

		// implemented
		COLORREF GetFallbackColor( COLORREF rawColor ) const
		{
			return ui::IsUndefinedColor( rawColor ) ? GetAutoColor() : rawColor;		// fallback to auto-color if color undefined (CLR_NONE)
		}

		COLORREF GetActualColor( void ) const
		{
			return GetFallbackColor( GetColor() );			// the color current selection amounts to
		}

		COLORREF GetDisplayColor( void ) const
		{
			return ui::EvalColor( GetActualColor() );		// evaluated actual color
		}


		bool IsForeignColor( void ) const;
		COLORREF GetForeignColor( void ) const { return IsForeignColor() ? GetColor() : CLR_NONE; }
	};


	interface IColorEditorHost : public IColorHost
	{
		virtual CWnd* GetHostWindow( void ) const = 0;

		virtual void SetColor( COLORREF rawColor, bool notify ) = 0;
		virtual void SetAutoColor( COLORREF autoColor, const TCHAR* pAutoLabel = mfc::CColorLabels::s_autoLabel ) = 0;
		virtual void SetSelColorTable( const CColorTable* pSelColorTable ) = 0;

		// implemented
		bool EditColorDialog( void );
		bool SwitchSelColorTable( const CColorTable* pSelColorTable );
	};
}


namespace mfc
{
	// CMFCButton protected access:
	void MfcButton_SetCaptured( CMFCButton* pButton, bool captured );

	// CMFCPopupMenu protected access:
	bool PopupMenu_InTrackMode( const CMFCPopupMenu* pPopupMenu );		// in modal tracking mode?
	void PopupMenu_SetTrackMode( CMFCPopupMenu* pPopupMenu, BOOL trackMode = true );

	void* PopupMenu_FindButtonItemData( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	// CMFCPopupMenuBar protected access:
	int PopupMenuBar_GetGutterWidth( CMFCPopupMenuBar* pPopupMenuBar );


	CMFCPopupMenu* GetSafePopupMenu( CMFCPopupMenu* pPopupMenu );
	CMFCToolBarButton* FindBarButton( const CMFCPopupMenu* pPopupMenu, UINT btnId );

	CMFCColorBar* GetColorMenuBar( const CMFCPopupMenu* pColorPopupMenu );
}


#endif // PopupMenus_fwd_h
