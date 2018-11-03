
#include "stdafx.h"
#include "FileModel.h"
#include "CommandModelService.h"
#include "FileCommands.h"
#include "IFileEditor.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "GeneralOptions.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/Command.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"

// for CFileModel::MakeFileEditor
#include "RenameFilesDialog.h"
#include "TouchFilesDialog.h"
#include "FindDuplicatesDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileModel::CFileModel( void )
{
}

CFileModel::~CFileModel()
{
	Clear();
}

void CFileModel::Clear( void )
{
	m_sourcePaths.clear();
	m_commonParentPath.Clear();

	utl::ClearOwningContainer( m_renameItems );
	utl::ClearOwningContainer( m_touchItems );
}

size_t CFileModel::SetupFromDropInfo( HDROP hDropInfo )
{
	ASSERT_PTR( hDropInfo );

	std::vector< fs::CPath > sourcePaths;
	for ( UINT i = 0, fileCount = ::DragQueryFile( hDropInfo, UINT_MAX, NULL, 0 ); i != fileCount; ++i )
	{
		TCHAR pathBuffer[ MAX_PATH ];
		::DragQueryFile( hDropInfo, i, pathBuffer, COUNT_OF( pathBuffer ) );

		sourcePaths.push_back( fs::CPath( pathBuffer ) );
	}

// to test multiple paths:
//str::Split( sourcePaths, _T("C:\\dev\\DevTools\\CommonUtl\\DemoUtl\\DemoUtl.rc|C:\\dev\\DevTools\\CommonUtl\\utl\\utl_ui.rc|C:\\dev\\DevTools\\FileRenShell\\FileRenShell.rc"), _T("|") );

	StoreSourcePaths( sourcePaths );
	return m_sourcePaths.size();
}

template< typename ContainerT >
void CFileModel::StoreSourcePaths( const ContainerT& sourcePaths )
{
	ASSERT( !sourcePaths.empty() );
	Clear();

	for ( typename ContainerT::const_iterator itSourcePath = sourcePaths.begin(); itSourcePath != sourcePaths.end(); ++itSourcePath )
		m_sourcePaths.push_back( func::PathOf( *itSourcePath ) );

	fs::SortDirectoriesFirst( m_sourcePaths );
	m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
}

CCommandModel* CFileModel::GetCommandModel( void )
{
	if ( NULL == m_pCommandModel.get() )
		m_pCommandModel.reset( new CCommandModel );

	return m_pCommandModel.get();
}

bool CFileModel::SafeExecuteCmd( IFileEditor* pEditor, utl::ICommand* pCmd )
{
	if ( NULL == pCmd )
		return false;

	bool executeInline = false;

	if ( pEditor != NULL && pEditor->IsRollMode() )
		executeInline = true;
	else if ( !CGeneralOptions::Instance().m_undoEditingCmds )
		executeInline = !cmd::IsPersistentCmd( pCmd );			// local editing command?

	if ( executeInline )
	{
		std::auto_ptr< utl::ICommand > pEditCmd( pCmd );		// take ownership
		return pEditCmd->Execute();
	}

	return GetCommandModel()->Execute( pCmd );
}

bool CFileModel::CanUndoRedo( cmd::StackType stackType, int typeId /*= 0*/ ) const
{
	if ( utl::ICommand* pTopCmd = PeekCmdAs< utl::ICommand >( stackType ) )
		if ( 0 == typeId || typeId == pTopCmd->GetTypeID() )
			return cmd::Undo == stackType
				? m_pCommandModel->CanUndo()
				: m_pCommandModel->CanRedo();

	return false;
}

bool CFileModel::UndoRedo( cmd::StackType stackType )
{
	return cmd::Undo == stackType ? GetCommandModel()->Undo() : GetCommandModel()->Redo();
}

void CFileModel::FetchFromStack( cmd::StackType stackType )
{
	// fills the data set from undo stack
	ASSERT_PTR( m_pCommandModel.get() );
	ASSERT( cmd::Undo == stackType ? m_pCommandModel->CanUndo() : m_pCommandModel->CanRedo() );

	utl::ICommand* pTopCmd = PeekCmdAs< utl::ICommand >( stackType );
	if ( const cmd::CFileMacroCmd* pFileMacroCmd = dynamic_cast< const cmd::CFileMacroCmd* >( pTopCmd ) )
	{
		Clear();

		switch ( pFileMacroCmd->GetTypeID() )
		{
			case cmd::RenameFile:	utl::for_each( pFileMacroCmd->GetSubCommands(), AddRenameItemFromCmd( this, stackType ) ); break;
			case cmd::TouchFile:	utl::for_each( pFileMacroCmd->GetSubCommands(), AddTouchItemFromCmd( this, stackType ) ); break;
		}
		m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
		//utl::for_each( m_renameItems, func::StripDisplayCode( m_commonParentPath ) );		// use always filename.ext for path diffs
		utl::for_each( m_touchItems, func::StripDisplayCode( m_commonParentPath ) );

		UpdateAllObservers( NULL );				// items changed
	}
	else
		UndoRedo( stackType );
}

