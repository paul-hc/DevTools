#ifndef IncludeTag_h
#define IncludeTag_h
#pragma once

#include "utl/Path.h"
#include "FileType.h"


class CIncludeTag
{
public:
	CIncludeTag( void ) { Clear(); }
	CIncludeTag( const TCHAR* pIncludeTag ) { Setup( pIncludeTag ); }
	CIncludeTag( const std::tstring& filePath, bool localInclude ) { Setup( filePath, localInclude ); }

	void Clear( void );
	void Setup( const TCHAR* pIncludeTag );
	void Setup( const std::tstring& filePath, bool localInclude );

	static std::tstring GetStrippedTag( const TCHAR* pIncludeTag );

	bool IsEmpty( void ) const { return m_filePath.IsEmpty(); }
	bool IsEven( void ) const { return m_evenTag; }
	bool IsLocalInclude( void ) const { return m_localInclude; }
	bool IsEnclosed( void ) const;

	const std::tstring& GetTag( void ) const { return m_includeTag; }
	const fs::CPath& GetFilePath( void ) const { return m_filePath; }
	const ft::FileType& GetFileType( void ) const { return m_fileType; }
	const std::tstring& GetSafeFileName( void ) const { return IsEnclosed() ? m_filePath.Get() : m_includeTag; }

	std::tstring GetIncludeDirectiveFormat( void ) const;
	void SetIncludeDirectiveFormat( const TCHAR* pDirective, bool isCppPreprocessor );
	std::tstring FormatIncludeStatement( void ) const;

	bool operator==( const CIncludeTag& right ) const { return m_localInclude == right.m_localInclude && m_filePath == right.m_filePath; }
	bool operator!=( const CIncludeTag& right ) const { return !operator==( right ); }
protected:
	std::tstring m_includeTag;				// "utl/Icon.h" or <atl/string.h>
	std::tstring m_directiveFormat;
	fs::CPath m_filePath;
	ft::FileType m_fileType;
	bool m_localInclude;
	bool m_evenTag;
};


#endif // IncludeTag_h
