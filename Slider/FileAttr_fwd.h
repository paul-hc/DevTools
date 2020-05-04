#ifndef FileAttr_fwd_h
#define FileAttr_fwd_h
#pragma once


class CFileAttr;
class CEnumTags;


namespace fattr
{
	enum Order
	{
		OriginalOrder,			// don't order files at all
		CustomOrder,			// custom order -> user-defined order through UI
		Shuffle,				// use different random seed on each generation process
		ShuffleSameSeed,		// use previous generated random seed for suffling
		ByFileNameAsc,
		ByFileNameDesc,
		ByFullPathAsc,
		ByFullPathDesc,
		BySizeAsc,
		BySizeDesc,
		ByDateAsc,
		ByDateDesc,
		ByDimensionAsc,
		ByDimensionDesc,

		// filter-ordering
		FilterFileSameSize,			// special filter for file duplicates based on file size
		FilterFileSameSizeAndDim,	// special filter for file duplicates based on file size and dimensions
		FilterCorruptedFiles		// special filter for detecting files with errors (same sort effect as OriginalOrder)
	};

	const CEnumTags& GetTags_Order( void );
}


#endif // FileAttr_fwd_h
