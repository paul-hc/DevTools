
#ifndef LayoutDialog_h
#define LayoutDialog_h
#pragma once

#include "Dialog_fwd.h"
#include "PopupDlgBase.h"
#include "LayoutMetrics.h"

#if _MSC_VER > 1500	// MSVC++ 9.0 (Visual Studio 2008)
	#include <afxdialogex.h>
	typedef CDialogEx MfcBaseDialog;
#else
	typedef CDialog MfcBaseDialog;
#endif


// base class for resizable dialogs with dynamic control layout

class CLayoutDialog : public MfcBaseDialog
					, public CPopupDlgBase
					, public ui::ILayoutEngine
					, public ui::ICustomCmdInfo
{
	void Construct( void );
protected:
	CLayoutDialog( void );			// for modeless dialog
public:
	CLayoutDialog( UINT templateId, CWnd* pParent = NULL );
	CLayoutDialog( const TCHAR* pTemplateName, CWnd* pParent = NULL );
	virtual ~CLayoutDialog();

	const std::tstring& GetSection( void ) const { return m_regSection; }
	bool IsModeless( void ) const { return HasFlag( m_dlgFlags, Modeless ); }

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void );
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );
	virtual bool HasControlLayout( void ) const;

	// overridables
	virtual void LoadFromRegistry( void );
	virtual void SaveToRegistry( void );

	virtual void LayoutDialog( void );
protected:
	virtual bool UseWindowPlacement( void ) const;
	virtual void PostRestorePlacement( int showCmd );
	virtual void OnIdleUpdateControls( void );
private:
	void RestorePlacement( void );
	void ModifySystemMenu( void );
private:
	enum Flag { Modeless = 1 << 0 };

	DWORD m_dlgFlags;
	std::auto_ptr< CLayoutEngine > m_pLayoutEngine;
	std::auto_ptr< CLayoutPlacement > m_pPlacement;		// persistent
protected:
	std::tstring m_regSection;

	// option flags
	bool m_initCollapsed;		// open dialog in collapsed state
public:
	// generated overrides
	protected:
	virtual void OnOK( void );
	virtual void OnCancel( void );
	virtual void DoDataExchange( CDataExchange* pDX );
	public:
	virtual BOOL DestroyWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnDestroy( void );
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
