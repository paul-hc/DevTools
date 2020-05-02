#ifndef ProgressService_h
#define ProgressService_h
#pragma once

#include "utl/FileSystem_fwd.h"
#include "utl/UI/ProgressDialog.h"


class CProgressService : private fs::IEnumerator
					   , private utl::noncopyable
{
public:
	CProgressService( CWnd* pParentWnd, const std::tstring& operationLabel = s_searching );
	~CProgressService();

	ui::IProgressService* GetService( void ) { return m_dlg.GetService(); }
	ui::IProgressHeader* GetHeader( void ) { return GetService()->GetHeader(); }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	void DestroyDialog( void );
private:
	// file enumerator callbacks
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
public:
	static const std::tstring s_searching;
};


#endif // ProgressService_h
