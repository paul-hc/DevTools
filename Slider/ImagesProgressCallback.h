#ifndef ImagesProgressCallback_h
#define ImagesProgressCallback_h
#pragma once

#include "utl/FileSystem_fwd.h"
#include "utl/UI/ProgressDialog.h"


class CImagesProgressCallback : private fs::IEnumerator
							  , private utl::noncopyable
{
public:
	CImagesProgressCallback( CWnd* pParentWnd, const std::tstring& operationLabel = s_searching );
	~CImagesProgressCallback();

	ui::IProgressCallback* GetCallback( void ) { return &m_dlg; }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	bool IsValidDialog( void ) const { return m_dlg.IsRunning(); }		// i.e. not aborted by user?
	void Section_OrderImageFiles( size_t fileCount );
private:
	// file enumerator callbacks
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
public:
	static const std::tstring s_searching;
};


#endif // ImagesProgressCallback_h
