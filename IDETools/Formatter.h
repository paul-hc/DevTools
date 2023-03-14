#ifndef Formatter_h
#define Formatter_h
#pragma once

#include "utl/Code_fwd.h"
#include "DocLanguage.h"
#include "LanguageSearchEngine.h"


namespace code
{
	class CFormatterOptions;

	typedef unsigned int TEditorColumn;

	enum LineBreakTokenMatch { LBT_NoMatch, LBT_OpenBrace, LBT_CloseBrace, LBT_BreakSeparator };


	class CFormatter
	{
	public:
		CFormatter( const CFormatterOptions& options );
		~CFormatter();

		DocLanguage getDocLanguage( void ) const { return m_docLanguage; }
		void setDocLanguage( const TCHAR* tagDocLanguage );
		void setDocLanguage( DocLanguage docLanguage );

		void setTabSize( int _tabSize ) { m_tabSize = _tabSize; }
		void setUseTabs( bool _useTabs ) { m_useTabs = _useTabs; }


		enum MultiWhitespacePolicy
		{
			UseOptionsPolicy,				// according to m_options.m_preserveMultipleWhiteSpace
			ReplaceMultipleWhiteSpace,		// multi-whitespaces -> single-whitespace
			UseSplitPrototypePolicy			// protect whitespaces up to the arg list (...)
		};

		CString formatCode( const TCHAR* pCodeText, bool protectLeadingWhiteSpace = true, bool justAdjustWhiteSpace = false );
		CString formatLineOfCode( const TCHAR* lineOfCode, bool protectLeadingWhiteSpace = true, bool justAdjustWhiteSpace = false );

		CString tabifyLineOfCode( const TCHAR* lineOfCode, bool doTabify = true );

		CString splitArgumentList( const TCHAR* pCodeText, TEditorColumn maxColumn = UINT_MAX, int targetBracketLevel = -1 );

		CString toggleComment( const TCHAR* pCodeText );

		CString generateConsecutiveNumbers( const TCHAR* pCodeText, unsigned int startingNumber = UINT_MAX ) throws_( CRuntimeException );
		CString sortLines( const TCHAR* pCodeText, bool ascending ) throws_( CRuntimeException );

		Spacing MustSpaceBrace( TCHAR chrBrace ) const;
	protected:
		void resetInternalState( void );
	protected:
		int splitMultipleLines( std::vector< CString >& outLinesOfCode, std::vector< CString >& outLineEnds,
								const TCHAR* pCodeText );
		CString unsplitMultipleLines( const std::vector< CString >& linesOfCode, const std::vector< CString >& lineEnds,
									  int lineCount = -1 ) const;
		CString getArgListCodeText( const std::vector< CString >& linesOfCode ) const;

		// formatting
		CString doFormatLineOfCode( const TCHAR lineOfCode[] );
		CString doAdjustWhitespaceLineOfCode( const TCHAR lineOfCode[] );
		bool IsBraceCharAt( const TCHAR code[], size_t pos ) const;

		int replaceMultipleWhiteSpace( CString& targetString, int pos, const TCHAR* newWhitespace = _T(" ") );
		int resolveSpaceAfterToken( CString& targetString, const TokenRange& tokenRange, bool mustSpaceIt );
		int resolveSpaceBeforeToken( CString& targetString, const TokenRange& tokenRange, bool mustSpaceIt );
		int formatBrace( CString& targetString, int pos );
		int formatUnicodePortableStringConstant( CString& targetString, int pos );
		int formatDefault( CString& targetString, int pos );

		// line splitter
		enum HandleSingleLineComments { RemoveComment, ToMultiLineComment };

		CString makeNormalizedFormattedPrototype( const TCHAR* pMethodProto, bool forImplementation = false );
		CString transformTrailingSingleLineComment( const TCHAR* lineOfCode, HandleSingleLineComments handleComments = ToMultiLineComment );
		int doSplitArgumentList( std::vector< CString >& brokenLines, const TokenRange& openBraceRange, int maxEditorColIndex );

		TEditorColumn computeVisualEditorColumn( const TCHAR* pCodeText, int index ) const;
		int computeVisualEditorIndex( const TCHAR* pCodeText, int index ) const { return (int)computeVisualEditorColumn( pCodeText, index ) - 1; }
		CString makeLineIndentWhiteSpace( int editorColIndex ) const;
		CString makeLineIndentWhiteSpace( int editorColIndex, bool doUseTabs ) const;

		// C++ implementation
		void cppFilterPrototypeForImplementation( CString& targetString ) const;

		// comment/uncomment
		CString comment( const TCHAR* pCodeText, bool isEntireLine, CommentState commentState ) const;
		CString uncomment( const TCHAR* pCodeText, bool isEntireLine ) const;
	protected:
		LineBreakTokenMatch findLineBreakToken( TokenRange* pOutToken, const TCHAR* pCodeText, int startPos = 0 ) const;
		TokenRange getWhiteSpaceRange( const TCHAR* pCodeText, int pos = 0, bool includingComments = true ) const;
	protected:
		// format/split parameters
		const CFormatterOptions& m_options;
		MultiWhitespacePolicy m_multiWhitespacePolicy;
		CString m_validArgListOpenBraces;

		// document parameters
		DocLanguage m_docLanguage;
		int m_tabSize;
		bool m_useTabs;

		LanguageSearchEngine m_languageEngine;

		// Internal state
		int m_disableBracketSpacingCounter;
	public:
		static const std::tstring s_cancelTag;
	};

} // namespace code


#endif // Formatter_h
