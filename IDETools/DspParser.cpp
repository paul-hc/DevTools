
#include "stdafx.h"
#include "DspParser.h"
#include "StringUtilitiesEx.h"
#include "PathInfo.h"


const CString nullString;

const TCHAR DspParser::chPreprocessor = _T('!');
const TCHAR DspParser::chProjectEntry = _T('#');
const TCHAR DspParser::chSingleLineComment = _T(';');
const TCHAR DspParser::whitespaces[] = _T(" \t\r\n");
const TCHAR DspParser::stringDelims[] = _T("\"\"");

DspParser::DspParser( const CString& _dspFilePath )
	: m_dspFilePath( _dspFilePath )
	, m_file( m_dspFilePath, CFile::modeRead | CFile::typeText | CFile::shareDenyNone )
	, m_line()
	, m_lineNo( 0 )
	, m_tokenNo( 0 )
	, m_tokenStart( 0 )
	, m_tokenEnd( 0 )
	, m_registeredTokens()
	, m_minRegTokenLen( INT_MAX )
{
}

DspParser::~DspParser()
{
}

const TCHAR* DspParser::getTokenString( TokenSemantic tokenSemantic )
{
	static const TCHAR* knownTokens[] =
	{
		NULL,					// tok_Invalid
		NULL,					// tok_Generic
		_T("IF"),				// tok_ppIF
		_T("ELSEIF"),			// tok_ppELSEIF
		_T("ENDIF"),			// tok_ppENDIF
		_T("MESSAGE"),			// tok_ppMESSAGE
		_T("TARGTYPE"),			// tok_TARGTYPE
		_T("PROP"),				// tok_PROP
		_T("IntDir"),			// tok_IntDir
		_T("ProjDir"),			// tok_ProjDir
		_T("InputPath"),		// tok_InputPath
		_T("TargetName"),		// tok_TargetName
		_T("ADD"),				// tok_ADD
		_T("SUBTRACT"),			// tok_SUBTRACT
		_T("SOURCE=")			// tok_SOURCE
	};

	return tokenSemantic >= tok_Invalid && tokenSemantic < tok_MaxCount ? knownTokens[ tokenSemantic ] : NULL;
}

bool DspParser::registerToken( const CString& token, TokenSemantic tokenSemantic /*= tok_Generic*/ )
{
	MapTokenToSemantic::const_iterator itToken = m_registeredTokens.find( token );

	ASSERT( !token.IsEmpty() );
	if ( itToken != m_registeredTokens.end() )
		return false;			// Token already registered
	m_registeredTokens[ token ] = tokenSemantic;

	int				tokenLen = token.GetLength();

	if ( tokenLen > 0 && tokenLen < m_minRegTokenLen )
		m_minRegTokenLen = tokenLen;			// Set the minimum token length (for minimizing token hit tests)
	return true;
}

void DspParser::getNextLine( void ) throws_( Exception )
{
	// Reset token specific data members
	m_tokenStart = m_tokenEnd = m_tokenNo = 0;
	m_currTokenSemantic = tok_Invalid;
	try
	{	// Read a new line string from the file
		if ( m_file.ReadString( m_line ) )
		{
			++m_lineNo;
			return;
		}
	}
	catch ( CException* exc )
	{
		exc->Delete();
	}
	throw EndOfFile;
}

CString DspParser::getNextToken( void ) throws_( Exception )
{
	const TCHAR*	linePtr = m_line;

	m_tokenStart = m_tokenEnd;
	++m_tokenNo;
	m_currTokenSemantic = tok_Invalid;

	// Skip leading whitespaces
	while ( str::isCharOneOf( linePtr[ m_tokenStart ], whitespaces ) )
		++m_tokenStart;

	switch ( linePtr[ m_tokenStart ] )
	{
		case _T('\0'):
		case chSingleLineComment:
			throw EndOfLine;
		case chPreprocessor:
		case chProjectEntry:
			if ( m_tokenNo == 1 )
			{	// Those are tokens themselves
				m_tokenEnd = m_tokenStart + 1;
				break;
			}
			// Else continue default processing
		default:
			if ( linePtr[ m_tokenStart ] == stringDelims[ 0 ] )
			{	// Skip to end of string
				m_tokenEnd = m_line.Find( stringDelims[ 1 ], m_tokenStart + 1 );
				if ( m_tokenEnd != -1 )
					++m_tokenEnd;
			}
			else
			{	// Skip to first whitespace or end of line
				m_tokenEnd = m_tokenStart + 1;
				while ( !isNormalTokenBreak() )
					++m_tokenEnd;
			}
			break;
	}

	if ( m_tokenEnd == m_tokenStart )
		throw EndOfLine;
	else if ( m_tokenEnd == -1 )
		throw SyntaxError;

	return m_line.Mid( m_tokenStart, m_tokenEnd - m_tokenStart );
}

