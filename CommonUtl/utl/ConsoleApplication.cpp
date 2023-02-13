
#include "pch.h"
#include "ConsoleApplication.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <iostream>
#include <fcntl.h>
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CConsoleApplication implementation

io::TranslationMode CConsoleApplication::s_translationMode = io::Ansi;

CConsoleApplication::CConsoleApplication( io::TranslationMode translationMode )
	: CAppTools()
	, m_stdOutput( STD_OUTPUT_HANDLE )
{
	SetTranslationMode( translationMode );

	// facilitate Unicode output - in a console that has a Unicode font defined!
	::setlocale( LC_ALL, "" );		// sets the locale to the default, which is obtained from the operating system

	m_modulePath = fs::GetModuleFilePath( NULL );
	m_pLogger.reset( new CLogger() );
}

CConsoleApplication::~CConsoleApplication()
{
}

void CConsoleApplication::SetTranslationMode( io::TranslationMode translationMode )
{
	s_translationMode = translationMode;

	// flush standard output/error streams before changing the global text mode
	std::cout.flush();
	std::cerr.flush();

	std::wcout.flush();
	std::wcerr.flush();

	::fflush( stdout );
	::fflush( stderr );

	switch ( s_translationMode )
	{
		case io::Ansi:
			::_setmode( _fileno( stdout ), _O_TEXT );
			::_setmode( _fileno( stderr ), _O_TEXT );
			break;
		case io::Utf8:
			//SetConsoleOutputCP( CP_UTF8 );		// doesn't seem to have an effect
			::_setmode( _fileno( stdout ), _O_U8TEXT );
			::_setmode( _fileno( stderr ), _O_U8TEXT );
			break;
		case io::Utf16:
			::_setmode( _fileno( stdout ), _O_U16TEXT );
			::_setmode( _fileno( stderr ), _O_U16TEXT );
			break;
	}
}

bool CConsoleApplication::IsConsoleApp( void ) const
{
	return true;
}

bool CConsoleApplication::IsInteractive( void ) const
{
	return true;
}

CLogger& CConsoleApplication::GetLogger( void )
{
	ASSERT_PTR( m_pLogger.get() );
	return *m_pLogger;
}

utl::CResourcePool& CConsoleApplication::GetSharedResources( void )
{
	return m_resourcePool;
}

bool CConsoleApplication::LazyInitAppResources( void )
{
	return false;		// no standard shared resources to delay initialization
}

bool CConsoleApplication::BeepSignal( app::MsgType msgType /*= app::Info*/ )
{
	switch ( msgType )
	{
		case app::Error:	::Beep( 220, 200 ); break;
		case app::Warning:	::Beep( 330, 70 ); break;
		case app::Info:		::Beep( 440, 70 ); break;
	}
	return false;
}

bool CConsoleApplication::ReportError( const std::tstring& message, app::MsgType msgType /*= app::Error*/ )
{
	io::CScopedRedirectStream scopedRedirect( std::cerr );

	std::cerr << app::FormatMsg( message, msgType ) << std::endl;
	return false;
}

int CConsoleApplication::ReportException( const std::exception& exc )
{
	io::CScopedRedirectStream scopedRedirect( std::cerr );

	app::TraceException( exc );
	std::cerr << app::FormatMsg( CRuntimeException::MessageOf( exc ), app::Error ) << std::endl;
	return 1;		// i.e. IDOK
}

int CConsoleApplication::ReportException( const CException* pExc )
{
	io::CScopedRedirectStream scopedRedirect( std::cerr );

	app::TraceException( pExc );
	std::cerr << app::FormatMsg( mfc::CRuntimeException::MessageOf( *pExc ), app::Error ) << std::endl;
	return 1;		// i.e. IDOK
}


namespace io
{
	CScopedRedirectStream::CScopedRedirectStream( std::ostream& rStdOutStream )
	{
		if ( io::Utf16 == CConsoleApplication::GetTranslationMode() )
			if ( &rStdOutStream == &std::cout )
				m_pRedirector.reset( new CRedirectStream<std::ostream, std::wostream>( std::cout, std::wcout ) );		// redirect std::cout => std::wcout
			else if ( &rStdOutStream == &std::cerr )
				m_pRedirector.reset( new CRedirectStream<std::ostream, std::wostream>( std::cerr, std::wcerr ) );		// redirect std::cerr => std::wcerr
			//else if ( &rStdOutStream == &std::clog )
			//	m_pRedirector.reset( new CRedirectStream<std::ostream, std::wostream>( std::clog, std::wclog ) );		// redirect std::clog => std::wclog
			else
				ASSERT( false );		// unknown standard output stream?
	}

