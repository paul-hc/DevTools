#ifndef PopupMenus_h
#define PopupMenus_h
#pragma once

#include "PopupMenus_fwd.h"
#include <afxpopupmenu.h>


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


#include <afxcolormenubutton.h>


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

		void SetSelected( bool isTableSelected = true );

		enum NotifCode { CMBN_COLORSELECTED = CBN_SELCHANGE };		// note: notifications are suppressed during parent's UpdateData()
	protected:
		CWnd* GetMessageWnd( void ) const;
		const CColorEntry* FindClickedBarColorEntry( void ) const;

		// base overrides
	public:
		virtual void SetImage( int iImage ) override;
		virtual void SetColor( COLORREF color, BOOL notify = TRUE ) override;
		virtual BOOL OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& rColor ) override;
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) override;
		virtual CMFCPopupMenu* CreatePopupMenu( void ) override;
	private:
		const CColorTable* m_pColorTable;
		const CColorTable* m_pDocColorTable;		// typically for the "Color Shades" table
	};
}


#include "utl/Range.h"
#include "WindowHook_fwd.h"
#include "Control_fwd.h"
#include <afxcolorpopupmenu.h>


class CWindowHook;
namespace nosy { struct CColorBar_; }


namespace mfc
{
	// Customized tracking popup color bar, that allows custom handling of color button tooltips and certain button commands.
	//
	class CColorPopupMenu : public CMFCColorPopupMenu
		, private ui::IToolTipsHandler
		, private ui::IWindowHookHandler
	{
	public:
		// general constructor (pParentMenuBtn could be null)
		CColorPopupMenu( CColorMenuButton* pParentMenuBtn,
						 const mfc::TColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 mfc::TColorList& docColors, int columns, int horzDockRows, int vertDockColumns,
						 COLORREF colorAuto, UINT uiCmdID, BOOL stdColorDlg = false );

		// color picker constructor
		CColorPopupMenu( CMFCColorButton* pParentPickerBtn,
						 const mfc::TColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 mfc::TColorList& docColors, int columns, COLORREF colorAuto );

		virtual ~CColorPopupMenu();

		nosy::CColorBar_* GetColorBar( void ) const { return m_pColorBar; }
		void SetColorEditorHost( ui::IColorEditorHost* pEditorHost );

		const CColorEntry* FindClickedBarColorEntry( void ) const;
	private:
		void StoreBtnColorEntries( void );
		void StoreBtnColorTableEntries( IN OUT Range<int>& rBtnIndex, const CColorTable* pColorTable );
		static void StoreButtonColorEntry( CMFCToolBarButton* pButton, const CColorEntry* pColorEntry );
		static bool OpenColorDialog( ui::IColorEditorHost* pEditorHost );

		const CColorEntry* FindColorEntry( COLORREF rawColor ) const;
		bool FormatColorTipText( OUT std::tstring& rTipText, const CMFCToolBarButton* pButton, int hitBtnIndex ) const;

		// ui::IToolTipsHandler interface
		virtual bool Handle_TtnNeedText( NMTTDISPINFO* pNmDispInfo, const CPoint& point ) override;

		// ui::IWindowHookHandler interface
		virtual bool Handle_HookMessage( OUT LRESULT& rResult, const MSG& msg, const CWindowHook* pHook ) override;	// to handle WM_LBUTTONUP on More Color button
	private:
		CColorMenuButton* m_pParentMenuBtn;
		ui::IColorEditorHost* m_pEditorHost;
		const CColorTable* m_pColorTable;
		const CColorTable* m_pDocColorTable;

		nosy::CColorBar_* m_pColorBar;						// points to CMFCColorPopupMenu::m_wndColorBar data-member, with access to protected data-members
		std::auto_ptr<CWindowHook> m_pColorBarHook;		// handles formatted color entry + some button clicks
		COLORREF m_rawAutoColor;
		COLORREF m_rawSelColor;

		// generated stuff
	protected:
		afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
	};
}


#endif // PopupMenus_h
