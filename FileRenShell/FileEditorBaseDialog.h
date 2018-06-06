#ifndef FileEditorBaseDialog_h
#define FileEditorBaseDialog_h
#pragma once

#include "IFileEditor.h"
#include "utl/BaseMainDialog.h"


class CFileModel;
class CPathItemBase;


abstract class CFileEditorBaseDialog : public CBaseMainDialog
									 , public IFileEditor
{
protected:
	CFileEditorBaseDialog( CFileModel* pFileModel, cmd::CommandType nativeCmdType, UINT templateId, CWnd* pParent );
	virtual ~CFileEditorBaseDialog();

	// IFileEditor interface (partial)
	virtual CFileModel* GetFileModel( void ) const;
	virtual CDialog* GetDialog( void );

	// ui::ICmdCallback interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	int PopStackRunCrossEditor( cmd::StackType stackType );
protected:
	CFileModel* m_pFileModel;
	cmd::CommandType m_nativeCmdType;
	std::vector< CPathItemBase* > m_errorItems;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnBnClicked_UndoRedo( UINT btnId );

	DECLARE_MESSAGE_MAP()
};


#endif // FileEditorBaseDialog_h
