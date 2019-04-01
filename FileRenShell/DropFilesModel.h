#ifndef DropFilesModel_h
#define DropFilesModel_h
#pragma once

#include "utl/Path.h"


class CEnumTags;
class CImageStore;


class CDropFilesModel : private utl::noncopyable
{
public:
	CDropFilesModel( const fs::CPath& destDirPath );
	~CDropFilesModel();

	void BuildFromClipboard( void );
	void Clear( void );

	bool HasDropPaths( void ) const { return !m_dropPaths.empty(); }
	const std::vector< fs::CPath >& GetDropPaths( void ) const { return m_dropPaths; }
	std::tstring FormatDropCounts( void ) const;

	bool HasSrcFolderPaths( void ) const { return !m_srcFolderPaths.empty(); }
	const std::vector< fs::CPath >& GetSrcFolderPaths( void ) const { return m_srcFolderPaths; }

	bool HasSrcDeepFolderPaths( void ) const { return HasSrcFolderPaths() && m_srcDeepFolderPaths.size() != m_srcFolderPaths.size(); }
	const std::vector< fs::CPath >& GetSrcDeepFolderPaths( void ) const { return m_srcDeepFolderPaths; }

	bool HasRelFolderPathSeq( void ) const { return !m_relFolderPathSeq.empty(); }
	const std::vector< fs::CPath >& GetRelFolderPathSeq( void ) const { return m_relFolderPathSeq; }
	CBitmap* GetRelFolderItemInfo( std::tstring& rItemText, size_t fldSeqPos ) const;

	bool CreateFolders( RecursionDepth depth ) { return CreateFolders( Shallow == depth ? m_srcFolderPaths : m_srcDeepFolderPaths ); }
	bool PasteDeep( const fs::CPath& relFolderPath, CWnd* pParentOwner );

	enum PasteOperation { PasteNone, PasteCopyFiles, PasteMoveFiles };

	static const CEnumTags& GetTags_PasteOperation( void );
	PasteOperation GetPasteOperation( void ) const;
private:
	void Init( const std::vector< fs::CPath >& dropPaths, DROPEFFECT dropEffect );
	void RegisterFolderImage( const fs::CPath& folderPath );
	bool CreateFolders( const std::vector< fs::CPath >& srcFolderPaths );
	fs::CPath MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& relFolderPath ) const;
private:
	fs::CPath m_destDirPath;
	std::vector< fs::CPath > m_dropPaths;				// dropped files (copied or cut)
	DROPEFFECT m_dropEffect;							// Copy=DROPEFFECT_COPY|DROPEFFECT_LINK, Paste=DROPEFFECT_MOVE

	fs::CPath m_srcCommonFolderPath;
	std::vector< fs::CPath > m_srcFolderPaths;			// drop source folders
	std::vector< fs::CPath > m_srcDeepFolderPaths;		// drop source folders and their sub-folders
	std::vector< fs::CPath > m_relFolderPathSeq;		// sequence of relative folders from the original source parent path

	std::auto_ptr< CImageStore > m_pImageStore;			// for folder images
	enum { BaseImageId = 100 };
};


#endif // DropFilesModel_h
