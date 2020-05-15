
#include "stdafx.h"
#include "Direct2D.h"
#include "BaseApp.h"
#include "Utilities.h"
#include <math.h>

#pragma comment( lib, "d2d1" )		// link to Direct2D

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CFactory implementation

	CFactory::CFactory( void )
	{
	#ifdef DEBUG_DIRECT2D
		D2D1_FACTORY_OPTIONS options;
		options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

		// Direct2D Debug Layer: see https://msdn.microsoft.com/en-us/library/dd940309%28VS.85%29.aspx
		HR_OK( ::D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &m_pFactory ) );
	#else
		HR_OK( ::D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory ) );
	#endif

		app::GetSharedResources().AddComPtr( m_pFactory );			// will release the factory singleton in ExitInstance()
	}

	CFactory::~CFactory()
	{
	}

	::ID2D1Factory* CFactory::Factory( void )
	{
		static CFactory s_factory;
		return s_factory.m_pFactory;
	}
}


namespace d2d
{
	D2D_SIZE_F GetScreenDpi( void )
	{
		D2D_SIZE_F dpiSize;
		d2d::CFactory::Factory()->GetDesktopDpi( &dpiSize.width, &dpiSize.height );		// usually 96 DPI
		return dpiSize;
	}

	D2D_SIZE_F GetScreenSize( void )
	{	// for the monitor where the main window is located
		return ToSizeF( ui::GetScreenSize() );
	}


	int GetCeiling( float number )
	{
		float ceiling = ::ceil( number );
		return static_cast< int >( ceiling );
	}

	int GetRounded( float number )
	{
		return static_cast< int >( number + 0.5f );
	}
}


namespace d2d
{
	// CGadgetBase implementation

	void CGadgetBase::SetRenderHost( IRenderHost* pRenderHost )
	{
		ASSERT_NULL( m_pRenderHost );
		m_pRenderHost = pRenderHost;
		ASSERT_PTR( m_pRenderHost );
	}

	IRenderHost* CGadgetBase::GetRenderHost( void ) const
	{
		ASSERT_PTR( m_pRenderHost );
		return m_pRenderHost;
	}

	void CGadgetBase::EraseBackground( const CViewCoords& coords )
	{
		coords;
	}
}
