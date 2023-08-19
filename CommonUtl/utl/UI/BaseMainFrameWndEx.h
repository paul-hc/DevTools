#ifndef BaseMainFrameWndEx_h
#define BaseMainFrameWndEx_h
#pragma once


template< typename BaseFrameWnd >
abstract class CBaseMainFrameWndEx : public BaseFrameWnd		// abstract base for main windows that customize application look, control bars, etc - e.g. BaseFrameWnd is CMDIFrameWndEx
{
	typedef BaseFrameWnd TBaseClass;
protected:
	CBaseMainFrameWndEx( void ) : BaseFrameWnd() {}
	virtual ~CBaseMainFrameWndEx();
public:
	enum { MaxUserToolbars = 10, FirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40, LastUserToolBarId = FirstUserToolBarId + MaxUserToolbars - 1 };

	// generated stuff
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnWindowManager( void );
	afx_msg LRESULT OnToolbarCreateNew( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseMainFrameWndEx_h
