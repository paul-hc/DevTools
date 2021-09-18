
#include "stdafx.h"
#include "ResourceFile.h"
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileUtils.hxx"


namespace rc
{
	// RC file syntax parsing

	bool SkipTrivial( TTokenIterator& rIt )
	{
		// skip whitespace and comments
		if ( rIt.SkipWhiteSpace().AtEnd() )
			return false;

		if ( rIt.Matches( "//" ) )
			return false;								// ignore commented line
		else if ( rIt.Matches( "/*" ) )					// skip "/* some comment */"
		{
			if ( !rIt.FindToken( "*/" ) )
				return false;							// comment not ended on current line
			if ( rIt.SkipWhiteSpace().AtEnd() )
				return false;
		}

		return true;
	}

	bool SkipDelimsAndTrivial( TTokenIterator& rIt, const char delims[] )
	{
		if ( !SkipTrivial( rIt ) )		// leading whitespace
			return false;

		rIt.SkipDelims( delims );
		return SkipTrivial( rIt );		// trailing whitespace
	}

	
	class CVersionInfoParser : public ILineParserCallback<std::string>
	{
	public:
		CVersionInfoParser( CResourceFile* pOwner ) : m_pOwner( pOwner ), m_lineNo( utl::npos ), m_viFlags( 0 ) {}

		static bool ReplaceEntry_VALUE( std::string& rValueLine, const char* pName, const char* pValue );
		static bool ReplaceEntry_BuildTimestamp( std::string& rValueLine, const CTime& buildTimestamp );
	private:
		// ILineParserCallback<std::string> interface
		virtual bool OnParseLine( const std::string& line, unsigned int lineNo );
		virtual void OnEndParsing( void ) { m_pOwner->OnEndParsing(); }

		bool ParseEntry_StringValue( rc::TTokenIterator& rIt );
		bool ParseEntry_BuildTimestamp( rc::TTokenIterator& rIt );

		bool InStringValuesBlock( void ) const;

		enum VersionInfoFlags
		{
			Found_VersionInfo = BIT_FLAG( 0 ),
			Done_VersionInfo = BIT_FLAG( 1 )
		};
		typedef int TVersionInfoFlags;
	private:
		CResourceFile* m_pOwner;
		size_t m_lineNo;
		TVersionInfoFlags m_viFlags;
		std::vector< std::string > m_viBlocksStack;

		static const char s_doubleQuote[];
	};
}


CResourceFile::CResourceFile( const fs::CPath& rcFilePath, app::TOption optionFlags ) throws_( CRuntimeException )
	: m_rcFilePath( rcFilePath )
	, m_optionFlags( optionFlags )
	, m_origFileState( fs::CFileState::ReadFromFile( m_rcFilePath ) )
{
	rc::CVersionInfoParser parserCallback( this );
	CTextFileParser<std::string> parser( &parserCallback );

	parser.ParseFile( m_rcFilePath );
}

CResourceFile::~CResourceFile()
{
}

CTime CResourceFile::StampBuildTime( const CTime& buildTimestamp )
{
	REQUIRE( HasBuildTimestamp() );
	m_newBuildTimestamp = buildTimestamp;

	rc::CVersionInfoParser::ReplaceEntry_BuildTimestamp( RefLine_BuildTimestamp(), m_newBuildTimestamp );
	return m_origBuildTimestamp;
}

void CResourceFile::Save( void ) const throws_( std::exception, CException* )
{
	utl::WriteLinesToFile( m_rcFilePath, m_lines );		// save all lines
	m_origFileState.WriteToFile();						// restore the original file access times
}

void CResourceFile::Report( std::ostream& os ) const
{
	REQUIRE( HasBuildTimestamp() );

	// c:\dev\DevTools\Slider\Slider.rc(920) : stamped build time to '17-09-2021 14:02:39'  (last was '17-09-2021 13:58:03').
	//
	os << m_rcFilePath << "(" << GetBuildTimestampLineNum() << ") : stamped build time to '" << time_utl::FormatTimestamp( m_newBuildTimestamp ) << "'";

	if ( time_utl::IsValid( m_origBuildTimestamp ) )
		os << "  (last was '" << time_utl::FormatTimestamp( m_origBuildTimestamp ) << "')";

	os << "." << std::endl;
}

void CResourceFile::OnEndParsing( void )
{
	if ( HasFlag( m_optionFlags, app::Add_BuildTimestamp ) )		// must augment BT?
		if ( !HasBuildTimestamp() )									// BT not found?
			if ( FoundFirstValue() )								// have the source VALUE entry
			{	// augment the "BuildTimestamp" entry
				std::string btValueLine = RefLine_FirstValue();		// clone first value

				rc::CVersionInfoParser::ReplaceEntry_VALUE( btValueLine, "BuildTimestamp", "..." );
				m_lines.insert( m_lines.begin() + m_pos.m_firstValue, btValueLine );
				m_pos.m_buildTimestamp = m_pos.m_firstValue;
			}
}


