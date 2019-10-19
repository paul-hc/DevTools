
#include "stdafx.h"
#include "FileModel.h"
#include "FileMacroCommands.h"
#include "EditingCommands.h"
#include "IFileEditor.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "resource.h"
#include "utl/Command.h"
#include "utl/ContainerUtilities.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"
#include "utl/UI/Clipboard.h"
#include "utl/UI/ObjectCtrlBase.h"
#include "utl/UI/ShellUtilities.h"

// for CFileModel::MakeFileEditor
#include "RenameFilesDialog.h"
#include "TouchFilesDialog.h"
#include "FindDuplicatesDialog.h"
#include "CmdDashboardDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileModel::CFileModel( svc::ICommandService* pCmdSvc )
	: m_pCmdSvc( pCmdSvc )
{
	ASSERT_PTR( m_pCmdSvc );
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
	std::vector< fs::CPath > sourcePaths;
	shell::QueryDroppedFiles( sourcePaths, hDropInfo );

// to test multiple paths:
//str::Split( sourcePaths, _T("C:\\dev\\DevTools\\CommonUtl\\DemoUtl\\DemoUtl.rc|C:\\dev\\DevTools\\CommonUtl\\utl\\utl_ui.rc|C:\\dev\\DevTools\\ShellGoodies\\ShellGoodies.rc"), _T("|") );

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

	fs::SortPathsDirsFirst( m_sourcePaths );
	m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
}

bool CFileModel::IsSourceSingleFolder( void ) const
{
	return 1 == m_sourcePaths.size() && fs::IsValidDirectory( m_sourcePaths.front().GetPtr() );
}

bool CFileModel::SafeExecuteCmd( IFileEditor* pEditor, utl::ICommand* pCmd )
{
	return m_pCmdSvc->SafeExecuteCmd( pCmd, pEditor != NULL && pEditor->IsRollMode() );
}

void CFileModel::FetchFromStack( svc::StackType stackType )
{
	// fills the data set from undo stack
	ASSERT( m_pCmdSvc->CanUndoRedo( stackType ) );

	utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType );
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
		m_pCmdSvc->UndoRedo( stackType );
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

std::tstring CFileModel::FormatPath( const fs::CPath& filePath, fmt::PathFormat format, const CDisplayFilenameAdapter* pDisplayAdapter )
{
	return pDisplayAdapter != NULL
		? pDisplayAdapter->FormatPath( format, filePath )
		: fmt::FormatPath( filePath, format );
}


// RENAME

bool CFileModel::CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter /*= NULL*/ ) const
{
	std::vector< std::tstring > sourcePaths;
	for ( std::vector< fs::CPath >::const_iterator itSrcPath = m_sourcePaths.begin(); itSrcPath != m_sourcePaths.end(); ++itSrcPath )
		sourcePaths.push_back( FormatPath( *itSrcPath, format, pDisplayAdapter ) );

	return CClipboard::CopyToLines( sourcePaths, pWnd );
}

utl::ICommand* CFileModel::MakeClipPasteDestPathsCmd( CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter ) throws_( CRuntimeException )
{
	REQUIRE( !m_renameItems.empty() );		// initialized?

	static const CRuntimeException s_noDestExc( _T("No destination file paths available to paste.") );

	std::vector< std::tstring > textPaths;
	if ( !CClipboard::CanPasteText() || !CClipboard::PasteFromLines( textPaths, pWnd ) )
		throw s_noDestExc;

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

	std::vector< fs::CPath > destPaths; destPaths.reserve( m_renameItems.size() );
	bool anyChanges = false;

	for ( size_t i = 0; i != textPaths.size(); ++i )
	{
		const CRenameItem* pRenameItem = m_renameItems[ i ];
		fs::CPath newFilePath = pDisplayAdapter->ParsePath( textPaths[ i ], pRenameItem->GetSafeDestPath() );

		if ( newFilePath.Get() != pRenameItem->GetDestPath().Get() )		// case-sensitive string compare
			anyChanges = true;

		destPaths.push_back( newFilePath );
	}

	if ( !anyChanges )
		return NULL;

	if ( !PromptExtensionChanges( destPaths ) )
		return NULL;

	return new CChangeDestPathsCmd( this, destPaths, _T("Paste destination file paths from clipboard") );
}

