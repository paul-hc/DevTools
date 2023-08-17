#ifndef BaseMainFrameWndEx_h
#define BaseMainFrameWndEx_h
#pragma once


template< typename BaseFrameWnd >
abstract class CBaseMainFrameWndEx : public BaseFrameWnd		// abstract base for main windows that customize application look, control bars, etc
{
	typedef BaseFrameWnd TBaseClass;
protected:
	CBaseMainFrameWndEx( void ) : BaseFrameWnd() {}
	virtual ~CBaseMainFrameWndEx();
public:

	// generated stuff
protected:
	afx_msg void OnWindowManager( void );
	afx_msg void OnViewCustomize( void );
	afx_msg LRESULT OnToolbarCreateNew( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseMainFrameWndEx_h
