#ifndef MultiThreading_h
#define MultiThreading_h
#pragma once

#include <afxmt.h>


class CScopedInitializeCom
{
public:
	CScopedInitializeCom( DWORD coInit = COINIT_APARTMENTTHREADED );		// Appartment Threaded is the default for the main thread
	~CScopedInitializeCom() { Uninitialize(); }

	void Uninitialize( void );
private:
	bool m_comInitialized;
};


namespace mt
{
	// initialize COM in the current thread (worker or UI thread)
	//
	class CScopedInitializeCom : public ::CScopedInitializeCom
	{
	public:
		CScopedInitializeCom( DWORD coInit = COINIT_MULTITHREADED ) : ::CScopedInitializeCom( coInit ) {}
	};


	// same as ::CSingleLock, but lock automatically on constructor
	//
	class CAutoLock : public CSingleLock
	{
	public:
		explicit CAutoLock( CSyncObject* pObject ) : CSingleLock( pObject, TRUE ) {}
	};
}


namespace st
{
	// Initialize OLE in the main application thread - current appartment, with the concurrency model as single-thread apartment (STA).
	// Used by console applications not linking to MFC - with no CWinApp object.

	class CScopedInitializeOle
	{
	public:
		CScopedInitializeOle( void );
		~CScopedInitializeOle() { Uninitialize(); }

		void Uninitialize( void );
	private:
		bool m_oleInitialized;
	};
}


#endif // MultiThreading_h
