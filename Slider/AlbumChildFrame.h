#ifndef AlbumChildFrame_h
#define AlbumChildFrame_h
#pragma once

#include "ChildFrame.h"
#include "SplitterWindow.h"
#include "INavigationBar.h"			// for IAlbumBar
#include "utl/UI/Dialog_fwd.h"		// for ui::ICustomCmdInfo
#include "utl/UI/ui_fwd.h"


class CAlbumThumbListView;
class CAlbumImageView;
namespace mfc { class CFixedToolBar; }


class CAlbumChildFrame : public CChildFrame
	, public IAlbumBar
	, public ui::ICustomCmdInfo
{
	DECLARE_DYNCREATE( CAlbumChildFrame )
protected:
	CAlbumChildFrame( void );
	virtual ~CAlbumChildFrame();
public:
	// base overrides
	virtual IImageView* GetImageView( void ) const override;		// could be either CImageView or CAlbumImageView (but not CAlbumThumbListView!)

	// view panes
	CAlbumThumbListView* GetThumbView( void ) const { return safe_ptr( m_pThumbsListView ); }
	CAlbumImageView* GetAlbumImageView( void ) const { return safe_ptr( m_pAlbumImageView ); }

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	// IAlbumBar interface
	virtual void ShowBar( bool show ) implement;
	virtual void OnCurrPosChanged( void ) implement;
	virtual void OnNavRangeChanged( void ) implement;
	virtual void OnSlideDelayChanged( void ) implement;

	bool InputSlideDelay( ui::ComboField byField );
	bool InputCurrentPos( void );

	void BuildAlbumToolbar( void );
private:
	enum SplitterPane { ThumbView, PictureView };

	std::auto_ptr<mfc::CFixedToolBar> m_pAlbumToolBar;

	CSplitterWindow m_splitterWnd;

	CAlbumThumbListView* m_pThumbsListView;
	CAlbumImageView* m_pAlbumImageView;
	bool m_doneInit;

	enum { DurationComboWidth = 70, SeekCurrPosSpinEditWidth = 60 };

	// generated stuff
protected:
	virtual BOOL OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext ) overrides(CChildFrame);
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );
	afx_msg void OnToggle_ViewAlbumPane( void );
	afx_msg void OnUpdate_ViewAlbumPane( CCmdUI* pCmdUI );
	afx_msg void OnEditInput_PlayDelayCombo( void );
	afx_msg void OnCBnSelChange_PlayDelayCombo( void );
	afx_msg void OnToggle_AutoSeekImagePos( void );
	afx_msg void OnUpdate_AutoSeekImagePos( CCmdUI* pCmdUI );
	afx_msg void OnEnUpdate_SeekCurrPosSpinEdit( void );
	afx_msg void On_SeekCurrPosSpinEdit( void );
	afx_msg void On_CopyCurrImagePath( void );
	afx_msg void OnUpdateAlways( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumChildFrame_h
