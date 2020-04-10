#ifndef SearchImagesProgressCallback_h
#define SearchImagesProgressCallback_h
#pragma once

#include "utl/FileSystem_fwd.h"
#include "utl/UI/ProgressDialog.h"


class CSearchImagesProgressCallback : private fs::IEnumerator
									, private utl::noncopyable

{
public:
	CSearchImagesProgressCallback( CWnd* pParent );
	~CSearchImagesProgressCallback();

	ui::IProgressCallback* GetProgress( void ) { return &m_dlg; }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }
private:
	// file enumerator callbacks
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
};


#endif // SearchImagesProgressCallback_h
