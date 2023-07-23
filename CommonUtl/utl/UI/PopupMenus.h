#ifndef PopupMenus_h
#define PopupMenus_h
#pragma once

#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>
#include <afxcolorpopupmenu.h>
#include <afxcolormenubutton.h>


class CColorTable;
class CColorStore;


namespace ui
{
	typedef CArray<COLORREF, COLORREF> TMFCColorArray;
	typedef CList<COLORREF, COLORREF> TMFCColorList;
}


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
	private:
		ui::ICustomPopupMenu* m_pCustomPopupMenu;
	};


	class CColorPopupMenu : public CMFCColorPopupMenu
	{
	public:
		CColorPopupMenu( CMFCColorButton* pParentBtn, const ui::TMFCColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 ui::TMFCColorList& docColors, int columns, COLORREF colorAuto );
		virtual ~CColorPopupMenu();

		void SetEnabledInCustomizeMode( BOOL enabledInCustomizeMode ) { m_bEnabledInCustomizeMode = enabledInCustomizeMode; }

		CMFCColorBar* GetColorMenuBar( void ) const { return checked_static_cast<CMFCColorBar*>( const_cast<CColorPopupMenu*>( this )->GetMenuBar() ); }
	private:

		// generated stuff
	protected:
		//virtual int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
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

		const CColorTable* GetColorTable( void ) const { return m_pColorTable; }

		void SetDocColorTable( const CColorTable* pDocColorTable );
		bool StoreImageByCmd( UINT uiCmdID ) { SetImage( GetCmdMgr()->GetCmdImage( uiCmdID ) ); }

		enum NotifCode { CMBN_COLORSELECTED = CBN_SELCHANGE };		// note: notifications are suppressed during parent's UpdateData()
	protected:
		CWnd* GetMessageWnd( void ) const;

		// base overrides
	public:
		virtual void SetColor( COLORREF color, BOOL notify = TRUE );
		virtual BOOL OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& colorRes );
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src );
		virtual CMFCPopupMenu* CreatePopupMenu( void );
		virtual void OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
							 BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE );
	private:
		const CColorTable* m_pColorTable;
		const CColorTable* m_pDocColorTable;		// typically for the "Shades Colors" table
	};
}


#endif // PopupMenus_h