	CScopedRedirectStream::CScopedRedirectStream( std::wostream& rStdOutStream )
	{
		if ( io::Utf8 == CConsoleApplication::GetTranslationMode() )
			if ( &rStdOutStream == &std::wcout )
				m_pRedirector.reset( new CRedirectStream<std::wostream, std::ostream>( std::wcout, std::cout ) );		// redirect std::wcout => std::cout
			else if ( &rStdOutStream == &std::wcerr )
				m_pRedirector.reset( new CRedirectStream<std::wostream, std::ostream>( std::wcerr, std::cerr ) );		// redirect std::wcerr => std::cerr
			//else if ( &rStdOutStream == &std::wclog )
			//	m_pRedirector.reset( new CRedirectStream<std::wostream, std::ostream>( std::wclog, std::clog ) );		// redirect std::wclog => std::clog
			else
				ASSERT( false );		// unknown standard output stream?
	}
}


namespace io
{
	const CEnumTags& GetTags_UserChoices( void )
	{
		static const CEnumTags s_tags( _T("Yes/No|Yes/No/All") );
		return s_tags;
	}


	char InputUserKey( bool skipWhitespace /*= true*/ )
	{
		std::string userKeyAnswer;

		if ( !skipWhitespace )
			std::cin.unsetf( std::ios::skipws );		// count whitespace as regular character
		std::cin >> userKeyAnswer;
		if ( !skipWhitespace )
			std::cin.setf( std::ios::skipws );			// restore

		size_t keyPos = userKeyAnswer.find_first_not_of( " \t" );
		return keyPos != std::tstring::npos ? (char)toupper( userKeyAnswer[ keyPos ] ) : '\0';
	}

	char PressAnyKey( const char* pMessage /*= "Press any key to continue..."*/ )
	{
		if ( pMessage != NULL )
		{	// use std::cerr to avoid output when redirecting output to file
			CScopedRedirectStream scopedRedirect( std::cerr );
			std::cerr << pMessage << std::endl;
		}

		return io::InputUserKey( false );
	}


	// CUserQuery implementation

	bool CUserQuery::Ask( const std::tstring& assetName )
	{
		CScopedRedirectStream scopedRedirect( std::cerr );

		ASSERT( MustAsk() );

		// use std::cerr to avoid output when redirecting output to file
		if ( !m_promptPrefix.empty() )
		{
			std::cerr << m_promptPrefix;

			if ( !::_istspace( m_promptPrefix[ m_promptPrefix.length() - 1 ] ) )
				std::cerr << _T(' ');
		}

		std::cerr << assetName << " ? (" << GetTags_UserChoices().FormatUi( m_choices ) << "): " << std::flush;

		while ( m_ask )
			switch ( io::InputUserKey() )
			{
				case 'N':
					m_response = No;
					return false;
				case 'Y':
					m_response = Yes;
					return true;
				case 'A':
					if ( YesNoAll == m_choices )
					{
						m_ask = false;
						m_response = Yes;
						return true;
					}
					// fall-through
				default:
					std::cerr << '? ' << std::flush;		// ask again
			}

		ASSERT( false );
		return false;
	}


	// CUserQueryCreateDirectory implementation

	fs::AcquireResult CUserQueryCreateDirectory::Acquire( const fs::CPath& directoryPath )
	{
		if ( IsDenied( directoryPath ) )
			return fs::CreationError;

		if ( fs::IsValidDirectory( directoryPath.GetPtr() ) )
			return fs::FoundExisting;

		if ( MustAsk() )
			if ( !Ask( arg::Enquote( directoryPath ) ) )
			{
				m_deniedDirPaths.push_back( directoryPath );
				return fs::CreationError;			// user rejected creation of this directory
			}

		try
		{
			fs::thr::CreateDirPath( directoryPath.GetPtr() );
			return fs::Created;
		}
		catch ( const CRuntimeException& exc )
		{
			m_deniedDirPaths.push_back( directoryPath );
			app::ReportException( exc );
			return fs::CreationError;
		}
	}

	bool CUserQueryCreateDirectory::IsDenied( const fs::CPath& directoryPath ) const
	{
		return std::find( m_deniedDirPaths.begin(), m_deniedDirPaths.end(), directoryPath ) != m_deniedDirPaths.end();
	}
}
