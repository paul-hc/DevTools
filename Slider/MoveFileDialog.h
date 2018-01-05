#ifndef MoveFileDialog_h
#define MoveFileDialog_h
#pragma once

#include "utl/Path.h"
#include "utl/LayoutDialog.h"


class CItemContentHistoryCombo;


class CMoveFileDialog : public CLayoutDialog
{
public:
	CMoveFileDialog( const std::vector< std::tstring >& filesToMove, CWnd* pParent = NULL );
	virtual ~CMoveFileDialog();
public:
	fs::CPath m_destFolderPath;
private:
	std::vector< fs::CPath > m_filesToMove;
private:
	// enum { IDD = IDD_FILE_MOVE_DIALOG };

	std::auto_ptr< CItemContentHistoryCombo > m_pDestFolderCombo;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnCBnEditChangeDestFolder( void );
	afx_msg void OnCBnSelChangeDestFolder( void );
	afx_msg void OnDropFiles( HDROP hDropInfo );

	DECLARE_MESSAGE_MAP()
};


#endif // MoveFileDialog_h
