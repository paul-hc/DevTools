
#include "pch.h"
#include "OutputActivator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static DWORD extFlag( UINT vKey )
{
	if( vKey == VK_INSERT || vKey == VK_HOME || vKey == VK_PRIOR || vKey == VK_NEXT ||
		vKey == VK_DELETE || vKey == VK_END || vKey == VK_LEFT || vKey == VK_RIGHT ||
		vKey == VK_UP || vKey == VK_DOWN )
		return KEYEVENTF_EXTENDEDKEY;
	return 0;
}

static void simulateKeyPress( UINT vKey )
{
	UINT			scanCode = MapVirtualKey( vKey, 0 );

	keybd_event( LOBYTE( vKey ), LOBYTE( scanCode ), extFlag( vKey ) | 0, 0 );
}

static void simulateKeyRelease( UINT vKey )
{
	UINT			scanCode = MapVirtualKey( vKey, 0 );

	keybd_event( LOBYTE( vKey ), LOBYTE( scanCode ), extFlag( vKey ) | KEYEVENTF_KEYUP, 0 );
}

OutputActivator::OutputActivator( HWND _hWndOutput, HWND _hWndTab, const TCHAR* _tabCaption )
	: hWndOutput( _hWndOutput )
	, hWndTab( _hWndTab )
	, tabCaption( _tabCaption )
{
	AfxOleLockApp();
}

OutputActivator::~OutputActivator()
{
	AfxOleUnlockApp();
}

BOOL OutputActivator::InitInstance( void )
{
	return TRUE;
}

void OutputActivator::waitForCaptionChange( const TCHAR* origCaption )
{
	bool seenOurMessage = false;
	MSG m;
	TCHAR caption[ 128 ];

	while( !seenOurMessage )
	{
		while( PeekMessage( &m, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &m );
			DispatchMessage( &m );
		}
		Sleep( 0 );

		if ( ::GetWindowText( hWndTab, caption, COUNT_OF( caption ) ) > 0 )
			if ( _tcsicmp( caption, origCaption ) )
				seenOurMessage = true;
	}
}

int OutputActivator::Run( void )
{
	DWORD			processID, thisThreadID, outputWndThreadID;

	thisThreadID = GetCurrentThreadId();
	outputWndThreadID = GetWindowThreadProcessId( hWndOutput, &processID );
	if( thisThreadID != outputWndThreadID )
		VERIFY( AttachThreadInput( thisThreadID, outputWndThreadID, TRUE ) );
	::SetFocus( hWndOutput );

	TCHAR			caption[ 128 ];

	for ( int i = 0; i < 20; ++i )
	{
		if ( ::GetWindowText( hWndTab, caption, COUNT_OF( caption ) ) > 0 )
			if ( !_tcsicmp( caption, tabCaption ) )
				break;

		simulateKeyPress( VK_CONTROL );
		simulateKeyPress( VK_PRIOR );
		simulateKeyRelease( VK_PRIOR );
		simulateKeyRelease( VK_CONTROL );

		waitForCaptionChange( caption );
	}

	simulateKeyPress( VK_CONTROL );
	simulateKeyPress( VK_END );
	simulateKeyRelease( VK_END );
	simulateKeyRelease( VK_CONTROL );
	Sleep( 5000 );

	if( thisThreadID != outputWndThreadID )
		VERIFY( AttachThreadInput( thisThreadID, outputWndThreadID, FALSE ) );

	return 0;
}
