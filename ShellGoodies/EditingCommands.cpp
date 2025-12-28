
#include "pch.h"
#include "EditingCommands.h"
#include "FileModel.h"
#include "RenameItem.h"
#include "TouchItem.h"
#include "utl/Algorithms_fwd.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/SelectionData.hxx"
#include "EditingCommands.hxx"


// CBaseChangeDestCmd implementation

CBaseChangeDestCmd::CBaseChangeDestCmd( cmd::CommandType cmdType, CFileModel* pFileModel, const std::tstring& cmdTag )
	: CCommand( cmdType, pFileModel, &cmd::GetTags_CommandType() )
	, m_cmdTag( cmdTag )
	, m_pFileModel( pFileModel )
	, m_hasOldDests( false )
{
	SetSubject( m_pFileModel );
}

std::tstring CBaseChangeDestCmd::Format( utl::Verbosity verbosity ) const override
{
	if ( !m_cmdTag.empty() )
		return m_cmdTag;

	return __super::Format( verbosity );
}

bool CBaseChangeDestCmd::Execute( void ) override
{
	return ToggleExecute();
}

bool CBaseChangeDestCmd::Unexecute( void ) override
{
	// Since Execute() toggles between m_destPaths and OldDestPaths, calling the second time has the effect of Unexecute().
	// This assumes that this command is always executed through the command model for UNDO/REDO.
	REQUIRE( m_hasOldDests );
	return ToggleExecute();
}

bool CBaseChangeDestCmd::IsUndoable( void ) const override
{
	return Changed == EvalChange();
}


// CChangeDestPathsCmd implementation

