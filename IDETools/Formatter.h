#ifndef Formatter_h
#define Formatter_h
#pragma once

#include "DocLanguage.h"
#include "LanguageSearchEngine.h"


namespace code
{
	enum TokenSpacing;
	struct CFormatterOptions;

	typedef unsigned int EditorColumn;

	enum LineBreakTokenMatch { LBT_NoMatch, LBT_OpenBrace, LBT_CloseBrace, LBT_BreakSeparator };


	class CFormatter
	{
	public:
		CFormatter( const CFormatterOptions& _options );
		~CFormatter();

		void setSpacingOpenBraces( const CString& _spacingOpenBraces );

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

		CString formatCode( const TCHAR* codeText, bool protectLeadingWhiteSpace = true,
							bool justAdjustWhiteSpace = false );
		CString formatLineOfCode( const TCHAR* lineOfCode, bool protectLeadingWhiteSpace = true,
								  bool justAdjustWhiteSpace = false );

		CString tabifyLineOfCode( const TCHAR* lineOfCode, bool doTabify = true );

		CString splitArgumentList( const TCHAR* codeText, EditorColumn maxColumn = UINT_MAX,
								   int targetBracketLevel = -1 );

		CString toggleComment( const TCHAR* codeText );

		CString generateConsecutiveNumbers( const TCHAR* codeText, unsigned int startingNumber = UINT_MAX ) throws_( CRuntimeException );
		CString sortLines( const TCHAR* codeText, bool ascending ) throws_( CRuntimeException );
	protected:
		void resetInternalState( void );
	protected:
		int splitMultipleLines( std::vector< CString >& outLinesOfCode, std::vector< CString >& outLineEnds,
								const TCHAR* codeText );
		CString unsplitMultipleLines( const std::vector< CString >& linesOfCode, const std::vector< CString >& lineEnds,
									  int lineCount = -1 ) const;
		CString getArgListCodeText( const std::vector< CString >& linesOfCode ) const;

		// formatting
		CString doFormatLineOfCode( const TCHAR* lineOfCode );
		CString doAdjustWhitespaceLineOfCode( const TCHAR* lineOfCode );
		TokenSpacing MustSpaceBrace( TCHAR chrBrace ) const;

		int replaceMultipleWhiteSpace( CString& targetString, int pos,
										const TCHAR* newWhitespace = _T(" ") );
		int resolveSpaceAfterToken( CString& targetString, const TokenRange& tokenRange,
									bool mustSpaceIt );
		int resolveSpaceBeforeToken( CString& targetString, const TokenRange& tokenRange,
									 bool mustSpaceIt );
		int formatBrace( CString& targetString, int pos );
		int formatUnicodePortableStringConstant( CString& targetString, int pos );
		int formatDefault( CString& targetString, int pos );

		// line splitter
		enum HandleSingleLineComments { RemoveComment, ToMultiLineComment };

		CString makeNormalizedFormattedPrototype( const TCHAR* methodPrototype,
												  bool forImplementation = false );
		CString transformTrailingSingleLineComment( const TCHAR* lineOfCode,
													HandleSingleLineComments handleComments = ToMultiLineComment );
		int doSplitArgumentList( std::vector< CString >& brokenLines, const TokenRange& openBraceRange,
								 int maxEditorColIndex );

		EditorColumn computeVisualEditorColumn( const TCHAR* codeText, int index ) const;
		int computeVisualEditorIndex( const TCHAR* codeText, int index ) const { return (int)computeVisualEditorColumn( codeText, index ) - 1; }
		CString makeLineIndentWhiteSpace( int editorColIndex ) const;
		CString makeLineIndentWhiteSpace( int editorColIndex, bool doUseTabs ) const;

		// C++ implementation
		void cppFilterPrototypeForImplementation( CString& targetString ) const;

		// comment/uncomment
		CString comment( const TCHAR* codeText, bool isEntireLine, CommentState commentState ) const;
		CString uncomment( const TCHAR* codeText, bool isEntireLine ) const;
	protected:
		LineBreakTokenMatch findLineBreakToken( TokenRange& outToken, const TCHAR* string, int startPos = 0 ) const;
		TokenRange getWhiteSpaceRange( const TCHAR* codeText, int pos = 0, bool includingComments = true ) const;
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
		static const CString m_cancelTag;
	};

} // namespace code


#endif // Formatter_h