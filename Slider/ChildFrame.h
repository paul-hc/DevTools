#ifndef ChildFrame_h
#define ChildFrame_h
#pragma once


interface IImageView;
class CAccelTable;


class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE( CChildFrame )
public:
	CChildFrame( void );
	virtual ~CChildFrame();

	virtual IImageView* GetImageView( void ) const;		// CImageView or CAlbumImageView
private:
	IImageView* m_pImageView;
	const CAccelTable* m_pImageAccel;
public:
	// generated stuff
	protected:
	virtual BOOL OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext );
	public:
	virtual BOOL PreCreateWindow( CREATESTRUCT& rCS );
	virtual void ActivateFrame( int cmdShow = -1 );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnWindowPosChanging( WINDOWPOS* pWndPos );
	afx_msg void OnNcLButtonDblClk( UINT hitTest, CPoint point );

	DECLARE_MESSAGE_MAP()
};


#endif // ChildFrame_h
