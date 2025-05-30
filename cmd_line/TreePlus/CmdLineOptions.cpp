
#include "pch.h"
#include "CmdLineOptions.h"
#include "Table.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"


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
	, m_optionFlags()
	, m_guidesProfileType( _ProfileCount )
	, m_maxDepthLevel( utl::npos )
	, m_maxDirFiles( utl::npos )
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

void CCmdLineOptions::ParseCommandLine( int argc, const TCHAR* const argv[] ) throws_( CRuntimeException )
{
	ParseOptionSwitches( argc, argv );		// positional independence: parse the switches first, so we input all the options before parsing the parameters
	ParseParameters( argc, argv );

	PostProcessArguments();
}

void CCmdLineOptions::ParseOptionSwitches( int argc, const TCHAR* const argv[] ) throws_( CRuntimeException )
{
	std::tstring value;

	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( arg::IsSwitch( m_pArg ) )
		{
			const TCHAR* pSwitch = m_pArg + 1;

			if ( arg::Equals( pSwitch, _T("f") ) )
				m_optionFlags.Set( app::DisplayFiles );
			else if ( arg::Equals( pSwitch, _T("h") ) )
				m_optionFlags.Set( app::ShowHiddenNodes );
			else if ( arg::Equals( pSwitch, _T("ns") ) )
				m_optionFlags.Set( app::NoSorting );
			else if ( ParseValue( value, pSwitch, _T("gs") ) )
			{
				if ( arg::StartsWith( value.c_str(), _T("G") ) )
					m_guidesProfileType = GraphGuides;
				else if ( arg::StartsWith( value.c_str(), _T("A") ) )
					m_guidesProfileType = AsciiGuides;
				else if ( arg::StartsWith( value.c_str(), _T("B") ) )
					m_guidesProfileType = BlankGuides;
				else if ( arg::StartsWith( value.c_str(), _T("T") ) )
					m_guidesProfileType = TabGuides;
				else
					ThrowInvalidArgument();

				m_optionFlags.Set( app::SkipFileGroupLine, value.length() > 1 && _T('-') == value[ 1 ] );		// has '-' suffix
			}
			else if ( ParseValue( value, pSwitch, _T("l") ) )
			{
				if ( !num::ParseNumber( m_maxDepthLevel, value ) )
					throw CRuntimeException( str::Format( _T("Invalid number in argument: '%s'"), m_pArg ) );
			}
			else if ( ParseValue( value, pSwitch, _T("max") ) )
			{
				if ( !num::ParseNumber( m_maxDirFiles, value ) )
					throw CRuntimeException( str::Format( _T("Invalid number in argument: '%s'"), m_pArg ) );
			}
			else if ( ParseValue( value, pSwitch, _T("e") ) )
			{
				if ( !app::GetTags_FileEncoding().ParseUiAs( m_fileEncoding, value ) )
					ThrowInvalidArgument();
			}
			else if ( arg::Equals( pSwitch, _T("no") ) )
				m_optionFlags.Set( app::NoOutput );
			else if ( arg::EqualsAnyOf( pSwitch, _T("?|h") ) )
			{
				m_optionFlags.Set( app::HelpMode );
				return;
			}
			else if ( arg::Equals( pSwitch, _T("t") ) )
				m_optionFlags.Set( app::ShowExecTimeStats );
			else if ( arg::Equals( pSwitch, _T("p") ) )
				m_optionFlags.Set( app::PauseAtEnd );
		#ifdef USE_UT
			else if ( arg::Equals( pSwitch, _T("ut") ) )
				m_optionFlags.Set( app::UnitTestMode );
		#endif
			else
				ThrowInvalidArgument();
		}
	}
}

void CCmdLineOptions::ParseParameters( int argc, const TCHAR* const argv[] ) throws_( CRuntimeException )
{
	std::tstring value;

	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( !arg::IsSwitch( m_pArg ) )
		{
			if ( ParseValue( value, m_pArg, _T("in") ) )
			{
				SetTableInputMode();
				m_pTable->ParseTextFile( value, !m_optionFlags.Has( app::NoSorting ) );
			}
			else if ( ParseValue( value, m_pArg, _T("out") ) )
				m_outputFilePath = value;
			else if ( arg::Equals( m_pArg, _T("in_clip") ) )
			{
				CTextClipboard::CMessageWnd msgWnd;		// use a temporary message-only window for clipboard paste
				std::vector< std::tstring > textRows;

				if ( CTextClipboard::PasteFromLines( textRows, msgWnd.GetWnd() ) )
				{
					SetTableInputMode();
					m_pTable->ParseRows( textRows, !m_optionFlags.Has( app::NoSorting ) );
				}
				else
					throw CRuntimeException( str::Format( _T("Cannot paste any tabbed text from clipboard for argument '%s'"), m_pArg ) );
			}
			else if ( arg::Equals( m_pArg, _T("out_clip") ) )
				m_optionFlags.Set( app::ClipboardOutputMode );
			else if ( m_dirPath.IsEmpty() )
				m_dirPath.Set( m_pArg );
			else
				throw CRuntimeException( str::Format( _T("Unrecognized argument '%s'"), m_pArg ) );
		}
	}
}

void CCmdLineOptions::PostProcessArguments( void ) throws_( CRuntimeException )
{
	if ( !HasOptionFlag( app::TableInputMode ) )
		if ( m_dirPath.IsEmpty() )
			m_dirPath = fs::GetCurrentDirectory();			// use the current working directory
		else if ( !fs::IsValidDirectory( m_dirPath.GetPtr() ) )
			throw CRuntimeException( std::tstring( _T("Invalid dir_path specification: ") ) + m_dirPath.Get() );

	if ( _ProfileCount == m_guidesProfileType )		// default guides (not specified)
		m_guidesProfileType = GraphGuides;			// use GraphGuides as effective default
}

void CCmdLineOptions::SetTableInputMode( void )
{
	m_pTable.reset( new CTable() );
	m_optionFlags |= app::TOption::Make( app::TableInputMode | app::DisplayFiles | app::SkipFileGroupLine );

	if ( _ProfileCount == m_guidesProfileType )		// default guides (not specified)
		m_guidesProfileType = TabGuides;
}

void CCmdLineOptions::ThrowInvalidArgument( void ) throws_( CRuntimeException )
{
	throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
}
