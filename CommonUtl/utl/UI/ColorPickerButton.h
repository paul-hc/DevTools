#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include "AccelTable.h"
#include "Dialog_fwd.h"
#include "PopupMenus_fwd.h"
#include "StdColors.h"
#include "utl/Registry_fwd.h"
#include <afxcolorbutton.h>


namespace ui
{
	template< typename ColorCtrlT >
	void DDX_ColorButton( CDataExchange* pDX, int ctrlId, ColorCtrlT& rCtrl, COLORREF* pColor )
	{
		::DDX_Control( pDX, ctrlId, rCtrl );

		if ( pColor != nullptr )
			if ( DialogOutput == pDX->m_bSaveAndValidate )
				rCtrl.SetColor( *pColor );
			else
				*pColor = rCtrl.GetColor();
	}

	void DDX_ColorText( CDataExchange* pDX, int ctrlId, COLORREF* pColor, bool doInput = false );
	void DDX_ColorRepoText( CDataExchange* pDX, int ctrlId, COLORREF color );
}


class CColorTable;
namespace mfc { class CColorPopupMenu; }


class CColorPickerButton : public CMFCColorButton
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( void );
	CColorPickerButton( ui::StdColorTable tableType );
	virtual ~CColorPickerButton();

	const CColorTable* GetColorTable( void ) const { return m_pColorTable; }

	void SetRegSection( const std::tstring& regSection ) { m_regSection = regSection; }

	void SetAutomaticColor( COLORREF autoColor, const TCHAR autoLabel[] = _T("Automatic") ) { EnableAutomaticButton( autoLabel, autoColor ); }

	void SetColors( const std::vector<COLORREF>& colors );
	void SetHalftoneColors( size_t halftoneSize = 256 );

	bool SetColorTable( const CColorTable* pColorTable );
	bool SetColorTable( ui::StdColorTable tableType );

	void SetDocumentColors( const CColorTable* pColorTable, const TCHAR* pDocLabel = nullptr );		// additional 'Document' section of colors (below the main colors)
private:
	void Construct( void );
	bool IsEmpty( void ) const { return m_Colors.IsEmpty() && m_lstDocColors.IsEmpty(); }
	void AddColorTablesSubMenu( CMenu* pContextMenu );

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	void NotifyColorSetChanged( void );
	void ShowColorPopupImpl( mfc::CColorPopupMenu* pTrackingColorPopup );
private:
	const CColorTable* m_pColorTable;
	size_t m_halftoneSize;
	bool m_useUserColors;					// neither from color tables, nor the halftone palette

	std::tstring m_regSection;
	CAccelTable m_accel;

	static std::vector<CColorPickerButton*> s_instances;		// for color table notifications
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	// base overrides:
	virtual void OnShowColorPopup( void ) override;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnCopy( void );
	afx_msg void OnPaste( void );
	afx_msg void OnUpdatePaste( CCmdUI* pCmdUI );
	afx_msg void On_CopyColorTable( void );
	afx_msg void On_HalftoneTable( UINT cmdId );
	afx_msg void OnUpdate_HalftoneTable( CCmdUI* pCmdUI );
	afx_msg void On_UseColorTable( UINT cmdId );
	afx_msg void OnUpdate_UseColorTable( CCmdUI* pCmdUI );
	afx_msg void OnUpdateEnable( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#include <afxmenubutton.h>


class CMenuPickerButton : public CMFCMenuButton
{
public:
	CMenuPickerButton( CWnd* pTargetWnd = nullptr );
	virtual ~CMenuPickerButton();

	void SetTargetWnd( CWnd* pTargetWnd ) { m_pTargetWnd = pTargetWnd; }
	CWnd* GetTargetWnd( void ) const;
private:
	CWnd* m_pTargetWnd;			// if null, parent dialog is the target

	// base overrides:
protected:
	virtual void OnShowMenu( void ) override;

	// generated stuff
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


class CColorStore;
class CScratchColorStore;
namespace mfc { class CColorMenuButton; }


class CColorStorePicker : public CMenuPickerButton
	, private ui::ICustomPopupMenu
{
public:
	CColorStorePicker( CWnd* pTargetWnd = nullptr );
	virtual ~CColorStorePicker();

	const CColorStore* GetMainStore( void ) const { return m_pMainStore; }
	void SetMainStore( const CColorStore* pMainStore );

	COLORREF GetColor( void ) const { return m_color; }
	void SetColor( COLORREF color );		// CLR_NONE: automatic
protected:
	/*virtual*/ void UpdateColor( COLORREF newColor );
	void UpdateShadesTable( void );
private:
	void ResetPopupMenu( void ) { m_popupMenu.DestroyMenu(); m_hMenu = nullptr; }
	void SetupPopupMenu( void );
	void ModifyPopupMenuTableItems( const CColorStore* pColorStore );
	void InsertPopupMenuTableItems( const CColorStore* pColorStore );
	CColorTable* LookupPopupColorTable( UINT colorBtnId ) const;

	mfc::CColorMenuButton* MakeColorMenuButton( UINT colorBtnId, const CColorTable* pColorTable ) const;
	static CMFCPopupMenu* GetTrackingPopupMenu( void );
	static const mfc::CColorMenuButton* LookupPopupColorButton( UINT colorBtnId );

	// ui::ICustomPopupMenu interface
	virtual void OnCustomizeMenuBar( CMFCPopupMenu* pMenuPopup ) override;
private:
	COLORREF m_color;
	const CColorStore* m_pMainStore;		// by default CColorRepository::Instance(); enumerates the main ID_REPO_COLOR_TABLE_MIN/MAX color tables
	const CColorTable* m_pSelColorTable;	// selected color table
	std::auto_ptr<CScratchColorStore> m_pScratchStore;

	CMenu m_popupMenu;						// template for the tracking CMFCPopupMenu (created on the fly)

	// base overrides:
protected:
	virtual void OnShowMenu( void ) override;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void On_ColorSelected( UINT selColorBtnId );
	afx_msg void On_UseColorTable( UINT selColorBtnId );
	afx_msg void OnUpdate_UseColorTable( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // ColorPickerButton_h
