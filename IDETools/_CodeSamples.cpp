/////////////////////////////////////////////////////////
//
// Copyright (C) 2006, Oracle. All rights reserved.
//

#include "StdAfx.h"
#include "_CodeSamples.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void f( void )
{
	// Structure used to store enumerated languages and code pages.

	HRESULT hr;

	struct TRANSLATION
	{
		WORD wLanguage;
		WORD wCodePage;
	} *lpTranslate;

	// read the list of languages and code pages
	VerQueryValue( pBlock, 
		_T("\\VarFileInfo\\Translation"),
		(LPVOID*)&lpTranslate,
		&cbTranslate );

	// Read the file description for each language and code page.

	for( i = 0; i < (cbTranslate / sizeof( struct TRANSLATION ) ); ++i )
	{
		HRESULT hr = StringCchPrintf( SubBlock, 50,
			_T("\\StringFileInfo\\%04x%04x\\FileDescription"),
			lpTranslate[ i ].wLanguage,
			lpTranslate[ i ].wCodePage );

		if (FAILED(hr))
		{
			// TODO: write error handler.
		}

		// retrieve file description for language and code page "i"
		VerQueryValue(pBlock, 
			SubBlock, 
			&lpBuffer, 
			&dwBytes); 
	}
}


INCLUDE=C:\Program Files\Microsoft Visual Studio .NET 2003\VC7\ATLMFC\INCLUDE;C:\Program Files\Microsoft Visual Studio .NET 2003\VC7\INCLUDE;C:\Program Files\Microsoft Visual Studio .NET 2003\VC7\PlatformSDK\include\prerelease;C:\Program Files\Microsoft Visual Studio .NET 2003\VC7\PlatformSDK\include;C:\Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\include;C:\Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\include\
JAVA_HOME=C:\Program Files\Java\jdk1.5.0_02

C>Program Files>Java>jdk

INC=abc;xyz,END

a
bc
123

	CString GetWindowText( const CWnd* pWnd );
	void SetWindowText( CWnd* pWnd, const TCHAR* pText );

	CString GetControlText( const CWnd* pWnd, UINT ctrlId );
	void SetControlText( CWnd* pWnd, UINT ctrlId, const TCHAR* pText );

	HICON smartLoadIcon( LPCTSTR iconID, bool asLarge = true, HINSTANCE hResInst = AfxGetResourceHandle() );

	bool smartEnableWindow( CWnd* pWnd, bool enable = true );
	void smartEnableControls( CWnd* pDlg, const UINT* pCtrlIds, size_t ctrlCount, bool enable = true );

	void commitMenuEnabling( CWnd& targetWnd, CMenu& popupMenu );
	void updateControlsUI( CWnd& targetWnd );
	CString GetCommandText( CCmdUI& rCmdUI );

	CRect& alignRect( CRect& restDest, const CRect& rectFixed, HorzAlign horzAlign = Horz_AlignCenter,
					  VertAlign vertAlign = Vert_AlignCenter, bool limitDest = false );

	CRect& centerRect( CRect& restDest, const CRect& rectFixed, bool horizontally = true, bool vertically = true,
					   bool limitDest = false, const CSize& addOffset = CSize( 0, 0 ) );


void function( const std::vector< CString >& rItems )
{
	CArray< const char* > myArray;
	CObjectList< CNmxObject* > myList;
	CList< CNmxObject > myList;

	std::vector< CString >rItems
	const std::vector< CString >& rItems
	const std::vector< CString >* pItems;
	std::vector< CString >&const rItems
	std::vector< CString > & const rItems,;

	const std::vector< CString >& rObject.GetParent()->GetItems() rItems
	const std::vector< CString >* rObject.GetParent()->GetItems() rItems

	const std::map< MyObject*, Class2& >& myMap
	const std::map< MyObject*, Class2& > &* myMap;
}


/**
	LineParser implementation
*/

const LPCTSTR LineParser::m_warning = _T("warning");
const LPCTSTR LineParser::m_error = _T("error");

	CAssociation( void );
	CAssociation( const CAssociation& rRight );
	virtual ~CAssociation();

	CAssociation& operator=( const CAssociation& rRight );

	bool IsEmpty( void ) const;

	void Clear( void );
/**
	\BUGBUG: PaulC 4-Dec-2006:
*/
template< typename T, int x >
LineParser::LineParser( void )
	: m_line()
{
	reset();
}

