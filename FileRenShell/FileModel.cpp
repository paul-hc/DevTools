
#include "stdafx.h"
#include "FileModel.h"
#include "CommandModelSerializer.h"
#include "FileCommands.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "utl/Clipboard.h"
#include "utl/Command.h"
#include "utl/ContainerUtilities.h"
#include "utl/FmtUtils.h"
#include "utl/RuntimeException.h"
#include "utl/StringRange.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_clipSep[] = _T("\t");

	void ParseTouchDest( CTouchItem* pTouchItem, const str::TStringRange& textRange ) throws_( CRuntimeException )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_clipSep ) )
		{
			fs::CFileState destFileState;
			destFileState.m_fullPath = textRange.ExtractLead( sepPos.m_start );

			if ( path::IsNameExt( destFileState.m_fullPath.GetPtr() ) )
				destFileState.m_fullPath = pTouchItem->GetKeyPath().GetParentPath() / destFileState.m_fullPath;		// convert to full path

			if ( destFileState.m_fullPath != pTouchItem->GetKeyPath() )
				throw CRuntimeException( str::Format( _T("Pasted destination file name is inconsistent with source path.\n\nSource: %s\nDestination: %s"),
					pTouchItem->GetKeyPath().GetPtr(), destFileState.m_fullPath.GetPtr() ) );

			str::TStringRange stateRange = textRange.MakeTrail( sepPos.m_end );
			if ( ParseFileState( destFileState, stateRange ) )
			{
				pTouchItem->RefDestState() = destFileState;
				return;
			}
		}

		throw CRuntimeException( str::Format( _T("Pasted destination file status is not valid: %s"), textRange.GetText().c_str() ) );
	}
}


// CFileModel implementation

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

	std::sort( m_sourcePaths.begin(), m_sourcePaths.end() );
	m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
}

CCommandModel* CFileModel::GetCommandModel( void )
{
	if ( NULL == m_pCommandModel.get() )
		m_pCommandModel.reset( new CCommandModel );

	return m_pCommandModel.get();
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

	if ( const cmd::CFileMacroCmd* pTopMacroCmd = dynamic_cast< const cmd::CFileMacroCmd* >( cmd::Undo == stackType ? m_pCommandModel->PeekUndo() : m_pCommandModel->PeekRedo() ) )
	{
		Clear();

		switch ( pTopMacroCmd->GetTypeID() )
		{
			case cmd::RenameFile:	utl::for_each( pTopMacroCmd->GetSubCommands(), AddRenameItemFromCmd( this, stackType ) ); break;
			case cmd::TouchFile:	utl::for_each( pTopMacroCmd->GetSubCommands(), AddTouchItemFromCmd( this, stackType ) ); break;
		}
		m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );
		//utl::for_each( m_renameItems, func::StripDisplayCode( m_commonParentPath ) );		// use always filename.ext for path diffs
		utl::for_each( m_touchItems, func::StripDisplayCode( m_commonParentPath ) );

		UpdateAllObservers( NULL );				// items changed
	}
	else
		ASSERT( false );
}

bool CFileModel::SaveCommandModel( void ) const
{
	if ( NULL == m_pCommandModel.get() )
		return false;

	const fs::CPath& undoLogPath = GetUndoLogPath();
	std::ofstream output( undoLogPath.GetUtf8().c_str(), std::ios_base::out | std::ios_base::trunc );

	if ( output.is_open() )
	{
		CCommandModelSerializer saver;

		saver.Save( output, *m_pCommandModel );
		output.close();
	}
	if ( output.fail() )
	{
		TRACE( _T(" * CFileModel::SaveCommandModel(): error saving undo changes log file: %s\n"), undoLogPath.GetPtr() );
		ASSERT( false );
		return false;
	}
	return true;
}

