#ifndef PathItemEdit_h
#define PathItemEdit_h
#pragma once

#include "ImageEdit.h"
#include "InternalChange.h"
#include "ObjectCtrlBase.h"
#include "ShellPidl.h"
#include "Dialog_fwd.h"		// for ui::ICustomCmdInfo interface


class CFileGlyphCustomDrawImager;


class CPathItemEdit : public CImageEdit
	, public CObjectCtrlBase
	, public ui::ICustomCmdInfo
{
public:
	CPathItemEdit( bool useDirPath = false );
	virtual ~CPathItemEdit();

	void SetUseDirPath( bool useDirPath ) { m_useDirPath = useDirPath; m_pathItem.SetUseDirPath( m_useDirPath ); }

	const shell::TPath& GetShellPath( void ) const { return m_pathItem.GetShellPath(); }
	const shell::CPidlAbsolute& GetPidl( void ) const { return m_pathItem.GetPidl(); }

	void SetShellPath( const shell::TPath& shellPath );
	void SetPidl( const shell::CPidlAbsolute& pidl );

	// base overrides
	virtual bool HasValidImage( void ) const override;
protected:
	virtual void UpdateControl( void ) override;
	virtual void DrawImage( CDC* pDC, const CRect& imageRect ) override;

	CMenu* GetPopupMenu( void );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const implement;
private:
	bool m_useDirPath;
	CPathPidlItem m_pathItem;
	fs::CPath m_evalFilePath;		// path with expanded environment varialed (if any)

	std::auto_ptr<CFileGlyphCustomDrawImager> m_pCustomImager;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg HBRUSH CtlColor( CDC* pDC, UINT ctlColor );
	virtual void OnEditCopy( void ) override;
	afx_msg void OnCopyFilename( void );
	afx_msg void OnCopyFolder( void );
	afx_msg void OnFileProperties( void );
	afx_msg void OnUpdateHasPath( CCmdUI* pCmdUI );
	afx_msg void OnUpdateHasAny( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // PathItemEdit_h
