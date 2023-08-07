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
	// Customizable tracking menu to be used by mfc::CContextMenuMgr::TrackPopupMenu().
	//	To customize content pass a ui::ICustomPopupMenu callback interface pointer.
	//
	class CTrackingPopupMenu : public CMFCPopupMenu
	{
	public:
		CTrackingPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu = nullptr, int trackingMode = 0 );
		virtual ~CTrackingPopupMenu();

		void SetTrackMode( BOOL trackMode ) { m_bTrackMode = trackMode; }
		void SetCustomPopupMenu( ui::ICustomPopupMenu* pCustomPopupMenu ) { m_pCustomPopupMenu = pCustomPopupMenu; }

		static void CloseActiveMenu( void );
		static CMFCToolBarButton* FindTrackingBarButton( UINT btnId );

		// base overrides
	protected:
		virtual BOOL InitMenuBar( void );
	private:
		ui::ICustomPopupMenu* m_pCustomPopupMenu;		// client code can customize the tracking menu content: replace buttons, etc
		int m_trackingMode;								// client-specific cookie
	};
}


#include <afxtoolbarmenubutton.h>


namespace mfc
{
	class CToolBarColorButton : public CMFCToolBarMenuButton		// a normal toolbar item, that draws the color instead of a glyph
	{
		DECLARE_SERIAL( CToolBarColorButton );

		CToolBarColorButton( void );
	public:
		CToolBarColorButton( UINT btnId, COLORREF color, const TCHAR* pText = nullptr );
		CToolBarColorButton( UINT btnId, const CColorEntry* pColorEntry );
		CToolBarColorButton( const CMFCToolBarButton* pSrcButton, COLORREF color );

		COLORREF GetColor( void ) const { return m_color; }
		void SetColor( COLORREF color );

		const CColorEntry* GetColorEntry( void ) const { return reinterpret_cast<const CColorEntry*>( m_dwdItemData ); }

		void SetChecked( bool checked = true );
		void UpdateSelectedColor( COLORREF selColor ) { SetChecked( m_color == selColor ); }

		static CToolBarColorButton* ReplaceWithColorButton( CMFCToolBar* pToolBar, UINT btnId, COLORREF color, OUT OPTIONAL int* pIndex = nullptr );
	private:
		COLORREF m_color;

		// base overrides
	public:
		virtual void SetImage( int iImage ) overrides(CMFCToolBarButton);
		virtual BOOL OnToolHitTest( const CWnd* pWnd, TOOLINFO* pToolInfo ) overrides(CMFCToolBarButton);
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarMenuButton);
		virtual void OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
							 BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE ) overrides(CMFCToolBarMenuButton);
	};
}


#include <afxcolormenubutton.h>


namespace mfc
{
	// Office-like color picker button, to be used via ReplaceButton() in MFC toolbars and popup menus; tracks an embedded CMFCColorBar.
	//
	class CColorMenuButton : public CMFCColorMenuButton
	{
		DECLARE_SERIAL( CColorMenuButton );

		CColorMenuButton( void );			// private constructor for serialization
	public:
		CColorMenuButton( UINT btnId, const CColorTable* pColorTable );
		virtual ~CColorMenuButton();

		const CColorTable* GetColorTable( void ) const { return m_pColorTable; }

		ui::IColorEditorHost* GetEditorHost( void ) const { return m_pEditorHost; }
		void SetEditorHost( ui::IColorEditorHost* pEditorHost );

		void SetSelectedTable( COLORREF color, COLORREF autoColor, const CColorTable* pDocColorsTable );
		void SetDisplayColorBox( UINT imageId = 0 );				// 0 for transparent: empty image with color band at bottom;  UINT_MAX for hiding the color box

		enum NotifCode { CMBN_COLORSELECTED = CBN_SELCHANGE };		// note: notifications are suppressed during parent's UpdateData()
	protected:
		CWnd* GetMessageWnd( void ) const;
		const CColorEntry* FindClickedBarColorEntry( void ) const;

