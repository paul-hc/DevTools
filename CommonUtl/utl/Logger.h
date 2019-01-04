#ifndef Logger_h
#define Logger_h
#pragma once

#include <afxmt.h>
#include "Path.h"


class CLogger
{
public:
	enum { DefaultSize = 512 * KiloByte };

	CLogger( const TCHAR* pFmtFname = NULL );		// could be _T("%s-suffix") for "exe_name-suffix.log"
	~CLogger();

	const fs::CPath& GetLogFilePath( void ) const;

	void Log( const TCHAR format[], ... );
	void LogV( const TCHAR format[], va_list argList );
	void LogLine( const TCHAR text[], bool useTimestamp = true );

	void LogString( const std::tstring& text ) { LogLine( text.c_str() ); }

	void LogTrace( const TCHAR format[], ... );

	void Clear( void );
	void SetOverwrite( void );
protected:
	fs::CPath MakeBackupLogFilePath( void ) const;
	bool CheckTruncate( void );
private:
	const TCHAR* m_pFmtFname;
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


#endif // Logger_h
