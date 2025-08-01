#ifndef RenameItem_h
#define RenameItem_h
#pragma once

#include "utl/FileStateItem.h"
#include "utl/PathRenamePairs.h"


class CRenameItem;
class CPathFormatter;


namespace ren
{
	bool MakePairsFromItems( CPathRenamePairs* pOutRenamePairs, const std::vector<CRenameItem*>& renameItems );
	void MakePairsToItems( std::vector<CRenameItem*>& rOutRenameItems, const CPathRenamePairs& renamePairs );
	void AssignPairsToItems( const std::vector<CRenameItem*>& items, const CPathRenamePairs& renamePairs );
	void QueryDestFnames( std::vector<std::tstring>& rDestFnames, const std::vector<CRenameItem*>& items );

	// special directory handling: treat ext as part of fname (no file type by extension)
	bool SplitPath( fs::CPathParts* pOutParts, const fs::CPath* pSrcFilePath, const fs::CPath& filePath );


	bool FormatHasValidEffect( const CPathFormatter& pathFormatter, const std::vector<CRenameItem*>& renameItems, bool avoidDups = false );		// valid format considering the rename effects?
}


class CRenameItem : public CFileStateItem
{
public:
	CRenameItem( const fs::CPath& srcPath );
	virtual ~CRenameItem();

	const fs::CPath& GetSrcPath( void ) const { return GetFilePath(); }
	const fs::CPath& GetDestPath( void ) const { return m_destPath; }
	const fs::CPath& GetSafeDestPath( void ) const { return !m_destPath.IsEmpty() ? m_destPath : GetSrcPath(); }	// use SRC if DEST emty

	bool IsModified( void ) const { return HasDestPath() && m_destPath.Get() != GetSrcPath().Get(); }		// case-sensitive string compare (not paths)
	bool HasDestPath( void ) const { return !m_destPath.IsEmpty(); }

	void Reset( void ) { m_destPath = GetSrcPath(); }
	fs::CPath& RefDestPath( void ) { return m_destPath; }

	// special directory handling
	bool SplitPath( fs::CPathParts* pOutParts, const fs::CPath& filePath ) const { return ren::SplitPath( pOutParts, &GetSrcPath(), filePath ); }
	bool SplitSafeDestPath( fs::CPathParts* pOutParts ) const { return SplitPath( pOutParts, GetSafeDestPath() ); }
private:
	fs::CPath m_destPath;
};


#include "utl/UI/ObjectCtrlBase.h"


namespace fmt { enum PathFormat; }


class CDisplayFilenameAdapter : public ui::ISubjectAdapter
{
public:
	CDisplayFilenameAdapter( bool ignoreExtension ) : m_ignoreExtension( ignoreExtension ) {}

	void SetIgnoreExtension( bool ignoreExtension ) { m_ignoreExtension = ignoreExtension; }

	// ui::ISubjectAdapter interface
	virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;

	std::tstring FormatFilename( const fs::CPath& filePath, bool isDirectory ) const;
	fs::CPath ParseFilename( const std::tstring& displayFilename, const fs::CPath& referencePath ) const;

	std::tstring FormatPath( fmt::PathFormat format, const fs::CPath& filePath ) const;
	fs::CPath ParsePath( const std::tstring& inputPath, const fs::CPath& referencePath ) const;

	static bool IsExtensionChange( const fs::CPath& referencePath, const fs::CPath& destPath );			// ignore case changes
private:
	static bool IsDirectoryItem( const utl::ISubject* pSubject );
	static std::tstring StripExtension( const TCHAR* pFilePath );
private:
	bool m_ignoreExtension;
};


namespace func
{
	struct AsSrcPath
	{
		const fs::CPath& operator()( const CRenameItem* pItem ) const { return pItem->GetSrcPath(); }
	};

	struct AsDestPath
	{
		const fs::CPath& operator()( const CRenameItem* pItem ) const { return pItem->GetDestPath(); }
	};
}


#endif // RenameItem_h
