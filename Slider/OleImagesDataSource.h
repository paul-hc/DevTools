#ifndef OleImagesDataSource_h
#define OleImagesDataSource_h
#pragma once

#include "utl/FlexPath.h"
#include "utl/StringCvt.h"


class CWicDibSection;


// takes input file paths and creates a file path set so that archived (embedded) images are cloned as temporary physical files;
// it deletes created temp files on destruction.

class CTempCloneFileSet
{
public:
	CTempCloneFileSet( void ) {}
	
	template< typename SrcContainerT >
	CTempCloneFileSet( const SrcContainerT& inputFilePaths ) { SetInputFilePaths( inputFilePaths ); }

	~CTempCloneFileSet();

	bool SetInputFilePaths( const std::vector<fs::CFlexPath>& inputFilePaths );

	template< typename SrcContainerT >
	bool SetInputFilePaths( const SrcContainerT& inputFilePaths );

	void DeleteClones( void );

	const std::vector<fs::CPath>& GetPhysicalFilePaths( void ) const { return m_physicalFilePaths; }

	static const fs::TDirPath& GetTempDirPath( void );
	static bool ClearAllTempFiles( void );
private:
	std::vector<fs::CPath> m_physicalFilePaths;									// physical file set to be used as source in shell operations (drag&drop, move, copy, etc)
	std::vector< std::pair<fs::CFlexPath, fs::CPath> > m_tempClonedImagePaths;	// map embedded path to physical clone path
};


#include "utl/UI/OleDataSource.h"


namespace ole
{
	// drag-drop that expands embedded files to temporary physical clones
	//
	class CImagesDataSource : public ole::CDataSource
	{
	public:
		CImagesDataSource( void ) {}
		virtual ~CImagesDataSource();

		// caching overrides
		virtual void CacheShellFilePaths( const std::vector<fs::CPath>& filePaths );

		// pass m_nullRect to disable the start drag delay
		DROPEFFECT DragAndDropImages( HWND hSrcWnd, DROPEFFECT dropEffect, const RECT* pStartDragRect = nullptr );
		DROPEFFECT DragAndDropImages( CWicDibSection* pBitmap, DROPEFFECT dropEffect, const RECT* pStartDragRect = nullptr );
	private:
		CTempCloneFileSet m_tempClones;
	};
}


#endif // OleImagesDataSource_h
