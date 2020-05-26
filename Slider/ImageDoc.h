#ifndef ImageDoc_h
#define ImageDoc_h
#pragma once

#include "DocumentBase.h"


class CImageDoc : public CDocumentBase
{
protected:
	DECLARE_DYNCREATE( CImageDoc )
public:
	CImageDoc( void );
	virtual ~CImageDoc();

	CWicImage* GetImage( UINT framePos ) const;
	const fs::CFlexPath& GetImagePath( void ) const { return m_imagePath; }
private:
	fs::CFlexPath m_imagePath;
public:
	// generated stuff
	public:
	virtual BOOL OnOpenDocument( LPCTSTR pFilePath );
	virtual BOOL OnSaveDocument( LPCTSTR pFilePath );
	protected:
	virtual BOOL OnNewDocument( void );
protected:
	afx_msg void OnUpdateFileSave( CCmdUI* pCmdUI );
	afx_msg void OnUpdateFileSaveAs( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageDoc_h
