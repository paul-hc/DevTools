
#include "stdafx.h"
#include "ConsoleInputOutput.h"
#include "EnumTags.h"
#include "FileSystem.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	const CEnumTags& GetTags_UserChoices( void )
	{
		static const CEnumTags s_tags( _T("Yes/No|Yes/No/All") );
		return s_tags;
	}


	void ReportException( const std::exception& exc )
	{
		if ( !str::IsEmpty( exc.what() ) )
		{
			std::cerr << "* Error: " << exc.what() << std::endl;
			TraceException( exc );
		}
	}

	void TraceException( const std::exception& exc )
	{
	#ifdef _DEBUG
		if ( !str::IsEmpty( exc.what() ) )
			TRACE( "* Exception: %s\n", exc.what() );
	#else
		exc;
	#endif
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
			io::ReportException( exc );
			return fs::CreationError;
		}
	}

	bool CUserQueryCreateDirectory::IsDenied( const fs::CPath& directoryPath ) const
	{
		return std::find( m_deniedDirPaths.begin(), m_deniedDirPaths.end(), directoryPath ) != m_deniedDirPaths.end();
	}
}
