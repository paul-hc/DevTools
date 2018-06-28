#ifndef LayoutPropertySheet_h
#define LayoutPropertySheet_h
#pragma once

#include "PopupDlgBase.h"
#include "LayoutMetrics.h"
#include "LayoutBasePropertySheet.h"


// base class for top level modeless or modal property sheets

abstract class CLayoutPropertySheet : public CLayoutBasePropertySheet
									, public CPopupDlgBase
									, public ui::ILayoutEngine
{
protected:
	CLayoutPropertySheet( const std::tstring& title, CWnd* pParent, UINT selPageIndex = 0 );
public:
	virtual ~CLayoutPropertySheet();

	enum SingleTransactionButtons { ShowOkCancel, ShowOnlyClose };

	bool IsSingleTransaction( void ) const { return HasFlag( m_psh.dwFlags, PSH_NOAPPLYNOW ); }
	void SetSingleTransaction( SingleTransactionButtons singleTransactionButtons = ShowOkCancel );

	bool HasHelpButton( void ) const { return HasFlag( m_psh.dwFlags, PSH_HASHELP ); }

	// ui::ILayoutEngine interface
	virtual CLayoutEngine& GetLayoutEngine( void );
	virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );
	virtual bool HasControlLayout( void ) const;

	// base overrides
	virtual void LoadFromRegistry( void );
	virtual void SaveToRegistry( void );
	virtual bool IsSheetModified( void ) const;
	virtual void LayoutSheet( void );
protected:
	virtual void OnIdleUpdateControls( void );
private:
	void SetupInitialSheet( void );
	void AdjustModelessSheet( void );
	void RestoreSheetPlacement( void );
	void ModifySystemMenu( void );

	CPoint GetCascadeByOffset( void ) const;
private:
	std::auto_ptr< CLayoutEngine > m_pLayoutEngine;
	std::auto_ptr< CLayoutPlacement > m_pSheetPlacement;		// persistent
public:
	enum RestorePos { PosNoRestore, PosRestore, PosAutoCascade };

	// options flags
	RestorePos m_restorePos;	// restore saved sheet position
	bool m_restoreSize;			// restore saved sheet size
	SingleTransactionButtons m_singleTransactionButtons;
	DWORD m_styleMinMax;		// WS_MINIMIZEBOX, WS_MAXIMIZEBOX extra style to apply on initialization
	bool m_alwaysModified;		// always enable the OK/APPLY buttons, pages don't need to call OnFieldModified()
private:
	// generated stuff
	public:
	virtual BOOL Create( CWnd* pParent = NULL, DWORD style = UINT_MAX, DWORD styleEx = 0 ); // for modeless creation
	virtual void PostNcDestroy( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL OnInitDialog( void );
protected:
	afx_msg BOOL OnNcCreate( CREATESTRUCT* pCreate );
	afx_msg void OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg LRESULT OnNcHitTest( CPoint point );
	afx_msg void OnPaint( void );
	afx_msg void OnSysCommand( UINT cmdId, LPARAM lParam );
	afx_msg void OnApplyNow( void );
	afx_msg void OnOk( void );
	afx_msg void OnCancel( void );
	afx_msg LRESULT OnKickIdle( WPARAM, LPARAM );

	DECLARE_MESSAGE_MAP()
};


// inline code

inline void CLayoutPropertySheet::SetSingleTransaction( SingleTransactionButtons singleTransactionButtons /*= ShowOkCancel*/ )
{
	ASSERT_NULL( m_hWnd ); // should be called before creation

	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_singleTransactionButtons = singleTransactionButtons;
}


#endif // LayoutPropertySheet_h
