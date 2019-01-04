#ifndef MultiThreading_h
#define MultiThreading_h
#pragma once

#include <afxmt.h>


namespace mt
{
	// same as ::CSingleLock, but lock automatically on constructor

	class CAutoLock : public CSingleLock
	{
	public:
		explicit CAutoLock( CSyncObject* pObject ) : CSingleLock( pObject, TRUE ) {}
	};


	// initialize COM in the current thread (worker or UI thread)

	class CScopedInitializeCom
	{
	public:
		CScopedInitializeCom( void );
		~CScopedInitializeCom() { Uninitialize(); }

		void Uninitialize( void );
	private:
		bool m_comInitialized;
	};
}


#endif // MultiThreading_h
