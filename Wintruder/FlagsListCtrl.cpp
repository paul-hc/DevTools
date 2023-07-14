
#include "stdafx.h"
#include "FlagsListCtrl.h"
#include "Application.h"
#include "AppService.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/UI/CtrlUiState.h"
#include "utl/UI/CheckStatePolicies.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/WndUtilsEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/UI/ReportListControl.hxx"


namespace pred
{
	struct IsRadioGroup
	{
		bool operator()( const CFlagGroup* pGroup ) const
		{
			return pGroup != NULL && pGroup->IsValueGroup();
		}
	};
}


CFlagsListCtrl::CFlagsListCtrl( void )
	: CReportListControl( IDC_FLAGS_EDITOR_LIST, LVS_EX_CHECKBOXES | LVS_EX_UNDERLINEHOT | lv::DefaultStyleEx )
	, CBaseFlagsCtrl( this )
{
	SetUseAlternateRowColoring();
	SetCheckStatePolicy( CheckRadio::Instance() );
	SetToggleCheckSelItems();

	ui::LoadPopupMenu( &m_contextMenu, IDR_CONTEXT_MENU, app::FlagsListPopup );
}

CFlagsListCtrl::~CFlagsListCtrl( void )
{
}

CMenu* CFlagsListCtrl::GetPopupMenu( ListPopup popupType )
{
	popupType;
	return &m_contextMenu;
}

void CFlagsListCtrl::InitControl( void )
{
	if ( NULL == m_hWnd )
		return;

	CScopedLockRedraw freeze( this );
	CScopedInternalChange internalChange( this );

	DeleteAllItems();
	RemoveAllGroups();

	const CFlagStore* pFlagStore = GetFlagStore();

	EnableWindow( pFlagStore != NULL );
	EnableGroupView( pFlagStore != NULL && pFlagStore->HasGroups() );

	if ( NULL == pFlagStore )
		return;

	bool useGroups = IsGroupViewEnabled() != FALSE;

	for ( unsigned int pos = 0, itemIndex = 0; pos != pFlagStore->m_flagInfos.size(); ++pos )
	{
		const CFlagInfo* pFlag = pFlagStore->m_flagInfos[ pos ];

		if ( !useGroups || !pFlag->IsSeparator() )			// add separators only if not using groups
			AddFlagItem( itemIndex++, pFlag );
	}

	if ( GetItemCount() != 0 )
		EnsureVisible( 0, FALSE );

	if ( useGroups )
	{
		static const std::tstring s_anonymousGroupText;

		for ( unsigned int groupId = 0; groupId != pFlagStore->m_groups.size(); ++groupId )
		{
			const CFlagGroup* pGroup = pFlagStore->m_groups[ groupId ];
			const std::tstring& header = pGroup->GetName( s_anonymousGroupText );

			InsertGroupHeader( groupId, groupId, header, LVGS_NORMAL | LVGS_COLLAPSIBLE );
			if ( !pGroup->IsReadOnlyGroup() )
				SetGroupTask( groupId, pGroup->IsValueGroup() ? _T("Toggle Next") : _T("Toggle All") );
		}

		for ( unsigned int row = 0, count = GetItemCount(); row != count; ++row )
			if ( const CFlagInfo* pFlag = GetFlagInfoAt( row ) )
				if ( !pFlag->IsSeparator() )
				{
					int groupId = FindGroupIdWithFlag( pFlag );
					if ( groupId != -1 )
						VERIFY( SetItemGroupId( row, groupId ) );
				}
	}
}

void CFlagsListCtrl::AddFlagItem( int itemIndex, const CFlagInfo* pFlag )
{
	enum Column { Flag, HexValue };

	if ( !pFlag->IsSeparator() )
	{
		std::tstring itemText; itemText.reserve( 128 );
		itemText = pFlag->GetName();
		if ( pFlag->IsReadOnly() )
			itemText += _T(" (read only)");
		if ( !pFlag->GetAliases().empty() )
			itemText += str::Format( _T(" (%s)"), pFlag->GetAliases().c_str() );

		InsertItem( itemIndex, itemText.c_str() );
		SetSubItemText( itemIndex, HexValue, num::FormatHexNumber( pFlag->m_value, _T("0x%08X") ) );
		SetFlagInfoAt( itemIndex, pFlag );
	}
	else
	{
		static const std::tstring separator( 10, _T('-') );
		static const TCHAR sepFormat[] = _T("-- %s --");
		std::tstring itemText = !str::IsEmpty( pFlag->m_pRawTag ) ? str::Format( sepFormat, pFlag->m_pRawTag ) : separator;

		InsertItem( itemIndex, itemText.c_str() );
		SetFlagInfoAt( itemIndex, pFlag );
	}
}

CFlagGroup* CFlagsListCtrl::GetFlagGroup( int groupId ) const
{
	if ( const CFlagStore* pFlagStore = GetFlagStore() )
		if ( groupId >= 0 && groupId < static_cast<int>( pFlagStore->m_groups.size() ) )
			return pFlagStore->m_groups[ groupId ];

	return NULL;
}

