
#include "pch.h"
#include "FileModel.h"
#include "FileMacroCommands.h"
#include "EditingCommands.h"
#include "GeneralOptions.h"
#include "IFileEditor.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "PathItemSorting.h"
#include "Application.h"
#include "resource.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/Command.h"
#include "utl/FileSystem.h"
#include "utl/FmtUtils.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"
#include "utl/TextClipboard.h"
#include "utl/UI/ObjectCtrlBase.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"

// for CFileModel::MakeFileEditor
#include "RenameFilesDialog.h"
#include "TouchFilesDialog.h"
#include "FindDuplicatesDialog.h"
#include "CmdDashboardDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_targetScope[] = _T("TargetScope");
	static const TCHAR entry_sortBy[] = _T("SortBy");
	static const TCHAR entry_sortAscending[] = _T("SortAscending");
}


const std::tstring CFileModel::section_filesSheet = _T("RenameDialog\\FilesSheet");

CFileModel::CFileModel( svc::ICommandService* pCmdSvc )
	: m_pCmdSvc( pCmdSvc )
	, m_targetScope( app::TargetAllItems )
	, m_renameSorting( ren::RecordDefault, true )
{
	ASSERT_PTR( m_pCmdSvc );

	//m_targetScope = static_cast<app::TargetScope>( AfxGetApp()->GetProfileInt( reg::section_Settings, reg::entry_targetScope, m_targetScope ) );
	m_renameSorting.first = static_cast<ren::SortBy>( AfxGetApp()->GetProfileInt( section_filesSheet.c_str(), reg::entry_sortBy, m_renameSorting.first ) );
	m_renameSorting.second = AfxGetApp()->GetProfileInt( section_filesSheet.c_str(), reg::entry_sortAscending, m_renameSorting.second ) != FALSE;
}

CFileModel::~CFileModel()
{
	RegSave();
	Clear();
}

void CFileModel::Clear( void )
{
	m_sourcePaths.clear();
	m_commonParentPath.Clear();
	m_srcFolderPaths.clear();

	utl::ClearOwningContainer( m_renameItems );
	utl::ClearOwningContainer( m_touchItems );
}

void CFileModel::RegSave( void )
{
	//AfxGetApp()->WriteProfileInt( reg::section_Settings, reg::entry_targetScope, m_targetScope );
	AfxGetApp()->WriteProfileInt( section_filesSheet.c_str(), reg::entry_sortBy, m_renameSorting.first );
	AfxGetApp()->WriteProfileInt( section_filesSheet.c_str(), reg::entry_sortAscending, m_renameSorting.second );
}


size_t CFileModel::SetupFromDropInfo( HDROP hDropInfo )
{
	std::vector<fs::CPath> sourcePaths;
	shell::QueryDroppedFiles( sourcePaths, hDropInfo );

	app::GetApp()->CheckReplaceSourcePaths( sourcePaths, _T("CFileModel::SetupFromDropInfo()") );		// allow advanced debugging
	/*
		- DBG example 1): simulate search results view with files in multiple folders: use test format="fld" to test duplicates (it depends on existing physical files):
			C:\download\#\images\flowers\1981 - Diary of a Madman\cover.jpg
			C:\download\#\images\flowers\1986 - The Ultimate Sin\cover.jpg
		- DBG example 2): test multiple paths:
			C:\dev\DevTools\CommonUtl\DemoUtl\DemoUtl.rc
			C:\dev\DevTools\CommonUtl\utl\utl_ui.rc
			C:\dev\DevTools\ShellGoodies\ShellGoodies.rc
	*/

	StoreSourcePaths( sourcePaths );
	return m_sourcePaths.size();
}

template< typename ContainerT >
void CFileModel::StoreSourcePaths( const ContainerT& sourcePaths )
{
	ASSERT( !sourcePaths.empty() );
	Clear();

	for ( const typename ContainerT::value_type& srcPath: sourcePaths )
		m_sourcePaths.push_back( func::PathOf( srcPath ) );

	fs::SortPathsDirsFirst( m_sourcePaths );

	//TRACE_ITEMS( m_sourcePaths, "CFileModel::m_sourcePaths", 50 );

	m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
	fs::QueryFolderPaths( m_srcFolderPaths, m_sourcePaths );		// folders of drop source files
}

