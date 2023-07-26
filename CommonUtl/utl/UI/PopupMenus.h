#ifndef PopupMenus_h
#define PopupMenus_h
#pragma once

#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>
#include <afxcolormenubutton.h>


class CColorEntry;
class CColorTable;
class CColorStore;


namespace mfc
{
	// Customized tracking menu to be used by mfc::CContextMenuMgr::TrackPopupMenu().
	//
	class CTrackingPopupMenu : public CMFCPopupMenu
	{
	public:
		CTrackingPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu = nullptr );
		virtual ~CTrackingPopupMenu();

		void SetTrackMode( BOOL trackMode ) { m_bTrackMode = trackMode; }
		void SetCustomPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu ) { m_pCustomPopupMenu = pCustomPopupMenu; }

		static void CloseActiveMenu( void );
		static CMFCToolBarButton* FindTrackingBarButton( UINT btnId );

		// base overrides
	protected:
		virtual BOOL InitMenuBar( void );
	private:
		ui::ICustomPopupMenu* m_pCustomPopupMenu;				// client code can customize the tracking menu content: replace buttons, etc
	};
}


namespace mfc
{
	// Office-like color picker button, to be used via ReplaceButton() in MFC toolbars and popup menus; tracks an embedded CMFCColorBar.
	//
	class CColorMenuButton : public CMFCColorMenuButton
	{
		DECLARE_SERIAL( CColorMenuButton )

		CColorMenuButton( void );			// private constructor for serialization
	public:
		CColorMenuButton( UINT uiCmdID, const CColorTable* pColorTable );
		virtual ~CColorMenuButton();

		const CColorTable* GetColorTable( void ) const { return m_pColorTable; }

		const CColorTable* GetDocColorTable( void ) const { return m_pDocColorTable; }
		void SetDocColorTable( const CColorTable* pDocColorTable );

		COLORREF GetRawColor( void ) const;
		void SetSelected( bool isTableSelected = true );

		enum NotifCode { CMBN_COLORSELECTED = CBN_SELCHANGE };		// note: notifications are suppressed during parent's UpdateData()
	protected:
		CWnd* GetMessageWnd( void ) const;

		const CColorEntry* FindClickedColorEntry( void ) const;
		size_t FindClickedColorButtonPos( void ) const;

		// base overrides
	public:
		virtual void SetImage( int iImage );
		virtual void SetColor( COLORREF color, BOOL notify = TRUE );
		virtual BOOL OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& colorRes );
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src );
		virtual CMFCPopupMenu* CreatePopupMenu( void );
	private:
		const CColorTable* m_pColorTable;
		const CColorTable* m_pDocColorTable;		// typically for the "Color Shades" table
	};
}


namespace mfc
{
	// Customized tracking popup color bar, that allows custom handling of color button tooltips
	//
	class CColorPopupMenu : public CMFCColorPopupMenu
	{
	public:
		// CColorMenuButton constructor (more general, pParentBtn could be null)
		CColorPopupMenu( CColorMenuButton* pParentMenuBtn,
						 const ui::TMFCColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 ui::TMFCColorList& docColors, int columns, int horzDockRows, int vertDockColumns,
						 COLORREF colorAuto, UINT uiCmdID, bool stdColorDlg = false );

		// color picker constructor
		CColorPopupMenu( CMFCColorButton* pParentBtn,
						 const ui::TMFCColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 ui::TMFCColorList& docColors, int columns, COLORREF colorAuto );

		virtual ~CColorPopupMenu();
	private:
		CColorMenuButton* m_pParentMenuBtn;

		// generated stuff
	protected:
		virtual int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
	};
}


#endif // PopupMenus_h
