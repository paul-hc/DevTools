#ifndef ColorPopupDialogs_h
#define ColorPopupDialogs_h
#pragma once

#include "ui_fwd.h"
#include "AccelTable.h"
#include "ThemeStatic.h"

// template implementation headers
#include "Color.h"
#include "CmdUpdate.h"
#include "Dialog_fwd.h"
#include "GdiCoords.h"
#include "WndUtils.h"
#include "resource.h"


template< typename BaseDlg >
class CBasePopupColorDialog : public BaseDlg
{
	typedef BaseDlg TBaseClass;
protected:
	CBasePopupColorDialog( COLORREF color, DWORD dwFlags, CWnd* pParentWnd );
public:
	virtual ~CBasePopupColorDialog()
	{
	}

	ui::PopupAlign GetPopupAlign( void ) const { return m_popupAlign; }
	void SetPopupAlign( ui::PopupAlign popupAlign = ui::DropDown ) { m_popupAlign = popupAlign; m_behaveLikeModeless = true; }

	void SetAlignScreenRect( const CRect& alignScreenRect )
	{
		m_alignScreenRect = alignScreenRect;
		m_behaveLikeModeless = true;
	}

	bool ShouldRestartDlg( void ) const { return m_shouldRestartDlg; }

	COLORREF GetDefaultColor( void ) const { return m_defaultColor; }
	void SetDefaultColor( COLORREF defaultColor ) { m_defaultColor = defaultColor; }

	// pure interface
	virtual COLORREF GetCurrentColor( void ) const = 0;
protected:
	virtual void ModifyColor( COLORREF newColor ) = 0;

	virtual void InitDialog( void );
	virtual void AdjustDlgWindowRect( CRect& rWindowRect ) { rWindowRect; }
	virtual bool IsTracking( void ) const { return false; }

	void CloseDialog( int cmdId );		// termination
private:
	void LayoutDialog( void );
	CRect GetAlignScreenRect( void ) const;
private:
	ui::PopupAlign m_popupAlign;		// popup dialog drop alignment
	bool m_behaveLikeModeless;			// popup dialog: click outside will close the dialog
	bool m_firstInit;
	bool m_destroying;
	bool m_shouldRestartDlg;
	CRect m_alignScreenRect;
protected:
	COLORREF m_defaultColor;
	CPickMenuStatic m_menuPickerStatic;
	CAccelTable m_accel;

