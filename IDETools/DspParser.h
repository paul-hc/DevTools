#ifndef DspParser_h
#define DspParser_h
#pragma once

#include <map>
#include "CodeUtilities.h"
#include "PathInfo.h"


class DspParser
{
public:
	enum Exception
	{
		EndOfLine,
		EndOfFile,
		SyntaxError
	};

	enum TokenSemantic
	{
		tok_Invalid,
		tok_Generic,

		// preprocessor tokens
		tok_ppIF,
		tok_ppELSEIF,
		tok_ppENDIF,
		tok_ppMESSAGE,

		// project entry tokens
		tok_TARGTYPE,
		tok_PROP,
		tok_IntDir,
		tok_ProjDir,
		tok_InputPath,
		tok_TargetName,
		tok_ADD,
		tok_SUBTRACT,
		tok_SOURCE,

		tok_MaxCount,
	};

	typedef std::map< CString, TokenSemantic >	MapTokenToSemantic;
public:
	DspParser( const CString& _dspFilePath );
	virtual ~DspParser();

	// token attributes and registration
	TokenSemantic findToken( const CString& token ) const;
	bool isTokenRegistered( const CString& token ) const;

	bool registerToken( TokenSemantic tokenSemantic );
	bool registerToken( const CString& token, TokenSemantic tokenSemantic = tok_Generic );

	static const TCHAR* getTokenString( TokenSemantic tokenSemantic );

	// parsing
	void getNextLine( void ) throws_( Exception );
	CString getNextToken( void ) throws_( Exception );
	TokenSemantic getCurrTokenSemantic( void );

	// DSP Operations
	bool findConfiguration( const TCHAR* configurationName );
	bool findFile( const TCHAR* fileFullPath );
	CString GetAdditionalIncludePath( const TCHAR* activeConfiguration = NULL, bool useAnyConfig = true );
	size_t querySourceFiles( std::vector< PathInfoEx >& rOutSourceFiles );
	size_t querySourceFiles( std::vector< CString >& rOutSourceFiles );

	bool seekAfterConfigurations( void );
	bool seekToSourceFiles( void );

	CString getNextSourceFile( void ) throws_( Exception );

	// implementation
	bool isTokenSequence( const TCHAR* const* tokenSeq, int seqCount = INT_MAX ) throws_( Exception );

	// helpers
	static bool isSourceFile( const PathInfoEx& filePath );
protected:
	bool isNormalTokenBreak( void );
	static bool isTokenCharMatch( const TCHAR* pStr, TCHAR tokenChar, str::CaseType caseType = str::Case );
	static bool isStringToken( const TCHAR* token );
	static bool isConditionalConfigToken( const TCHAR* token );
	static CString getSafeStringCore( const CString& stringToken );
public:
	CString m_dspFilePath;
protected:
	CStdioFile m_file;
	CString m_line;
	int m_lineNo, m_tokenNo;		// 1 based indexes of line and token in line
	int m_tokenStart, m_tokenEnd;	// Current token start and end
	TokenSemantic m_currTokenSemantic;
	MapTokenToSemantic m_registeredTokens;		// Tokens recognized in current parsing (fill it only if necessary since will decrease performance)
	int m_minRegTokenLen;
public:
	// DSP syntax
	static const TCHAR chPreprocessor;
	static const TCHAR chProjectEntry;
	static const TCHAR chSingleLineComment;
	static const TCHAR whitespaces[];
	static const TCHAR stringDelims[];
};


// inline code

inline bool DspParser::isTokenCharMatch( const TCHAR* pStr, TCHAR tokenChar, str::CaseType caseType /*= str::Case*/ )
{
	return
		pStr != NULL &&
		( caseType == str::Case ? ( pStr[ 0 ] == tokenChar )
								: ( _totlower( pStr[ 0 ] ) == _totlower( tokenChar ) ) ) &&
		_T('\0') == pStr[ 1 ];
}

inline bool DspParser::isStringToken( const TCHAR* token )
{
	int len = str::Length( token );

	return len >= 2 && token[ 0 ] == stringDelims[ 0 ] && token[ len - 1 ] == stringDelims[ 1 ];
}

inline CString DspParser::getSafeStringCore( const CString& stringToken )
{
	if ( !isStringToken( stringToken ) )
		return stringToken;
	return stringToken.Mid( 1, stringToken.GetLength() - 2 );
}

inline bool DspParser::registerToken( TokenSemantic tokenSemantic )
{
	CString token = getTokenString( tokenSemantic );

	return !token.IsEmpty() && registerToken( token, tokenSemantic );
}

inline DspParser::TokenSemantic DspParser::findToken( const CString& token ) const
{
	MapTokenToSemantic::const_iterator itToken = m_registeredTokens.find( token );

	if ( itToken == m_registeredTokens.end() )
		return tok_Invalid;
	return ( *itToken ).second;
}

inline bool DspParser::isTokenRegistered( const CString& token ) const
{
	return findToken( token ) != tok_Invalid;
}

// usable only after a getNextToken() call !!!
inline DspParser::TokenSemantic DspParser::getCurrTokenSemantic( void )
{
	ASSERT( m_tokenStart < m_tokenEnd );
	return m_currTokenSemantic;
}


#endif // DspParser_h