bool CFileModel::SaveCommandModel( void ) const
{
	if ( NULL == m_pCommandModel.get() || !CGeneralOptions::Instance().m_undoLogPersist )
		return false;

	// cleanup the command stacks before saving
	m_pCommandModel->RemoveExpiredCommands( 60 );
	m_pCommandModel->RemoveCommandsThat( pred::IsZombieCmd() );		// zombie command: it has no effect on files (in most cases empty macros due to non-existing files)

	return CCommandModelService::SaveUndoLog( *m_pCommandModel, CGeneralOptions::Instance().m_undoLogFormat );
}

bool CFileModel::LoadCommandModel( void )
{
	if ( m_pCommandModel.get() != NULL )
		return false;				// already loaded once

	return
		CGeneralOptions::Instance().m_undoLogPersist &&
		CCommandModelService::LoadUndoLog( GetCommandModel() );			// load the most recently modified log file (regardless of CGeneralOptions::m_undoLogFormat)
}

std::vector< CRenameItem* >& CFileModel::LazyInitRenameItems( void )
{
	if ( m_renameItems.empty() )
	{
		ASSERT( !m_sourcePaths.empty() );

		for ( std::vector< fs::CPath >::const_iterator itSource = m_sourcePaths.begin(); itSource != m_sourcePaths.end(); ++itSource )
		{
			CRenameItem* pNewItem = new CRenameItem( *itSource );

			//pNewItem->StripDisplayCode( m_commonParentPath );		// use always filename.ext for path diffs
			m_renameItems.push_back( pNewItem );
		}
	}

	return m_renameItems;
}

std::vector< CTouchItem* >& CFileModel::LazyInitTouchItems( void )
{
	if ( m_touchItems.empty() )
	{
		ASSERT( !m_sourcePaths.empty() );

		for ( std::vector< fs::CPath >::const_iterator itSource = m_sourcePaths.begin(); itSource != m_sourcePaths.end(); ++itSource )
		{
			CTouchItem* pNewItem = new CTouchItem( fs::CFileState::ReadFromFile( *itSource ) );

			pNewItem->StripDisplayCode( m_commonParentPath );
			m_touchItems.push_back( pNewItem );
		}
	}

	return m_touchItems;
}

const std::tstring& CFileModel::GetCode( void ) const
{
	static const std::tstring s_code = _T("File Model");
	return s_code;
}

void CFileModel::UpdateAllObservers( utl::IMessage* pMessage )
{
	if ( !IsInternalChange() )
		CSubject::UpdateAllObservers( pMessage );
}

void CFileModel::ResetDestinations( void )
{
	utl::for_each( m_renameItems, func::ResetItem() );
	utl::for_each( m_touchItems, func::ResetItem() );

	UpdateAllObservers( NULL );				// path items changed
}


// RENAME

bool CFileModel::CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd ) const
{
	std::vector< std::tstring > sourcePaths;
	for ( std::vector< fs::CPath >::const_iterator itSrcPath = m_sourcePaths.begin(); itSrcPath != m_sourcePaths.end(); ++itSrcPath )
		sourcePaths.push_back( fmt::FormatPath( *itSrcPath, format ) );

	return CClipboard::CopyText( str::Join( sourcePaths, _T("\r\n") ), pWnd );
}

utl::ICommand* CFileModel::MakeClipPasteDestPathsCmd( CWnd* pWnd ) throws_( CRuntimeException )
{
	REQUIRE( !m_renameItems.empty() );		// initialized?

	static const CRuntimeException s_noDestExc( _T("No destination file paths available to paste.") );

	std::tstring text;
	if ( !CClipboard::CanPasteText() || !CClipboard::PasteText( text, pWnd ) )
		throw s_noDestExc;

	std::vector< std::tstring > textPaths;
	str::Split( textPaths, text.c_str(), _T("\r\n") );

	for ( std::vector< std::tstring >::iterator itPath = textPaths.begin(); itPath != textPaths.end(); )
	{
		str::Trim( *itPath );
		if ( itPath->empty() )
			itPath = textPaths.erase( itPath );
		else
			++itPath;
	}

	if ( textPaths.empty() )
		throw s_noDestExc;
	else if ( m_sourcePaths.size() != textPaths.size() )
		throw CRuntimeException( str::Format( _T("Destination file count of %d doesn't match source file count of %d."),
											  textPaths.size(), m_sourcePaths.size() ) );

	std::vector< fs::CPath > destPaths; destPaths.reserve( GetRenameItems().size() );
	bool anyChanges = false;

	for ( size_t i = 0; i != textPaths.size(); ++i )
	{
		const CRenameItem* pRenameItem = m_renameItems[ i ];
		fs::CPath newFilePath( textPaths[ i ] );

		if ( !newFilePath.HasParentPath() )
			newFilePath.SetDirPath( pRenameItem->GetSrcPath().GetParentPath().Get() );		// qualify with SRC dir path

		if ( newFilePath.Get() != pRenameItem->GetDestPath().Get() )		// case-sensitive string compare
			anyChanges = true;

		destPaths.push_back( newFilePath );
	}

	return anyChanges ? new CChangeDestPathsCmd( this, destPaths, _T("Paste destination file paths from clipboard") ) : NULL;
}


