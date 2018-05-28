#ifndef FileWorkingSet_fwd_h
#define FileWorkingSet_fwd_h
#pragma once

#include "utl/FileState.h"


namespace fmt { enum PathFormat; }


typedef std::pair< const fs::CPath, fs::CPath > TPathPair;
typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;


#endif // FileWorkingSet_fwd_h
