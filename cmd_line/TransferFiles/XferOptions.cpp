
#include "pch.h"
#include "XferOptions.h"
#include "TransferItem.h"
#include "utl/ConsoleApplication.h"
#include "utl/FileSystem.h"
#include "utl/RuntimeException.h"
#include "utl/StringUtilities.h"
#include <iostream>


const TCHAR CXferOptions::m_specDelims[] = _T(";,");


CXferOptions::CXferOptions( void )
	: m_pArg( NULL )
	, m_helpMode( false )
	, m_recurseSubDirectories( true )
	, m_mustHaveFileAttr( 0 )
	, m_mustNotHaveFileAttr( 0 )
	, m_filterBy( NoCheck )
	, m_earliestTimestamp( 0 )
	, m_justCreateTargetDirs( false )
	, m_overrideReadOnlyFiles( false )
	, m_transferOnlyExistentTargetFiles( false )
	, m_transferOnlyToExistentTargetDirs( false )
	, m_displayFileNames( true )
	, m_fileAction( FileCopy )
	, m_transferMode( ExecuteTransfer )
	, m_userPrompt( PromptOnIssues )
{
}

CXferOptions::~CXferOptions()
{
}

bool CXferOptions::PassFilter( const CTransferItem& transferNode ) const
{
	REQUIRE( transferNode.m_source.IsValid() );

	BYTE sourceAttributes = transferNode.m_source.m_attributes;

	if ( m_mustHaveFileAttr != 0 )
		if ( ( sourceAttributes & m_mustHaveFileAttr ) != m_mustHaveFileAttr )
			return false;

	if ( m_mustNotHaveFileAttr != 0 )
		if ( ( sourceAttributes & m_mustNotHaveFileAttr ) != 0 )
			return false;

	if ( m_filterBy != NoCheck )
		if ( !transferNode.PassesFileFilter( this ) )
			return false;

	if ( m_transferOnlyToExistentTargetDirs )
	{
		fs::TDirPath targetDirPath = transferNode.m_target.m_fullPath;

		if ( !fs::IsValidDirectory( targetDirPath.GetPtr() ) )
			targetDirPath = targetDirPath.GetParentPath();

		if ( !fs::IsValidDirectory( targetDirPath.GetPtr() ) )	// target directory exists?
			return false;
	}

	if ( m_transferOnlyExistentTargetFiles )
		if ( !transferNode.m_target.IsValid() )
			return false;

	if ( !m_excludeWildSpec.empty() )
		if ( path::MatchWildcard( transferNode.m_source.m_fullPath.GetPtr(), m_excludeWildSpec.c_str() ) != _T('\0') )
			return false;

	if ( !m_excludeFindSpecs.empty() )
		for ( std::vector< std::tstring >::const_iterator itSubstrSpec = m_excludeFindSpecs.begin(); itSubstrSpec != m_excludeFindSpecs.end(); ++itSubstrSpec )
			if ( *path::Find( transferNode.m_source.m_fullPath.GetPtr(), itSubstrSpec->c_str() ) != _T('\0') )
				return false;

	return true;
}

void CXferOptions::ParseFileAction( const std::tstring& value ) throws_( CRuntimeException )
{
	if ( !value.empty() )
		switch ( str::CharTraits::ToUpper( value[ 0 ] ) )
		{
			case _T('C'): m_fileAction = FileCopy; return;
			case _T('M'): m_fileAction = FileMove; return;
			case _T('R'): m_fileAction = TargetFileDelete; return;
		}

	ThrowInvalidArgument();
}

void CXferOptions::ParseFileChangesFilter( const std::tstring& value ) throws_( CRuntimeException )
{
	if ( value.empty() )
		m_filterBy = CheckTimestamp;
	else if ( arg::Equals( _T("SIZE"), value.c_str() ) )
		m_filterBy = CheckFileSize;
	else if ( arg::Equals( _T("CRC32"), value.c_str() ) )
		m_filterBy = CheckFullContent;
	else
		ThrowInvalidArgument();
}

void CXferOptions::ParseFileAttributes( const std::tstring& value ) throws_( CRuntimeException )
{
	for ( std::tstring::const_iterator itCh = value.begin(); itCh != value.end(); ++itCh )
	{
		BYTE* pFileAttr = &m_mustHaveFileAttr;
		if ( _T('-') == *itCh )
		{
			pFileAttr = &m_mustNotHaveFileAttr;
			++itCh;
		}

		switch ( str::CharTraits::ToUpper( *itCh ) )
		{
			case _T('A'): *pFileAttr |= CFile::archive; break;
			case _T('R'): *pFileAttr |= CFile::readOnly; break;
			case _T('H'): *pFileAttr |= CFile::hidden; break;
			case _T('S'): *pFileAttr |= CFile::system; break;
			case _T('D'): *pFileAttr |= CFile::directory; break;
			default: ThrowInvalidArgument();
		}
	}
}

// argument format: "/D=m-d-y h:min:s"

