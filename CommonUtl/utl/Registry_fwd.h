#ifndef Registry_fwd_h
#define Registry_fwd_h
#pragma once

#include "Path.h"


namespace reg
{
	typedef fs::CPath TKeyPath;

	void SplitEntryFullPath( TKeyPath* pOutSection, std::tstring* pOutEntry, const TCHAR* pEntryFullPath );		// e.g. "Settings\\Options|Size" -> section="Settings\\Options", entry="Size"
}


#endif // Registry_fwd_h
