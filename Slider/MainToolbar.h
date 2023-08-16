#ifndef MainToolbar_h
#define MainToolbar_h
#pragma once

#include "utl/UI/ui_fwd.h"
#include "utl/UI/IZoomBar.h"
#include "utl/UI/ToolbarStrip.h"
#include "INavigationBar.h"


class CEnumComboBox;
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
	virtual bool OutputScalingMode( ui::ImageScalingMode scalingMode );
	virtual ui::ImageScalingMode InputScalingMode( void ) const;
	virtual bool OutputZoomPct( UINT zoomPct );
	virtual UINT InputZoomPct( ui::ComboField byField ) const;		// return 0 on error

	// INavigationBar interface
	virtual bool OutputNavigRange( UINT imageCount );
	virtual bool OutputNavigPos( int imagePos );
	virtual int InputNavigPos( void ) const;
private:
	enum ControlHorizPadding { PadLeft = 2, PadRight = 5 };

	template< typename CtrlType >
	void CreateBarCtrl( CtrlType* pCtrl, UINT ctrlId, DWORD style, int width, int padLeft = PadLeft, int padRight = PadRight );

	template< typename CtrlType >
	bool CreateControl( CtrlType* pCtrl, UINT ctrlId, DWORD style, const CRect& ctrlRect );
private:
	std::auto_ptr<CEnumComboBox> m_pScalingCombo;
	std::auto_ptr<CZoomComboBox> m_pZoomCombo;
	CButton m_smoothCheck;
	CSliderCtrl m_navigSliderCtrl;
	CFont m_ctrlFont;

	static const UINT s_buttons[];

	// generated stuff
protected:
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnOk( void );
	afx_msg BOOL On_EscapeKey( UINT cmdId );
	afx_msg void On_FocusOnZoomCombo( void );
	afx_msg void OnCBnCloseUp_ZoomCombo( void );

	// navigation bar:
	afx_msg void OnHScroll( UINT sbCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void On_FocusOnSliderCtrl( void );
	afx_msg BOOL OnToolTipText_NavigSliderCtrl( UINT, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


#endif // MainToolbar_h