int CFlagsListCtrl::FindGroupIdWithFlag( const CFlagInfo* pFlag ) const
{
	ASSERT_PTR( pFlag );
	if ( pFlag->m_pGroup != NULL )
		if ( const CFlagStore* pFlagStore = GetFlagStore() )
		{
			const std::vector< CFlagGroup* >& groups = pFlagStore->m_groups;
			std::vector< CFlagGroup* >::const_iterator itGroup = std::find( groups.begin(), groups.end(), pFlag->m_pGroup );
			if ( itGroup != groups.end() )
				return static_cast<int>( std::distance( groups.begin(), itGroup ) );
		}
	return -1;
}

void CFlagsListCtrl::QueryCheckedFlagWorkingSet( std::vector< const CFlagInfo* >& rFlagSet, int index ) const
{
	if ( IsSelected( index ) )
		QuerySelectionAs( rFlagSet );
	else
		rFlagSet.push_back( GetPtrAt< CFlagInfo >( index ) );
}

bool CFlagsListCtrl::AnyFlagCheckConflict( const std::vector< const CFlagInfo* >& flagSet, bool checked ) const
{
	std::vector< CFlagGroup* > flagGroups;		// includes radio, check-box or NULL (groupless)

	for ( std::vector< const CFlagInfo* >::const_iterator itFlag = flagSet.begin(); itFlag != flagSet.end(); ++itFlag )
		if ( ( *itFlag )->IsReadOnly() )
			return true;				// can't modify read-only flags
		else
		{
			utl::AddUnique( flagGroups, ( *itFlag )->m_pGroup );

			if ( ( *itFlag )->m_pGroup != NULL && ( *itFlag )->m_pGroup->IsValueGroup() )
				if ( !checked )
					return true;		// can't uncheck a radio button
		}

	std::vector< CFlagGroup* >::iterator itCheckGroup = std::partition( flagGroups.begin(), flagGroups.end(), pred::IsRadioGroup() );
	size_t radioGroupCount = std::distance( flagGroups.begin(), itCheckGroup );
	size_t checkGroupCount = std::distance( itCheckGroup, flagGroups.end() );

	if ( radioGroupCount > 1 )
		return true;					// more that 1 radio groups in selected set
	else if ( 1 == radioGroupCount && checkGroupCount != 0 )
		return true;					// can't mix radio group with checbox groups

	return false;						// no conflicts
}

bool CFlagsListCtrl::SetFlagsChecked( const std::vector< const CFlagInfo* >& flagSet, bool checked )
{
	DWORD flags = GetFlags();

	for ( std::vector< const CFlagInfo* >::const_iterator itFlag = flagSet.begin(); itFlag != flagSet.end(); ++itFlag )
		( *itFlag )->SetTo( &flags, checked );

	UserSetFlags( flags );
	return true;
}

void CFlagsListCtrl::OutputFlags( void )
{
	if ( NULL == m_hWnd )
		return;

	CScopedInternalChange change( this );
	DWORD flags = GetFlags();

	for ( int i = 0, count = GetItemCount(); i != count; ++i )
		if ( const CFlagInfo* pFlag = GetFlagInfoAt( i ) )
			if ( !pFlag->IsSeparator() )
				ModifyCheckState( i, CheckRadio::MakeCheckState( pFlag->IsValue(), pFlag->IsOn( flags ), !pFlag->IsReadOnly() ) );
}

DWORD CFlagsListCtrl::InputFlags( void ) const
{
	ASSERT( m_hWnd != NULL && HasValidMask() );
	DWORD flags = 0;

	for ( int i = 0, count = GetItemCount(); i != count; ++i )
		if ( const CFlagInfo* pFlag = GetFlagInfoAt( i ) )
			if ( !pFlag->IsSeparator() )
				pFlag->SetTo( &flags, IsChecked( i ) );

	return flags;
}

void CFlagsListCtrl::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	const CFlagInfo* pFlag = CReportListControl::AsPtr<CFlagInfo>( rowKey );
	if ( pFlag->IsReadOnly() || pFlag->IsSeparator() )
		rTextEffect.m_textColor = color::Gray60;
	else if ( pFlag->IsOn( GetFlags() ) )
		rTextEffect.m_textColor = app::HotFieldColor;

	CReportListControl::CombineTextEffectAt( rTextEffect, rowKey, subItem, pCtrl );
}

