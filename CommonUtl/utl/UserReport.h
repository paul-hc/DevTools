#ifndef UserReport_h
#define UserReport_h
#pragma once


class CEnumTags;


namespace ui
{
	class CIssueStore;


	interface IUserReport
	{
		virtual int MessageBox( const std::tstring& message, UINT mbType = MB_OK ) = 0;
		virtual int ReportError( CException* pExc, UINT mbType = MB_OK ) = 0;

		virtual int ReportIssues( const CIssueStore& issues, UINT mbType = MB_OK );
	};

	class CSilentMode : public IUserReport
	{
	public:
		static IUserReport& Instance( void );

		virtual int MessageBox( const std::tstring& message, UINT mbType = MB_OK );
		virtual int ReportError( CException* pExc, UINT mbType = MB_OK );
	};

	class CInteractiveMode : public IUserReport
	{
	public:
		static IUserReport& Instance( void );

		virtual int MessageBox( const std::tstring& message, UINT mbType = MB_OK );
		virtual int ReportError( CException* pExc, UINT mbType = MB_OK );
	};


	// accumulates transaction errors and warnings

	class CIssueStore : private utl::noncopyable
	{
	public:
		CIssueStore( const std::tstring& header = std::tstring() ) : m_header( header ) {}
		~CIssueStore() { Clear(); }

		void Clear( void );
		void Reset( const std::tstring& header ) { Clear(); m_header = header; }

		const std::tstring& GetHeader( void ) const { return m_header; }

		enum Issue { Warning, Error };
		static const CEnumTags& GetTags_Issue( void );

		enum Exception { StdException, MfcException };
		struct CIssue;

		bool IsEmpty( void ) const { return m_issues.empty(); }
		bool HasErrors( void ) const;
		const std::vector< CIssue* >& GetIssues( void ) const { return m_issues; }
		CIssue& BackIssue( void ) { ASSERT( !m_issues.empty() ); return *m_issues.back(); }

		void AddIssue( const std::tstring& message, Issue issue = Warning );
		void AddIssue( const std::exception& exc, Issue issue = Error );
		void AddIssue( const CException* pExc, Issue issue = Error );

		std::tstring FormatIssues( void ) const;
		bool ThrowErrors( Exception exceptionType );
	private:
		void StreamIssues( std::tostringstream& oss, const std::vector< CIssue* >& issues ) const;
	public:
		struct CIssue
		{
			CIssue( Issue issue, const std::tstring& message ) : m_issue( issue ), m_message( message ) {}

			void Stream( std::tostringstream& oss, size_t pos ) const;
			void Throw( Exception exceptionType );
		public:
			const Issue m_issue;
			const std::tstring m_message;
		};
	private:
		std::tstring m_header;
		std::vector< CIssue* > m_issues;
	};
}


namespace pred
{
	struct IsWarning
	{
		bool operator()( const ui::CIssueStore::CIssue* pIssue ) const
		{
			return ui::CIssueStore::Warning == pIssue->m_issue;
		}
	};

	struct IsError
	{
		bool operator()( const ui::CIssueStore::CIssue* pIssue ) const
		{
			return ui::CIssueStore::Error == pIssue->m_issue;
		}
	};
}


#endif // UserReport_h
