#ifndef ArchivingModel_fwd_h
#define ArchivingModel_fwd_h
#pragma once

#include "utl/FlexPath.h"


typedef std::pair< fs::CFlexPath, fs::CFlexPath > TTransferPathPair;		// SRC file path -> DEST file path


class CFileAttr;
class CEnumTags;


// (*) don't change constant values
enum FileOp { FOP_FileCopy, FOP_FileMove, FOP_Resequence };
enum DestType { ToDirectory, ToArchiveStg };

const CEnumTags& GetTags_FileOp( void );


namespace func
{
	struct RefPairDest
	{
		fs::CFlexPath& operator()( TTransferPathPair& rTransferPair )
		{
			return rTransferPair.second;
		}
	};
}


#endif // ArchivingModel_fwd_h
