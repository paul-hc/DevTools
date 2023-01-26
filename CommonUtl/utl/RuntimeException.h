#ifndef RuntimeException_h
#define RuntimeException_h
#pragma once

#include <exception>


class CRuntimeException : public std::exception
{
public:
	CRuntimeException( void );
	CRuntimeException( const std::string& message );
	CRuntimeException( const std::wstring& message );
	virtual ~CRuntimeException() throw();

	virtual const char* what( void ) const throw() { return m_message.c_str(); }

	bool HasMessage( void ) const throw() { return !m_message.empty(); }
	std::tstring GetMessage( void ) const throw();
	void ReportError( void ) const throw();

	static std::tstring MessageOf( const std::exception& exc ) throw();
	static void __declspec(noreturn) ThrowFromMfc( CException* pExc ) throws_( CRuntimeException );
private:
	std::string m_message;		// UTF8
};


class CUserAbortedException : public CRuntimeException
{
public:
	CUserAbortedException( void ) : CRuntimeException( "Aborted by user!" ) {}
	CUserAbortedException( const std::tstring& message ) : CRuntimeException( message ) {}
};


namespace mfc
{
	class CRuntimeException : public CException
	{
	public:
		CRuntimeException( const std::tstring& message );		// w. auto-delete
		virtual ~CRuntimeException();

		virtual BOOL GetErrorMessage( TCHAR* pMessage, UINT count, UINT* pHelpContext = NULL ) const;

		static std::tstring MessageOf( const CException& exc ) throw();
	protected:
		std::tstring m_message;
	};


	class CUserAbortedException : public CRuntimeException
	{
	public:
		CUserAbortedException( void ) : CRuntimeException( _T("<aborted by user>") ) {}

		virtual int ReportError( UINT mbType = MB_OK, UINT messageId = 0 );
	};


	// for constructing auto (static/global) MFC exceptions

	template< class BaseClass >
	class CAutoException : public BaseClass
	{
	public:
		CAutoException( void ) { BaseClass::m_bAutoDelete = false; }
	};
}


namespace str
{
	inline std::tstring FormatException( const std::exception& exc, const TCHAR* pFormat = _T("%s") )
	{
		return str::Format( pFormat, CRuntimeException::MessageOf( exc ).c_str() );
	}

	inline std::tstring FormatException( const CException* pExc, const TCHAR* pFormat = _T("%s") )
	{
		return str::Format( pFormat, ::mfc::CRuntimeException::MessageOf( *pExc ).c_str() );
	}
}


#endif // RuntimeException_h
