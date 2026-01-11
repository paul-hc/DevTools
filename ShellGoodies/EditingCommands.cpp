
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

CChangeDestPathsCmd::CChangeDestPathsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>* pSelItems, const std::vector<fs::CPath>& newDestPaths,
										  const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestPaths, pFileModel, cmdTag )
	, m_destPaths( newDestPaths )
{
	REQUIRE( !m_pFileModel->GetRenameItems().empty() );		// should be initialized

	if ( nullptr == pSelItems )
		pSelItems = &m_pFileModel->GetRenameItems();				// all items constructor (implicit)
	else if ( pSelItems->size() == m_pFileModel->GetRenameItems().size() )
		REQUIRE( *pSelItems == m_pFileModel->GetRenameItems() );	// all items constructor (explicit)
	else
	{	// selected items constructor
		REQUIRE( pSelItems->size() < m_pFileModel->GetRenameItems().size() );
		REQUIRE( utl::ContainsSubSet( m_pFileModel->GetRenameItems(), *pSelItems ) );
	}
	REQUIRE( !pSelItems->empty() && pSelItems->size() == newDestPaths.size() );

	utl::transform( *pSelItems, m_srcPaths, func::AsSrcPath() );

	ENSURE( m_srcPaths.size() == m_destPaths.size() );
}

CChangeDestPathsCmd::~CChangeDestPathsCmd()
{
}

CChangeDestPathsCmd* CChangeDestPathsCmd::MakeResetItemsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>& selItems )
{
	std::vector<fs::CPath> newDestPaths;

	utl::transform( selItems, newDestPaths, func::AsSrcPath() );
	return new CChangeDestPathsCmd( pFileModel, &selItems, newDestPaths, cmd::FormatResetItemsTag( selItems.size(), pFileModel->GetRenameItems().size() ) );
}

bool CChangeDestPathsCmd::HasSelItems( void ) const override
{
	return m_srcPaths.size() < m_pFileModel->GetRenameItems().size();
}

std::vector<CRenameItem*> CChangeDestPathsCmd::MakeSelItems( void ) const
{
	REQUIRE( HasSelItems() );

	std::vector<CRenameItem*> selItems;

	utl::transform( m_srcPaths, selItems, [this]( const fs::CPath& srcPath ) { return m_pFileModel->FindRenameItem( srcPath ); } );
	return selItems;
}