CChangeDestPathsCmd::CChangeDestPathsCmd( CFileModel* pFileModel, std::vector<fs::CPath>& rNewDestPaths, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestPaths, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetRenameItems().size() == rNewDestPaths.size() );

	m_srcPaths.reserve( m_pFileModel->GetRenameItems().size() );
	for ( std::vector<CRenameItem*>::const_iterator itItem = m_pFileModel->GetRenameItems().begin(); itItem != m_pFileModel->GetRenameItems().end(); ++itItem )
		m_srcPaths.push_back( (*itItem)->GetSrcPath() );

	m_destPaths.swap( rNewDestPaths );
	ENSURE( m_srcPaths.size() == m_destPaths.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestPathsCmd::EvalChange( void ) const override
{
	REQUIRE( m_srcPaths.size() == m_destPaths.size() );

	if ( m_destPaths.size() != m_pFileModel->GetRenameItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
	{
		const CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[i];

		if ( pRenameItem->GetFilePath() != m_srcPaths[i] )					// keys different?
			return Expired;
		else if ( pRenameItem->GetDestPath().Get() != m_destPaths[i].Get() )	// case sensitive string compare
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestPathsCmd::ToggleExecute( void ) override
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetRenameItems().size(); ++i )
			{
				CRenameItem* pRenameItem = m_pFileModel->GetRenameItems()[i];

				std::swap( pRenameItem->RefDestPath(), m_destPaths[i] );		// from now on m_destPaths stores OldDestPaths
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

void CChangeDestPathsCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const
{
	ASSERT( m_srcPaths.size() == m_destPaths.size() );

	rLines.clear();
	rLines.reserve( m_srcPaths.size() );

	for ( size_t i = 0; i != m_srcPaths.size(); ++i )
		rLines.push_back( fmt::FormatRenameEntry( m_srcPaths[i], m_destPaths[i] ) );
}


// CChangeDestFileStatesCmd implementation

CChangeDestFileStatesCmd::CChangeDestFileStatesCmd( CFileModel* pFileModel, std::vector<fs::CFileState>& rNewDestStates, const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestFileStates, pFileModel, cmdTag )
{
	REQUIRE( !m_pFileModel->GetTouchItems().empty() );		// should be initialized
	REQUIRE( m_pFileModel->GetTouchItems().size() == rNewDestStates.size() );

	m_srcStates.reserve( m_pFileModel->GetTouchItems().size() );
	for ( std::vector<CTouchItem*>::const_iterator itItem = m_pFileModel->GetTouchItems().begin(); itItem != m_pFileModel->GetTouchItems().end(); ++itItem )
		m_srcStates.push_back( ( *itItem )->GetSrcState() );

	m_destStates.swap( rNewDestStates );
	ENSURE( m_srcStates.size() == m_destStates.size() );
}

CBaseChangeDestCmd::ChangeType CChangeDestFileStatesCmd::EvalChange( void ) const override
{
	REQUIRE( m_srcStates.size() == m_destStates.size() );

	if ( m_destStates.size() != m_pFileModel->GetTouchItems().size() )
		return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
	{
		const CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[i];

		if ( pTouchItem->GetFilePath() != m_srcStates[i].m_fullPath )			// keys different?
			return Expired;
		else if ( pTouchItem->GetDestState() != m_destStates[i] )
			changeType = Changed;
	}

	return changeType;
}

bool CChangeDestFileStatesCmd::ToggleExecute( void ) override
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_pFileModel->GetTouchItems().size(); ++i )
			{
				CTouchItem* pTouchItem = m_pFileModel->GetTouchItems()[i];

				std::swap( pTouchItem->RefDestState(), m_destStates[i] );		// from now on m_destStates stores OldDestStates
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

size_t CChangeDestFileStatesCmd::GetFileCount( void ) const
{
	return m_srcStates.size();
}

void CChangeDestFileStatesCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const
{
	ASSERT( m_srcStates.size() == m_destStates.size() );

	rLines.clear();
	rLines.reserve( m_srcStates.size() );

	for ( size_t i = 0; i != m_srcStates.size(); ++i )
		rLines.push_back( fmt::FormatTouchEntry( m_srcStates[i], m_destStates[i] ) );
}


// CResetDestinationsCmd implementation

CResetDestinationsCmd::CResetDestinationsCmd( CFileModel* pFileModel )
	: CMacroCommand( _T("Reset"), cmd::ResetDestinations )
{
	if ( !pFileModel->GetRenameItems().empty() )		// lazy initialized?
	{
		std::vector<fs::CPath> destPaths; destPaths.reserve( pFileModel->GetRenameItems().size() );
		for ( std::vector<CRenameItem*>::const_iterator itRenameItem = pFileModel->GetRenameItems().begin(); itRenameItem != pFileModel->GetRenameItems().end(); ++itRenameItem )
			destPaths.push_back( ( *itRenameItem )->GetSrcPath() );		// DEST = SRC

		AddCmd( new CChangeDestPathsCmd( pFileModel, destPaths, m_userInfo ) );
	}

	if ( !pFileModel->GetTouchItems().empty() )			// lazy initialized?
	{
		std::vector<fs::CFileState> destStates; destStates.reserve( pFileModel->GetTouchItems().size() );
		for ( std::vector<CTouchItem*>::const_iterator itTouchItem = pFileModel->GetTouchItems().begin(); itTouchItem != pFileModel->GetTouchItems().end(); ++itTouchItem )
			destStates.push_back( ( *itTouchItem )->GetSrcState() );

		AddCmd( new CChangeDestFileStatesCmd( pFileModel, destStates, m_userInfo ) );
	}
}

std::tstring CResetDestinationsCmd::Format( utl::Verbosity verbosity ) const override
{
	return GetSubCommands().front()->Format( verbosity );
}


// selected items subset commands:

// CChangeSelDestPathsCmd implementation

CChangeSelDestPathsCmd::CChangeSelDestPathsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>& selItems, const std::vector<fs::CPath>& newDestPaths,
												const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeSelDestPaths, pFileModel, cmdTag )
	, m_selItems( selItems )
	, m_destPaths( newDestPaths )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized
	REQUIRE( m_selItems.size() == m_destPaths.size() );
}

CBaseChangeDestCmd::ChangeType CChangeSelDestPathsCmd::EvalChange( void ) const override
{
	REQUIRE( m_selItems.size() == m_destPaths.size() );

	for ( size_t i = 0; i != m_selItems.size(); ++i )
	{
		const CRenameItem* pRenameItem = m_selItems[i];

		if ( !utl::Contains( m_pFileModel->GetRenameItems(), pRenameItem ) )
			return Expired;

		if ( pRenameItem->GetDestPath().Get() != m_destPaths[i].Get() )		// case sensitive string compare
			return Changed;
	}

	return Unchanged;
}

bool CChangeSelDestPathsCmd::ToggleExecute( void ) override
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_selItems.size(); ++i )
			{
				CRenameItem* pRenameItem = m_selItems[i];

				std::swap( pRenameItem->RefDestPath(), m_destPaths[i] );	// from now on m_destPaths stores OldDestPaths
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

void CChangeSelDestPathsCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const
{
	ASSERT( m_selItems.size() == m_destPaths.size() );

	rLines.clear();
	rLines.reserve( m_selItems.size() );

	for ( size_t i = 0; i != m_selItems.size(); ++i )
		rLines.push_back( fmt::FormatRenameEntry( m_selItems[i]->GetSrcPath(), m_destPaths[i] ) );
}


// CResetSelDestPathsCmd implementation

CResetSelDestPathsCmd::CResetSelDestPathsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>& selItems )
	: CChangeSelDestPathsCmd( pFileModel, selItems, MakeResetDestPaths( selItems ), str::Format( _T("Rollback %d selected items to original"), selItems.size() ) )
{
}

std::vector<fs::CPath> CResetSelDestPathsCmd::MakeResetDestPaths( const std::vector<CRenameItem*>& selItems )
{
	std::vector<fs::CPath> newDestPaths;

	utl::transform( selItems, newDestPaths, func::AsSrcPath() );
	return newDestPaths;
}


// CChangeSelDestFileStatesCmd implementation

CChangeSelDestFileStatesCmd::CChangeSelDestFileStatesCmd( CFileModel* pFileModel, const std::vector<CTouchItem*>& selItems, const std::vector<fs::CFileState>& newDestStates,
														  const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeSelDestFileStates, pFileModel, cmdTag )
	, m_selItems( selItems )
	, m_destStates( newDestStates )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized
	REQUIRE( m_selItems.size() == newDestStates.size() );
}

