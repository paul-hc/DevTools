#ifndef ProgressService_h
#define ProgressService_h
#pragma once

#include "utl/FileSystem_fwd.h"
#include "utl/UI/IProgressService.h"


class CProgressDialog;


class CProgressService : private fs::IEnumerator
					   , private utl::noncopyable
{
public:
	CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel = s_searching );
	CProgressService( void );			// null progress
	~CProgressService();

	ui::IProgressService* GetService( void );
	ui::IProgressHeader* GetHeader( void ) { return GetService()->GetHeader(); }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	bool IsInteractive( void ) const { return m_pDlg.get() != NULL; }

	void DestroyDialog( void );
private:
	// file enumerator callbacks
	virtual void OnAddFileInfo( const CFileFind& foundFile ) throws_( CUserAbortedException );
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	std::auto_ptr< CProgressDialog > m_pDlg;
	fs::CPath m_lastFilePath;
public:
	static const std::tstring s_searching;
};


#endif // ProgressService_h
