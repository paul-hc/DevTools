#ifndef Logger_h
#define Logger_h
#pragma once

#include <afxmt.h>


class CLogger
{
public:
	enum { DefaultSize = 512 * KiloByte };

	CLogger( const TCHAR* pFnameFmt = NULL );		// could be _T("%s-suffix") for "exe_name-suffix.log"
	~CLogger();

	const std::tstring& GetLogFilePath( void ) const;

	void Log( const TCHAR* pFormat, ... );
	void LogV( const TCHAR* pFormat, va_list argList );
	void LogLine( const TCHAR* pText, bool useTimestamp = true );

	void LogString( const std::tstring& text ) { Log( text.c_str() ); }

	void Clear( void );
	void SetOverwrite( void );
protected:
	std::tstring MakeBackupLogFilePath( void ) const;
	bool CheckTruncate( void );
private:
	const TCHAR* m_pFnameFmt;
public:
	bool m_enabled;
	bool m_prependTimestamp;
	int m_logFileMaxSize;
	int m_checkLogLineCount;		// count of logs when the file overflow is checked
	int m_logCount;

	bool m_addSessionNewLine;		// adds a new line on first log entry; prevents adding consecutive empty session lines if no logging
private:
	CCriticalSection m_cs;
	mutable std::tstring m_logFilePath;
};


#endif // Logger_h
