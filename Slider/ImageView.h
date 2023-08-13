#ifndef ImageView_h
#define ImageView_h
#pragma once

#include "utl/Path_fwd.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/ImageZoomViewD2D.h"
#include "utl/UI/ObjectCtrlBase.h"
#include "ImageNavigator_fwd.h"
#include "IImageView.h"


class CImageState;
class CImageDoc;
class CChildFrame;
interface INavigationBar;


class CImageView : public CImageZoomViewD2D
	, public CObjectCtrlBase
	, public IImageView
{
	typedef CImageZoomViewD2D TBaseClass;
protected:
	DECLARE_DYNCREATE( CImageView )

	CImageView( void );
	virtual ~CImageView();
public:
	CImageDoc* GetDocument( void ) const;

	COLORREF GetRawBkColor( void ) const { return m_bkColor; }
	void SetBkColor( COLORREF bkColor, bool doRedraw = true );

	// overridables
	virtual HICON GetDocTypeIcon( void ) const;
	virtual CMenu& GetDocContextMenu( void ) const;

	// ui::IZoomView interface
	virtual ui::TDisplayColor GetBkColor( void ) const implements(ui::IZoomView);

	// ui::IImageZoomView interface (inherited from CImageZoomViewD2D)
	virtual CWicImage* GetImage( void ) const implements(ui::IImageZoomView, IImageView);
	virtual CWicImage* QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const implements(ui::IImageZoomView);

	// IImageView interface
	virtual fs::TImagePathKey GetImagePathKey( void ) const implements(IImageView);
	virtual CScrollView* GetScrollView( void ) implements(IImageView);
	virtual void RegainFocus( RegainAction regainAction, int ctrlId = 0 ) implements(IImageView);
	virtual void EventChildFrameActivated( void ) implements(IImageView);
	virtual void EventNavigSliderPosChanged( bool thumbTracking ) implements(IImageView);

	// overrideables
	virtual CImageState* GetLoadingImageState( void ) const;

	CWicImage* ForceReloadImage( void );

	// view state persistence
	void MakeImageState( CImageState* pImageState ) const;
private:
	void RestoreState( const CImageState& loadingImageState );
protected:
	virtual void OnImageContentChanged( void );
	virtual bool OutputNavigSlider( void );
	virtual bool CanEnterDragMode( void ) const;

	static nav::Navigate CmdToNavigate( UINT cmdId );
private:
	UINT m_imageFramePos;						// only used for single-image view (not by album view sub-class)
	COLORREF m_bkColor;							// self-encapsulated, call GetBkColor()

	bool m_initialized;							// false, flipped to true on OnInitialUpdate

	static CAccelTable s_imageAccel;
protected:
	INavigationBar* m_pNavigBar;
	CChildFrame* m_pMdiChildFrame;

	// generated stuff
public:
	virtual void OnInitialUpdate( void );		// first time after construct
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual void OnActivateView( BOOL bActivate, CView* pActivateView, CView* pDeactiveView ) overrides(CView);
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint ) overrides(CView);
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnLButtonDown( UINT mkFlags, CPoint point );
	afx_msg void OnLButtonDblClk( UINT mkFlags, CPoint point );
	afx_msg void OnMButtonDown( UINT mkFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT mkFlags, CPoint point );
	virtual BOOL OnMouseWheel( UINT mkFlags, short zDelta, CPoint point );

	virtual void On_NavigSeek( UINT cmdId );
	virtual void OnUpdate_NavigSeek( CCmdUI* pCmdUI );
	afx_msg void OnEditCopy( void );
	afx_msg void OnUpdateEditCopy( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_NavigSliderCtrl( CCmdUI* pCmdUI );
	afx_msg void OnRadio_ImageScalingMode( UINT cmdId );
	afx_msg void OnUpdate_ImageScalingMode( CCmdUI* pCmdUI );
	afx_msg void On_ZoomNormal100( void );
	afx_msg void On_Zoom( UINT cmdId );
	afx_msg void CmResizeViewToFit( void );
	afx_msg void On_EditBkColor( void );
	afx_msg void CmScroll( UINT cmdId );
	afx_msg void OnCBnSelChange_ImageScalingModeCombo( void );
	afx_msg void OnCBnSelChange_ZoomCombo( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageView_h
