#ifndef ColorPickerButton_h
#define ColorPickerButton_h
#pragma once

#include "AccelTable.h"
#include "Color.h"
#include "Dialog_fwd.h"
#include "PopupMenus_fwd.h"
#include "StdColors.h"
#include "utl/Registry_fwd.h"
#include <afxcolorbutton.h>


namespace ui
{
	template< typename ColorCtrlT >
	void DDX_ColorButton( CDataExchange* pDX, int ctrlId, ColorCtrlT& rCtrl, COLORREF* pColor, bool evalColor = false );

	void DDX_ColorText( CDataExchange* pDX, int ctrlId, COLORREF* pColor, bool doInput = false );
	void DDX_ColorRepoText( CDataExchange* pDX, int ctrlId, COLORREF color );
}


class CColorTable;
class CColorStore;
class CScratchColorStore;
namespace mfc { class CColorMenuButton; }

class CColorMenuTrackingImpl;


class CColorPickerButton : public CMFCColorButton
	, public ui::IColorEditorHost
	, public ui::ICustomCmdInfo
{
public:
	CColorPickerButton( const CColorTable* pSelColorTable = nullptr );
	virtual ~CColorPickerButton();

	void SetRegSection( const std::tstring& regSection ) { m_regSection = regSection; }

	const CColorStore* GetMainStore( void ) const;
	void SetMainStore( const CColorStore* pMainStore );

	void SetAutomaticColor( COLORREF autoColor, const TCHAR autoLabel[] = mfc::CColorLabels::s_autoLabel ) { EnableAutomaticButton( autoLabel, autoColor ); }
	void SetDocColorTable( const CColorTable* pDocColorTable );		// additional 'Document' section of colors (below the main colors)
	void SetUserColors( const std::vector<COLORREF>& userColors, int columnCount = 0 );		// custom colors, not based on a color table

	enum PickingMode { PickColorBar, PickMenuColorTables };

	PickingMode GetPickingMode( void ) const { return m_pickingMode; }
	void SetPickingMode( PickingMode pickingMode ) { m_pickingMode = pickingMode; }

	// ui::IColorHost interface
	virtual COLORREF GetColor( void ) const override { return CMFCColorButton::GetColor(); }
	virtual const CColorEntry* GetRawColor( void ) const override;
	virtual COLORREF GetAutoColor( void ) const override { return GetAutomaticColor(); }

	virtual const CColorTable* GetSelColorTable( void ) const override { return m_pSelColorTable; }
	virtual const CColorTable* GetDocColorTable( void ) const override { return m_pDocColorTable; }
	virtual bool UseUserColors( void ) const override;

	// ui::IColorEditorHost interface
	virtual CWnd* GetHostWindow( void ) const override { return const_cast<CColorPickerButton*>( this ); }
	virtual void SetColor( COLORREF rawColor, bool notify = false ) override;
	virtual void SetSelColorTable( const CColorTable* pSelColorTable ) override;

	enum TrackingMode { NoTracking, TrackingColorBar, TrackingMenuColorTables, TrackingContextMenu };
protected:
	void UpdateShadesTable( void );
private:
	bool IsEmpty( void ) const { return m_Colors.IsEmpty() && m_lstDocColors.IsEmpty(); }

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	enum ChangedField { SelColorTableChanged, PickingModeChanged };
	void NotifyMatchingPickers( ChangedField field );

	void ShowColorTablePopup( void );
	void TrackMenuColorTables( void );

	UINT TrackModalPopupImpl( HMENU hMenuPopup, CMFCPopupMenu* pPopupMenu, bool sendCommand, CPoint screenPos = CPoint( -1, -1 ) );		// returns the command
private:
	persist const CColorTable* m_pSelColorTable;
	persist PickingMode m_pickingMode;						// by default PickColorBar (single color table), or PickMenuColorTables (shows a menu with multiple color tables)

	const CColorEntry* m_pSelColorEntry;					// raw color entry
	const CColorTable* m_pDocColorTable;

	std::tstring m_regSection;
	CAccelTable m_accel;

	std::auto_ptr<CColorMenuTrackingImpl> m_pMenuImpl;		// a CCmdTarget that manages set up of the popup menu, and has the color stores

	TrackingMode m_trackingMode;

	static std::vector<CColorPickerButton*> s_instances;	// for color table notifications
protected:
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override;

	// base overrides:
	virtual void UpdateColor( COLORREF color ) overrides( CMFCColorButton );
	virtual void OnShowColorPopup( void ) overrides( CMFCColorButton );
	virtual void OnDraw( CDC* pDC, const CRect& rect, UINT uiState ) overrides( CMFCColorButton );

	// generated stuff
public:
	virtual void PreSubclassWindow( void ) override;
	virtual BOOL PreTranslateMessage( MSG* pMsg ) override;
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) override;
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnCopy( void );
	afx_msg void OnPaste( void );
	afx_msg void OnUpdatePaste( CCmdUI* pCmdUI );
	afx_msg void On_ResetColor( void );
	afx_msg void OnUpdate_ResetColor( CCmdUI* pCmdUI );
	afx_msg void On_CopyColorTable( void );
	afx_msg void On_SelectColorTable( UINT colorTableId );
	afx_msg void OnUpdate_SelectColorTable( CCmdUI* pCmdUI );
	afx_msg void On_PickingMode( UINT pickRadioId );
	afx_msg void OnUpdate_PickingMode( CCmdUI* pCmdUI );
	afx_msg void OnUpdateEnable( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


// template code

namespace ui
{
	template< typename ColorCtrlT >
	void DDX_ColorButton( CDataExchange* pDX, int ctrlId, ColorCtrlT& rCtrl, COLORREF* pColor, bool evalColor /*= false*/ )
	{	// pass evalColor: true for CMFCColorButton, but false for CColorPickerButton (which manages well logical colors)
		::DDX_Control( pDX, ctrlId, rCtrl );

		if ( pColor != nullptr )
			if ( DialogOutput == pDX->m_bSaveAndValidate )
				rCtrl.SetColor( ui::EvalColorIf( *pColor, evalColor ) );
			else
				*pColor = ui::EvalColorIf( rCtrl.GetColor(), evalColor );
	}
}


#endif // ColorPickerButton_h
