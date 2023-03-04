#ifndef CodeUtils_h
#define CodeUtils_h
#pragma once


namespace code
{
	// constants
	extern const TCHAR* g_pLineEnd;
	extern const TCHAR* g_pLineEndUnix;


	std::tstring ExtractFilenameIdentifier( const fs::CPath& filePath );
}


#endif // CodeUtils_h