CBaseChangeDestCmd::ChangeType CChangeDestPathsCmd::EvalChange( void ) const override
{
	REQUIRE( m_srcPaths.size() == m_destPaths.size() );

	if ( !HasSelItems() )
		if ( m_destPaths.size() != m_pFileModel->GetRenameItems().size() )
			return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t pos = 0; pos != m_srcPaths.size(); ++pos )
	{
		const CRenameItem* pRenameItem = GetRenameItemAt( pos );

		if ( pRenameItem->GetFilePath() != m_srcPaths[pos] )					// keys different?
			return Expired;
		else if ( pRenameItem->GetDestPath().Get() != m_destPaths[pos].Get() )	// case sensitive string compare
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
			for ( size_t pos = 0; pos != m_srcPaths.size(); ++pos )
			{
				CRenameItem* pRenameItem = GetRenameItemAt( pos );

				std::swap( pRenameItem->RefDestPath(), m_destPaths[pos] );		// from now on, m_destPaths stores OldDestPaths
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

void CChangeDestPathsCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const override
{
	ASSERT( m_srcPaths.size() == m_destPaths.size() );

	rLines.clear();
	rLines.reserve( m_srcPaths.size() );

	for ( size_t i = 0; i != m_srcPaths.size(); ++i )
		rLines.push_back( fmt::FormatRenameEntry( m_srcPaths[i], m_destPaths[i] ) );
}

CRenameItem* CChangeDestPathsCmd::GetRenameItemAt( size_t posSrcPath ) const
{
	REQUIRE( posSrcPath < m_srcPaths.size() );

	if ( HasSelItems() )
		return m_pFileModel->FindRenameItem( m_srcPaths[posSrcPath] );

	return m_pFileModel->GetRenameItems()[ posSrcPath ];
}


// CChangeDestFileStatesCmd implementation

CChangeDestFileStatesCmd::CChangeDestFileStatesCmd( CFileModel* pFileModel, const std::vector<CTouchItem*>* pSelItems, const std::vector<fs::CFileState>& newDestStates,
													const std::tstring& cmdTag /*= std::tstring()*/ )
	: CBaseChangeDestCmd( cmd::ChangeDestFileStates, pFileModel, cmdTag )
	, m_destStates( newDestStates )
{
	REQUIRE( !m_pFileModel->GetTouchItems().empty() );		// should be initialized

	if ( nullptr == pSelItems )
		pSelItems = &m_pFileModel->GetTouchItems();					// all items constructor (implicit)
	else if ( pSelItems->size() == m_pFileModel->GetTouchItems().size() )
		REQUIRE( *pSelItems == m_pFileModel->GetTouchItems() );		// all items constructor (explicit)
	else
	{	// selected files constructor
		REQUIRE( !pSelItems->empty() && pSelItems->size() < m_pFileModel->GetTouchItems().size() );
		REQUIRE( utl::ContainsSubSet( m_pFileModel->GetTouchItems(), *pSelItems ) );
	}
	REQUIRE( !pSelItems->empty() && pSelItems->size() == newDestStates.size() );

	utl::transform( *pSelItems, m_srcStates, func::AsSrcState() );
	ENSURE( m_srcStates.size() == m_destStates.size() );
}

CChangeDestFileStatesCmd* CChangeDestFileStatesCmd::MakeResetItemsCmd( CFileModel* pFileModel, const std::vector<CTouchItem*>& selItems )
{
	std::vector<fs::CFileState> newDestStates;

	utl::transform( selItems, newDestStates, func::AsSrcState() );
	return new CChangeDestFileStatesCmd( pFileModel, &selItems, newDestStates, cmd::FormatResetItemsTag( selItems.size(), pFileModel->GetTouchItems().size() ) );
}

bool CChangeDestFileStatesCmd::HasSelItems( void ) const override
{
	return m_srcStates.size() < m_pFileModel->GetTouchItems().size();
}

std::vector<CTouchItem*> CChangeDestFileStatesCmd::MakeSelItems( void ) const
{
	REQUIRE( HasSelItems() );

	std::vector<CTouchItem*> selItems;

	utl::transform( m_srcStates, selItems, [this]( const fs::CFileState& srcState ) { return m_pFileModel->FindTouchItem( srcState.m_fullPath ); } );
	return selItems;
}

CBaseChangeDestCmd::ChangeType CChangeDestFileStatesCmd::EvalChange( void ) const override
{
	REQUIRE( m_srcStates.size() == m_destStates.size() );

	if ( !HasSelItems() )
		if ( m_destStates.size() != m_pFileModel->GetTouchItems().size() )
			return Expired;

	ChangeType changeType = Unchanged;

	for ( size_t pos = 0; pos != m_srcStates.size(); ++pos )
	{
		const CTouchItem* pTouchItem = GetTouchItemAt( pos );

		if ( pTouchItem->GetFilePath() != m_srcStates[pos].m_fullPath )			// keys different?
			return Expired;
		else if ( pTouchItem->GetDestState() != m_destStates[pos] )
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
			for ( size_t pos = 0; pos != m_srcStates.size(); ++pos )
			{
				CTouchItem* pTouchItem = GetTouchItemAt( pos );

				std::swap( pTouchItem->RefDestState(), m_destStates[pos] );		// from now on m_destStates stores OldDestStates
			}
			break;
		case Expired:
			return false;
	}

	NotifyObservers();
	m_hasOldDests = !m_hasOldDests;				// m_dest..s swapped with OldDest..s
	return true;
}

void CChangeDestFileStatesCmd::QueryDetailLines( std::vector<std::tstring>& rLines ) const
{
	ASSERT( m_srcStates.size() == m_destStates.size() );

	rLines.clear();
	rLines.reserve( m_srcStates.size() );

	for ( size_t i = 0; i != m_srcStates.size(); ++i )
		rLines.push_back( fmt::FormatTouchEntry( m_srcStates[i], m_destStates[i] ) );
}

CTouchItem* CChangeDestFileStatesCmd::GetTouchItemAt( size_t posSrcState ) const
{
	REQUIRE( posSrcState < m_srcStates.size() );

	if ( HasSelItems() )
		return m_pFileModel->FindTouchItem( m_srcStates[ posSrcState ].m_fullPath );

	return m_pFileModel->GetTouchItems()[ posSrcState ];
}


// CResetDestinationsCmd implementation

CResetDestinationsCmd::CResetDestinationsCmd( CFileModel* pFileModel )
	: CMacroCommand( _T("Reset macro"), cmd::ResetDestinations )
{
	if ( !pFileModel->GetRenameItems().empty() )		// lazy initialized?
		AddCmd( CChangeDestPathsCmd::MakeResetItemsCmd( pFileModel, pFileModel->GetRenameItems() ) );

	if ( !pFileModel->GetTouchItems().empty() )			// lazy initialized?
		AddCmd( CChangeDestFileStatesCmd::MakeResetItemsCmd( pFileModel, pFileModel->GetTouchItems() ) );
}

std::tstring CResetDestinationsCmd::Format( utl::Verbosity verbosity ) const override
{
	return GetSubCommands().front()->Format( verbosity );
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
	TRACE( _T("# [%d] COnRenameListSelChangedCmd::DoExecute('%s')\n\tSelection change: caret='%s' selected=%s\n"), ++count,
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
	TRACE( _T(" > OnUpdate() in %s: caret='%s' selected=%s\n"),
		   GetUiTypeName( pPage ).c_str(), dbg::GetSafeFileName( m_selData.GetCaretItem() ).c_str(),
		   dbg::FormatFileNames( m_selData.GetSelItems() ).c_str() );
}
