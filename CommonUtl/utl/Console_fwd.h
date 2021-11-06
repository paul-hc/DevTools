#ifndef Console_fwd_h
#define Console_fwd_h
#pragma once

#include <iosfwd>


namespace io
{
	enum TranslationMode
	{
		Ansi,		// file mode _O_TEXT: the default - no redirection of global output streams
		Utf8,		// file mode _O_U8TEXT: output natively to std::cout|std::cerr, and redirect std::wcout|std::wcerr to std::cout|std::cerr - IMP: not really functional on Windows!
		Utf16		// file mode _O_U16TEXT: output natively to std::wcout|std::wcerr, and redirect std::cout|std::cerr to std::wcout|std::wcerr
	};


	// Handles redirection of std::cout|std::cerr to std::wcout|std::wcerr and viceversa depending on CConsoleApplication::s_outputTextMode.
	// This prevents assertion failures in CRT when trying to for example output to std::cout when the global console mode is ioUtf16 (and std::wcout should be used throughout the application)
	//
	class CScopedRedirectStream
	{
	public:
		CScopedRedirectStream( std::ostream& rStdOutStream );		// i.e. std::cout|std::cerr
		CScopedRedirectStream( std::wostream& rStdOutStream );		// i.e. std::wcout|std::wcerr
		~CScopedRedirectStream() { Flush(); }

		void Flush( void ) { m_pRedirector.reset(); }
	private:
		template< typename ProxyStreamT, typename DestStreamT >
		class CRedirectStream : public utl::IMemoryManaged
		{
			typedef typename ProxyStreamT::char_type TProxyChar;
			typedef std::basic_streambuf< TProxyChar > TOStreamBuf;
			typedef std::basic_ostringstream< TProxyChar > TOStringStream;
		public:
			CRedirectStream( ProxyStreamT& rProxyOStream, DestStreamT& rDestOStream )		// i.e. std::cout|std::cerr, std::wcout|std::wcerr
				: m_rProxyOStream( rProxyOStream )
				, m_pOrigStdStreamBuf( m_rProxyOStream.rdbuf() )
				, m_rDestOStream( rDestOStream )
			{
				m_rProxyOStream.rdbuf( m_tempOutStringStream.rdbuf() );		// redirect cout

				// now output is redirected into the temporary string stream in the current scope
			}

			virtual ~CRedirectStream()
			{
				m_rProxyOStream.rdbuf( m_pOrigStdStreamBuf );				// restore old cout
				m_rDestOStream << m_tempOutStringStream.str();				// output our temporary string to the intended destination output stream
			}
		private:
			ProxyStreamT& m_rProxyOStream;
			TOStreamBuf* m_pOrigStdStreamBuf;
			TOStringStream m_tempOutStringStream;

			DestStreamT& m_rDestOStream;			// where the output has to be redirected to
		};
	private:
		std::auto_ptr<utl::IMemoryManaged> m_pRedirector;
	};
}


#endif // Console_fwd_h
