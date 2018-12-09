
#include "stdafx.h"
#include "XferOptions.h"
#include "TransferItem.h"
#include "utl/ConsoleInputOutput.h"
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
	, m_filterByTimestamp( false )
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
	DWORD sourceAttributes = transferNode.m_sourceFileInfo.m_attributes;

	ASSERT( sourceAttributes != INVALID_FILE_ATTRIBUTES );

	if ( m_mustHaveFileAttr != 0 )
		if ( ( sourceAttributes & m_mustHaveFileAttr ) != m_mustHaveFileAttr )
			return false;

	if ( m_mustNotHaveFileAttr != 0 )
		if ( ( sourceAttributes & m_mustNotHaveFileAttr ) != 0 )
			return false;

	if ( m_filterByTimestamp )
		if ( m_earliestTimestamp != CTime( 0 ) )
		{
			TRACE( _T("<earliest: %s>  <source: %s>\n"),
				   (LPCTSTR)m_earliestTimestamp.Format( _T("%b-%d-%Y") ),
				   (LPCTSTR)transferNode.m_sourceFileInfo.m_lastModifyTime.Format( _T("%b-%d-%Y") ) );

			if ( transferNode.m_sourceFileInfo.m_lastModifyTime < m_earliestTimestamp )
				return false;

			TRACE( _T("  copy newer: %s\n"), transferNode.m_sourceFileInfo.m_filePath.GetPtr() );
		}
		else
		{
			TRACE( _T("<source: %s>"),
				(LPCTSTR)transferNode.m_sourceFileInfo.m_lastModifyTime.Format( _T("%b-%d-%Y") ) );

			if ( transferNode.m_targetFileInfo.Exist() )
				TRACE( _T("  <target: %s>"),
					(LPCTSTR)transferNode.m_targetFileInfo.m_lastModifyTime.Format( _T("%b-%d-%Y") ) );
			TRACE( _T("\n") );

			if ( transferNode.m_targetFileInfo.Exist() )
				if ( transferNode.m_sourceFileInfo.m_lastModifyTime <= transferNode.m_targetFileInfo.m_lastModifyTime )
					return false;

			TRACE( _T("  copy newer: %s\n  to:         %s\n"),
				transferNode.m_sourceFileInfo.m_filePath.GetPtr(),
				transferNode.m_targetFileInfo.m_filePath.GetPtr() );
		}

	if ( m_transferOnlyToExistentTargetDirs )
		if ( !transferNode.m_targetFileInfo.DirPathExist() )
			return false;

	if ( m_transferOnlyExistentTargetFiles )
		if ( !transferNode.m_targetFileInfo.Exist() )
			return false;

	if ( !m_excludeWildSpec.empty() )
		if ( path::MatchWildcard( transferNode.m_sourceFileInfo.m_filePath.GetPtr(), m_excludeWildSpec.c_str() ) != _T('\0') )
			return false;

	if ( !m_excludeFindSpecs.empty() )
		for ( std::vector< std::tstring >::const_iterator itSubstrSpec = m_excludeFindSpecs.begin(); itSubstrSpec != m_excludeFindSpecs.end(); ++itSubstrSpec )
			if ( *path::Find( transferNode.m_sourceFileInfo.m_filePath.GetPtr(), itSubstrSpec->c_str() ) != _T('\0') )
				return false;

	return true;
}

bool CXferOptions::ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt /*= AsIs*/ )
{
	if ( arg::ParseValuePair( rValue, pArg, pNameList ) )
		switch ( caseCvt )
		{
			case UpperCase: str::ToUpper( rValue ); return true;
			case LowerCase: str::ToLower( rValue ); return true;
		}

	return false;
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

			if ( ParseValue( value, pSwitch, _T("TRANSFER|T"), UpperCase ) )
				ParseFileAction( value );
			else if ( ParseValue( value, pSwitch, _T("ATTRIBUTES|A"), UpperCase ) )
				ParseFileAttributes( value );
			else if ( ParseValue( value, pSwitch, _T("D") ) )
				ParseTimestamp( value );
			else if ( ParseValue( value, pSwitch, _T("EXCLUDE") ) )
			{
				if ( !path::ContainsWildcards( value.c_str() ) )
					str::Tokenize( m_excludeFindSpecs, value.c_str(), m_specDelims );
				else
					throw CRuntimeException( str::Format( _T("Cannot use wildcards in exclusion argument '%s'"), m_pArg ) );
			}
			else if ( ParseValue( value, pSwitch, _T("EW") ) )
				m_excludeWildSpec = value;
			else if ( arg::ParseOptionalValuePair( &value, pSwitch, _T("BK") ) )
				m_pBackupDirPath.reset( new fs::CPath( value ) );
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

void CXferOptions::ParseFileAction( const std::tstring& value ) throws_( CRuntimeException )
{
	if ( !value.empty() )
		switch ( value[ 0 ] )
		{
			case _T('C'): m_fileAction = FileCopy; return;
			case _T('M'): m_fileAction = FileMove; return;
			case _T('R'): m_fileAction = TargetFileDelete; return;
		}

	ThrowInvalidArgument();
}

void CXferOptions::ParseFileAttributes( const std::tstring& value ) throws_( CRuntimeException )
{
	for ( std::tstring::const_iterator itCh = value.begin(); itCh != value.end(); ++itCh )
	{
		DWORD* pFileAttr = &m_mustHaveFileAttr;
		if ( _T('-') == *itCh )
		{
			pFileAttr = &m_mustNotHaveFileAttr;
			++itCh;
		}

		switch ( *itCh )
		{
			case _T('A'): *pFileAttr |= FILE_ATTRIBUTE_ARCHIVE; break;
			case _T('R'): *pFileAttr |= FILE_ATTRIBUTE_READONLY; break;
			case _T('H'): *pFileAttr |= FILE_ATTRIBUTE_HIDDEN; break;
			case _T('S'): *pFileAttr |= FILE_ATTRIBUTE_SYSTEM; break;
			case _T('D'): *pFileAttr |= FILE_ATTRIBUTE_DIRECTORY; break;
			default: ThrowInvalidArgument();
		}
	}
}

// argument format: "/D=m-d-y h:min:s"

void CXferOptions::ParseTimestamp( const std::tstring& value ) throws_( CRuntimeException )
{
	m_filterByTimestamp = true;
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
		m_searchSpecs = m_sourceDirPath.GetNameExt();
		m_sourceDirPath = m_sourceDirPath.GetParentPath();
	}
//	else
//		path::SetBackslash( m_sourceDirPath );

	if ( m_searchSpecs.empty() )
		m_searchSpecs = _T("*.*");

	fs::CvtAbsoluteToCWD( m_sourceDirPath );
//	if ( fs::IsValidDirectory( m_sourceDirPath.c_str() ) )
//		path::SetBackslash( m_sourceDirPath );

	fs::CvtAbsoluteToCWD( m_targetDirPath );
//	path::SetBackslash( m_targetDirPath );
}

void CXferOptions::ThrowInvalidArgument( void ) throws_( CRuntimeException )
{
	throw CRuntimeException( str::Format( _T("invalid argument '%s'"), m_pArg ) );
}
