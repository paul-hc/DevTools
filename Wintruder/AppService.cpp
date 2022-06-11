
#include "stdafx.h"
#include "AppService.h"
#include "Application.h"
#include "Observers.h"
#include "resource.h"
#include "wnd/WndUtils.h"
#include "utl/Algorithms.h"
#include "utl/Logger.h"
#include "utl/StringUtilities.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	CLogger& GetPrivateLogger( void )
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


// CAppService implementation

CAppService::CAppService( void )
	: m_options( this )
	, m_targetWnd( ::GetDesktopWindow() )
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

	if ( IWndObserver* pWndObserver = dynamic_cast<IWndObserver*>( pObserver ) )
		utl::PushUnique( m_wndObservers, pWndObserver );

	if ( IEventObserver* pEventObserver = dynamic_cast<IEventObserver*>( pObserver ) )
		utl::PushUnique( m_eventObservers, pEventObserver );
}

void CAppService::RemoveObserver( CWnd* pObserver )
{
	ASSERT_PTR( pObserver );

	if ( IWndObserver* pWndObserver = dynamic_cast<IWndObserver*>( pObserver ) )
		utl::RemoveExisting( m_wndObservers, pWndObserver );

	if ( IEventObserver* pEventObserver = dynamic_cast<IEventObserver*>( pObserver ) )
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
