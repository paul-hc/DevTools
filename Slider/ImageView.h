#ifndef ImageView_h
#define ImageView_h
#pragma once

#include "IImageView.h"
#include "utl/UI/AccelTable.h"
#include "utl/UI/ImageZoomViewD2D.h"
#include "utl/UI/ObjectCtrlBase.h"


class CImageState;
class CImageDoc;
class CChildFrame;
interface INavigationBar;
namespace fs { class CPath; }


class CImageView : public CImageZoomViewD2D
				 , public CObjectCtrlBase
				 , public IImageView
{
	typedef CImageZoomViewD2D BaseClass;
protected:
	DECLARE_DYNCREATE( CImageView )

	CImageView( void );
	virtual ~CImageView();
public:
	CImageDoc* GetDocument( void ) const;

	COLORREF GetRawBkColor( void ) const { return m_bkColor; }
	void SetBkColor( COLORREF bkColor, bool doRedraw = true );

	// base overrides
	virtual HICON GetDocTypeIcon( void ) const;
	virtual CMenu& GetDocContextMenu( void ) const;
	virtual COLORREF GetBkColor( void ) const;
	virtual bool IsAccented( void ) const;

	// IImageView interface
	virtual const fs::ImagePathKey& GetImagePathKey( void ) const;
	virtual CWicImage* GetImage( void ) const;
	virtual CScrollView* GetView( void );
	virtual void RegainFocus( RegainAction regainAction, int ctrlId = 0 );
	virtual void EventChildFrameActivated( void );
	virtual void EventNavigSliderPosChanged( bool thumbTracking );

	// overrideables
	virtual CImageState* GetLoadingImageState( void ) const;

	CWicImage* ForceReloadImage( void ) const;

	// view state persistence
	void MakeImageState( CImageState* pImageState ) const;
private:
	void RestoreState( const CImageState& loadingImageState );
protected:
	virtual void OnImageContentChanged( void );
	virtual bool OutputNavigSlider( void );
	virtual bool CanEnterDragMode( void ) const;
protected:
	INavigationBar* m_pNavigBar;
	CChildFrame* m_pMdiChildFrame;
private:
	COLORREF m_bkColor;							// self-encapsulated, call GetBkColor()
	static CAccelTable s_imageAccel;
public:
	// generated stuff
	public:
	virtual void OnInitialUpdate( void );		// first time after construct
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	protected:
	virtual void OnActivateView( BOOL bActivate, CView* pActivateView, CView* pDeactiveView );
	virtual void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
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

	afx_msg void OnEditCopy( void );
	afx_msg void OnUpdateEditCopy( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_NavigSliderCtrl( CCmdUI* pCmdUI );
	afx_msg void OnRadio_ImageScalingMode( UINT cmdId );
	afx_msg void OnUpdate_ImageScalingMode( CCmdUI* pCmdUI );
	afx_msg void On_ZoomNormal100( void );
	afx_msg void On_Zoom( UINT cmdId );
	afx_msg void CmResizeViewToFit( void );
	afx_msg void On_EditBkColor( void );
	afx_msg void CmExploreImage( void );
	afx_msg void OnUpdateAnyFileShellOperation( CCmdUI* pCmdUI );
	afx_msg void OnUpdatePhysicalFileShellOperation( CCmdUI* pCmdUI );
	virtual void CmDeleteFile( UINT cmdId );
	virtual void CmMoveFile( void );
	afx_msg void CmScroll( UINT cmdId );
	afx_msg void OnCBnSelChange_ImageScalingModeCombo( void );
	afx_msg void OnCBnSelChange_ZoomCombo( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageView_h
