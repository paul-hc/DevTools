
#ifndef LayoutDialog_h
#define LayoutDialog_h
#pragma once

#include "Dialog_fwd.h"
#include "PopupDlgBase.h"
#include "LayoutMetrics.h"

#if _MSC_VER >= VS_2008		// MSVC++ 9.0 (Visual Studio 2008)
	#include <afxdialogex.h>
	typedef CDialogEx TMfcBaseDialog;
#else
	typedef CDialog TMfcBaseDialog;
#endif


// base class for resizable dialogs with dynamic control layout

class CLayoutDialog : public TMfcBaseDialog
	, public CPopupDlgBase
	, public ui::ILayoutEngine
	, public ui::ICustomCmdInfo
{
	void Construct( void );

	// hidden base methods
	using TMfcBaseDialog::Create;
	using TMfcBaseDialog::CreateIndirect;
protected:
	CLayoutDialog( void );			// for modeless dialog
public:
	CLayoutDialog( UINT templateId, CWnd* pParent = nullptr );
	CLayoutDialog( const TCHAR* pTemplateName, CWnd* pParent = nullptr );
	virtual ~CLayoutDialog();

	bool CreateModeless( UINT templateId = 0, CWnd* pParentWnd = nullptr, int cmdShow = SW_SHOW );		// works with both modal and modeless constructors
	bool IsModeless( void ) const { return m_modeless; }

	const std::tstring& GetSection( void ) const { return m_regSection; }

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void ) override;
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) override;
	virtual bool HasControlLayout( void ) const override;

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	// overridables
	virtual void LoadFromRegistry( void );
	virtual void SaveToRegistry( void );

	virtual void LayoutDialog( void );
protected:
	virtual bool UseWindowPlacement( void ) const;
	virtual void PostRestorePlacement( int showCmd, bool restoreToMaximized );
	virtual void OnIdleUpdateControls( void );			// override to update specific controls
private:
	void RestorePlacement( void );
	void ModifySystemMenu( void );
private:
	std::auto_ptr<CLayoutEngine> m_pLayoutEngine;
	std::auto_ptr<CLayoutPlacement> m_pPlacement;		// persistent
protected:
	std::tstring m_regSection;

	// option flags
	bool m_initCollapsed;		// open dialog in collapsed state

	// generated stuff
protected:
	virtual void PreSubclassWindow( void ) override;
	virtual void PostNcDestroy( void ) override;
	virtual void DoDataExchange( CDataExchange* pDX ) override;
	virtual void OnOK( void ) override;
	virtual void OnCancel( void ) override;
public:
	virtual BOOL DestroyWindow( void ) override;
	virtual BOOL PreTranslateMessage( MSG* pMsg ) override;
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override;
protected:
	virtual void OnDestroy( void );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg void OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo );
	afx_msg LRESULT OnNcHitTest( CPoint screenPoint );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnPaint( void );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnSysCommand( UINT cmdId, LPARAM lParam );
	afx_msg BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg LRESULT OnKickIdle( WPARAM, LPARAM );

	DECLARE_MESSAGE_MAP()
};


#endif // LayoutDialog_h
