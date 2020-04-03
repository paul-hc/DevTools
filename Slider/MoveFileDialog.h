#ifndef MoveFileDialog_h
#define MoveFileDialog_h
#pragma once

#include "utl/Path.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/TextEdit.h"


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
	CTextEdit m_srcFilesEdit;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );	// DDX/DDV support
protected:
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnCBnEditChange_DestFolder( void );
	afx_msg void OnCBnSelChange_DestFolder( void );
	afx_msg void OnCnDetailsChanged_DestFolder( void );

	DECLARE_MESSAGE_MAP()
};


#endif // MoveFileDialog_h
