#ifndef WorkspaceDialog_h
#define WorkspaceDialog_h
#pragma once

#include "utl/SpinEdit.h"
#include "Workspace.h"


class CWorkspaceDialog : public CDialog
{
public:
	CWorkspaceDialog( CWnd* pParent = NULL );
	virtual ~CWorkspaceDialog();
public:
	CWorkspaceData m_data;
	int m_thumbnailerFlags;
	bool m_enlargeSmoothing;
private:
	// enum { IDD = IDD_WORKSPACE_DIALOG };

	CComboBox m_autoImageSizeCombo;
	CSpinEdit m_mruCountEdit;
	CSpinEdit m_thumbListColCountEdit;
	CComboBox m_thumbBoundsSizeCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );			// DDX/DDV support
protected:
	afx_msg void OnDrawItem( int ctlId, DRAWITEMSTRUCT* pDIS );
	afx_msg void CmEditBkColor( void );
	afx_msg void OnSaveAndClose( void );
	afx_msg void CmEditImageSelColor( void );
	afx_msg void CmEditImageSelTextColor( void );
	afx_msg void CmClearThumbCache( void );

	DECLARE_MESSAGE_MAP()
};


#endif // WorkspaceDialog_h