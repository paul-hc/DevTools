#ifndef Directory_h
#define Directory_h
#pragma once

#include "CmdLineOptions_fwd.h"
#include "OutputProfile.h"
#include "utl/FileEnumerator.h"
#include <iosfwd>


struct CCmdLineOptions;
struct CGuideParts;


class CDirectory : private utl::noncopyable
{
	CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath );
public:
	CDirectory( const fs::CPath& dirPath );		// root node constructor

	void List( std::wostream& os, const CCmdLineOptions& options, const CGuideParts& guideParts, const std::wstring& parentNodePrefix );
private:
	const fs::CPath& m_dirPath;
	size_t m_level;

	static const TCHAR s_wildSpec[];
};


#endif // Directory_h
