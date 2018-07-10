#ifndef FileModel_h
#define FileModel_h
#pragma once

#include "utl/Subject.h"
#include "utl/InternalChange.h"
#include "PathItemBase.h"


class CCommandModel;
class CRenameItem;
class CTouchItem;
interface IFileEditor;
namespace fmt { enum PathFormat; }
namespace cmd { enum CommandType; enum StackType; }


class CFileModel : public CSubject
				 , public CInternalChange
				 , private utl::noncopyable
{
public:
	CFileModel( void );
	~CFileModel();

	void Clear( void );
	size_t SetupFromDropInfo( HDROP hDropInfo );

	const std::vector< fs::CPath >& GetSourcePaths( void ) const { return m_sourcePaths; }
	CCommandModel* GetCommandModel( void );

	bool SaveCommandModel( void ) const;
	bool LoadCommandModel( void );

	bool CanUndoRedo( cmd::StackType stackType, int typeId = 0 ) const;
	bool UndoRedo( cmd::StackType stackType );
	void FetchFromStack( cmd::StackType stackType );				// fetches data set from undo stack (macro command)

	template< typename CmdType >
	CmdType* PeekCmdAs( cmd::StackType stackType ) const;

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual void UpdateAllObservers( utl::IMessage* pMessage );

	std::vector< CRenameItem* >& LazyInitRenameItems( void );
	std::vector< CTouchItem* >& LazyInitTouchItems( void );

	const std::vector< CRenameItem* >& GetRenameItems( void ) const { ASSERT( !m_renameItems.empty() ); return m_renameItems; }
	const std::vector< CTouchItem* >& GetTouchItems( void ) const { ASSERT( !m_touchItems.empty() ); return m_touchItems; }

	void ResetDestinations( void );

	IFileEditor* MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent );

	// RENAME
	bool CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd ) const;
	void PasteClipDestinationPaths( CWnd* pWnd ) throws_( CRuntimeException );

	template< typename FuncType >
	void ForEachRenameDestination( const FuncType& func );

	// TOUCH
	bool CopyClipSourceFileStates( CWnd* pWnd ) const;
	void PasteClipDestinationFileStates( CWnd* pWnd ) throws_( CRuntimeException );
private:
	template< typename ContainerT >
	void StoreSourcePaths( const ContainerT& sourcePaths );

	struct AddRenameItemFromCmd
	{
		AddRenameItemFromCmd( CFileModel* pFileModel, cmd::StackType stackType )
			: m_pFileModel( pFileModel )
			, m_stackType( stackType )
		{
			ASSERT( m_pFileModel != NULL && m_pFileModel->m_sourcePaths.empty() );
		}

		void operator()( const utl::ICommand* pCmd );
	private:
		CFileModel* m_pFileModel;
		cmd::StackType m_stackType;
	};

	struct AddTouchItemFromCmd
	{
		AddTouchItemFromCmd( CFileModel* pFileModel, cmd::StackType stackType )
			: m_pFileModel( pFileModel )
			, m_stackType( stackType )
		{
			ASSERT( m_pFileModel != NULL && m_pFileModel->m_sourcePaths.empty() );
		}

		void operator()( const utl::ICommand* pCmd );
	private:
		CFileModel* m_pFileModel;
		cmd::StackType m_stackType;
	};

	friend struct AddRenameItemFromCmd;
	friend struct AddTouchItemFromCmd;
private:
	std::vector< fs::CPath > m_sourcePaths;
	fs::CPath m_commonParentPath;						// for paths in multiple directories

	// lazy init
	std::auto_ptr< CCommandModel > m_pCommandModel;		// self-encapsulated
	std::vector< CRenameItem* > m_renameItems;
	std::vector< CTouchItem* > m_touchItems;
};


// template code

template< typename CmdType >
CmdType* CFileModel::PeekCmdAs( cmd::StackType stackType ) const
{
	if ( m_pCommandModel.get() != NULL )
		return dynamic_cast< CmdType* >( cmd::Undo == stackType ? m_pCommandModel->PeekUndo() : m_pCommandModel->PeekRedo() );

	return NULL;
}

template< typename FuncType >
void CFileModel::ForEachRenameDestination( const FuncType& func )
{
	REQUIRE( !m_renameItems.empty() );		// should be initialized

	for ( std::vector< CRenameItem* >::const_iterator itItem = m_renameItems.begin(); itItem != m_renameItems.end(); ++itItem )
	{
		fs::CPathParts destParts( ( *itItem )->GetSafeDestPath().Get() );
		func( destParts );
		( *itItem )->RefDestPath() = destParts.MakePath();
	}

	UpdateAllObservers( NULL );			// rename items changed
}


#endif // FileModel_h