bool CFileModel::PromptExtensionChanges( const std::vector< fs::CPath >& destPaths ) const
{
	ASSERT( m_renameItems.size() == destPaths.size() );

	size_t extChangeCount = 0;

	for ( size_t i = 0; i != m_renameItems.size(); ++i )
		if ( CDisplayFilenameAdapter::IsExtensionChange( m_renameItems[ i ]->GetSafeDestPath(), destPaths[ i ] ) )
			++extChangeCount;

	if ( extChangeCount != 0 )
	{
		std::tstring prefix = str::Format( _T("You are about to change the extension on %s files!\n\nDo you want to proceed?"),
			m_renameItems.size() == extChangeCount ? _T("all") : num::FormatNumber( extChangeCount ).c_str() );

		if ( ui::MessageBox( prefix, MB_ICONWARNING | MB_YESNO ) != IDYES )
			return false;
	}

	return true;
}


// TOUCH

bool CFileModel::CopyClipSourceFileStates( CWnd* pWnd ) const
{
	REQUIRE( !m_touchItems.empty() );		// should be initialized

	std::vector< std::tstring > sourcePaths; sourcePaths.reserve( m_sourcePaths.size() );

	for ( std::vector< CTouchItem* >::const_iterator itTouchItem = m_touchItems.begin(); itTouchItem != m_touchItems.end(); ++itTouchItem )
		sourcePaths.push_back( fmt::FormatClipFileState( ( *itTouchItem )->GetSrcState(), fmt::FilenameExt ) );

	return CClipboard::CopyToLines( sourcePaths, pWnd );
}

utl::ICommand* CFileModel::MakeClipPasteDestFileStatesCmd( CWnd* pWnd ) throws_( CRuntimeException )
{
	REQUIRE( !m_touchItems.empty() );		// should be initialized

	std::vector< std::tstring > lines;
	if ( CClipboard::CanPasteText() )
		CClipboard::PasteFromLines( lines, pWnd );

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
	if ( svc::Undo == m_stackType )
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
	if ( svc::Undo == m_stackType )
		std::swap( srcState, destState );			// swap DEST and SRC for undo

	CTouchItem* pTouchItem = new CTouchItem( srcState );
	pTouchItem->RefDestState() = destState;

	m_pFileModel->m_sourcePaths.push_back( pTouchItem->GetFilePath() );
	m_pFileModel->m_touchItems.push_back( pTouchItem );
}

bool CFileModel::HasFileEditor( cmd::CommandType cmdType )
{
	switch ( cmdType )
	{
		case cmd::RenameFile:
		case cmd::ChangeDestPaths:
		case cmd::TouchFile:
		case cmd::FindDuplicates:
			return true;
	}
	return false;
}

IFileEditor* CFileModel::MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent )
{
	switch ( cmdType )
	{	// keep case statements in sync with HasFileEditor()
		case cmd::RenameFile:
		case cmd::ChangeDestPaths:
			return new CRenameFilesDialog( this, pParent );
		case cmd::TouchFile:
			return new CTouchFilesDialog( this, pParent );
		case cmd::FindDuplicates:
			return new CFindDuplicatesDialog( this, pParent );
	}

	// files group commands may be editor-less
	//ASSERT( false );
	return NULL;			// assume is a persistent command
}

std::pair< IFileEditor*, bool > CFileModel::HandleUndoRedo( svc::StackType stackType, CWnd* pParent )
{
	std::pair< IFileEditor*, bool > resultPair( NULL, false );

	if ( utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType ) )
	{
		if ( !ui::IsKeyPressed( VK_CONTROL ) )			// force the dashboard dialog (even on editor-based commands)?
			if ( IFileEditor* pEditor = MakeFileEditor( static_cast< cmd::CommandType >( pTopCmd->GetTypeID() ), pParent ) )
			{
				resultPair.first = pEditor;				// have an editor, not handled yet
				return resultPair;
			}

		if ( cmd::IsPersistentCmd( pTopCmd ) )
		{	// command has no editor in particular
			if ( ui::IsKeyPressed( VK_SHIFT ) )			// skip the dashboard dialog?
				m_pCmdSvc->UndoRedo( stackType );
			else
			{
				CCmdDashboardDialog dlg( this, stackType, pParent );
				dlg.DoModal();
			}
			resultPair.second = true;					// handled
		}
	}

	return resultPair;
}


// command handlers

BEGIN_MESSAGE_MAP( CFileModel, CCmdTarget )
END_MESSAGE_MAP()
