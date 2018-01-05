
#include "stdafx.h"
#include "AppService.h"
#include "Observers.h"
#include "resource.h"
#include "wnd/WindowClass.h"
#include "wnd/WndUtils.h"
#include "utl/ContainerUtilities.h"
#include "utl/Logger.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	CLogger& GetLogger( void )
	{
		static CLogger logger;
		static bool init = false;
		if ( !init )
		{
			logger.m_logFileMaxSize = MegaByte;
			//logger.m_prependTimestamp = false;
			//logger.SetOverwrite();			// overwrite the log
			init = true;
		}
		return logger;
	}

	bool CheckValidTargetWnd( Feedback feedback /*= Report*/ )
	{
		const CWndSpot& targetWnd = GetTargetWnd();

		if ( targetWnd.IsNull() )
			return false;
		else if ( targetWnd.IsValid() )
			return true;

		if ( Beep == feedback || Report == feedback )
			ui::BeepSignal( MB_ICONWARNING );

		if ( Report == feedback )
			AfxMessageBox( str::Format( IDS_WND_EXPIRED_FMT, targetWnd.m_hWnd ).c_str(), MB_OK | MB_ICONSTOP );

		return false;
	}
}


namespace ui
{
	std::tstring FormatBriefWndInfo( HWND hWnd )
	{
		std::tstring info; info.reserve( 128 );
		static const TCHAR sep[] = _T(" "), commaSep[] = _T(", ");

		stream::Tag( info, wnd::FormatWindowHandle( hWnd ), sep );

		if ( ui::IsValidWindow( hWnd ) )
		{
			stream::Tag( info, str::Format( _T("\"%s\" %s"), ui::GetWindowText( hWnd ).c_str(), wc::FormatClassName( hWnd ).c_str() ), sep );

			DWORD style = ui::GetStyle( hWnd );
			std::tstring status;

			if ( ui::IsTopMost( hWnd ) )
				stream::Tag( status, _T("Top-most"), commaSep );
			if ( !HasFlag( style, WS_VISIBLE ) )
				stream::Tag( status, _T("Hidden"), commaSep );
			if ( HasFlag( style, WS_DISABLED ) )
				stream::Tag( status, _T("Disabled"), commaSep );

			if ( !status.empty() )
				stream::Tag( info, str::Format( _T("(%s)"), status.c_str() ), sep );
		}
		else
			stream::Tag( info, _T("<EXPIRED>"), sep );

		return info;
	}

	CRect GetCaptionRect( HWND hWnd )
	{	// returns the caption rect in client coordinates
		CRect captionRect;
		::GetWindowRect( hWnd, &captionRect );
		ScreenToClient( hWnd, captionRect );

		DWORD style = GetStyle( hWnd );
		int edgeExtent = ::GetSystemMetrics( HasFlag( style, WS_THICKFRAME ) ? SM_CXSIZEFRAME : SM_CYDLGFRAME );
		int captionExtent = ::GetSystemMetrics( HasFlag( GetStyleEx( hWnd ), WS_EX_TOOLWINDOW ) ? SM_CYSMCAPTION : SM_CYCAPTION );

		//enum { MinMaxButtonWidth = 27, CloseButtonWidth = 47 };		// Win7 magic numbers
		// reliable on themed UI?
		NONCLIENTMETRICS ncm = { sizeof( NONCLIENTMETRICS ) };
		VERIFY( SystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &ncm, 0 ) );
		ncm.iCaptionWidth; // 35

		int buttonsWidth = 0;
		if ( HasFlag( style, WS_SYSMENU ) )
			buttonsWidth += ncm.iCaptionWidth;
		if ( HasFlag( style, WS_MINIMIZEBOX ) )
			buttonsWidth += ncm.iCaptionWidth;
		if ( HasFlag( style, WS_MAXIMIZEBOX ) )
			buttonsWidth += ncm.iCaptionWidth;

		captionRect.DeflateRect( edgeExtent, edgeExtent, edgeExtent + captionExtent + buttonsWidth, 0 );
		captionRect.bottom = captionRect.top + captionExtent;
		return captionRect;
	}
}


// CAppService implementation

CAppService::CAppService( void )
	: m_targetWnd( ::GetDesktopWindow() )
	, m_dirtyDetails( false )
{
}

CAppService::~CAppService()
{
	ASSERT( m_wndObservers.empty() );
	ASSERT( m_eventObservers.empty() );
}

CAppService& CAppService::Instance( void )
{
	static CAppService appService;
	return appService;
}

void CAppService::AddObserver( CWnd* pObserver )
{
	ASSERT_PTR( pObserver );

	if ( IWndObserver* pWndObserver = dynamic_cast< IWndObserver* >( pObserver ) )
		utl::PushUnique( m_wndObservers, pWndObserver );

	if ( IEventObserver* pEventObserver = dynamic_cast< IEventObserver* >( pObserver ) )
		utl::PushUnique( m_eventObservers, pEventObserver );
}

void CAppService::RemoveObserver( CWnd* pObserver )
{
	ASSERT_PTR( pObserver );

	if ( IWndObserver* pWndObserver = dynamic_cast< IWndObserver* >( pObserver ) )
		utl::RemoveExisting( m_wndObservers, pWndObserver );

	if ( IEventObserver* pEventObserver = dynamic_cast< IEventObserver* >( pObserver ) )
		utl::RemoveExisting( m_eventObservers, pEventObserver );
}

void CAppService::PublishEvent( app::Event appEvent, IEventObserver* pSender /*= NULL*/ )
{
	for ( std::deque< IEventObserver* >::const_iterator itObserver = m_eventObservers.begin(); itObserver != m_eventObservers.end(); ++itObserver )
		if ( pSender != *itObserver && ( *itObserver )->CanNotify() )
			( *itObserver )->OnAppEvent( appEvent );
}

void CAppService::SetTargetWnd( const CWndSpot& targetWnd, IWndObserver* pSender /*= NULL*/ )
{
	m_targetWnd = targetWnd;

	// notify observers
	for ( std::deque< IWndObserver* >::const_iterator itObserver = m_wndObservers.begin(); itObserver != m_wndObservers.end(); ++itObserver )
		if ( pSender != *itObserver && ( *itObserver )->CanNotify() )
			( *itObserver )->OnTargetWndChanged( m_targetWnd );

	SetDirtyDetails( false );
}

bool CAppService::SetDirtyDetails( bool dirtyDetails /*= true*/ )
{
	if ( m_dirtyDetails == dirtyDetails )
		return false;

	m_dirtyDetails = dirtyDetails;
	PublishEvent( app::DirtyChanged );
	return true;
}