// TOUCH

bool CFileModel::CopyClipSourceFileStates( CWnd* pWnd ) const
{
	REQUIRE( !m_touchItems.empty() );		// should be initialized

	std::vector< std::tstring > sourcePaths; sourcePaths.reserve( m_sourcePaths.size() );

	for ( std::vector< CTouchItem* >::const_iterator itTouchItem = m_touchItems.begin(); itTouchItem != m_touchItems.end(); ++itTouchItem )
		sourcePaths.push_back( fmt::FormatClipFileState( ( *itTouchItem )->GetSrcState(), fmt::FilenameExt ) );

	return CClipboard::CopyText( str::Join( sourcePaths, _T("\r\n") ), pWnd );
}

utl::ICommand* CFileModel::MakeClipPasteDestFileStatesCmd( CWnd* pWnd ) throws_( CRuntimeException )
{
	REQUIRE( !m_touchItems.empty() );		// should be initialized

	std::vector< std::tstring > lines;
	std::tstring text;
	if ( CClipboard::CanPasteText() && CClipboard::PasteText( text, pWnd ) )
		str::Split( lines, text.c_str(), _T("\r\n") );

	for ( std::vector< std::tstring >::iterator itLine = lines.begin(); itLine != lines.end(); )
	{
		str::Trim( *itLine );
		if ( itLine->empty() )
			itLine = lines.erase( itLine );
		else
			++itLine;
	}

	if ( lines.empty() )
		throw CRuntimeException( _T("No destination file states available to paste.") );
	else if ( m_touchItems.size() != lines.size() )
		throw CRuntimeException( str::Format( _T("Destination file state count of %d doesn't match source file count of %d."),
											  lines.size(), m_touchItems.size() ) );

	std::vector< fs::CFileState > destFileStates; destFileStates.reserve( GetTouchItems().size() );
	bool anyChanges = false;

	for ( size_t i = 0; i != GetTouchItems().size(); ++i )
	{
		const CTouchItem* pTouchItem = m_touchItems[ i ];

		fs::CFileState newFileState;
		fmt::ParseClipFileState( newFileState, lines[ i ], &pTouchItem->GetFilePath() );

		if ( newFileState != pTouchItem->GetDestState() )
			anyChanges = true;

		destFileStates.push_back( newFileState );
	}

	return anyChanges ? new CChangeDestFileStatesCmd( this, destFileStates, _T("Paste destination file attribute states from clipboard") ) : NULL;
}


// CFileModel::AddRenameItemFromCmd implementation

void CFileModel::AddRenameItemFromCmd::operator()( const utl::ICommand* pCmd )
{
	const CRenameFileCmd* pRenameCmd = checked_static_cast< const CRenameFileCmd* >( pCmd );

	fs::CPath srcPath = pRenameCmd->m_srcPath, destPath = pRenameCmd->m_destPath;
	if ( cmd::Undo == m_stackType )
		std::swap( srcPath, destPath );			// swap DEST and SRC for undo

	CRenameItem* pRenameItem = new CRenameItem( srcPath );
	pRenameItem->RefDestPath() = destPath;

	m_pFileModel->m_sourcePaths.push_back( pRenameItem->GetFilePath() );
	m_pFileModel->m_renameItems.push_back( pRenameItem );
}


// CFileModel::AddTouchItemFromCmd implementation

void CFileModel::AddTouchItemFromCmd::operator()( const utl::ICommand* pCmd )
{
	const CTouchFileCmd* pTouchCmd = checked_static_cast< const CTouchFileCmd* >( pCmd );

	fs::CFileState srcState = pTouchCmd->m_srcState, destState = pTouchCmd->m_destState;
	if ( cmd::Undo == m_stackType )
		std::swap( srcState, destState );			// swap DEST and SRC for undo

	CTouchItem* pTouchItem = new CTouchItem( srcState );
	pTouchItem->RefDestState() = destState;

	m_pFileModel->m_sourcePaths.push_back( pTouchItem->GetFilePath() );
	m_pFileModel->m_touchItems.push_back( pTouchItem );
}

IFileEditor* CFileModel::MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent )
{
	switch ( cmdType )
	{
		case cmd::RenameFile:
		case cmd::ChangeDestPaths:
			return new CRenameFilesDialog( this, pParent );
		case cmd::TouchFile:
			return new CTouchFilesDialog( this, pParent );
		case cmd::FindDuplicates:
			return new CFindDuplicatesDialog( this, pParent );
	}
	ASSERT( false );
	return NULL;
}


// command handlers

BEGIN_MESSAGE_MAP( CFileModel, CCmdTarget )
END_MESSAGE_MAP()
