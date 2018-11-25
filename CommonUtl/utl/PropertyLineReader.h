#ifndef PropertyLineReader_h
#define PropertyLineReader_h
#pragma once

class CPropertyLineReader
{
public:
	CPropertyLineReader( std::istream& rPropertyStream, int lineBufferSize = 1024, int streamBufferSize = 8192 );
	void SetCommentStyle( const std::string& rCommentIndicators, bool allowInlineComment/* = false*/ );

	int ReadLine( void );

public:
	std::auto_ptr<char> m_lineBuffer;

private:
	std::istream& m_rPropertyStream;
	std::string m_commentIndicators;
	bool m_allowInlineComment;

	int m_lineBufferSize;
	const int m_streamBufferSize;

	std::streamsize m_charsInBuffer;
	int m_bufferIndex;
	std::auto_ptr<char> m_streamBuffer;
};


#endif	// PropertyLineReader_h