bool DspParser::isNormalTokenBreak( void )
{
	ASSERT( m_tokenStart <= m_tokenEnd );

	const TCHAR		*linePtr = m_line;
	TCHAR			endChr = linePtr[ m_tokenEnd ];
	int				tokenLen = m_tokenEnd - m_tokenStart;
	bool			_isTokenBreak = endChr == _T('\0') ||						// End of line
									str::isCharOneOf( endChr, whitespaces ) ||	// Whitespace
									endChr == stringDelims[ 0 ];				// String begin

	if ( tokenLen >= m_minRegTokenLen )
		// Also check if this is a registered token
		if ( ( m_currTokenSemantic = findToken( m_line.Mid( m_tokenStart, tokenLen ) ) ) != tok_Invalid )
			_isTokenBreak = true;
	return _isTokenBreak;
}

// NOTE: tok_SOURCE must be registered in order to make this work!
CString DspParser::getNextSourceFile( void ) throws_( Exception )
{
	while ( true )
	{
		getNextLine();
		try
		{
			getNextToken();
			if ( m_currTokenSemantic == tok_SOURCE )
			{
				CString				token = getSafeStringCore( getNextToken() );

				if ( !token.IsEmpty() )
					return token;
			}
		}
		catch ( Exception exc )
		{
			if ( exc == SyntaxError )
			{
				TRACE( _T("* DspParser::getNextSourceFile(): SyntaxError thrown!\n") );
				return nullString;
			}
		}
	}
}

bool DspParser::isConditionalConfigToken( const TCHAR* token )
{
	return str::isTokenMatch( token, _T("IF") ) ||
		   str::isTokenMatch( token, _T("ELSEIF") ) ||
		   str::isTokenMatch( token, _T("ENDIF") );
}

bool DspParser::isTokenSequence( const TCHAR* const* tokenSeq, int seqCount /*= INT_MAX*/ ) throws_( Exception )
{
	int i = 0;

	for ( ; i < seqCount && tokenSeq[ i ] != NULL; ++i )
		if ( !str::isTokenMatch( getNextToken(), tokenSeq[ i ] ) )
			return false;
	return i == seqCount || tokenSeq[ i ] == NULL;
}

bool DspParser::findConfiguration( const TCHAR* configurationName )
{
	try
	{
		while ( true )
		{
			getNextLine();
			try
			{
				if ( isTokenCharMatch( getNextToken(), chPreprocessor ) )
				{
					CString			token = getNextToken();

					if ( str::isTokenMatch( token, _T("IF") ) || str::isTokenMatch( token, _T("ELSEIF") ) )
					{
						if ( str::isTokenMatch( getNextToken(), _T("\"$(CFG)\"") ) )
							if ( str::isTokenMatch( getNextToken(), _T("==") ) )
							{
								token = getNextToken();
								if ( isStringToken( token ) )
								{
									token = getSafeStringCore( token );
									if ( token == configurationName )
										return true;
								}
							}
					}
					else if ( str::isTokenMatch( token, _T("ENDIF") ) )
						return false;
				}
			}
			catch ( Exception exc )
			{
				if ( exc == SyntaxError )
				{
					TRACE( _T("* DspParser::findConfiguration(): SyntaxError thrown!\n") );
					return false;
				}
			}
		}
	}
	catch ( Exception exc )
	{
		exc;
		return false;
	}
	return false;
}

CString DspParser::GetAdditionalIncludePath( const TCHAR* activeConfiguration /*= NULL*/, bool useAnyConfig /*= true*/ )
{
	CString			additionalIncludePath;
	bool			foundFirstConfig = false, foundEntry = false;

	try
	{	// Search for the active configuration if it's specified
		if ( activeConfiguration != NULL && *activeConfiguration != _T('\0') )
			if ( findConfiguration( activeConfiguration ) )
				foundFirstConfig = true;
			else if ( useAnyConfig )
			{
				TRACE( _T("Specified configuration [%s] couldn't be found! Parsing the first configuration...\n"),
					   activeConfiguration );
				m_file.SeekToBegin();
			}
			else
				return nullString;

		while ( !foundEntry )
		{
			getNextLine();
			try
			{
				CString			token = getNextToken();

				if ( isTokenCharMatch( token, chPreprocessor ) )
				{
					if ( isConditionalConfigToken( getNextToken() ) )
						if ( !foundFirstConfig )
							foundFirstConfig = true;
						else
							return nullString;
				}
				else if ( foundFirstConfig )
					if ( isTokenCharMatch( token, chProjectEntry ) )
						if ( str::isTokenMatch( getNextToken(), _T("ADD") ) && str::isTokenMatch( getNextToken(), _T("CPP") ) )
						{	// This is the project configuration entry where additional includes might be found
							foundEntry = true;
							while ( true )
								if ( str::isTokenMatch( token = getNextToken(), _T("/I") ) )
								{
									if ( !additionalIncludePath.IsEmpty() )
										additionalIncludePath += _T(";");
									additionalIncludePath += getSafeStringCore( getNextToken() );
								}
						}
			}
			catch ( Exception exc )
			{
				if ( exc == SyntaxError )
				{
					TRACE( _T("* DspParser::GetAdditionalIncludePath(): SyntaxError thrown!\n") );
					return nullString;
				}
			}
		}
	}
	catch ( Exception exc )
	{
		exc;
	}
	return additionalIncludePath;
}

