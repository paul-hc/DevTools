#ifndef AlbumImageView_h
#define AlbumImageView_h
#pragma once

#include "ImageView.h"
#include "SlideData.h"
#include "utl/FlexPath.h"
#include "utl/UI/WindowTimer.h"


class CAlbumDoc;
class CAlbumThumbListView;
interface IAlbumBar;


class CAlbumImageView : public CImageView
{
	DECLARE_DYNCREATE( CAlbumImageView )

	CAlbumImageView( void );
	virtual ~CAlbumImageView();
public:
	void StorePeerView( CAlbumThumbListView* pPeerThumbView, IAlbumBar* pAlbumBar );

	CAlbumDoc* GetDocument( void ) const;

	const CSlideData& GetSlideData( void ) const { return m_slideData; }
	CSlideData* PtrSlideData( void ) { return &m_slideData; }

	CAlbumThumbListView* GetPeerThumbView( void ) const { return safe_ptr( m_pPeerThumbView ); }

	// base overrides
	virtual HICON GetDocTypeIcon( void ) const overrides(CImageView);
	virtual CMenu& GetDocContextMenu( void ) const overrides(CImageView);
	virtual CImageState* GetLoadingImageState( void ) const overrides(CImageView);

	// ui::IImageZoomView overrides
	virtual CWicImage* GetImage( void ) const overrides(CImageView);		// also an IImageView override
	virtual CWicImage* QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const overrides(CImageView);

	// IImageView overrides
	virtual fs::TImagePathKey GetImagePathKey( void ) const overrides(CImageView);
	virtual void EventChildFrameActivated( void ) overrides(CImageView);
protected:
	// base overrides
	virtual void OnImageContentChanged( void ) overrides(CImageView);
	virtual bool OutputNavigSlider( void ) overrides(CImageView);
	virtual void HandleNavigSliderPosChanging( int newPos, bool thumbTracking ) overrides(CImageView);
	virtual std::tstring FormatTipText_NavigSliderCtrl( void ) const overrides(CImageView);
	virtual bool CanEnterDragMode( void ) const overrides(CImageView);
public:
	// navigation support
	bool IsValidIndex( size_t index ) const;
	bool IsPlayOn( void ) const { return m_navTimer.IsStarted(); }
	bool IsDropTargetEnabled( void ) const { return m_isDropTargetEnabled; }

	// operations
	void LateInitialUpdate( void );				// public for PostCall
	bool UpdateImage( void );
	bool TogglePlay( bool doBeep = true );
	void SetSlideDelay( UINT slideDelay );

	void HandleNavTick( void );
	void NavigateTo( int pos, bool relative = false );
	void NavigateBy( int deltaPos ) { NavigateTo( deltaPos, true ); }

	// events
	void OnAlbumModelChanged( AlbumModelChange reason = AM_Init );
	void OnAutoDropRecipientChanged( void );
	void OnCurrPosChanged( bool alsoSliderCtrl = true );
	void OnSlideDataChanged( bool setToModified = true );
	void OnDocSlideDataChanged( void );
	void OnSelChangeThumbList( void );

	template< typename PathContainerT >
	bool QuerySelImagePaths( PathContainerT& rSelImagePaths ) const;
private:
	void UpdateChildBarsState( bool onInit = false );
	void RestartPlayTimer( void );

	void QueryNeighbouringPathKeys( std::vector<fs::TImagePathKey>& rNeighbourKeys ) const;
private:
	CSlideData m_slideData;
	CWindowTimer m_navTimer;
	bool m_isDropTargetEnabled;			// internal flag that synchronizes with DragAcceptFiles( TRUE/FALSE )

	CAlbumThumbListView* m_pPeerThumbView;
	IAlbumBar* m_pAlbumBar;

	enum { ID_NAVIGATION_TIMER = 4000 };

	// generated stuff
public:
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) overrides(CImageView);

	virtual void OnInitialUpdate( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnKeyDown( UINT chr, UINT repCnt, UINT vkFlags );
	afx_msg void OnTimer( UINT_PTR eventId );
	virtual BOOL OnMouseWheel( UINT mkFlags, short zDelta, CPoint point );
	afx_msg void On_EditAlbum( void );
	afx_msg void OnToggleSiblingView( void );
	afx_msg void OnUpdateSiblingView( CCmdUI* pCmdUI );
	afx_msg void OnToggle_NavigPlay( void );
	afx_msg void OnUpdate_NavigPlay( CCmdUI* pCmdUI );
	virtual void On_NavigSeek( UINT cmdId );
	virtual void OnUpdate_NavigSeek( CCmdUI* pCmdUI );
	afx_msg void OnRadio_NavigDirection( UINT cmdId );
	afx_msg void OnUpdate_NavigDirection( CCmdUI* pCmdUI );
	afx_msg void OnToggle_NavigWrapMode( void );
	afx_msg void OnUpdate_NavigWrapMode( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_NavigSliderCtrl( CCmdUI* pCmdUI );
	afx_msg void CmAutoDropImage( UINT cmdId );
	afx_msg void OnUpdateAutoDropImage( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


// CAlbumImageView template code

template< typename PathContainerT >
bool CAlbumImageView::QuerySelImagePaths( PathContainerT& rSelImagePaths ) const
{
	CListViewState files( StoreByString );
	m_pPeerThumbView->GetListViewState( files, false );

	utl::Assign( rSelImagePaths, files.m_pStringImpl->m_selItems, func::tor::StringOf() );
	return !rSelImagePaths.empty();
}


#endif // AlbumImageView_h
