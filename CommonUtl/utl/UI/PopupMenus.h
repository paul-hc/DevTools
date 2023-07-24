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
		COLORREF GetRawColor( void ) const;

		void SetDocColorTable( const CColorTable* pDocColorTable );
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


#endif // PopupMenus_h
