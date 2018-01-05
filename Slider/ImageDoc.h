#ifndef ImageDoc_h
#define ImageDoc_h
#pragma once

#include "utl/ImagePathKey.h"
#include "DocumentBase.h"


class CWicImage;


class CImageDoc : public CDocumentBase
{
protected:
	DECLARE_DYNCREATE( CImageDoc )
public:
	CImageDoc( void );
	virtual ~CImageDoc();

	CWicImage* GetImage( void ) const;
public:
	fs::ImagePathKey m_imagePathKey;			// path, frame
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
