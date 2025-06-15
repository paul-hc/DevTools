#ifndef BaseMainFrameWndEx_h
#define BaseMainFrameWndEx_h
#pragma once


template< typename BaseFrameWndT >
abstract class CBaseMainFrameWndEx : public BaseFrameWndT		// abstract base for main windows that customize application look, control bars, etc - e.g. BaseFrameWndT is CMDIFrameWndEx
{
	typedef BaseFrameWndT TBaseClass;
protected:
	CBaseMainFrameWndEx( void ) : BaseFrameWndT() {}
	virtual ~CBaseMainFrameWndEx();
public:
	enum { MaxUserToolbars = 10, FirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40, LastUserToolBarId = FirstUserToolBarId + MaxUserToolbars - 1 };

	// generated stuff
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& cs );
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	afx_msg void OnWindowManager( void );
	afx_msg LRESULT OnToolbarCreateNew( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseMainFrameWndEx_h
