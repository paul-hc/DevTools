
#include "stdafx.h"
#include "Logger.h"
#include "Path.h"
#include "Utilities.h"
#include <fstream>


CLogger::CLogger( const TCHAR* pFnameFmt /*= NULL*/ )
	: m_pFnameFmt( pFnameFmt )
	, m_enabled( true )
	, m_prependTimestamp( true )
	, m_logFileMaxSize( DefaultSize )
	, m_checkLogLineCount( 100 )
	, m_logCount( 0 )
{
}

CLogger::~CLogger()
{
}

void CLogger::Clear( void )
{
	if ( FILE* pLogFile = _tfopen( GetLogFilePath().c_str(), _T("wt") ) )
		fclose( pLogFile );
}

void CLogger::SetOverwrite( void )
{
	std::tstring backupLogPath = MakeBackupLogFilePath();

	if ( fs::FileExist( backupLogPath.c_str(), fs::Write ) )
		::DeleteFile( backupLogPath.c_str() );

	const std::tstring& logPath = GetLogFilePath();
	if ( fs::FileExist( logPath.c_str(), fs::Write ) )
		::MoveFile( logPath.c_str(), backupLogPath.c_str() );

	Clear();
}

const std::tstring& CLogger::GetLogFilePath( void ) const
{
	if ( m_logFilePath.empty() )
	{
		fs::CPathParts parts( ui::GetModuleFileName() );

		if ( !str::IsEmpty( m_pFnameFmt ) )
			parts.m_fname = str::Format( m_pFnameFmt, parts.m_fname.c_str() );
		parts.m_ext = _T(".log");

		m_logFilePath = parts.MakePath();
		ENSURE( path::IsValid( m_logFilePath ) );
	}
	return m_logFilePath;
}

std::tstring CLogger::MakeBackupLogFilePath( void ) const
{
	fs::CPathParts parts( GetLogFilePath() );
	parts.m_fname += _T("_bak");
	return parts.MakePath();
}

void CLogger::Log( const TCHAR* pFormat, ... )
{
	va_list argList;

	va_start( argList, pFormat );
	LogV( pFormat, argList );
	va_end( argList );
}

void CLogger::LogV( const TCHAR* pFormat, va_list argList )
{
	CString entry;
	entry.FormatV( pFormat, argList );
	LogLine( entry );
}

void CLogger::LogLine( const TCHAR* pText, bool useTimestamp /*= true*/ )
{
	ASSERT_PTR( pText );
	if ( !m_enabled )
		return;

	CSingleLock logLocker( &m_cs, true );		// serialize access to log file

	if ( m_checkLogLineCount > 0 )
		if ( !( ++m_logCount % m_checkLogLineCount ) )
			CheckTruncate();

	std::ofstream output( str::ToUtf8( GetLogFilePath().c_str() ).c_str(), std::ios_base::out | std::ios_base::app );
	if ( output.is_open() )
	{
		if ( m_prependTimestamp && useTimestamp )
			output << CTime::GetCurrentTime().Format( _T("[%d-%b-%Y %H:%M:%S]> ") ).GetString();

		output << pText << std::endl;
		output.close();
	}
	else
		ASSERT( false );
}

bool CLogger::CheckTruncate( void )
{
	std::tstring newLogFilePath = GetLogFilePath() + _T(".new");

	try
	{
		CStdioFile logFile;

		if ( !logFile.Open( GetLogFilePath().c_str(), CFile::modeRead | CFile::typeText ) )
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

		if ( !newLogFile.Open( newLogFilePath.c_str(), CFile::modeCreate | CFile::modeWrite | CFile::typeText ) )
			return false;

		// copy the remaining contents of old log file to the new file
		while ( logFile.ReadString( lineBuffer, 2048 ) != NULL )
			newLogFile.WriteString( lineBuffer );
	}
	catch ( CException* pExc )
	{
		TCHAR excMessage[ 512 ];
		pExc->GetErrorMessage( excMessage, 512 );
		TRACE( _T("* C++ exception: %s\n"), excMessage );
		pExc->Delete();
	}

	::DeleteFile( GetLogFilePath().c_str() );
	::MoveFile( newLogFilePath.c_str(), GetLogFilePath().c_str() );
	return true;
}
