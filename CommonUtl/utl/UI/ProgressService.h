#ifndef ProgressService_h
#define ProgressService_h
#pragma once

#include "utl/FileSystem_fwd.h"
#include "utl/IProgressService.h"


class CProgressDialog;


class CProgressService : private fs::IEnumeratorImpl
{
public:
	CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel );
	CProgressService( void );			// null progress
	~CProgressService();

	utl::IProgressService* GetService( void );
	utl::IProgressHeader* GetHeader( void ) { return GetService()->GetHeader(); }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	void DestroyDialog( void );
private:
	// file enumerator callbacks
	virtual void OnAddFileInfo( const CFileFind& foundFile );
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	std::auto_ptr<CProgressDialog> m_pDlg;
public:
	static std::tstring s_dialogTitle;
	static std::tstring s_stageLabel;
	static std::tstring s_itemLabel;
};


#endif // ProgressService_h
