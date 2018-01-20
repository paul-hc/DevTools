#pragma once


class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE( CChildFrame )
public:
	CChildFrame( void );
	virtual ~CChildFrame();
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	DECLARE_MESSAGE_MAP()
};
