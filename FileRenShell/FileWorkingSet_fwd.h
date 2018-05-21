#ifndef FileWorkingSet_fwd_h
#define FileWorkingSet_fwd_h
#pragma once

#include "utl/Path.h"
#include "utl/FileState.h"


typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;		// ptr stored in m_fileListCtrl


namespace fmt
{
	enum PathFormat;
}


namespace app
{
	enum CustomColors
	{
		ColorDeletedText = color::Red,
		ColorModifiedText = color::Blue,
		ColorErrorBk = color::PastelPink
	};
}


#endif // FileWorkingSet_fwd_h
