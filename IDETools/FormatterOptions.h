#ifndef FormatterOptions_h
#define FormatterOptions_h
#pragma once


namespace code
{
	enum TokenSpacing { RemoveSpace, InsertOneSpace, PreserveSpace };


	class CFormatterOptions
	{
	public:
		struct CBraceRule;
		struct COperatorRule;

		CFormatterOptions( void );
		~CFormatterOptions();

		void LoadFromRegistry( void );
		void SaveToRegistry( void ) const;

		CBraceRule* FindBraceRule( TCHAR chr ) const;

		TokenSpacing MustSpaceBrace( TCHAR chr ) const;
		bool IsArgListBrace( TCHAR chr ) const;

		COperatorRule* FindOperatorRule( const TCHAR* pOpStart ) const;

		const std::tstring* FindBreakSeparator( const TCHAR* pBreakSepStart ) const;

		std::tstring GetArgListOpenBraces( void ) const;
	public:
		// Helpers
		static TokenSpacing SpacingFromChar( TCHAR chr );
		static TCHAR SpacingToChar( TokenSpacing spacing );
	public:
		// formatting rules for braces
		struct CBraceRule
		{
			CBraceRule( TCHAR braceOpen = _T('\0'), TCHAR braceClose = _T('\0'), TokenSpacing spacing = PreserveSpace, bool isArgList = false )
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
			TokenSpacing m_spacing;
			bool m_isArgList;
		private:
			std::tstring m_regEntry;
		};

		// formatting rules for special operators (commas, semi-colons, etc)
		struct COperatorRule
		{
			COperatorRule( const TCHAR* pOperator = _T(""), TokenSpacing spaceBefore = PreserveSpace, TokenSpacing spaceAfter = PreserveSpace )
				: m_pOperator( pOperator ), m_spaceBefore( spaceBefore ), m_spaceAfter( spaceAfter ), m_regEntry( m_pOperator ) {}

			void LoadFromRegistry( void );
			void SaveToRegistry( void ) const;

			size_t GetOperatorLength( void ) const { return str::GetLength( m_pOperator ); }
		public:
			const TCHAR* m_pOperator;
			TokenSpacing m_spaceBefore;
			TokenSpacing m_spaceAfter;
		private:
			std::tstring m_regEntry;
		};
	public:
		const std::vector< COperatorRule >& GetOperatorRules( void ) const { return m_operatorRules; }
		void SetOperatorRules( const std::vector< COperatorRule >& operatorRules ) { m_operatorRules = operatorRules; }
	public:
		std::vector< std::tstring > m_breakSeparators;
		bool m_preserveMultipleWhiteSpace;
		bool m_deleteTrailingWhiteSpace;
		int m_linesBetweenFunctionImpls;
		bool m_returnTypeOnSeparateLine;
		bool m_commentOutDefaultParams;

		std::vector< CBraceRule > m_braceRules;
	private:
		std::vector< COperatorRule > m_operatorRules;
		std::vector< COperatorRule > m_sortedOperatorRules;		// rules ordered descending by length: longest-first - for looking up matches
	};

} // namespace code


#endif // FormatterOptions_h
