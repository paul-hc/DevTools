
#include "pch.h"
#include "Logger.h"
#include "FileSystem.h"
#include "RuntimeException.h"
#include "AppTools.h"
#include "Algorithms.h"
#include "TimeUtils.h"
#include <fstream>


CLogger::CLogger( const TCHAR* pFmtFname /*= NULL*/ )
	: m_pFmtFname( pFmtFname )
	, m_enabled( true )
	, m_prependTimestamp( true )
	, m_logFileMaxSize( DefaultSize )
	, m_checkLogLineCount( 100 )
	, m_logCount( 0 )
	, m_addSessionNewLine( true )
{
	SetIndentLinesBy( 2 );
}

CLogger::~CLogger()
{
}

void CLogger::Clear( void )
{
	if ( FILE* pLogFile = _tfopen( GetLogFilePath().GetPtr(), _T("wt") ) )
		::fclose( pLogFile );
}

void CLogger::SetOverwrite( void )
{
	fs::CPath backupLogPath = MakeBackupLogFilePath();

	if ( fs::FileExist( backupLogPath.GetPtr(), fs::Write ) )
		::DeleteFile( backupLogPath.GetPtr() );

	const fs::CPath& logPath = GetLogFilePath();
	if ( fs::FileExist( logPath.GetPtr(), fs::Write ) )
		::MoveFile( logPath.GetPtr(), backupLogPath.GetPtr() );

	Clear();
}

const fs::CPath& CLogger::GetLogFilePath( void ) const
{
	if ( m_logFilePath.IsEmpty() )
	{
		fs::CPathParts parts( app::GetModulePath().Get() );

		if ( !str::IsEmpty( m_pFmtFname ) )
			parts.m_fname = str::Format( m_pFmtFname, parts.m_fname.c_str() );
		parts.m_ext = _T(".log");

		m_logFilePath = parts.MakePath();
		ENSURE( m_logFilePath.IsValid() );
	}
	return m_logFilePath;
}

fs::CPath CLogger::MakeBackupLogFilePath( void ) const
{
	fs::CPathParts parts( GetLogFilePath().Get() );
	parts.m_fname += _T("_bak");
	return parts.MakePath();
}

void CLogger::Log( const TCHAR format[], ... )
{
	va_list argList;

	va_start( argList, format );
	LogV( format, argList );
	va_end( argList );
}

void CLogger::LogTrace( const TCHAR format[], ... )
{
	CString entry;
	va_list argList;

	va_start( argList, format );
	entry.FormatV( format, argList );
	va_end( argList );

	TRACE( _T("-LogTrace: %s\n"), entry.GetString() );
	LogLine( entry.GetString() );
}

void CLogger::LogV( const TCHAR format[], va_list argList )
{
	CString entry;
	entry.FormatV( format, argList );
	LogLine( entry.GetString() );
}

void CLogger::LogLine( const TCHAR text[], bool useTimestamp /*= true*/ )
{
	if ( !m_enabled )
		return;

	CSingleLock logLocker( &m_cs, true );		// serialize access to log file

	if ( m_checkLogLineCount > 0 )
		if ( !( ++m_logCount % m_checkLogLineCount ) )
			CheckTruncate();

	std::ofstream output( str::ToUtf8( GetLogFilePath().GetPtr() ).c_str(), std::ios_base::out | std::ios_base::app );
	if ( output.is_open() )
	{
		if ( m_addSessionNewLine )
		{
			if ( CheckNeedSessionNewLine() )
				output << std::endl;

			m_addSessionNewLine = false;
		}

		if ( m_prependTimestamp && useTimestamp )
			output << CTime::GetCurrentTime().Format( _T("[%d-%b-%Y %H:%M:%S]> ") ).GetString();

		output << FormatMultiLineText( text ) << std::endl;
		output.close();
	}
	else
		ASSERT( false );
}

void CLogger::SetIndentLinesBy( size_t indentLinesBy )
{
	m_indentLineEnd.assign( indentLinesBy + 1, _T(' ') );
	m_indentLineEnd[ 0 ] = _T('\n');
}

const TCHAR* CLogger::FormatMultiLineText( const TCHAR text[] )
{
	ASSERT_PTR( text );
	ASSERT( !m_indentLineEnd.empty() );		// at least should contain the "\n"

	if ( 1 == m_indentLineEnd.size() || nullptr == _tcschr( text, _T('\n') ) )
	{
		m_multiLineText.clear();
		return text;			// single line, no buffered indentation
	}

	m_multiLineText = text;
	str::Replace( m_multiLineText, _T("\n"), m_indentLineEnd.c_str() );
	return m_multiLineText.c_str();
}

bool CLogger::CheckTruncate( void )
{
	fs::CPath newLogFilePath = GetLogFilePath().Get() + _T(".new");

	try
	{
		CStdioFile logFile;

		if ( !logFile.Open( GetLogFilePath().GetPtr(), CFile::modeRead | CFile::typeText ) )
			return false;
		if ( logFile.GetLength() <= m_logFileMaxSize )
			return false;

		TCHAR lineBuffer[ 2048 ];

		// move to 1/5 of the maximum length of the log file in order to preserve it
		logFile.Seek( -( m_logFileMaxSize / 5 ), CFile::end );

		// skip the incomplete line
		if ( !logFile.ReadString( lineBuffer, 2048 ) )
			return false;

		CStdioFile newLogFile;

		if ( !newLogFile.Open( newLogFilePath.GetPtr(), CFile::modeCreate | CFile::modeWrite | CFile::typeText ) )
			return false;

		// copy the remaining contents of old log file to the new file
		while ( logFile.ReadString( lineBuffer, 2048 ) != NULL )
			newLogFile.WriteString( lineBuffer );
	}
	catch ( CException* pExc )
	{
		TRACE( _T("* C++ exception: %s\n"), mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
		pExc->Delete();
	}

	::DeleteFile( GetLogFilePath().GetPtr() );
	::MoveFile( newLogFilePath.GetPtr(), GetLogFilePath().GetPtr() );
	return true;
}

bool CLogger::CheckNeedSessionNewLine( void ) const
{
	const fs::CPath& logFilePath = GetLogFilePath();
	const CTime lastModifyTime = fs::ReadLastModifyTime( logFilePath );

	if ( !time_utl::IsValid( lastModifyTime ) )
		return false;			// no existing log file

	CTimeSpan expireBy( 0, 0, 10, 0 );		// ten minutes
	CTime referenceTime = CTime::GetCurrentTime() - expireBy;

	if ( CTime::GetCurrentTime() - lastModifyTime < expireBy )		// less than 10 minutes session timeout?
		return false;			// don't bother checking: not long-enough time has passed since last file modification

	std::ifstream input( logFilePath.GetPtr(), std::ios_base::in );
	if ( !input.is_open() )
		return false;			// can't read the file

	if ( !input.seekg( -2, std::ios_base::end ) )
		return false;			// file is no large-enough for double empty lines check

	std::pair<TCHAR, TCHAR> lastChars;

	if ( !input >> lastChars.first >> lastChars.second )
		return false;			// realing last 2 chars failed

	if ( _T('\n') == lastChars.first && _T('\n') == lastChars.second )		 // log file contains already 2 last empty lines?
		return false;			// avoid triple-ing the last empty lines

	TRACE( _T(" (!) CLogger::CheckNeedSessionNewLine(): adding a new session line in the log file: '%s'\n"), logFilePath.GetPtr() );
	return true;
}
