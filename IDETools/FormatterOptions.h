#ifndef FormatterOptions_h
#define FormatterOptions_h
#pragma once

#include "utl/Code_fwd.h"


namespace code
{
	class CFormatterOptions
	{
	public:
		struct CBraceRule;
		struct COperatorRule;

		CFormatterOptions( bool testMode = false );		// by default production mode
		~CFormatterOptions();

		void LoadFromRegistry( void );
		void SaveToRegistry( void ) const;

		CBraceRule* FindBraceRule( TCHAR chr ) const;

		Spacing MustSpaceBrace( TCHAR chr ) const;
		bool IsArgListBrace( TCHAR chr ) const;

		COperatorRule* FindOperatorRule( const TCHAR* pOpStart ) const;

		const std::tstring* FindBreakSeparator( const TCHAR* pBreakSepStart ) const;

		std::tstring GetArgListOpenBraces( void ) const;
	public:
		// Helpers
		static Spacing SpacingFromChar( TCHAR chr );
		static TCHAR SpacingToChar( Spacing spacing );
	public:
		// formatting rules for braces
		struct CBraceRule
		{
			CBraceRule( TCHAR braceOpen = '\0', TCHAR braceClose = '\0', Spacing spacing = RetainSpace, bool isArgList = false )
				: m_braceOpen( braceOpen )
				, m_braceClose( braceClose )
				, m_spacing( spacing )
				, m_isArgList( isArgList )
				, m_regEntry( str::Format( _T("%c%c"), m_braceOpen, m_braceClose ) )
			{
			}

			void LoadFromRegistry( void );
			void SaveToRegistry( void ) const;
		public:
			TCHAR m_braceOpen;
			TCHAR m_braceClose;
			Spacing m_spacing;
			bool m_isArgList;			// i.e. splitable
		private:
			std::tstring m_regEntry;
		};

		// formatting rules for special operators (commas, semi-colons, etc)
		struct COperatorRule
		{
			COperatorRule( const TCHAR* pOperator = _T(""), Spacing spaceBefore = RetainSpace, Spacing spaceAfter = RetainSpace )
				: m_pOperator( pOperator ), m_spaceBefore( spaceBefore ), m_spaceAfter( spaceAfter ), m_regEntry( m_pOperator ) {}

			void LoadFromRegistry( void );
			void SaveToRegistry( void ) const;

			size_t GetOperatorLength( void ) const { return str::GetLength( m_pOperator ); }
		public:
			const TCHAR* m_pOperator;
			Spacing m_spaceBefore;
			Spacing m_spaceAfter;
		private:
			std::tstring m_regEntry;
		};
	public:
		const std::vector<COperatorRule>& GetOperatorRules( void ) const { return m_operatorRules; }
		void SetOperatorRules( const std::vector<COperatorRule>& operatorRules ) { m_operatorRules = operatorRules; }
	public:
		const bool m_testMode;

		std::vector<std::tstring> m_breakSeparators;
		bool m_preserveMultipleWhiteSpace;
		bool m_deleteTrailingWhiteSpace;
		int m_linesBetweenFunctionImpls;
		bool m_returnTypeOnSeparateLine;
		bool m_commentOutDefaultParams;

		std::vector<CBraceRule> m_braceRules;
	private:
		std::vector<COperatorRule> m_operatorRules;
		std::vector<COperatorRule> m_sortedOperatorRules;		// rules ordered descending by length: longest-first - for looking up matches
	};

} // namespace code


#endif // FormatterOptions_h
