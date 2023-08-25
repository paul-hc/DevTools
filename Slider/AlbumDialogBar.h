#ifndef AlbumDialogBar_h
#define AlbumDialogBar_h
#pragma once

#include "utl/UI/ui_fwd.h"
#include "utl/UI/LayoutPaneDialog.h"


class CAlbumImageView;
class CDialogToolBar;
class CDurationComboBox;
class CTextEdit;


class CAlbumDialogPane : public CLayoutPaneDialog
{
public:
	CAlbumDialogPane( void );
	virtual ~CAlbumDialogPane();

	void InitAlbumImageView( CAlbumImageView* pAlbumView );

	bool SetCurrentPos( int currIndex, bool forceLoad = false );
	bool InputSlideDelay( ui::ComboField byField );

	// events
	void OnCurrPosChanged( void );
	void OnNavRangeChanged( void );
	void OnSlideDelayChanged( void );

	// base overrides
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const overrides(CLayoutPaneDialog);
private:
	std::auto_ptr<CDialogToolBar> m_pToolbar;
	std::auto_ptr<CDurationComboBox> m_pSlideDelayCombo;
	CEdit m_navPosEdit;
	CSpinButtonCtrl m_scrollSpin;
	CStatic m_infoStatic;
	std::auto_ptr<CTextEdit> m_pImagePathEdit;

	CFont m_boldFont;
	CAlbumImageView* m_pAlbumView;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor );
	afx_msg void OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg void OnCBnSelChange_SlideDelay( void );
	afx_msg void OnCBnCloseUp_SlideDelay( void );
	afx_msg void OnCBnInputError_SlideDelay( void );
	afx_msg void OnUpdate_SlideDelay( CCmdUI* pCmdUI );
	afx_msg void OnEnKillFocus_SeekCurrPos( void );
	afx_msg void OnOk( void );
	afx_msg void On_EscapeKey( void );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumDialogBar_h
