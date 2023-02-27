#ifndef Logger_h
#define Logger_h
#pragma once

#include <afxmt.h>
#include "Path.h"


class CLogger
{
public:
	enum { DefaultSize = 512 * KiloByte };

	CLogger( const TCHAR* pFmtFname = nullptr );		// could be _T("%s-suffix") for "exe_name-suffix.log"
	~CLogger();

	const fs::CPath& GetLogFilePath( void ) const;

	size_t GetIndentLinesBy( void ) const { return m_indentLineEnd.size() - 1; }		// exluding the '\n'
	void SetIndentLinesBy( size_t indentLinesBy );

	// pass '\n' for line ends, since the log file is open in text mode
	void Log( const TCHAR format[], ... );
	void LogV( const TCHAR format[], va_list argList );
	void LogLine( const TCHAR text[], bool useTimestamp = true );

	void LogString( const std::tstring& text ) { LogLine( text.c_str() ); }

	void LogTrace( const TCHAR format[], ... );			// output to log file and TRACE to output window in VC++

	void Clear( void );
	void SetOverwrite( void );
protected:
	fs::CPath MakeBackupLogFilePath( void ) const;
	const TCHAR* FormatMultiLineText( const TCHAR text[] );
	bool CheckTruncate( void );
	bool CheckNeedSessionNewLine( void ) const;
private:
	const TCHAR* m_pFmtFname;
	std::tstring m_indentLineEnd;	// by default 2 spaces - for multi-line logging: indentation size of sub-lines
	std::tstring m_multiLineText;	// temporary buffer for indenting multiple lines
public:
	bool m_enabled;
	bool m_prependTimestamp;
	int m_logFileMaxSize;
	int m_checkLogLineCount;		// count of logs when the file overflow is checked
	int m_logCount;

	bool m_addSessionNewLine;		// adds a new line on first log entry; prevents adding consecutive empty session lines if no logging
private:
	CCriticalSection m_cs;
	mutable fs::CPath m_logFilePath;
};


#if !defined( BUILD_UTL_BASE ) && !defined( BUILD_UTL_UI )		// compiling application code?
	#include "utl/AppTools.h"		// for app::GetLogger()
#endif


#ifdef _DEBUG
	#define LOG_TRACE app::GetLogger()->LogTrace
#else
	#define LOG_TRACE __noop
#endif


#endif // Logger_h
