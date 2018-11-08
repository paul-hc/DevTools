#pragma once


class CMainFrame : public CFrameWnd
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();

	// generated stuff
protected:
	virtual BOOL OnCreateClient( CREATESTRUCT* pCs, CCreateContext* pContext );
protected:
	DECLARE_MESSAGE_MAP()
};