		// base overrides
	public:
		virtual void SetImage( int iImage ) overrides(CMFCToolBarButton);
		virtual void SetColor( COLORREF color, BOOL notify = TRUE ) overrides(CMFCColorMenuButton);
		virtual BOOL OpenColorDialog( const COLORREF colorDefault, OUT COLORREF& rColor ) overrides(CMFCColorMenuButton);
		virtual void OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages, BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
							 BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE ) overrides(CMFCColorMenuButton);
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCColorMenuButton);
		virtual CMFCPopupMenu* CreatePopupMenu( void ) overrides(CMFCColorMenuButton);
	private:
		const CColorTable* m_pColorTable;			// required field
		ui::IColorEditorHost* m_pEditorHost;		// optional field; if null, it sends CMBN_COLORSELECTED notifications
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
		// general constructor: pParentMenuBtn could be null, and it provides the color table
		CColorPopupMenu( CColorMenuButton* pParentMenuBtn,
						 const mfc::TColorArray& colors, COLORREF color,
						 const TCHAR* pAutoColorLabel, const TCHAR* pMoreColorLabel, const TCHAR* pDocColorsLabel,
						 mfc::TColorList& docColors, int columns, int horzDockRows, int vertDockColumns,
						 COLORREF colorAuto, UINT uiCmdID, BOOL stdColorDlg = false );

		// color picker constructor: uses the selected color table
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

		const CColorEntry* FindColorEntry( COLORREF rawColor ) const;
		bool FormatColorTipText( OUT std::tstring& rTipText, const CMFCToolBarButton* pButton, int hitBtnIndex ) const;

		// ui::IToolTipsHandler interface
		virtual bool Handle_TtnNeedText( NMTTDISPINFO* pNmDispInfo, const CPoint& point ) override;

		// ui::IWindowHookHandler interface
		virtual bool Handle_HookMessage( OUT LRESULT& rResult, const MSG& msg, const CWindowHook* pHook ) override;		// to handle WM_LBUTTONUP on More Color button
	private:
		ui::IColorEditorHost* m_pEditorHost;
		const CColorTable* m_pColorTable;
		const CColorTable* m_pDocColorTable;

		nosy::CColorBar_* m_pColorBar;					// points to CMFCColorPopupMenu::m_wndColorBar data-member, with access to protected data-members
		std::auto_ptr<CWindowHook> m_pColorBarHook;		// handles formatted color entry + some button clicks
		COLORREF m_rawAutoColor;
		COLORREF m_rawSelColor;

		// generated stuff
	protected:
		afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
	};
}


namespace mfc
{
	class CColorTableBar;


	class CColorTablePopupMenu : public CMFCPopupMenu		// displays named colors (CToolBarColorButton) on multiple column grid layout, typically for Windows System colors
	{
	public:
		CColorTablePopupMenu( CColorMenuButton* pParentMenuBtn );		// pParentMenuBtn provides the color table
		CColorTablePopupMenu( ui::IColorEditorHost* pEditorHost );		// picker constructor: uses the selected color table, runs in modeless popup mode
		virtual ~CColorTablePopupMenu();
	private:
		std::auto_ptr<CColorTableBar> m_pColorBar;

		enum { ToolBarId = 1, ToolBarStyle = AFX_DEFAULT_TOOLBAR_STYLE | CBRS_TOOLTIPS | CBRS_FLYBY };

		// base overrides
	public:
		virtual CMFCPopupMenuBar* GetMenuBar( void ) overrides(CMFCPopupMenuBar);
		virtual BOOL OnCmdMsg( UINT btnId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) overrides(CMFCPopupMenu);

		// generated stuff
	protected:
		afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
	};


	struct CColorButtonsGridLayout;


	class CColorTableBar : public CMFCPopupMenuBar
	{
	public:
		CColorTableBar( const CColorTable* pColorTable, ui::IColorEditorHost* pEditorHost );
		virtual ~CColorTableBar();

		void SetupButtons( void );
		void StoreParentPicker( CMFCColorButton* pParentPickerButton ) { m_pParentPickerButton = pParentPickerButton; }

		bool IsColorBtnId( UINT btnId ) const;
	private:
		const CColorTable* m_pColorTable;			// required field
		ui::IColorEditorHost* m_pEditorHost;		// required field
		UINT m_columnCount;

		CMFCColorButton* m_pParentPickerButton;		// set only when created as modeless popup from the picker button
		bool m_isModelessPopup;
		std::auto_ptr<CColorButtonsGridLayout> m_pLayout;

		enum { AutoId = 70, MoreColorsId, ColorIdMin, PadId = 0 };

		// base overrides
	public:
		virtual void OnFillBackground( CDC* pDC ) overrides(CMFCToolBar);
	protected:
		virtual CSize CalcSize( BOOL vertDock ) overrides(CMFCPopupMenuBar);
		virtual void AdjustLocations( void ) overrides(CMFCPopupMenuBar);
		virtual BOOL OnSendCommand( const CMFCToolBarButton* pButton ) overrides(CMFCPopupMenuBar);

		// generated stuff
	protected:
		afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );

		DECLARE_MESSAGE_MAP()
	};
}


#endif // PopupMenus_h
