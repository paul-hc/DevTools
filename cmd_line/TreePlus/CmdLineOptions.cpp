
#include "stdafx.h"
#include "CmdLineOptions.h"
#include "OutputProfile.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"


namespace app
{
	const CEnumTags& GetTags_FileEncoding( void )
	{
		static const CEnumTags s_tags( _T("ANSI|UTF8|UTF16|UTF16-BE") );
		return s_tags;
	}
}

CCmdLineOptions::CCmdLineOptions( void )
	: m_pArg( NULL )
	, m_optionFlags( 0 )
	, m_outProfileType( Graphical )
	, m_maxDepthLevel( utl::npos )
	, m_fileEncoding( fs::ANSI_UTF8 )
{
}

CCmdLineOptions::~CCmdLineOptions()
{
}

bool CCmdLineOptions::ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt /*= AsIs*/ )
{
	if ( arg::ParseValuePair( rValue, pArg, pNameList ) )
	{
		switch ( caseCvt )
		{
			case UpperCase: str::ToUpper( rValue ); break;
			case LowerCase: str::ToLower( rValue ); break;
		}
		return true;
	}

	return false;
}

void CCmdLineOptions::ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException )
{
	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( arg::IsSwitch( m_pArg ) )
		{
			const TCHAR* pSwitch = m_pArg + 1;
			std::tstring value;

			if ( arg::Equals( pSwitch, _T("f") ) )
				SetFlag( m_optionFlags, app::DisplayFiles );
			else if ( arg::Equals( pSwitch, _T("ns") ) )
				SetFlag( m_optionFlags, app::NoSorting );
			else if ( ParseValue( value, pSwitch, _T("gs") ) )
			{
				if ( arg::Equals( value.c_str(), _T("G") ) )
					m_outProfileType = Graphical;
				else if ( arg::Equals( value.c_str(), _T("A") ) )
					m_outProfileType = NormalAscii;
				else if ( arg::Equals( value.c_str(), _T("B") ) )
					m_outProfileType = BlankSpace;
				else
					ThrowInvalidArgument();
			}
			else if ( ParseValue( value, pSwitch, _T("l") ) )
			{
				if ( !num::ParseNumber( m_maxDepthLevel, value ) )
					throw CRuntimeException( str::Format( _T("Invalid number in argument: '%s'"), m_pArg ) );
			}
			else if ( ParseValue( value, pSwitch, _T("e") ) )
			{
				if ( !app::GetTags_FileEncoding().ParseUiAs( m_fileEncoding, value ) )
					ThrowInvalidArgument();
			}
			else if ( arg::Equals( pSwitch, _T("no") ) )
				SetFlag( m_optionFlags, app::NoOutput );
			else if ( arg::EqualsAnyOf( pSwitch, _T("?|h") ) )
			{
				SetFlag( m_optionFlags, app::HelpMode );
				return;
			}
			else if ( arg::Equals( pSwitch, _T("p") ) )
				SetFlag( m_optionFlags, app::PauseAtEnd );
		#ifdef USE_UT
			else if ( arg::Equals( pSwitch, _T("ut") ) )
				SetFlag( m_optionFlags, app::UnitTestMode );
		#endif
			else
				ThrowInvalidArgument();
		}
		else
		{
			if ( m_dirPath.IsEmpty() )
				m_dirPath.Set( m_pArg );
			else
				throw CRuntimeException( str::Format( _T("Unrecognized argument '%s'"), m_pArg ) );
		}
	}

	PostProcessArguments();
}

void CCmdLineOptions::PostProcessArguments( void ) throws_( CRuntimeException )
{
	if ( m_dirPath.IsEmpty() )
		m_dirPath = fs::GetCurrentDirectory();			// use the current working directory
	else if ( !fs::IsValidDirectory( m_dirPath.GetPtr() ) )
		throw CRuntimeException( std::tstring( _T("Invalid dir_path specification: ") ) + m_dirPath.Get() );
}

void CCmdLineOptions::ThrowInvalidArgument( void ) throws_( CRuntimeException )
{
	throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
}
