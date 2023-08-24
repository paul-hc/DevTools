#ifndef ChildFrame_h
#define ChildFrame_h
#pragma once


interface IImageView;
class CAccelTable;


class CChildFrame : public CMDIChildWndEx
{
	DECLARE_DYNCREATE( CChildFrame )
public:
	CChildFrame( void );
	virtual ~CChildFrame();

	virtual IImageView* GetImageView( void ) const;		// CImageView or CAlbumImageView
private:
	IImageView* m_pImageView;
	const CAccelTable* m_pImageAccel;

	// generated stuff
protected:
	virtual BOOL OnCreateClient( CREATESTRUCT* pCS, CCreateContext* pContext ) overrides(CFrameWnd);
public:
	virtual void ActivateFrame( int cmdShow = -1 ) override;
	virtual BOOL PreTranslateMessage( MSG* pMsg ) override;
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnWindowPosChanging( WINDOWPOS* pWndPos );
	afx_msg void OnNcLButtonDblClk( UINT hitTest, CPoint point );

	DECLARE_MESSAGE_MAP()
};


#endif // ChildFrame_h
