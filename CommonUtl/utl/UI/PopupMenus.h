#ifndef PopupMenus_h
#define PopupMenus_h
#pragma once

#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>
#include <afxcolormenubutton.h>


class CColorTable;
class CColorStore;


namespace mfc
{
	class CTrackingPopupMenu : public CMFCPopupMenu
	{
	public:
		CTrackingPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu = nullptr );

		void SetTrackMode( BOOL trackMode ) { m_bTrackMode = trackMode; }
		void SetCustomPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu ) { m_pCustomPopupMenu = pCustomPopupMenu; }

		static void CloseActiveMenu( void );

		// base overrides
	protected:
		virtual BOOL InitMenuBar( void );
	protected:
		ui::ICustomPopupMenu* m_pCustomPopupMenu;
	};
}


namespace mfc
{
	class CColorMenuButton : public CMFCColorMenuButton			// Office-like color picker button, used in MFC toolbars and popup menus; tracks an embedded CMFCColorBar
	{
		DECLARE_SERIAL( CColorMenuButton )

		CColorMenuButton( void );			// private constructor for serialization
	public:
		CColorMenuButton( UINT uiCmdID, const CColorTable* pColorTable );
		virtual ~CColorMenuButton();

		bool StoreImageByCmd( UINT uiCmdID ) { SetImage( GetCmdMgr()->GetCmdImage( uiCmdID ) ); }

		// base overrides
		virtual void SetColor( COLORREF color, BOOL notify = TRUE );
		virtual BOOL OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& colorRes );
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src );

		CWnd* GetMessageWnd( void ) const;
	private:
		const CColorTable* m_pColorTable;
	};
}


#endif // PopupMenus_h
