#ifndef AlbumDialogBar_h
#define AlbumDialogBar_h
#pragma once

#include "utl/ui_fwd.h"


class CAlbumImageView;


class CAlbumDialogBar : public CDialogBar
{
public:
	CAlbumDialogBar( void );
	virtual ~CAlbumDialogBar();

	void InitAlbumImageView( CAlbumImageView* pAlbumView );

	void ShowBar( bool show );
	bool SetCurrentPos( int currIndex, bool forceLoad = false );
	bool InputSlideDelay( ui::ComboField byField );

	// events
	void OnCurrPosChanged( void );
	void OnNavRangeChanged( void );
	void OnSlideDelayChanged( void );
protected:
	void LayoutControls( void );
private:
	CComboBox m_slideDelayCombo;
	CEdit m_navEdit;
	CSpinButtonCtrl m_scrollSpin;
	CStatic m_infoStatic;
	CEdit m_fileNameEdit;

	CFont m_boldFont;
	CAlbumImageView* m_pAlbumView;
public:
	// generated stuff
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnOk( void );
	afx_msg void CmEscapeKey( void );
	afx_msg void OnCBnSelChange_SlideDelay( void );
	afx_msg void OnCBnCloseUp_SlideDelay( void );
	afx_msg void OnEnKillFocusCurrPos( void );
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor );
	afx_msg void OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg LRESULT HandleInitDialog( WPARAM wParam, LPARAM lParam );
	afx_msg BOOL OnToolTipText( UINT, NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnUpdate_SlideDelay( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumDialogBar_h
