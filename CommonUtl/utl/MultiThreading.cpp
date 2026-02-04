
#include "pch.h"
#include "MultiThreading.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CScopedInitializeCom::CScopedInitializeCom( DWORD coInit /*= COINIT_APARTMENTTHREADED*/ )
	: m_comInitialized( false )
{
	HRESULT hResult =
#if ( ( _WIN32_WINNT >= 0x0400 ) || defined( _WIN32_DCOM ) )	// DCOM
		::CoInitializeEx( nullptr, coInit );
#else
		::CoInitialize( nullptr );
#endif

	if ( HR_OK( hResult ) )
		m_comInitialized = true;
	else
		// ignore RPC_E_CHANGED_MODE if CLR is loaded. Error is due to CLR initializing COM and InitializeCOM trying to initialize COM with different flags
		if ( hResult != RPC_E_CHANGED_MODE || nullptr == GetModuleHandle( _T("Mscoree.dll") ) )
			return;
}

void CScopedInitializeCom::Uninitialize( void )
{
	if ( m_comInitialized )
	{
		::CoUninitialize();
		m_comInitialized = false;
	}
}


namespace st
{
	CScopedInitializeOle::CScopedInitializeOle( void )
		: m_oleInitialized( HR_OK( ::OleInitialize( nullptr ) ) )
	{
	}

	void CScopedInitializeOle::Uninitialize( void )
	{
		if ( m_oleInitialized )
		{
			::OleUninitialize();
			m_oleInitialized = false;
		}
	}
}
