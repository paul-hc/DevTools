#ifndef StdOutput_h
#define StdOutput_h
#pragma once

#include "Encoding.h"
#include "Path.h"


namespace io
{
	enum OutputMode { ConsoleOutput, FileRedirectedOutput, OtherOutput };


	// provides super-fast console output, or if redirected to a file handles the output with the desired BOM encoding
	//
	class CStdOutput
	{
	public:
		CStdOutput( DWORD stdHandle = STD_OUTPUT_HANDLE );		// STD_OUTPUT_HANDLE or STD_ERROR_HANDLE
		~CStdOutput() { Flush(); }

		bool IsValid( void ) const { return m_hStdOutput != INVALID_HANDLE_VALUE; }
		bool IsConsoleOutput( void ) const { return m_isConsoleOutput; }
		OutputMode GetOutputMode( void ) const { ASSERT( IsValid() ); return m_outputMode; }
		const fs::CPath& GetFileRedirectPath( void ) const { return m_fileRedirectPath; }

		void Write( const std::string& allText, fs::Encoding fileEncoding = fs::UTF8_bom ) throws_( CRuntimeException );
		void Write( const std::wstring& allText, fs::Encoding fileEncoding = fs::UTF16_LE_bom ) throws_( CRuntimeException );

		bool Flush( void );
	private:
		template< typename CharT >
		void WriteToConsole( const CharT* pText, size_t length ) throws_( CRuntimeException );

		template< typename CharT >
		void WriteToFile( const CharT* pText, size_t length, fs::Encoding fileEncoding ) throws_( CRuntimeException );
	private:
		HANDLE m_hStdOutput;
		DWORD m_fileType;
		bool m_isConsoleOutput;
		OutputMode m_outputMode;
		fs::CPath m_fileRedirectPath;
	public:
		static size_t s_maxBatchSize;		// 8 KB by default (large-enough for high speed output)
	};
}


#endif // StdOutput_h