bool CFileModel::IsSourceSingleFolder( void ) const
{
	return 1 == m_sourcePaths.size() && fs::IsValidDirectory( m_sourcePaths.front().GetPtr() );
}

bool CFileModel::SafeExecuteCmd( IFileEditor* pEditor, utl::ICommand* pCmd )
{
	return m_pCmdSvc->SafeExecuteCmd( pCmd, pEditor != nullptr && pEditor->IsRollMode() );
}

void CFileModel::FetchFromStack( svc::StackType stackType )
{
	// fills the data set from undo stack
	ASSERT( m_pCmdSvc->CanUndoRedo( stackType ) );

	utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType );
	if ( const cmd::CFileMacroCmd* pFileMacroCmd = dynamic_cast<const cmd::CFileMacroCmd*>( pTopCmd ) )
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

		UpdateAllObservers( nullptr );				// items changed
	}
	else
		m_pCmdSvc->UndoRedo( stackType );
}

std::vector<CRenameItem*>& CFileModel::LazyInitRenameItems( void )
{
	if ( m_renameItems.empty() )
	{
		ASSERT( !m_sourcePaths.empty() );
		m_renameItems.reserve( m_sourcePaths.size() );

		for ( const fs::CPath& srcPath: m_sourcePaths )
		{
			CRenameItem* pNewItem = new CRenameItem( srcPath );

			//pNewItem->StripDisplayCode( m_commonParentPath );		// use always filename.ext for path diffs
			m_renameItems.push_back( pNewItem );
			m_srcPatchToItemsMap[ srcPath ].first = pNewItem;
		}

		SortRenameItems();		// initial sort
	}

	return m_renameItems;
}

