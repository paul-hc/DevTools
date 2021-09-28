
#include "stdafx.h"
#include "ConsoleApplication.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CConsoleApplication implementation

CConsoleApplication::CConsoleApplication( void )
	: CAppTools()
{
	// facilitate Unicode output - in a console that has a Unicode font defined!
	::setlocale( LC_ALL, "" );		// sets the locale to the default, which is obtained from the operating system

	m_modulePath = fs::GetModuleFilePath( NULL );
	m_pLogger.reset( new CLogger );
}

CConsoleApplication::~CConsoleApplication()
{
}

utl::CResourcePool& CConsoleApplication::GetSharedResources( void )
{
	return m_resourcePool;
}

CLogger& CConsoleApplication::GetLogger( void )
{
	ASSERT_PTR( m_pLogger.get() );
	return *m_pLogger;
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
	std::cerr << app::FormatMsg( message, msgType ) << std::endl;
	return false;
}

int CConsoleApplication::ReportException( const std::exception& exc )
{
	app::TraceException( exc );
	std::cerr << app::FormatMsg( CRuntimeException::MessageOf( exc ), app::Error ) << std::endl;
	return 1;		// i.e. IDOK
}

int CConsoleApplication::ReportException( const CException* pExc )
{
	app::TraceException( pExc );
	std::cerr << app::FormatMsg( mfc::CRuntimeException::MessageOf( *pExc ), app::Error ) << std::endl;
	return 1;		// i.e. IDOK
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
			std::cout << pMessage << std::endl;

		return io::InputUserKey( false );
	}


	// CUserQuery implementation

	bool CUserQuery::Ask( const std::tstring& assetName )
	{
		ASSERT( MustAsk() );

		if ( !m_promptPrefix.empty() )
		{
			std::cout << m_promptPrefix;

			if ( !::_istspace( m_promptPrefix[ m_promptPrefix.length() - 1 ] ) )
				std::cout << _T(' ');
		}

		std::cout << assetName << " ? (" << GetTags_UserChoices().FormatUi( m_choices ) << "): ";

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
					std::cout << '? ';			// ask again
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
