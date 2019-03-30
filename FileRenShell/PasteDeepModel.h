#ifndef PasteDeepModel_h
#define PasteDeepModel_h
#pragma once

#include "utl/Path.h"


class CEnumTags;
class CImageStore;


class CPasteDeepModel : private utl::noncopyable
{
public:
	CPasteDeepModel( const fs::CPath& destDirPath );
	~CPasteDeepModel();

	void Init( const std::vector< fs::CPath >& srcPaths, DROPEFFECT dropEffect );
	void BuildFromClipboard( void );
	void Clear( void );

	static bool AlsoCopyFilesAsPaths( CWnd* pParentOwner );		// if files Copied or Pasted on clipboard, also store their paths as text

	bool HasSrcPaths( void ) const { return !m_srcPaths.empty(); }
	const std::vector< fs::CPath >& GetSrcPaths( void ) const { return m_srcPaths; }

	bool HasRelFolderPaths( void ) const { return !m_relFolderPaths.empty(); }
	const std::vector< fs::CPath >& GetRelFolderPaths( void ) const { return m_relFolderPaths; }

	CBitmap* GetItemInfo( std::tstring& rItemText, UINT fldPos ) const;
	bool PasteDeep( const fs::CPath& relFolderPath, CWnd* pParentOwner );

	enum PasteOperation { PasteNone, PasteCopyFiles, PasteMoveFiles };

	static const CEnumTags& GetTags_PasteOperation( void );
	PasteOperation GetPasteOperation( void ) const;
private:
	static void QueryClipboardData( std::vector< fs::CPath >& rSrcPaths, DROPEFFECT& rDropEffect );
	void RegisterFolderImage( const fs::CPath& folderPath );
	fs::CPath MakeDeepTargetFilePath( const fs::CPath& srcFilePath, const fs::CPath& relFolderPath ) const;
private:
	fs::CPath m_destDirPath;
	std::vector< fs::CPath > m_srcPaths;
	DROPEFFECT m_dropEffect;			// Copy=DROPEFFECT_COPY|DROPEFFECT_LINK, Paste=DROPEFFECT_MOVE

	fs::CPath m_srcParentPath;
	std::vector< fs::CPath > m_relFolderPaths;		// relative folders from the original source parent path

	std::auto_ptr< CImageStore > m_pImageStore;		// for folder images
	enum { BaseImageId = 100 };
};


#endif // PasteDeepModel_h
