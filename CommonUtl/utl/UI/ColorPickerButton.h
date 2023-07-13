#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include "AccelTable.h"
#include "Dialog_fwd.h"
#include "StdColors.h"
#include "utl/Registry_fwd.h"
#include <afxcolorbutton.h>


class CColorTable;


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

	void DDX_Color( CDataExchange* pDX, int ctrlId, COLORREF* pColor );
private:
	void Construct( void );
	bool IsEmpty( void ) const { return m_Colors.IsEmpty() && m_lstDocColors.IsEmpty(); }
	void AddColorTablesSubMenu( CMenu* pContextMenu );

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	static void RegisterColorNames( const CColorTable* pColorTable );
private:
	const CColorTable* m_pColorTable;
	size_t m_halftoneSize;
	bool m_useUserColors;					// neither from color tables, nor the halftone palette

	std::tstring m_regSection;
	CAccelTable m_accel;

	static std::vector<CColorPickerButton*> s_instances;		// for color table notifications
protected:
	void NotifyColorSetChanged( void );

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
	virtual void OnShowMenu( void );

	// generated stuff
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


#endif // ColorPickerButton_h
