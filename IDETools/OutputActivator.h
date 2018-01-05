#ifndef OutputActivator_h
#define OutputActivator_h
#pragma once


class OutputActivator: public CWinThread
{
public:
	OutputActivator( HWND _hWndOutput, HWND _hWndTab, const TCHAR* _tabCaption );
	virtual ~OutputActivator();

	virtual BOOL InitInstance( void );
	virtual int Run( void );
private:
	void waitForCaptionChange( const TCHAR* origCaption );
private:
	HWND hWndOutput, hWndTab;
	CString tabCaption;
};


#endif // OutputActivator_h
