#ifndef GeneralPage_h
#define GeneralPage_h
#pragma once

#include "utl/UI/LayoutPropertyPage.h"
#include "Observers.h"


enum PromptField { PF_None = -1, PF_Handle, PF_Class, PF_Caption, PF_ID, PF_Style, PF_StyleEx, PF_WindowRect, PF_ClientRect, PF_PastEnd };


class CGeneralPage : public CLayoutPropertyPage
				   , public IWndObserver
{
	friend class CInfoEditBox;
public:
	CGeneralPage( void );
	virtual ~CGeneralPage();

	bool DrillDownField( PromptField field );
private:
	// IWndObserver interface
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	bool IsFieldEditable( PromptField field ) const;
	void OnCaretLineChanged( int caretLine );

	CButton* GetButton( void ) { return &m_drillButton; }
private:
	PromptField m_promptField;
private:
	// enum { IDD = IDD_GENERAL_PAGE };
	std::auto_ptr< CInfoEditBox > m_pInfoEdit;
	CButton m_drillButton;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void CmFieldSetup( void );
	afx_msg HBRUSH OnCtlColor( CDC* dc, CWnd* pWnd, UINT ctlColor );

	DECLARE_MESSAGE_MAP()
};


#endif // GeneralPage_h