bool DspParser::findFile( const TCHAR* fileFullPath )
{
	PathInfoEx		cmpFilePath( fileFullPath );

	registerToken( tok_SOURCE );		// Ensure that tok_SOURCE is already registered
	if ( cmpFilePath.exist() && seekToSourceFiles() )
	{
		try
		{
			while ( true )
			{
				PathInfoEx		sourceFile( m_dspFilePath );

				sourceFile.assignNameExt( getNextSourceFile() );
				if ( sourceFile == cmpFilePath )
					return true;
			}
		}
		catch ( Exception exc )
		{
			if ( exc == SyntaxError )
			{
				TRACE( _T("* DspParser::findFile(): SyntaxError thrown!\n") );
				return false;
			}
		}
	}
	return false;
}

size_t DspParser::querySourceFiles( std::vector< PathInfoEx >& rOutSourceFiles )
{
	registerToken( tok_SOURCE );		// Ensure that tok_SOURCE is already registered

	if ( seekToSourceFiles() )
	{
		try
		{
			CString projectFolderPath = PathInfo( m_dspFilePath ).getDirPath();

			while ( true )
			{

				PathInfoEx		sourceFile( projectFolderPath + getNextSourceFile() );

				sourceFile.makeAbsolute();
				rOutSourceFiles.push_back( sourceFile );
			}
		}
		catch ( Exception exc )
		{
			if ( exc == SyntaxError )
				TRACE( _T("* DspParser::querySourceFiles(): SyntaxError thrown!\n") );
		}
	}

	return rOutSourceFiles.size();
}

size_t DspParser::querySourceFiles( std::vector< CString >& rOutSourceFiles )
{
	std::vector< PathInfoEx > sourceFilePaths;

	querySourceFiles( sourceFilePaths );

	rOutSourceFiles.resize( sourceFilePaths.size() );
	for ( unsigned int i = 0; i != sourceFilePaths.size(); ++i )
		rOutSourceFiles[ i ] = sourceFilePaths[ i ].fullPath;

	return rOutSourceFiles.size();
}

bool DspParser::seekAfterConfigurations( void )
{
	bool			foundAnyConfig = false, done = false;

	try
	{
		while ( !done )
		{
			getNextLine();
			try
			{
				if ( isTokenCharMatch( getNextToken(), chPreprocessor ) )
					if ( !foundAnyConfig )
						foundAnyConfig = str::isTokenMatch( getNextToken(), _T("IF") );
					else
						done = str::isTokenMatch( getNextToken(), _T("ENDIF") );
			}
			catch ( Exception exc )
			{
				if ( exc == SyntaxError )
				{
					TRACE( _T("* DspParser::seekAfterConfigurations(): SyntaxError thrown!\n") );
					return false;
				}
			}
		}
	}
	catch ( Exception exc )
	{
		exc;
		return false;
	}
	return done;
}

// Searches from the current file position for the line marking the beginning of source files ("# Begin Target")
bool DspParser::seekToSourceFiles( void )
{
	bool			done = false;

	try
	{
		while ( !done )
		{
			getNextLine();
			try
			{
				done = isTokenCharMatch( getNextToken(), chProjectEntry ) &&
					   str::isTokenMatch( getNextToken(), _T("Begin") ) &&
					   str::isTokenMatch( getNextToken(), _T("Target") );
			}
			catch ( Exception exc )
			{
				if ( exc == SyntaxError )
				{
					TRACE( _T("* DspParser::seekToSourceFiles(): SyntaxError thrown!\n") );
					return false;
				}
			}
		}
	}
	catch ( Exception exc )
	{
		exc;
		return false;
	}
	return done;
}

bool DspParser::isSourceFile( const PathInfoEx& filePath )
{
	static const TCHAR* sourceFileTypes[] =
	{
		_T(".h"),
		_T(".hxx"),
		_T(".hpp"),
		_T(".inl"),
		_T(".t"),
		_T(".c"),
		_T(".cpp"),
		_T(".cxx"),
		_T(".idl"),
		_T(".odl"),
		_T(".rc"),
		_T(".rc2"),
		_T(".def"),
		_T(".bmp"),
		_T(".dib"),
		_T(".jpg"),
		_T(".gif"),
		_T(".ico"),
		_T(".cur")
	};

	for ( int i = 0; i != COUNT_OF( sourceFileTypes ); ++i )
		if ( path::EquivalentPtr( filePath.ext, sourceFileTypes[ i ] ) )
			return true;

	return false;
}
