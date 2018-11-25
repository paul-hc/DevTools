#ifndef OptionsPage_h
#define OptionsPage_h
#pragma once

#include "utl/LayoutPropertyPage.h"
#include "utl/SpinEdit.h"
#include "Observers.h"


struct COptions;


class COptionsPage : public CLayoutPropertyPage
				   , public IEventObserver
{
public:
	COptionsPage( void );
	virtual ~COptionsPage();

	// IEventObserver interface
	virtual void OnAppEvent( app::Event appEvent );
	virtual bool CanNotify( void ) const { return true; }		// allow saving options even when page not created
private:
	COptions* m_pOptions;

	// enum { IDD = IDD_OPTIONS_PAGE };
	CComboBox m_frameStyleCombo;
	CSpinEdit m_frameSizeEdit;
	CComboBox m_queryWndIconsCombo;
	CComboBox m_auTargetCombo;
	CSpinEdit m_auTimeoutEdit;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnCbnSelchange_FrameStyle( void );
	afx_msg void OnEnChange_FrameSize( void );
	afx_msg void OnCbnSelchange_QueryWndIcons( void );
	afx_msg void OnCbnSelchange_AutoUpdateTarget( void );
	afx_msg void OnEnChange_AutoUpdateRate( void );

	DECLARE_MESSAGE_MAP()
};


#endif // OptionsPage_h
