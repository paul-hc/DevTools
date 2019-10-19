#ifndef FileModel_h
#define FileModel_h
#pragma once

#include "utl/Subject.h"
#include "utl/PathItemBase.h"
#include "utl/ICommand.h"
#include "utl/UI/InternalChange.h"

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


class CFileModel : public CCmdTarget
				 , public CSubject
				 , public CInternalChange
				 , private utl::noncopyable
{
public:
	CFileModel( svc::ICommandService* pCmdSvc );
	~CFileModel();

	void Clear( void );
	size_t SetupFromDropInfo( HDROP hDropInfo );

	const std::vector< fs::CPath >& GetSourcePaths( void ) const { return m_sourcePaths; }
	bool IsSourceSingleFolder( void ) const;						// single selected directory as paste target?

	bool SafeExecuteCmd( IFileEditor* pEditor, utl::ICommand* pCmd );
	void FetchFromStack( svc::StackType stackType );				// fetches data set from undo stack (macro command)

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual void UpdateAllObservers( utl::IMessage* pMessage );

	std::vector< CRenameItem* >& LazyInitRenameItems( void );
	std::vector< CTouchItem* >& LazyInitTouchItems( void );

	const std::vector< CRenameItem* >& GetRenameItems( void ) const { return m_renameItems; }
	const std::vector< CTouchItem* >& GetTouchItems( void ) const { return m_touchItems; }

	void ResetDestinations( void );

	IFileEditor* MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent );
	std::pair< IFileEditor*, bool > HandleUndoRedo( svc::StackType stackType, CWnd* pParent );			// from the corresponding stack top: if an editor-based cmd, return editor; otherwise execute it
	static bool HasFileEditor( cmd::CommandType cmdType );

	// RENAME
	template< typename FuncType >
	utl::ICommand* MakeChangeDestPathsCmd( const FuncType& func, const std::tstring& cmdTag );

	bool CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter = NULL ) const;
	utl::ICommand* MakeClipPasteDestPathsCmd( CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter ) throws_( CRuntimeException );

	bool PromptExtensionChanges( const std::vector< fs::CPath >& destPaths ) const;

	// TOUCH
	bool CopyClipSourceFileStates( CWnd* pWnd ) const;
	utl::ICommand* MakeClipPasteDestFileStatesCmd( CWnd* pWnd ) throws_( CRuntimeException );
private:
	static std::tstring FormatPath( const fs::CPath& filePath, fmt::PathFormat format, const CDisplayFilenameAdapter* pDisplayAdapter );

	template< typename ContainerT >
	void StoreSourcePaths( const ContainerT& sourcePaths );

	struct AddRenameItemFromCmd
	{
		AddRenameItemFromCmd( CFileModel* pFileModel, svc::StackType stackType )
			: m_pFileModel( pFileModel )
			, m_stackType( stackType )
		{
			ASSERT( m_pFileModel != NULL && m_pFileModel->m_sourcePaths.empty() );
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
			ASSERT( m_pFileModel != NULL && m_pFileModel->m_sourcePaths.empty() );
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
	std::vector< fs::CPath > m_sourcePaths;
	fs::CPath m_commonParentPath;						// for paths in multiple directories

	// lazy init
	std::vector< CRenameItem* > m_renameItems;
	std::vector< CTouchItem* > m_touchItems;

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


// template code

template< typename FuncType >
utl::ICommand* CFileModel::MakeChangeDestPathsCmd( const FuncType& func, const std::tstring& cmdTag )
{
	REQUIRE( !m_renameItems.empty() );		// initialized?

	bool anyChanges = false;
	std::vector< fs::CPath > destPaths; destPaths.reserve( m_renameItems.size() );

	for ( std::vector< CRenameItem* >::const_iterator itItem = m_renameItems.begin(); itItem != m_renameItems.end(); ++itItem )
	{
		fs::CPathParts destParts( ( *itItem )->GetSafeDestPath().Get() );
		func( destParts );

		destPaths.push_back( destParts.MakePath() );

		if ( destPaths.back().Get() != ( *itItem )->GetDestPath().Get() )		// case-sensitive string compare
			anyChanges = true;
	}

	return anyChanges ? new CChangeDestPathsCmd( this, destPaths, cmdTag ) : NULL;
}


#endif // FileModel_h