namespace rc
{
	// CVersionInfoParser implementation

	const char CVersionInfoParser::s_doubleQuote[] = "\"";

	bool CVersionInfoParser::OnParseLine( const std::string& line, unsigned int lineNo )
	{
		m_pOwner->m_lines.push_back( line );
		m_lineNo = lineNo;

		rc::TTokenIterator it( line );

		if ( rc::SkipTrivial( it ) )
		{
			if ( !HasFlag( m_viFlags, Found_VersionInfo ) )
				if ( it.Matches( "VS_VERSION_INFO" ) )
					SetFlag( m_viFlags, Found_VersionInfo );

			if ( HasFlag( m_viFlags, Found_VersionInfo ) && !HasFlag( m_viFlags, Done_VersionInfo ) )		// parsing in VS_VERSION_INFO?
			{
				if ( it.Matches( "BLOCK" ) )
				{	// BLOCK "<block_name>"
					if ( rc::SkipTrivial( it ) )
					{
						std::string blockName;
						if ( it.ExtractEnclosedText( blockName, s_doubleQuote ) )
							m_viBlocksStack.push_back( blockName );
					}
				}
				else if ( it.Matches( "END" ) )
				{
					m_viBlocksStack.pop_back();

					if ( m_viBlocksStack.empty() )
						SetFlag( m_viFlags, Done_VersionInfo );			// final END, close VS_VERSION_INFO parsing
				}
				else if ( !m_pOwner->HasBuildTimestamp() )
					ParseEntry_StringValue( it );
			}
		}

		return true;
	}

	bool CVersionInfoParser::ParseEntry_StringValue( rc::TTokenIterator& rIt )
	{
		if ( rIt.Matches( "VALUE" ) )
			if ( rc::SkipTrivial( rIt ) )
				if ( *s_doubleQuote == *rIt )
				{
					if ( ParseEntry_BuildTimestamp( rIt ) )
						m_pOwner->m_pos.m_buildTimestamp = m_lineNo - 1;
					else if ( !m_pOwner->FoundFirstValue() )
						if ( InStringValuesBlock() )
							m_pOwner->m_pos.m_firstValue = m_lineNo - 1;

					return true;
				}

		return false;
	}

	bool CVersionInfoParser::ParseEntry_BuildTimestamp( rc::TTokenIterator& rIt )
	{
		if ( rIt.Matches( "\"BuildTimestamp\"" ) )
			if ( rc::SkipDelimsAndTrivial( rIt, "," ) )
			{
				std::string value;
				if ( rIt.ExtractEnclosedText( value, s_doubleQuote ) )
					m_pOwner->m_origBuildTimestamp = time_utl::ParseStdTimestamp( str::FromAnsi( value.c_str() ) );		// store the original value of the timestamp

				return true;
			}

		return false;
	}

	bool CVersionInfoParser::InStringValuesBlock( void ) const
	{
		return 2 == m_viBlocksStack.size() && m_viBlocksStack[ 0 ] == "StringFileInfo";
	}


	bool CVersionInfoParser::ReplaceEntry_VALUE( std::string& rValueLine, const char* pName, const char* pValue )
	{
		rc::TTokenIterator it( rValueLine );

		if ( rc::SkipTrivial( it ) )
			if ( it.Matches( "VALUE" ) )
				if ( rc::SkipTrivial( it ) )
					if ( *s_doubleQuote == *it )		// on 1st quoted token: value name
					{
						std::string text;
						Range< size_t > tokenRange;

						if ( it.ExtractEnclosedText( text, s_doubleQuote, &tokenRange ) )
						{
							if ( pName != NULL )
								it.ReplaceToken( tokenRange, pName );

							if ( rc::SkipDelimsAndTrivial( it, "," ) )		// on 2nd quoted token: value content
							{
								size_t offset = it.GetPos() + 1;

								if ( it.ExtractEnclosedText( text, s_doubleQuote, &tokenRange ) )
									it.ReplaceToken( tokenRange, pValue );
								else
									it.ReplaceToken( Range< size_t >( offset, rValueLine.length() ), str::Enquote( pValue, *s_doubleQuote ).c_str() );		// best guess: add the trailing part

								return true;
							}
						}
					}

		return false;
	}

	bool CVersionInfoParser::ReplaceEntry_BuildTimestamp( std::string& rValueLine, const CTime& buildTimestamp )
	{
		std::string tsText( "..." );

		if ( time_utl::IsValid( buildTimestamp ) )
			tsText = str::AsNarrow( time_utl::FormatTimestamp( buildTimestamp ) );

		return ReplaceEntry_VALUE( rValueLine, NULL, tsText.c_str() );
	}

}	// namespace rc
