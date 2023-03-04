
#include "pch.h"
#include "CodeUtils.h"
#include "utl/Path.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	const TCHAR* g_pLineEnd = _T("\r\n");
	const TCHAR* g_pLineEndUnix = _T("\n");


	std::tstring ExtractFilenameIdentifier( const fs::CPath& filePath )
	{
		std::tstring identifier = filePath.GetFname();

		str::ReplaceDelims( identifier, _T("-+.;()[] \t"), _T("_") );
		str::TrimLeft( identifier, _T("_") );
		return identifier;
	}
}
