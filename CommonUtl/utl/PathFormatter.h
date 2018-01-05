#ifndef PathFormatter_h
#define PathFormatter_h
#pragma once

#include "Path.h"


class CPathFormatter
{
public:
	CPathFormatter( const std::tstring& format );

	bool IsValidFormat( void ) const { return m_isNumericFormat || m_isWildcardFormat; }

	void SetMoveDestDirPath( const std::tstring& moveDestDirPath );

	fs::CPath FormatPath( const std::tstring& srcPath, UINT seqCount, UINT dupCount = 0 ) const;
	bool ParseSeqCount( UINT& rSeqCount, const std::tstring& srcPath ) const;

	static std::tstring FormatPart( const std::tstring& format, const std::tstring& src, UINT seqCount, bool* pSyntaxOk = NULL );
	static bool ParsePart( UINT& rSeqCount, const std::tstring& format, const std::tstring& src );
private:
	static bool IsNumericFormat( const std::tstring& format );
	static bool SkipStar( std::tistringstream& issSrc, std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd );
public:
	const bool m_isNumericFormat;
	const bool m_isWildcardFormat;
private:
	fs::CPathParts m_formatParts;							// F-FNAME, F-EXT
	std::auto_ptr< fs::CPathParts > m_pMoveDestDirPath;		// generate for moving to destDirPath
};


#endif // PathFormatter_h