LineParser::LineParser( const LineParser& src )
	: m_line( src.m_line )
	, m_posFile( src.m_posFile )
	, m_posBuildCode( src.m_posBuildCode )
	, m_lineNumberArg( src.m_lineNumberArg )
	, m_lenBuildCode( src.m_lenBuildCode )
	, m_infoFlags( src.m_infoFlags )
{
}
	static char* =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<ps-persistence version=\"8.12.1\">\n"
		"<scenarioList>\n"
		"<scenario>\n"
		"<scenarioCode>Schedule #1</scenarioCode>\n"
		"<model>\n"
		"<horizon>\n"
		"<scenarioStart>2006-10-01T08:00:00</scenarioStart>\n"
		"<scenarioEnd>2006-12-31T16:30:00</scenarioEnd>\n"
		"<calendarStart>2006-08-29T08:00:00</calendarStart>\n"
		"<calendarEnd>2006-12-31T16:30:00</calendarEnd>\n"
		"<workDayStartTime>08:00</workDayStartTime>\n"
		"<workWeekStartDay>Monday</workWeekStartDay>\n"
		"</horizon>\n"
		;


	bool hasPrompt( int flags, bool strict = false ) const;
// abc
// xyz
/* comment */ /* comment */
/* comment */ /* comment */
/* comment */ /* comment */
/* comment */ /* comment */
template< typename Type, typename Policy >
LineParser& LineParser< Type, Policy >::operator=( const LineParser& src )
// hgjkhg
//
// more comments
LineParser::LineParser( CString src )
	: m_line( src )
{
	parse();
}

LineParser::~LineParser()
{
}

LineParser& LineParser::operator=( const LineParser& src )
{
	m_line = src.m_line;
	m_infoFlags = src.m_infoFlags;
	m_posFile = src.m_posFile;
	m_posBuildCode = src.m_posBuildCode;
	m_lenBuildCode = src.m_lenBuildCode;
	m_lineNumberArg = src.m_lineNumberArg;
	return *this;
}

bool LineParser::hasPrompt( int flags, bool strict /*= false*/ ) const
{
	if ( strict )
		return ( m_infoFlags & flags ) == flags;
	else
		return ( m_infoFlags & flags ) != 0;
}

CString LineParser::extract( int flags ) const
{
	_ASSERTE( !( flags & PF_File ) != !( flags & PF_Build ) );
	_ASSERTE( hasPrompt( flags ) );

	if ( flags & PF_File && hasPrompt( PF_File ) )
		return m_line.Left( m_posFile );

	if ( flags & PF_Build && hasPrompt( PF_Build ) )
		return m_line.Mid( m_posBuildCode, m_lenBuildCode );

	_ASSERTE( false );
	return _T("");
}

	bool hasPrompt( int flags, bool strict = false ) const;
void LineParser::reset( void )
{
	m_infoFlags = 0;
	m_posFile = 0;
	m_posBuildCode = 0;
	m_lenBuildCode = 0;
	m_lineNumberArg = -1;
}

	pRootWindow = AfxGetMainWnd();
	const TCHAR* windowType = NULL;

	if ( pRootWindow != NULL )
		windowType = _T("Main");
	else if ( pRootWindow = CWnd::GetFocus() )
		windowType = _T("Focus");
	else if ( pRootWindow = CWnd::GetActiveWindow() )
		windowType = _T("Active");
	else if ( pRootWindow = CWnd::GetForegroundWindow() )
		windowType = _T("Foreground");

	DWORD threadId = GetWindowThreadProcessId( pRootWindow->m_hWnd, NULL );

	GUITHREADINFO threadInfo =
	{
		sizeof( GUITHREADINFO ),
		GUI_CARETBLINKING | GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE
	};

	if ( GetGUIThreadInfo( threadId, &threadInfo ) )
	{
		DEBUG_LOG( _T("BEGIN\n") );

		DEBUG_LOG( _T("hwndActive: %s\n"), (LPCTSTR)getWindowInfo( threadInfo.hwndActive ) );
		DEBUG_LOG( _T("hwndFocus: %s\n"), (LPCTSTR)getWindowInfo( threadInfo.hwndFocus ) );
		DEBUG_LOG( _T("hwndCapture: %s\n"), (LPCTSTR)getWindowInfo( threadInfo.hwndCapture ) );
		DEBUG_LOG( _T("hwndCaret: %s\n"), (LPCTSTR)getWindowInfo( threadInfo.hwndCaret ) );
		DEBUG_LOG( _T("END\n") );
	}

	return pRootWindow;