	enum { IDC_EDIT_PICK_STATIC = 3355 };

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg BOOL OnNcActivate( BOOL active );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	virtual void OnOK( void );
	virtual void OnCancel( void );
	afx_msg void OnBnClicked_EditPickMenu( void );
	afx_msg void OnCopy( void );
	afx_msg void OnUpdateCopy( CCmdUI* pCmdUI );
	afx_msg void OnPaste( void );
	afx_msg void OnUpdatePaste( CCmdUI* pCmdUI );
	afx_msg void On_ResetDefaultColor( void );
	afx_msg void OnUpdate_ResetDefaultColor( CCmdUI* pCmdUI );
	afx_msg void On_ColorDialogStyle( UINT cmdId );
	afx_msg void OnUpdate_ColorDialogStyle( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


class CColorPopupDialog : public CBasePopupColorDialog<CColorDialog>			// Common controls style colors dialog
{
public:
	CColorPopupDialog( CWnd* pParentWnd, COLORREF color, DWORD dwFlags = CC_FULLOPEN | CC_ANYCOLOR | CC_RGBINIT );

	// base overrides
	virtual COLORREF GetCurrentColor( void ) const override;
protected:
	virtual void ModifyColor( COLORREF newColor ) override { SetCurrentColor( newColor ); }

	virtual void InitDialog( void ) override;
	virtual void AdjustDlgWindowRect( CRect& rWindowRect ) override;
private:
	void CreateSpin( UINT editId, CSpinButtonCtrl& rSpinButton, UINT spinId, int maxValue );
	void OffsetControl( UINT ctrlId, int offsetX );
public:
	// enum { IDD = DLG_COLOR };

	CSpinButtonCtrl m_hueSpin;
	CSpinButtonCtrl m_satSpin;
	CSpinButtonCtrl m_lumSpin;

	CSpinButtonCtrl m_redSpin;
	CSpinButtonCtrl m_greenSpin;
	CSpinButtonCtrl m_blueSpin;

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


#include "afxcolordialog.h"


class COfficeColorPopupDialog : public CBasePopupColorDialog<CMFCColorDialog>		// MS-Office style MFC colors dialog
{
public:
	COfficeColorPopupDialog( CWnd* pParentWnd, COLORREF color );

	// base overrides
	virtual COLORREF GetCurrentColor( void ) const override { return GetColor(); }
protected:
	virtual void ModifyColor( COLORREF newColor ) override;

	virtual void InitDialog( void ) override;
	virtual bool IsTracking( void ) const { return m_bPickerMode != FALSE; }
};


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBasePopupColorDialog, BaseDlg, TBaseClass )
	ON_WM_NCACTIVATE()
	ON_WM_INITMENUPOPUP()
	ON_BN_CLICKED( IDC_EDIT_PICK_STATIC, OnBnClicked_EditPickMenu )
	ON_COMMAND( ID_EDIT_COPY, OnCopy )
	ON_UPDATE_COMMAND_UI( ID_EDIT_COPY, OnUpdateCopy )
	ON_COMMAND( ID_EDIT_PASTE, OnPaste )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdatePaste )
	ON_COMMAND( ID_RESET_DEFAULT, On_ResetDefaultColor )
	ON_UPDATE_COMMAND_UI( ID_RESET_DEFAULT, OnUpdate_ResetDefaultColor )
	ON_COMMAND_RANGE( ID_USE_COLOR_DIALOG_MFC, ID_USE_COLOR_DIALOG_COMCTRL, On_ColorDialogStyle )
	ON_UPDATE_COMMAND_UI_RANGE( ID_USE_COLOR_DIALOG_MFC, ID_USE_COLOR_DIALOG_COMCTRL, OnUpdate_ColorDialogStyle )
END_MESSAGE_MAP()


// CBasePopupColorDialog template code

template< typename BaseDlg >
CBasePopupColorDialog<BaseDlg>::CBasePopupColorDialog( COLORREF color, DWORD dwFlags, CWnd* pParentWnd )
	: TBaseClass( color, dwFlags, pParentWnd )
	, m_popupAlign( ui::DropDown )
	, m_behaveLikeModeless( false )
	, m_firstInit( true )
	, m_destroying( false )
	, m_shouldRestartDlg( false )
	, m_alignScreenRect( 0, 0, 0, 0 )
	, m_defaultColor( color )		// if not explicitly set, use the initial color
	, m_menuPickerStatic( ui::DropDown )
	, m_accel( IDR_EDIT_ACCEL )
{
	m_menuPickerStatic.m_useText = true;
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::InitDialog( void ) override
{
	REQUIRE( m_firstInit );

	LayoutDialog();
	m_firstInit = false;
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::CloseDialog( int cmdId )
{
	REQUIRE( IDOK == cmdId || IDCANCEL == cmdId );
	this->PostMessage( WM_COMMAND, MAKEWPARAM( cmdId, BN_CLICKED ) );			// common dialogs (e.g. CColorDialog) do not use EndDialog
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::LayoutDialog( void )
{
	CRect windowRect, oldWindowRect;
	this->GetWindowRect( &oldWindowRect );

	if ( m_behaveLikeModeless )
	{
		this->GetClientRect( &oldWindowRect );
		AdjustDlgWindowRect( oldWindowRect );

		this->ModifyStyle( WS_BORDER | WS_DLGFRAME | WS_SYSMENU | DS_MODALFRAME, 0 );
		this->ModifyStyleEx( WS_EX_WINDOWEDGE, 0 );

		::AdjustWindowRectEx( &oldWindowRect, this->GetStyle(), FALSE, this->GetExStyle() );

		ui::SetRectSize( windowRect, oldWindowRect.Size() );
		ui::AlignPopupWindowRect( windowRect, GetAlignScreenRect(), m_popupAlign );
	}
	else
	{	// enlarge slightly on the right
		windowRect = oldWindowRect;
		AdjustDlgWindowRect( windowRect );
	}

	this->SetWindowPos( NULL, windowRect.left, windowRect.top, windowRect.Width(), windowRect.Height(),
						SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOZORDER /*| SWP_SHOWWINDOW*/ );
}

template< typename BaseDlg >
CRect CBasePopupColorDialog<BaseDlg>::GetAlignScreenRect( void ) const
{
	CRect alignScreenRect = m_alignScreenRect;

	if ( alignScreenRect.IsRectNull() )
		if ( this->m_pParentWnd != nullptr )
			this->m_pParentWnd->GetWindowRect( &alignScreenRect );	// align to parent

	return alignScreenRect;
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::DoDataExchange( CDataExchange* pDX )
{
	if ( DialogOutput == pDX->m_bSaveAndValidate )
		if ( m_firstInit )
			InitDialog();

	__super::DoDataExchange( pDX );
}

template< typename BaseDlg >
BOOL CBasePopupColorDialog<BaseDlg>::PreTranslateMessage( MSG* pMsg )
{
	if ( m_accel.Translate( pMsg, this->m_hWnd ) )
		return TRUE;

	return __super::PreTranslateMessage( pMsg );
}

template< typename BaseDlg >
BOOL CBasePopupColorDialog<BaseDlg>::OnNcActivate( BOOL active )
{
 	BOOL deactivateOk = __super::OnNcActivate( active );

	// simulate a IDCANCEL command when user deactivates this modal dialog
	if ( m_behaveLikeModeless && !m_firstInit && !m_destroying && !IsTracking() )
		if ( !active && deactivateOk )
			CloseDialog( IDCANCEL );			// close the popup dialog on deactivation

	return deactivateOk;
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	ui::HandleInitMenuPopup( this, pPopupMenu, !isSysMenu );
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnOK( void )
{
	m_destroying = true;
	__super::OnOK();
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnCancel( void )
{
	m_destroying = true;
	__super::OnCancel();
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnBnClicked_EditPickMenu( void )
{
	m_menuPickerStatic.TrackMenu( this, IDR_STD_CONTEXT_MENU, ui::ColorPopupDialog, false );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnCopy( void )
{
	ui::CopyColor( GetCurrentColor() );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnUpdateCopy( CCmdUI* /*pCmdUI*/ )
{
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnPaste( void )
{
	COLORREF newColor;

	if ( ui::PasteColor( &newColor ) )
		ModifyColor( newColor );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnUpdatePaste( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( ui::CanPasteColor() );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::On_ResetDefaultColor( void )
{
	ModifyColor( m_defaultColor );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnUpdate_ResetDefaultColor( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( this->GetColor() != m_defaultColor && !ui::IsNullColor( m_defaultColor ) );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::On_ColorDialogStyle( UINT cmdId )
{
	ui::SaveColorDialogStyle( ID_USE_COLOR_DIALOG_MFC == cmdId ? ui::OfficeColorDialog : ui::ClassicColorDialog );

	m_shouldRestartDlg = true;			// will restart the color dialog with alternate style
	CloseDialog( IDOK );
}

template< typename BaseDlg >
void CBasePopupColorDialog<BaseDlg>::OnUpdate_ColorDialogStyle( CCmdUI* pCmdUI )
{
	bool isCurrent = ( ID_USE_COLOR_DIALOG_MFC == pCmdUI->m_nID ) == ( ui::OfficeColorDialog == ui::LoadColorDialogStyle() );

	pCmdUI->Enable( !isCurrent );
	pCmdUI->SetRadio( isCurrent );
}


#endif // ColorPopupDialogs_h
