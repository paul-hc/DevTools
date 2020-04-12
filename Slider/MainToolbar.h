#ifndef MainToolbar_h
#define MainToolbar_h
#pragma once

#include "utl/UI/ui_fwd.h"
#include "utl/UI/IZoomBar.h"
#include "utl/UI/ToolbarStrip.h"
#include "INavigationBar.h"


class CZoomComboBox;


class CMainToolbar : public CToolbarStrip
				   , public ui::IZoomBar
				   , public INavigationBar
{
public:
	CMainToolbar( void );
	virtual ~CMainToolbar();

	bool InitToolbar( void );
	bool HandleCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );		// select cmdIds

	// ui::IZoomBar interface
	virtual bool OutputAutoSize( ui::AutoImageSize autoImageSize );
	virtual ui::AutoImageSize InputAutoSize( void ) const;
	virtual bool OutputZoomPct( UINT zoomPct );
	virtual UINT InputZoomPct( ui::ComboField byField ) const;		// return 0 on error

	// INavigationBar interface
	virtual bool OutputNavigRange( UINT imageCount );
	virtual bool OutputNavigPos( int imagePos );
	virtual int InputNavigPos( void ) const;
private:
	template< typename CtrlType >
	bool CreateBarCtrl( CtrlType* pCtrl, UINT ctrlId, DWORD style, int width, UINT tbButtonStyle = TBBS_SEPARATOR );

	template< typename CtrlType >
	bool CreateControl( CtrlType* pCtrl, UINT ctrlId, DWORD style, const CRect& ctrlRect );
private:
	CComboBox m_autoImageSizeCombo;
	std::auto_ptr< CZoomComboBox > m_pZoomCombo;
	CButton m_smoothCheck;
	CSliderCtrl m_navigSlider;
	CFont m_ctrlFont;

	static const UINT s_buttons[];
public:
	// generated stuff
protected:
	afx_msg void OnHScroll( UINT sbCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnOk( void );
	afx_msg BOOL CmEscapeKey( UINT cmdId );
	afx_msg void CmFocusZoom( void );
	afx_msg void CmFocusSlider( void );
	afx_msg void OnCBnCloseUp_ZoomCombo( void );
	afx_msg BOOL OnToolTipText_NavigSlider( UINT, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // MainToolbar_h
