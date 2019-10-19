#ifndef PathFormatter_h
#define PathFormatter_h
#pragma once

#include "Path.h"


class CPathFormatter
{
	friend class CPathGeneratorTests;
public:
	CPathFormatter( const std::tstring& format, bool ignoreExtension );

	bool IsValidFormat( void ) const { return m_isNumericFormat || m_isWildcardFormat; }
	bool IsNumericFormat( void ) const { return m_isNumericFormat; }
	bool IsWildcardFormat( void ) const { return m_isWildcardFormat; }
	const fs::CPath& GetFormat( void ) const { return m_format; }

	bool IsConsistent( void ) const;
	CPathFormatter MakeConsistent( void ) const;

	void SetMoveDestDirPath( const fs::CPath& moveDestDirPath );

	fs::CPath FormatPath( const fs::CPath& srcPath, UINT seqCount, UINT dupCount = 0 ) const;
	bool ParseSeqCount( UINT& rSeqCount, const fs::CPath& srcPath ) const;
private:
	static std::tstring FormatPart( const std::tstring& part, const std::tstring& format, UINT seqCount, bool* pSyntaxOk = NULL );
	static bool ParsePart( UINT& rSeqCount, const std::tstring& format, const std::tstring& src );

	static bool IsNumericFormat( const std::tstring& format );
	static bool SkipStar( std::tistringstream& issSrc, std::tstring::const_iterator& itFmt, std::tstring::const_iterator itFmtEnd );
private:
	fs::CPath m_format;
	bool m_ignoreExtension;
	bool m_isNumericFormat;
	bool m_isWildcardFormat;

	std::tstring m_fnameFormat, m_extFormat;
	fs::CPath m_moveDestDirPath;
};


#endif // PathFormatter_h