std::vector<CTouchItem*>& CFileModel::LazyInitTouchItems( void )
{
	if ( m_touchItems.empty() )
	{
		ASSERT( !m_sourcePaths.empty() );

		for ( const fs::CPath& srcPath: m_sourcePaths )
		{
			CTouchItem* pNewItem = new CTouchItem( fs::CFileState::ReadFromFile( srcPath ) );

			pNewItem->StripDisplayCode( m_commonParentPath );
			m_touchItems.push_back( pNewItem );
			m_srcPatchToItemsMap[ srcPath ].second = pNewItem;
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
		TSubject::UpdateAllObservers( pMessage );
}

CRenameItem* CFileModel::FindRenameItem( const fs::CPath& srcPath ) const
{
	const TItemPair* pItemsPair = utl::FindValuePtr( m_srcPatchToItemsMap, srcPath );
	return pItemsPair != nullptr ? pItemsPair->first : nullptr;
}

CTouchItem* CFileModel::FindTouchItem( const fs::CPath& srcPath ) const
{
	const TItemPair* pItemsPair = utl::FindValuePtr( m_srcPatchToItemsMap, srcPath );
	return pItemsPair != nullptr ? pItemsPair->second : nullptr;
}

void CFileModel::ResetDestinations( void )
{
	utl::for_each( m_renameItems, func::ResetItem() );
	utl::for_each( m_touchItems, func::ResetItem() );

	UpdateAllObservers( nullptr );				// path items changed
}

std::tstring CFileModel::FormatPath( const fs::CPath& filePath, fmt::PathFormat format, const CDisplayFilenameAdapter* pDisplayAdapter )
{
	return pDisplayAdapter != nullptr
		? pDisplayAdapter->FormatPath( format, filePath )
		: fmt::FormatPath( filePath, format );
}

bool CFileModel::CopyClipSourcePaths( fmt::PathFormat format, CWnd* pWnd ) const
{
	std::vector<std::tstring> srcLines; srcLines.reserve( m_sourcePaths.size() );

	for ( const fs::CPath& srcFilePath: m_sourcePaths )
		srcLines.push_back( fmt::FormatPath( srcFilePath, format ) );

	return CTextClipboard::CopyToLines( srcLines, pWnd->GetSafeHwnd() );
}


// RENAME

bool CFileModel::CopyClipRenameSrcPaths( const std::vector<CRenameItem*>& renameItems, fmt::PathFormat format, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter )
{
	std::vector<std::tstring> srcLines; srcLines.reserve( renameItems.size() );

	for ( const CRenameItem* pRenameItem: renameItems )
		srcLines.push_back( FormatPath( pRenameItem->GetSrcPath(), format, pDisplayAdapter ) );

	return CTextClipboard::CopyToLines( srcLines, pWnd->GetSafeHwnd() );
}

utl::ICommand* CFileModel::MakeClipPasteDestPathsCmd( const std::vector<CRenameItem*>& renameItems, CWnd* pWnd, const CDisplayFilenameAdapter* pDisplayAdapter ) const throws_( CRuntimeException )
{
	static const CRuntimeException s_noDestExc( _T("No destination file paths available to paste.") );

	std::vector<std::tstring> fileNames;

	if ( !CTextClipboard::CanPasteText() || !CTextClipboard::PasteFromLines( fileNames, pWnd->GetSafeHwnd() ) )
		throw s_noDestExc;

	size_t origCount = fileNames.size();
	pred::ValidateFilename validateFunc( CGeneralOptions::Instance().m_trimFname );
	std::pair<size_t, pred::ValidateFilename> resultPair = utl::RemoveIfPred( fileNames, validateFunc, utl::NegatePred );

	if ( resultPair.first != 0 )
	{
		TRACE_( "* CFileModel::MakeClipPasteDestPathsCmd(): ignoring %d invalid path out of %d paths!\n", resultPair.first, origCount ); origCount;
		ui::BeepSignal( MB_ICONERROR );
	}
	else if ( resultPair.second.m_amendedCount != 0 )
		ui::BeepSignal( MB_ICONWARNING );

	if ( fileNames.empty() )
		throw s_noDestExc;
	else if ( renameItems.size() != fileNames.size() )
		throw CRuntimeException( str::Format( _T("Destination file count of %d doesn't match source file count of %d."), fileNames.size(), renameItems.size() ) );

	std::vector<fs::CPath> destPaths; destPaths.reserve( renameItems.size() );
	bool anyChange = false;

	for ( size_t i = 0; i != fileNames.size(); ++i )
	{
		const CRenameItem* pRenameItem = renameItems[i];
		fs::CPath newFilePath = pDisplayAdapter->ParsePath( fileNames[i], pRenameItem->GetSafeDestPath() );

		destPaths.push_back( newFilePath );
		anyChange |= newFilePath.Get() != pRenameItem->GetDestPath().Get();		// case-sensitive string compare
	}

	if ( !anyChange )
		return nullptr;

	if ( !PromptExtensionChanges( renameItems, destPaths ) )
		return nullptr;

	return new CChangeDestPathsCmd( const_cast<CFileModel*>( this ), &renameItems, destPaths, _T("Paste destination file paths from clipboard") );
}

bool CFileModel::PromptExtensionChanges( const std::vector<CRenameItem*>& renameItems, const std::vector<fs::CPath>& destPaths ) const
{
	ASSERT( renameItems.size() == destPaths.size() );

	size_t extChangeCount = 0;

	for ( size_t i = 0; i != renameItems.size(); ++i )
		if ( CDisplayFilenameAdapter::IsExtensionChange( renameItems[ i ]->GetSafeDestPath(), destPaths[ i ] ) )
			++extChangeCount;

	if ( extChangeCount != 0 )
	{
		std::tstring prefix = str::Format( _T("You are about to change the extension on %s files!\n\nDo you want to proceed?"),
										   renameItems.size() == extChangeCount ? _T("all") : num::FormatNumber( extChangeCount ).c_str() );

		if ( ui::MessageBox( prefix, MB_ICONWARNING | MB_YESNO ) != IDYES )
			return false;
	}

	return true;
}

void CFileModel::SetRenameSorting( const ren::TSortingPair& renameSorting )
{
	m_renameSorting = renameSorting;
	RegSave();

	SortRenameItems();
}

void CFileModel::SortRenameItems( void )
{
	std::sort( m_renameItems.begin(), m_renameItems.end(), pred::MakeLessPtr( pred::CompareRenameItem( m_renameSorting ) ) );
}

void CFileModel::SwapRenameSequence( std::vector<CRenameItem*>& rListSequence, const ren::TSortingPair& renameSorting )
{
	// we do no sorting here - rListSequence contains the list items sorted according to current sort criteria in the list-ctrl
	REQUIRE( m_renameItems.size() == rListSequence.size() );		// is list consistent in size?
	m_renameItems.swap( rListSequence );			// input the sorted items
	m_renameSorting = renameSorting;
}


// TOUCH

bool CFileModel::CopyClipSourceFileStates( CWnd* pWnd, const std::vector<CTouchItem*>& touchItems )
{
	REQUIRE( !touchItems.empty() );		// should be initialized

	std::vector<std::tstring> srcLines; srcLines.reserve( touchItems.size() );

	for ( const CTouchItem* pTouchItem: touchItems )
		srcLines.push_back( fmt::FormatClipFileState( pTouchItem->GetSrcState(), fmt::FilenameExt ) );

	return CTextClipboard::CopyToLines( srcLines, pWnd->GetSafeHwnd() );
}

utl::ICommand* CFileModel::MakeClipPasteDestFileStatesCmd( const std::vector<CTouchItem*>& touchItems, CWnd* pWnd ) throws_( CRuntimeException )
{
	REQUIRE( !touchItems.empty() );

	std::vector<std::tstring> lines;

	if ( CTextClipboard::CanPasteText() )
		CTextClipboard::PasteFromLines( lines, pWnd->GetSafeHwnd() );

	for ( std::vector<std::tstring>::iterator itLine = lines.begin(); itLine != lines.end(); )
	{
		if ( CGeneralOptions::Instance().m_trimFname )
			str::Trim( *itLine );

		if ( itLine->empty() )
			itLine = lines.erase( itLine );
		else
			++itLine;
	}

	if ( lines.empty() )
		throw CRuntimeException( _T("No destination file states available to paste.") );
	else if ( touchItems.size() != lines.size() )
		throw CRuntimeException( str::Format( _T("Destination file state count of %d doesn't match source file count of %d."),
											  lines.size(), touchItems.size() ) );

	std::vector<fs::CFileState> destFileStates; destFileStates.reserve( touchItems.size() );
	bool anyChange = false;

	for ( size_t i = 0; i != touchItems.size(); ++i )
	{
		const CTouchItem* pTouchItem = touchItems[i];

		fs::CFileState newFileState;
		fmt::ParseClipFileState( newFileState, lines[i], &pTouchItem->GetFilePath() );

		destFileStates.push_back( newFileState );
		anyChange |= newFileState != pTouchItem->GetDestState();
	}

	return anyChange ? new CChangeDestFileStatesCmd( this, &touchItems, destFileStates, _T("Paste destination file attribute states from clipboard") ) : nullptr;
}


// CFileModel::AddRenameItemFromCmd implementation

void CFileModel::AddRenameItemFromCmd::operator()( const utl::ICommand* pCmd )
{
	const CRenameFileCmd* pRenameCmd = checked_static_cast<const CRenameFileCmd*>( pCmd );

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
	const CTouchFileCmd* pTouchCmd = checked_static_cast<const CTouchFileCmd*>( pCmd );

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
	return nullptr;			// assume is a persistent command
}

std::pair<IFileEditor*, bool> CFileModel::HandleUndoRedo( svc::StackType stackType, CWnd* pParent )
{
	std::pair<IFileEditor*, bool> resultPair( nullptr, false );

	if ( utl::ICommand* pTopCmd = m_pCmdSvc->PeekCmd( stackType ) )
	{
		if ( !ui::IsKeyPressed( VK_CONTROL ) )			// force the dashboard dialog (even on editor-based commands)?
			if ( IFileEditor* pEditor = MakeFileEditor( static_cast<cmd::CommandType>( pTopCmd->GetTypeID() ), pParent ) )
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
