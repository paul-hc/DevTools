#ifndef PathItemEdit_h
#define PathItemEdit_h
#pragma once

#include "ImageEdit.h"
#include "InternalChange.h"
#include "ObjectCtrlBase.h"
#include "utl/PathItemBase.h"


class CFileGlyphCustomDrawImager;


class CPathItemEdit : public CImageEdit
	, public CObjectCtrlBase
{
public:
	CPathItemEdit( void );
	virtual ~CPathItemEdit();

	const fs::CPath& GetFilePath( void ) const { return m_pathItem.GetFilePath(); }
	void SetFilePath( const fs::CPath& filePath );

	// base overrides
	virtual bool HasValidImage( void ) const override;
protected:
	virtual void DrawImage( CDC* pDC, const CRect& imageRect ) override;

	CMenu* GetPopupMenu( void );
private:
	CPathItem m_pathItem;

	std::auto_ptr<CFileGlyphCustomDrawImager> m_pCustomImager;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg HBRUSH CtlColor( CDC* pDC, UINT ctlColor );
	afx_msg void OnCopyFilename( void );
	afx_msg void OnCopyFolder( void );
	afx_msg void OnFileProperties( void );
	afx_msg void OnUpdateHasPath( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // PathItemEdit_h
