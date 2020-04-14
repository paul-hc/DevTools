#ifndef OptionsPage_h
#define OptionsPage_h
#pragma once

#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/SpinEdit.h"
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
	CEnumComboBox m_frameStyleCombo;
	CSpinEdit m_frameSizeEdit;
	CEnumComboBox m_queryWndIconsCombo;
	CEnumComboBox m_auTargetCombo;
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