void CXferOptions::ParseTimestamp( const std::tstring& value ) throws_( CRuntimeException )
{
	m_filterBy = CheckTimestamp;
	if ( value.empty() )
		return;

	int month = -1, day = -1, year = -1, hour = 0, min = 0, sec = 0;

	if ( _stscanf( m_pArg + 3, _T("%d-%d-%d %d:%d:%d"), &month, &day, &year, &hour, &min, &sec ) >= 3 )
		if ( day >= 1 && day <= 31 &&
			 month >= 1 && month <= 12 &&
			 year >= 1900 &&
			 hour >= 0 && hour < 24 &&
			 min >= 0 && min < 60 &&
			 sec >= 0 && sec < 60 )
		{
			m_earliestTimestamp = CTime( year, month, day, hour, min, sec );
			return;
		}

	throw CRuntimeException( str::Format( _T("Invalid date-time in argument '%s'"), m_pArg ) );
}

void CXferOptions::PostProcessArguments( void ) throws_( CRuntimeException )
{
	if ( m_sourceDirPath.IsEmpty() )
		throw CRuntimeException( _T("Missing 'source_filter' argument!") );

	// split m_sourceDirPath into path and search specifiers
	if ( !fs::IsValidDirectory( m_sourceDirPath.GetPtr() ) )
	{
		m_searchSpecs = m_sourceDirPath.GetFilename();
		m_sourceDirPath = m_sourceDirPath.GetParentPath();
	}

	if ( m_searchSpecs.empty() )
		m_searchSpecs = _T("*.*");

	fs::CvtAbsoluteToCWD( m_sourceDirPath );
	fs::CvtAbsoluteToCWD( m_targetDirPath );
}

void CXferOptions::ThrowInvalidArgument( void ) throws_( CRuntimeException )
{
	throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
}


void CXferOptions::ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException )
{
	for ( int i = 1; i != argc; ++i )
	{
		m_pArg = argv[ i ];

		if ( arg::IsSwitch( m_pArg ) )
		{
			const TCHAR* pSwitch = m_pArg + 1;
			std::tstring value;

			if ( arg::ParseValuePair( value, pSwitch, _T("TRANSFER|T") ) )
				ParseFileAction( value );
			else if ( arg::ParseOptionalValuePair( &value, pSwitch, _T("BK") ) )
				m_pBackupDirPath.reset( new fs::TDirPath( value ) );
			else if ( arg::ParseOptionalValuePair( &value, pSwitch, _T("CH") ) )
				ParseFileChangesFilter( value );
			else if ( arg::ParseValuePair( value, pSwitch, _T("ATTRIBUTES|A") ) )
				ParseFileAttributes( value );
			else if ( arg::ParseValuePair( value, pSwitch, _T("D") ) )
				ParseTimestamp( value );
			else if ( arg::ParseValuePair( value, pSwitch, _T("EXCLUDE") ) )
			{
				if ( !path::ContainsWildcards( value.c_str() ) )
					str::Tokenize( m_excludeFindSpecs, value.c_str(), m_specDelims );
				else
					throw CRuntimeException( str::Format( _T("Cannot use wildcards in exclusion argument '%s'"), m_pArg ) );
			}
			else if ( arg::ParseValuePair( value, pSwitch, _T("EW") ) )
				m_excludeWildSpec = value;
			else if ( arg::Equals( pSwitch, _T("Q") ) )
				m_displayFileNames = false;
			else if ( arg::Equals( pSwitch, _T("LS") ) )
				m_transferMode = JustDisplaySourceFile;
			else if ( arg::Equals( pSwitch, _T("LT") ) )
				m_transferMode = JustDisplayTargetFile;
			else if ( arg::EqualsAnyOf( pSwitch, _T("S|S+") ) )
				m_recurseSubDirectories = true;
			else if ( arg::Equals( pSwitch, _T("S-") ) )
				m_recurseSubDirectories = false;
			else if ( arg::Equals( pSwitch, _T("JD") ) )
				m_justCreateTargetDirs = true;
			else if ( arg::Equals( pSwitch, _T("R") ) )
				m_overrideReadOnlyFiles = true;
			else if ( arg::Equals( pSwitch, _T("U") ) )
				m_transferOnlyExistentTargetFiles = true;
			else if ( arg::Equals( pSwitch, _T("UD") ) )
				m_transferOnlyToExistentTargetDirs = true;
			else if ( arg::Equals( pSwitch, _T("Y") ) )
				m_userPrompt = PromptNever;
			else if ( arg::Equals( pSwitch, _T("Y-") ) )
				m_userPrompt = PromptAlways;
			else if ( arg::EqualsAnyOf( pSwitch, _T("?|H") ) )
			{
				m_helpMode = true;
				return;
			}
			else if ( arg::EqualsAnyOf( pSwitch, _T("UT|DEBUG|NDEBUG|NODEBUG") ) )
				continue;							// consume known debug args
			else
				ThrowInvalidArgument();
		}
		else
		{
			if ( m_sourceDirPath.IsEmpty() )
				m_sourceDirPath.Set( m_pArg );
			else if ( m_targetDirPath.IsEmpty() )
				m_targetDirPath.Set( m_pArg );
			else
				throw CRuntimeException( str::Format( _T("Unrecognized argument '%s'"), m_pArg ) );
		}
	}

	PostProcessArguments();
}
