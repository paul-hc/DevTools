#ifndef FileModel_h
#define FileModel_h
#pragma once

#include "utl/Subject.h"
#include "utl/PathItemBase.h"
#include "utl/ICommand.h"
#include "utl/UI/InternalChange.h"
#include "Application_fwd.h"
#include <unordered_map>


class CRenameItem;
class CTouchItem;
class CDisplayFilenameAdapter;
interface IFileEditor;
namespace fmt { enum PathFormat; }
namespace cmd { enum CommandType; }


namespace func
{
	struct ResetItem
	{
		template< typename ItemType >
		void operator()( ItemType* pItem )
		{
			pItem->Reset();
		}
	};
}


class CFileModel : public CCmdTarget		// owned by CShellMenuController
	, public TSubject
	, public CInternalChange
	, private utl::noncopyable
{
public:
	CFileModel( svc::ICommandService* pCmdSvc );
	~CFileModel();

	void Clear( void );
	size_t SetupFromDropInfo( HDROP hDropInfo );

	const std::vector<fs::CPath>& GetSourcePaths( void ) const { return m_sourcePaths; }
	bool IsSourceSingleFolder( void ) const;						// single selected directory as paste target?

	const std::vector<fs::CPath>& GetSrcFolderPaths( void ) const { return m_srcFolderPaths; }

	app::TargetScope GetTargetScope( void ) const { return m_targetScope; }
	bool SetTargetScope( app::TargetScope targetScope ) { return utl::ModifyValue( m_targetScope, targetScope ); }

	bool SafeExecuteCmd( IFileEditor* pEditor, utl::ICommand* pCmd );
	void FetchFromStack( svc::StackType stackType );				// fetches data set from undo stack (macro command)

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual void UpdateAllObservers( utl::IMessage* pMessage );

	std::vector<CRenameItem*>& LazyInitRenameItems( void );
	std::vector<CTouchItem*>& LazyInitTouchItems( void );

	const std::vector<CRenameItem*>& GetRenameItems( void ) const { return m_renameItems; }
	const std::vector<CTouchItem*>& GetTouchItems( void ) const { return m_touchItems; }

	CRenameItem* FindRenameItem( const fs::CPath& srcPath ) const;
	CTouchItem* FindTouchItem( const fs::CPath& srcPath ) const;

	void ResetDestinations( void );

	IFileEditor* MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent );
	std::pair<IFileEditor*, bool> HandleUndoRedo( svc::StackType stackType, CWnd* pParent );			// from the corresponding stack top: if an editor-based cmd, return editor; otherwise execute it
	static bool HasFileEditor( cmd::CommandType cmdType );

	bool CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd ) const;

	// RENAME:
	template< typename PathPartsFuncT >
	utl::ICommand* MakeChangeDestPathsCmd( const PathPartsFuncT& pathPartsFunc, const std::vector<CRenameItem*>& renameItems, const std::tstring& cmdTag ) const;

	static bool CopyClipRenameSrcPaths( const std::vector<CRenameItem*>& renameItems, fmt::PathFormat format, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter );
	utl::ICommand* MakeClipPasteDestPathsCmd( const std::vector<CRenameItem*>& renameItems, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter ) const throws_( CRuntimeException );

	const ren::TSortingPair& GetRenameSorting( void ) const { return m_renameSorting; }
	void SetRenameSorting( const ren::TSortingPair& renameSorting );
	void SwapRenameSequence( std::vector<CRenameItem*>& rListSequence, const ren::TSortingPair& renameSorting );

	// TOUCH:
	static bool CopyClipSourceFileStates( CWnd* pWnd, const std::vector<CTouchItem*>& touchItems );
	utl::ICommand* MakeClipPasteDestFileStatesCmd( const std::vector<CTouchItem*>& touchItems, CWnd* pWnd ) throws_( CRuntimeException );
private:
	static std::tstring FormatPath( const fs::CPath& filePath, fmt::PathFormat format, const CDisplayFilenameAdapter* pDisplayAdapter );
	bool PromptExtensionChanges( const std::vector<CRenameItem*>& renameItems, const std::vector<fs::CPath>& destPaths ) const;

	void SortRenameItems( void );
	void RegSave( void );

	template< typename ContainerT >
	void StoreSourcePaths( const ContainerT& sourcePaths );

	struct AddRenameItemFromCmd
	{
		AddRenameItemFromCmd( CFileModel* pFileModel, svc::StackType stackType )
			: m_pFileModel( pFileModel )
			, m_stackType( stackType )
		{
			ASSERT( m_pFileModel != nullptr && m_pFileModel->m_sourcePaths.empty() );
		}

		void operator()( const utl::ICommand* pCmd );
	private:
		CFileModel* m_pFileModel;
		svc::StackType m_stackType;
	};

	struct AddTouchItemFromCmd
	{
		AddTouchItemFromCmd( CFileModel* pFileModel, svc::StackType stackType )
			: m_pFileModel( pFileModel )
			, m_stackType( stackType )
		{
			ASSERT( m_pFileModel != nullptr && m_pFileModel->m_sourcePaths.empty() );
		}

		void operator()( const utl::ICommand* pCmd );
	private:
		CFileModel* m_pFileModel;
		svc::StackType m_stackType;
	};

	friend struct AddRenameItemFromCmd;
	friend struct AddTouchItemFromCmd;
private:
	svc::ICommandService* m_pCmdSvc;
	std::vector<fs::CPath> m_sourcePaths;
	fs::CPath m_commonParentPath;					// for paths in multiple directories
	std::vector<fs::CPath> m_srcFolderPaths;		// all folders referenced

	/*persist*/ app::TargetScope m_targetScope;
	persist ren::TSortingPair m_renameSorting;

	// lazy init
	typedef std::pair<CRenameItem*, CTouchItem*> TItemPair;

	std::vector<CRenameItem*> m_renameItems;
	std::vector<CTouchItem*> m_touchItems;
	std::unordered_map<fs::CPath, TItemPair > m_srcPatchToItemsMap;
public:
	static const std::tstring section_filesSheet;

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


// template code

template< typename PathPartsFuncT >
utl::ICommand* CFileModel::MakeChangeDestPathsCmd( const PathPartsFuncT& pathPartsFunc, const std::vector<CRenameItem*>& renameItems, const std::tstring& cmdTag ) const
{
	std::vector<fs::CPath> destPaths; destPaths.reserve( renameItems.size() );
	bool anyChange = false;

	for ( const CRenameItem* pItem: renameItems )
	{
		fs::CPathParts destParts;
		pItem->SplitSafeDestPath( &destParts );		// for directories treat extension as part of the fname

		pathPartsFunc( destParts );

		destPaths.push_back( destParts.MakePath() );

		anyChange |= destPaths.back().Get() != pItem->GetDestPath().Get();		// case-sensitive string compare
	}

	return anyChange ? new CChangeDestPathsCmd( const_cast<CFileModel*>( this ), &renameItems, destPaths, cmdTag ) : nullptr;
}


#endif // FileModel_h
