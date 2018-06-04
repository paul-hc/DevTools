
#include "stdafx.h"
#include "FileModel.h"
#include "UndoLogSerializer.h"
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
	if ( m_pCommandModel.get() != NULL )		// undo log loaded?
		SaveUndoLog();

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

bool CFileModel::CanUndo( cmd::Command cmdType ) const
{
	if ( utl::ICommand* pTopCmd = PeekUndoCmdAs< utl::ICommand >() )
		if ( cmdType == pTopCmd->GetTypeID() )
			return m_pCommandModel->CanUndo();

	return false;
}

void CFileModel::PopUndo( void )
{
	// fills the data set from undo stack
	ASSERT_PTR( m_pCommandModel.get() );
	ASSERT( m_pCommandModel->CanUndo() );

	if ( const cmd::CFileMacroCmd* pTopMacroCmd = dynamic_cast< const cmd::CFileMacroCmd* >( m_pCommandModel->PeekUndo() ) )
	{
		Clear();

		switch ( pTopMacroCmd->GetTypeID() )
		{
			case cmd::RenameFile:	std::for_each( pTopMacroCmd->GetSubCommands().begin(), pTopMacroCmd->GetSubCommands().end(), AddRenameItemFromCmd( this ) ); break;
			case cmd::TouchFile:	std::for_each( pTopMacroCmd->GetSubCommands().begin(), pTopMacroCmd->GetSubCommands().end(), AddTouchItemFromCmd( this ) ); break;
		}
		m_commonParentPath = path::ExtractCommonParentPath( m_sourcePaths );

		UpdateAllObservers( NULL );				// items changed
	}
	else
		ASSERT( false );
}

bool CFileModel::SaveUndoLog( void ) const
{
	if ( NULL == m_pCommandModel.get() )
		return false;

	const fs::CPath& undoLogPath = GetUndoLogPath();
	std::ofstream output( undoLogPath.GetUtf8().c_str(), std::ios_base::out | std::ios_base::trunc );

	if ( output.is_open() )
	{
		CUndoLogSerializer saver( &m_pCommandModel->RefUndoStack() );

		saver.Save( output );
		output.close();
	}
	if ( output.fail() )
	{
		TRACE( _T(" * CFileModel::SaveUndoLog(): error saving undo changes log file: %s\n"), undoLogPath.GetPtr() );
		ASSERT( false );
		return false;
	}
	return true;
}

bool CFileModel::LoadUndoLog( void )
{
	const fs::CPath& undoLogPath = GetUndoLogPath();
	std::ifstream input( undoLogPath.GetUtf8().c_str() );

	if ( !input.is_open() )
		return false;				// undo log file doesn't exist

	CUndoLogSerializer loader( &GetCommandModel()->RefUndoStack() );

	loader.Load( input );
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

			pNewItem->StripDisplayCode( m_commonParentPath );
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
	std::for_each( m_renameItems.begin(), m_renameItems.end(), func::ResetItem() );
	std::for_each( m_touchItems.begin(), m_touchItems.end(), func::ResetItem() );

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

	int pos = 0;

	for ( std::vector< CRenameItem* >::iterator itItem = m_renameItems.begin(); itItem != m_renameItems.end(); ++itItem )
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

	// swap DEST and SRC with undo semantics
	CRenameItem* pRenameItem = new CRenameItem( pRenameCmd->m_destPath );
	pRenameItem->RefDestPath() = pRenameCmd->m_srcPath;

	m_pFileModel->m_renameItems.push_back( pRenameItem );
	m_pFileModel->m_sourcePaths.push_back( pRenameItem->GetKeyPath() );
}


// CFileModel::AddTouchItemFromCmd implementation

void CFileModel::AddTouchItemFromCmd::operator()( const utl::ICommand* pCmd )
{
	const CTouchFileCmd* pTouchCmd = checked_static_cast< const CTouchFileCmd* >( pCmd );

	// swap DEST and SRC with undo semantics
	CTouchItem* pTouchItem = new CTouchItem( pTouchCmd->m_destState );
	pTouchItem->RefDestState() = pTouchCmd->m_srcState;

	m_pFileModel->m_touchItems.push_back( pTouchItem );
	m_pFileModel->m_sourcePaths.push_back( pTouchItem->GetKeyPath() );
}