CBaseChangeDestCmd::ChangeType CChangeSelDestFileStatesCmd::EvalChange( void ) const override
{
	REQUIRE( m_selItems.size() == m_destStates.size() );

	for ( size_t i = 0; i != m_selItems.size(); ++i )
	{
		const CTouchItem* pTouchItem = m_selItems[i];

		if ( !utl::Contains( m_pFileModel->GetTouchItems(), pTouchItem ) )
			return Expired;

		if ( pTouchItem->GetDestState() != m_destStates[i] )
			return Changed;
	}

	return Unchanged;
}

bool CChangeSelDestFileStatesCmd::ToggleExecute( void ) override
{
	ChangeType changeType = EvalChange();
	switch ( changeType )
	{
		case Changed:
			for ( size_t i = 0; i != m_selItems.size(); ++i )
			{
				CTouchItem* pTouchItem = m_selItems[i];

				std::swap( pTouchItem->RefDestState(), m_destStates[i] );		// from now on m_destStates stores OldDestStates
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

void CChangeSelDestFileStatesCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const
{
	ASSERT( m_selItems.size() == m_destStates.size() );

	rLines.clear();
	rLines.reserve( m_selItems.size() );

	for ( size_t i = 0; i != m_selItems.size(); ++i )
		rLines.push_back( fmt::FormatTouchEntry( m_selItems[i]->GetSrcState(), m_destStates[i] ) );
}


// CResetSelDestFileStatesCmd implementation

CResetSelDestFileStatesCmd::CResetSelDestFileStatesCmd( CFileModel* pFileModel, const std::vector<CTouchItem*>& selItems )
	: CChangeSelDestFileStatesCmd( pFileModel, selItems, MakeResetDestStates( selItems ), str::Format( _T("Rollback %d selected items to original"), selItems.size() ) )
{
}

std::vector<fs::CFileState> CResetSelDestFileStatesCmd::MakeResetDestStates( const std::vector<CTouchItem*>& selItems )
{
	std::vector<fs::CFileState> newDestStates;

	utl::transform( selItems, newDestStates, func::AsSrcState() );
	return newDestStates;
}


#include "utl/UI/ReportListControl.hxx"


// CSortRenameListCmd implementation

CSortRenameListCmd::CSortRenameListCmd( CFileModel* pFileModel, CReportListControl* pFileListCtrl, const ren::TSortingPair& sorting )
	: CBaseRenamePageObjectCommand<CFileModel>( cmd::SortRenameList, pFileModel, pFileListCtrl != nullptr ? checked_static_cast<CBaseRenamePage*>( pFileListCtrl->GetParent() ) : nullptr )
	, m_pFileListCtrl( pFileListCtrl )
	, m_sorting( sorting )
{
}

bool CSortRenameListCmd::DoExecute( void ) override
{
	if ( m_pFileListCtrl != nullptr )		// invoked by sorted list?
	{	// fetch new rename items order
		std::vector<CRenameItem*> renameItems;
		m_pFileListCtrl->QueryObjectsSequence( renameItems );

		m_pObject->SwapRenameSequence( renameItems, m_sorting );	// m_pObject: CFileModel*
	}
	else
		m_pObject->SetRenameSorting( m_sorting );

	return true;
}


// COnRenameListSelChangedCmd implementation

COnRenameListSelChangedCmd::COnRenameListSelChangedCmd( CBaseRenamePage* pPage, const ui::CSelectionData<CRenameItem>& selData )
	: CBaseRenamePageObjectCommand<CLayoutBasePropertySheet>( cmd::OnRenameListSelChanged, pPage->GetParentSheet(), pPage )
	, m_selData( selData )
{
}

COnRenameListSelChangedCmd::COnRenameListSelChangedCmd( CLayoutBasePropertySheet* pChildSheet, const ui::CSelectionData<CRenameItem>& selData )
	: CBaseRenamePageObjectCommand<CLayoutBasePropertySheet>( cmd::OnRenameListSelChanged, pChildSheet, nullptr )
	, m_selData( selData )
{
}

bool COnRenameListSelChangedCmd::DoExecute( void ) override
{
#if defined( _DEBUG ) && 1
	static int count = 0;
	TRACE_( _T("# [%d] COnRenameListSelChangedCmd::DoExecute('%s')\n\tSelection change: caret='%s' selected=%s\n"), ++count,
			GetUiTypeName( GetPage() ).c_str(), dbg::GetSafeFileName( m_selData.GetCaretItem() ).c_str(),
			dbg::FormatFileNames( m_selData.GetSelItems() ).c_str() );
#endif

	return true;
}

std::tstring COnRenameListSelChangedCmd::GetUiTypeName( const CBaseRenamePage* pPage ) const
{
	return str::GetTypeName( pPage != nullptr ? typeid( *pPage ) : typeid( *GetSubject() ) );
}

void COnRenameListSelChangedCmd::dbgTraceSelData( const CBaseRenamePage* pPage ) const
{
	TRACE_( _T(" > OnUpdate() in %s: caret='%s' selected=%s\n"),
			GetUiTypeName( pPage ).c_str(), dbg::GetSafeFileName( m_selData.GetCaretItem() ).c_str(),
			dbg::FormatFileNames( m_selData.GetSelItems() ).c_str() );
}