void CFlagsListCtrl::PreSubclassWindow( void )
{
	CReportListControl::PreSubclassWindow();

	if ( HasValidMask() )
	{
		InitControl();
		OutputFlags();
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CFlagsListCtrl, CReportListControl )
	ON_NOTIFY_REFLECT_EX( lv::LVN_ToggleCheckState, OnLvnToggleCheckState_Reflect )
	ON_NOTIFY_REFLECT_EX( lv::LVN_CheckStatesChanged, OnLvnCheckStatesChanged_Reflect )
	ON_NOTIFY_REFLECT_EX( LVN_LINKCLICK, OnLvnLinkClick_Reflect )
	ON_COMMAND( CM_COPY_FORMATTED, OnCopy )
	ON_UPDATE_COMMAND_UI( CM_COPY_FORMATTED, OnUpdateCopy )
	ON_COMMAND( CM_COPY_SELECTED, OnCopySelected )
	ON_UPDATE_COMMAND_UI( CM_COPY_SELECTED, OnUpdateCopySelected )
END_MESSAGE_MAP()

BOOL CFlagsListCtrl::PreTranslateMessage( MSG* pMsg )
{
	return m_accel.Translate( pMsg, m_hWnd ) || CReportListControl::PreTranslateMessage( pMsg );
}

BOOL CFlagsListCtrl::OnLvnToggleCheckState_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmToggleCheckState* pToggleInfo = (lv::CNmToggleCheckState*)pNmHdr;
	ASSERT( !IsInternalChange() );
	*pResult = 0;

	std::vector< const CFlagInfo* > flagSet;
	QueryCheckedFlagWorkingSet( flagSet, pToggleInfo->m_pListView->iItem );

	if ( AnyFlagCheckConflict( flagSet, GetCheckStatePolicy()->IsCheckedState( pToggleInfo->m_newCheckState ) ) )
	{
		ui::BeepSignal();
		*pResult = TRUE;	// prevent conflicting toggle
		return TRUE;		// skip routing to parent
	}

	return FALSE;			// continue routing to parent
}

BOOL CFlagsListCtrl::OnLvnCheckStatesChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	lv::CNmCheckStatesChanged* pInfo = (lv::CNmCheckStatesChanged*)pNmHdr;
	ASSERT( !IsInternalChange() );
	ASSERT( !pInfo->m_itemIndexes.empty() );
	*pResult = 0;

	// user has just toggled the check-state
	std::vector< const CFlagInfo* > flagSet;
	QueryObjectsByIndex( flagSet, pInfo->m_itemIndexes );		// get all flags impacted by the toggle

	if ( AnyFlagCheckConflict( flagSet, IsChecked( pInfo->m_itemIndexes.front() ) ) )		// first index is the toggled reference
	{
		ui::BeepSignal();
		OutputFlags();
		*pResult = TRUE;	// prevent conflicting toggle
		return TRUE;		// skip routing to parent
	}

	DWORD newFlags = InputFlags();
	UserSetFlags( newFlags );
	return FALSE;			// continue routing to parent
}

BOOL CFlagsListCtrl::OnLvnLinkClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVLINK* pLinkInfo = (NMLVLINK*)pNmHdr;
	int groupId = pLinkInfo->iSubItem;

	if ( CFlagGroup* pFlagGroup = GetFlagGroup( groupId ) )
	{
		DWORD flags = GetFlags();
		if ( pFlagGroup->IsValueGroup() )
		{
			if ( const CFlagInfo* pValueOn = pFlagGroup->FindOnValue( flags ) )
			{	// toggle to next value (previous if SHIFT is down)
				int pos = static_cast<int>( utl::LookupPos( pFlagGroup->GetFlags(), pValueOn ) );
				pos = utl::CircularAdvance( pos, static_cast<int>( pFlagGroup->GetFlags().size() ), !ui::IsKeyPressed( VK_SHIFT ) );
				pFlagGroup->GetFlags()[ pos ]->SetTo( &flags, true );
			}
		}
		else
		{
			bool checkAll = NULL == pFlagGroup->FindOnFlag( flags );
			for ( std::vector< const CFlagInfo* >::const_iterator itFlag = pFlagGroup->GetFlags().begin(); itFlag != pFlagGroup->GetFlags().end(); ++itFlag )
				( *itFlag )->SetTo( &flags, checkAll );
		}

		UserSetFlags( flags );
	}

	*pResult = 0;
	return FALSE;
}

void CFlagsListCtrl::OnCopy( void )
{
	CTextClipboard::CopyText( Format(), m_hWnd );
}

void CFlagsListCtrl::OnUpdateCopy( CCmdUI* pCmdUI )
{
	pCmdUI;
}

void CFlagsListCtrl::OnCopySelected( void )
{
	std::vector< CFlagInfo* > selFlags;
	QuerySelectionAs( selFlags );

	if ( !selFlags.empty() )
	{
		std::vector< std::tstring > lines;

		for ( std::vector< CFlagInfo* >::const_iterator itSelFlag = selFlags.begin(); itSelFlag != selFlags.end(); ++itSelFlag )
			lines.push_back( ( *itSelFlag )->GetName() );

		CTextClipboard::CopyToLines( lines, m_hWnd );
	}
}

void CFlagsListCtrl::OnUpdateCopySelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( FindItemWithState( LVNI_SELECTED ) != -1 );
}
