#ifndef OptionsPage_h
#define OptionsPage_h
#pragma once

#include "utl/LayoutPropertyPage.h"
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
	CComboBox m_autoUpdateTargetCombo;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnFieldModified( void );
	afx_msg void OnFieldModified( UINT ctrlId );
	afx_msg void OnEnChange_AutoUpdateRate( void );
	afx_msg void OnSelChange_AutoUpdateTarget( void );

	DECLARE_MESSAGE_MAP()
};


#endif // OptionsPage_h