bool CFileModel::LoadCommandModel( void )
{
	if ( m_pCommandModel.get() != NULL )
		return false;				// loaded once

	const fs::CPath& undoLogPath = GetUndoLogPath();
	std::ifstream input( undoLogPath.GetUtf8().c_str() );

	if ( !input.is_open() )
		return false;				// undo log file doesn't exist

	CCommandModelSerializer loader;

	loader.Load( input, GetCommandModel() );
	input.close();
	return true;
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

const fs::CPath& CFileModel::GetUndoLogPath( void )
{
	static fs::CPath undoLogPath;
	if ( undoLogPath.IsEmpty() )
	{
		TCHAR fullPath[ _MAX_PATH ];
		::GetModuleFileName( AfxGetApp()->m_hInstance, fullPath, COUNT_OF( fullPath ) );

		fs::CPathParts parts( fullPath );
		parts.m_fname += _T("_undo");
		parts.m_ext = _T(".log");
		undoLogPath.Set( parts.MakePath() );
	}

	return undoLogPath;
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

	std::tstring multiPath = str::Join( sourcePaths, _T("\r\n") );
	return CClipboard::CopyText( multiPath, pWnd );
}

void CFileModel::PasteClipDestinationPaths( CWnd* pWnd ) throws_( CRuntimeException )
{
	REQUIRE( !m_renameItems.empty() );		// should be initialized

	static const CRuntimeException s_noDestExc( _T("No destination file paths available to paste.") );

	std::tstring text;
	if ( !CClipboard::CanPasteText() || !CClipboard::PasteText( text, pWnd ) )
		throw s_noDestExc;

	std::vector< std::tstring > destPaths;
	str::Split( destPaths, text.c_str(), _T("\r\n") );

	for ( std::vector< std::tstring >::iterator itPath = destPaths.begin(); itPath != destPaths.end(); )
	{
		str::Trim( *itPath );
		if ( itPath->empty() )
			itPath = destPaths.erase( itPath );
		else
			++itPath;
	}

	if ( destPaths.empty() )
		throw s_noDestExc;
	else if ( m_sourcePaths.size() != destPaths.size() )
		throw CRuntimeException( str::Format( _T("Destination file count of %d doesn't match source file count of %d."),
											  destPaths.size(), m_sourcePaths.size() ) );

	size_t pos = 0;
	for ( std::vector< CRenameItem* >::iterator itItem = m_renameItems.begin(); itItem != m_renameItems.end(); ++itItem, ++pos )
	{
		fs::CPath newFilePath( destPaths[ pos ] );

		( *itItem )->GetSrcPath().QualifyWithSameDirPathIfEmpty( newFilePath );
		( *itItem )->RefDestPath() = newFilePath;
	}

	UpdateAllObservers( NULL );			// rename items changed
}


// TOUCH

bool CFileModel::CopyClipSourceFileStates( CWnd* pWnd ) const
{
	REQUIRE( !m_touchItems.empty() );		// should be initialized

	std::vector< std::tstring > sourcePaths; sourcePaths.reserve( m_sourcePaths.size() );

	for ( std::vector< CTouchItem* >::const_iterator itTouchItem = m_touchItems.begin(); itTouchItem != m_touchItems.end(); ++itTouchItem )
		sourcePaths.push_back( ( *itTouchItem )->GetDisplayCode() );

	std::tstring text = str::Join( sourcePaths, _T("\r\n") );
	return CClipboard::CopyText( text, pWnd );
}

void CFileModel::PasteClipDestinationFileStates( CWnd* pWnd ) throws_( CRuntimeException )
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

	int pos = 0;
	for ( std::vector< CTouchItem* >::iterator itTouchItem = m_touchItems.begin(); itTouchItem != m_touchItems.end(); ++itTouchItem, ++pos )
		fmt::ParseTouchDest( *itTouchItem, str::TStringRange( lines[ pos ] ) );

	UpdateAllObservers( NULL );			// touch items changed
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

	m_pFileModel->m_sourcePaths.push_back( pRenameItem->GetKeyPath() );
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

	m_pFileModel->m_sourcePaths.push_back( pTouchItem->GetKeyPath() );
	m_pFileModel->m_touchItems.push_back( pTouchItem );
}


#include "MainRenameDialog.h"
#include "TouchFilesDialog.h"

IFileEditor* CFileModel::MakeFileEditor( cmd::CommandType cmdType, CWnd* pParent )
{
	switch ( cmdType )
	{
		case cmd::RenameFile:	return new CMainRenameDialog( this, pParent );
		case cmd::TouchFile:	return new CTouchFilesDialog( this, pParent );
	}
	ASSERT( false );
	return NULL;
}