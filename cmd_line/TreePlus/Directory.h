#ifndef Directory_h
#define Directory_h
#pragma once

#include "GuidesOutput.h"
#include <iosfwd>


struct CCmdLineOptions;
struct CGuideParts;


class CDirectory : private utl::noncopyable
{
	CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath );
public:
	CDirectory( const CCmdLineOptions& options );		// root node constructor

	void List( std::wostream& os, const CGuideParts& guideParts, const std::wstring& parentNodePrefix );
private:
	const fs::CPath& m_dirPath;
	size_t m_level;
	const CCmdLineOptions& m_options;

	static const TCHAR s_wildSpec[];
};


#endif // Directory_h
