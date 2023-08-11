
#include "pch.h"
#include "UserReport.h"
#include "AppTools.h"
#include "WndUtils.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"
#include "utl/RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// IUserReport implementation

	int IUserReport::ReportIssues( const CIssueStore& issues, UINT mbType /*= MB_OK*/ )
	{
		if ( issues.IsEmpty() )
			return CSilentMode::Instance().MessageBox( std::tstring(), mbType );

		return MessageBox( issues.FormatIssues(), mbType );
	}


	// CSilentMode implementation

	IUserReport& CSilentMode::Instance( void )
	{
		static CSilentMode s_silentMode;
		return s_silentMode;
	}

	int CSilentMode::MessageBox( const std::tstring& message, UINT mbType /*= MB_OK*/ )
	{
		message; mbType;
		switch ( mbType & MB_TYPEMASK )
		{
			default:
			case MB_OK:
			case MB_OKCANCEL:
				return IDOK;
			case MB_YESNOCANCEL:
			case MB_YESNO:
				return IDYES;
			case MB_RETRYCANCEL:
				return IDCANCEL;
			case MB_ABORTRETRYIGNORE:
				return IDIGNORE;
			case MB_CANCELTRYCONTINUE:
				return IDCONTINUE;
		}
	}

	int CSilentMode::ReportError( CException* pExc, UINT mbType /*= MB_OK*/ )
	{
		app::TraceException( pExc );
		pExc->Delete();
		return MessageBox( std::tstring(), mbType );
	}


	// CInteractiveMode implementation

	IUserReport& CInteractiveMode::Instance( void )
	{
		static CInteractiveMode s_interactiveMode;
		return s_interactiveMode;
	}

	int CInteractiveMode::MessageBox( const std::tstring& message, UINT mbType /*= MB_OK*/ )
	{
		return ui::MessageBox( message, mbType );
	}

	int CInteractiveMode::ReportError( CException* pExc, UINT mbType /*= MB_OK*/ )
	{
		ASSERT_PTR( pExc );
		int result = pExc->ReportError( mbType );
		pExc->Delete();
		return result;
	}


	// CIssueStore implementation

	const CEnumTags& CIssueStore::GetTags_Issue( void )
	{
		static const CEnumTags s_tags( _T("WARNING|ERROR") );
		return s_tags;
	}

	void CIssueStore::Clear( void )
	{
		utl::ClearOwningContainer( m_issues );
	}

	bool CIssueStore::HasErrors( void ) const
	{
		return std::find_if( m_issues.begin(), m_issues.end(), pred::IsError() ) != m_issues.end();
	}

	void CIssueStore::AddIssue( const std::tstring& message, Issue issue /*= Warning*/ )
	{
		m_issues.push_back( new CIssue( issue, message ) );
	}

	void CIssueStore::AddIssue( const std::exception& exc, Issue issue /*= Error*/ )
	{
		m_issues.push_back( new CIssue( issue, CRuntimeException::MessageOf( exc ) ) );
	}

	void CIssueStore::AddIssue( const CException* pExc, Issue issue /*= Error*/ )
	{
		m_issues.push_back( new CIssue( issue, mfc::CRuntimeException::MessageOf( *pExc ) ) );
	}

	std::tstring CIssueStore::FormatIssues( void ) const
	{
		std::tostringstream oss;
		if ( !m_header.empty() )
			oss << m_header << _T(":") << std::endl;

		if ( 1 == m_issues.size() )
			m_issues.front()->Stream( oss, utl::npos );			// no issue index
		else
			for ( size_t i = 0; i != m_issues.size(); ++i )
				m_issues[ i ]->Stream( oss, i );

		return oss.str();
	}

	bool CIssueStore::ThrowErrors( Exception exceptionType )
	{
		std::vector<CIssue*> errors;
		utl::QueryThat( errors, m_issues, pred::IsError() );
		if ( errors.empty() )
			return false;

		std::tostringstream oss;
		StreamIssues( oss, errors );

		switch ( exceptionType )
		{
			default: ASSERT( false );
			case StdException: throw CRuntimeException( oss.str() );
			case MfcException: throw new mfc::CRuntimeException( oss.str() );
		}
	}

	void CIssueStore::StreamIssues( std::tostringstream& oss, const std::vector<CIssue*>& issues ) const
	{
		if ( issues.empty() )
			return;

		if ( !m_header.empty() )
			oss << m_header << _T(":") << std::endl;

		for ( size_t i = 0; i != issues.size(); ++i )
			issues[ i ]->Stream( oss, i );
	}


	// CIssueStore::CIssue implementation

	void CIssueStore::CIssue::Stream( std::tostringstream& oss, size_t pos ) const
	{
		oss
			<< std::endl
			<< CIssueStore::GetTags_Issue().FormatUi( m_issue );
		if ( pos != utl::npos )
			oss << _T(" ") << ( pos + 1 );
		oss
			<< _T(":") << std::endl
			<< m_message << std::endl;
	}

	void CIssueStore::CIssue::Throw( Exception exceptionType )
	{
		ASSERT( Error == m_issue );
		switch ( exceptionType )
		{
			case StdException: throw CRuntimeException( m_message );
			case MfcException: throw new mfc::CRuntimeException( m_message );
		}
	}

} //namespace ui
