#ifndef StdOutput_h
#define StdOutput_h
#pragma once

#include <stdexcept>


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
		const std::string& GetFileRedirectPath( void ) const { return m_fileRedirectPath; }

		void Write( const std::string& allText ) throws_( std::runtime_error );

		bool Flush( void );
	private:
		void WriteToConsole( const char* pText, size_t length ) throws_( std::runtime_error );

		void WriteToFile( const char* pText, size_t length ) throws_( std::runtime_error );
	private:
		HANDLE m_hStdOutput;
		DWORD m_fileType;
		bool m_isConsoleOutput;
		OutputMode m_outputMode;
		std::string m_fileRedirectPath;
	public:
		static size_t s_maxBatchSize;		// 8 KB by default (large-enough for high speed output)
	};
}


#endif // StdOutput_h
