#ifndef ImageDoc_h
#define ImageDoc_h
#pragma once

#include "DocumentBase.h"
#include "CatalogStorageHost.h"


class CImageDoc : public CDocumentBase
{
protected:
	DECLARE_DYNCREATE( CImageDoc )
public:
	CImageDoc( void );
	virtual ~CImageDoc();

	CWicImage* GetImage( UINT framePos ) const;
	const fs::CFlexPath& GetImagePath( void ) const { return m_imagePath; }
protected:
	// base overrides
	virtual CWicImage* GetCurrentImage( void ) const;
	virtual bool QuerySelectedImagePaths( std::vector< fs::CFlexPath >& rSelImagePaths ) const;
private:
	fs::CFlexPath m_imagePath;
	CCatalogStorageHost m_storageHost;		// holds the storage if image is embedded

	// generated stuff
public:
	virtual BOOL OnOpenDocument( LPCTSTR pFilePath );
	virtual BOOL OnSaveDocument( LPCTSTR pFilePath );
protected:
	virtual BOOL OnNewDocument( void );
protected:
	afx_msg void OnUpdateFileSave( CCmdUI* pCmdUI );
	afx_msg void OnUpdateFileSaveAs( CCmdUI* pCmdUI );
	afx_msg void On_ImageDelete( void );
	afx_msg void On_ImageMove( void );
	afx_msg void OnUpdate_AlterPhysicalImageFile( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // ImageDoc_h
