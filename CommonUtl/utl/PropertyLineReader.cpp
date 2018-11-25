
#include "StdAfx.h"
#include "PropertyLineReader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*
	Read in a "logical line" from an input stream, skip all comment
	and blank lines and filter out those leading whitespace characters
	from the beginning of a "natural line". Returns the char length of the "logical line"
	and stores the line in a line buffer.
*/

CPropertyLineReader::CPropertyLineReader( std::istream& rPropertyStream, int lineBufferSize/* = 1024*/, int streamBufferSize/* = 8192*/ )
	: m_lineBuffer( new char[ lineBufferSize ] )
	, m_rPropertyStream( rPropertyStream )
	, m_commentIndicators( "#!" )
	, m_allowInlineComment( false )
	, m_lineBufferSize( lineBufferSize )
	, m_streamBufferSize( streamBufferSize )
	, m_charsInBuffer( 0 )
	, m_bufferIndex( 0 )
	, m_streamBuffer( new char[ streamBufferSize ] )
{
}

void CPropertyLineReader::SetCommentStyle( const std::string& rCommentIndicators, bool allowInlineComment/* = false*/ )
{
	m_commentIndicators = rCommentIndicators;
	m_allowInlineComment = allowInlineComment;
}

int CPropertyLineReader::ReadLine( void )
{
	int lineSize = 0;

	bool skipWhiteSpace = true;
	bool isCommentLine = false;
	bool hasInlineComments = false;
	bool isNewLine = true;
	bool appendedLineBegin = false;
	bool precedingBackslash = false;
	bool skipLF = false;

	while ( true )
	{
		if ( m_bufferIndex >= m_charsInBuffer )
		{
			m_rPropertyStream.read( m_streamBuffer.get(), m_streamBufferSize );

			m_charsInBuffer = m_rPropertyStream.gcount();
			m_bufferIndex = 0;

			if ( 0 == m_charsInBuffer )
			{
				if ( lineSize == 0 || isCommentLine )
					return -1;
				else
					return lineSize;
			}
		}

		char c = m_streamBuffer.get()[m_bufferIndex++];

		if ( skipLF )
		{
			skipLF = false;
			if ( c == '\n' )
				continue;
		}

		if ( skipWhiteSpace )
		{
			if ( c == ' ' || c == '\t' || c == '\f' )
				continue;

			if ( !appendedLineBegin && ( c == '\r' || c == '\n' ) )
				continue;

			skipWhiteSpace = false;
			appendedLineBegin = false;
		}

		if ( isNewLine )
		{
			isNewLine = false;

			if ( m_commentIndicators.find_first_of( c ) != std::string::npos )
			{
				isCommentLine = true;
				continue;
			}
		}

		if ( m_allowInlineComment && !isCommentLine && !hasInlineComments )
		{
			if ( !precedingBackslash && m_commentIndicators.find_first_of( c ) != std::string::npos )
			{
				hasInlineComments = true;
				continue;
			}
		}

		if ( c != '\n' && c != '\r' )
		{
			if ( !hasInlineComments )	// skip inline comments
			{
				m_lineBuffer.get()[lineSize++] = c;

				// double line buffer size on long lines
				if ( lineSize == m_lineBufferSize )
				{
					char* extendedLineBuffer = new char[m_lineBufferSize * 2];
					memcpy( extendedLineBuffer, m_lineBuffer.get(), m_lineBufferSize );

					m_lineBufferSize *= 2;

					m_lineBuffer.reset( extendedLineBuffer );
				}

				//flip the preceding backslash flag
				if ( c == '\\' )
					precedingBackslash = !precedingBackslash;
				else
					precedingBackslash = false;
			}
		}
		else // reached EOL
		{
			if ( isCommentLine || lineSize == 0 ) // skip comment line and blank line
			{
				isCommentLine = false;
				isNewLine = true;
				skipWhiteSpace = true;
				lineSize = 0;
				continue;
			}

			// prepare to read the next line if there is a preceding backslash
			if ( m_bufferIndex < m_charsInBuffer || !m_rPropertyStream.eof() )
			{
				if ( precedingBackslash )
				{
					lineSize -= 1;

					//skip the leading whitespace characters in following line
					skipWhiteSpace = true;
					appendedLineBegin = true;
					precedingBackslash = false;

					if ( c == '\r' )
						skipLF = true;
				}
				else
					return lineSize;
			}
		}
	}
}
