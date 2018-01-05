#ifndef AlbumChildFrame_h
#define AlbumChildFrame_h
#pragma once

#include "ChildFrame.h"
#include "AlbumDialogBar.h"
#include "SplitterWindow.h"


class CAlbumThumbListView;
class CAlbumImageView;


class CAlbumChildFrame : public CChildFrame
{
	DECLARE_DYNCREATE( CAlbumChildFrame )
protected:
	CAlbumChildFrame( void );
	virtual ~CAlbumChildFrame();
public:
	// base overrides
	virtual IImageView* GetImageView( void ) const;		// could be either CImageView or CAlbumImageView (but not CAlbumThumbListView!)

	// view panes
	CAlbumThumbListView* GetThumbView( void ) const { return safe_ptr( m_pThumbsListView ); }
	CAlbumImageView* GetAlbumImageView( void ) const { return safe_ptr( m_pAlbumImageView ); }
private:
	enum SplitterPane { ThumbView, PictureView };

	CAlbumDialogBar m_albumInfoBar;
	CSplitterWindow m_splitterWnd;

	CAlbumThumbListView* m_pThumbsListView;
	CAlbumImageView* m_pAlbumImageView;
public:
	// generated stuff
	protected:
	virtual BOOL OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );
	afx_msg BOOL OnBarCheck( UINT dlgBarId );
	afx_msg void OnUpdateBarCheck( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumChildFrame_h
