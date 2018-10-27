
#include "stdafx.h"
#include "FlagsListCtrl.h"
#include "Application.h"
#include "AppService.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/CtrlUiState.h"
#include "utl/ContainerUtilities.h"
#include "utl/MenuUtilities.h"
#include "utl/StringUtilities.h"
#include "utl/UtilitiesEx.h"
#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFlagsListCtrl::CFlagsListCtrl( void )
	: CReportListControl( IDC_FLAGS_EDITOR_LIST, LVS_EX_CHECKBOXES | LVS_EX_UNDERLINEHOT | DefaultStyleEx )
	, CBaseFlagsCtrl( this )
{
	SetUseAlternateRowColoring();
	ui::LoadPopupMenu( m_contextMenu, IDR_CONTEXT_MENU, app::FlagsListPopup );
}

CFlagsListCtrl::~CFlagsListCtrl( void )
{
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
						VERIFY( SetRowGroupId( row, groupId ) );
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
		if ( groupId >= 0 && groupId < static_cast< int >( pFlagStore->m_groups.size() ) )
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
			return static_cast< int >( std::distance( groups.begin(), itGroup ) );
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
	std::set< CFlagGroup* > exclusiveGroups;

	for ( std::vector< const CFlagInfo* >::const_iterator itFlag = flagSet.begin(); itFlag != flagSet.end(); ++itFlag )
		if ( ( *itFlag )->IsReadOnly() )
			return true;				// can't modify read-only
		else if ( ( *itFlag )->m_pGroup != NULL && ( *itFlag )->m_pGroup->IsValueGroup() )
			if ( !exclusiveGroups.insert( ( *itFlag )->m_pGroup ).second )
				return true;
			else if ( !checked )
				return true;			// can't uncheck a value

	return false;
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
			{
				ASSERT_PTR( pFlag );

				bool off = !pFlag->IsOn( flags );
				CheckState checkState;

				if ( pFlag->IsValue() )
					checkState = off ? LVIS_RADIO_UNCHECKED : LVIS_RADIO_CHECKED;
				else
					checkState = off ? LVIS_UNCHECKED : ( pFlag->IsReadOnly() ? LVIS_CHECKEDGRAY : LVIS_CHECKED );

				if ( checkState != GetCheckState( i ) )
					SetCheckState( i, checkState );
			}
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

void CFlagsListCtrl::PreSubclassWindow( void )
{
	CReportListControl::PreSubclassWindow();

	SetupExtendedCheckStates();
	if ( HasValidMask() )
	{
		InitControl();
		OutputFlags();
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CFlagsListCtrl, CReportListControl )
	ON_WM_CONTEXTMENU()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_INITMENUPOPUP()
	ON_NOTIFY_REFLECT_EX( LVN_LINKCLICK, OnGroupTaskClick_Reflect )
	ON_COMMAND( CM_COPY_FORMATTED, OnCopy )
	ON_UPDATE_COMMAND_UI( CM_COPY_FORMATTED, OnUpdateCopy )
	ON_COMMAND( CM_COPY_SELECTED, OnCopySelected )
	ON_UPDATE_COMMAND_UI( CM_COPY_SELECTED, OnUpdateCopySelected )
	ON_COMMAND( CM_EXPAND_GROUPS, OnExpandGroups )
	ON_UPDATE_COMMAND_UI( CM_EXPAND_GROUPS, OnUpdateExpandGroups )
	ON_COMMAND( CM_COLLAPSE_GROUPS, OnCollapseGroups )
	ON_UPDATE_COMMAND_UI( CM_COLLAPSE_GROUPS, OnUpdateCollapseGroups )
END_MESSAGE_MAP()

BOOL CFlagsListCtrl::PreTranslateMessage( MSG* pMsg )
{
	return m_accel.Translate( pMsg, m_hWnd ) || CReportListControl::PreTranslateMessage( pMsg );
}

void CFlagsListCtrl::OnContextMenu( CWnd* /*pWnd*/, CPoint point )
{
	m_contextMenu.TrackPopupMenu( TPM_RIGHTBUTTON, point.x, point.y, this );
}

void CFlagsListCtrl::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	CReportListControl::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

HBRUSH CFlagsListCtrl::CtlColor( CDC* pDC, UINT ctlType )
{
	pDC;
	if ( ctlType == CTLCOLOR_LISTBOX && !HasValidMask() )
		return ::GetSysColorBrush( COLOR_3DFACE );
	return NULL;
}

BOOL CFlagsListCtrl::OnLvnItemChanging_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	if ( CReportListControl::OnLvnItemChanging_Reflect( pNmHdr, pResult ) )
		return TRUE;			// internal change

	NMLISTVIEW* pListInfo = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( HasFlag( pListInfo->uChanged, LVIF_STATE ) && StateChanged( pListInfo->uNewState, pListInfo->uOldState, LVIS_STATEIMAGEMASK ) )		// check state changed
	{
		std::vector< const CFlagInfo* > flagSet;
		QueryCheckedFlagWorkingSet( flagSet, pListInfo->iItem );

		CheckState checkState = AsCheckState( pListInfo->uNewState );
		if ( AnyFlagCheckConflict( flagSet, IsCheckedState( checkState ) ) )
		{
			*pResult = 1;			// prevent change with conflict
			ui::BeepSignal();
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CFlagsListCtrl::OnLvnItemChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	if ( CReportListControl::OnLvnItemChanged_Reflect( pNmHdr, pResult ) )
		return TRUE;			// internal change

	NMLISTVIEW* pListInfo = (NMLISTVIEW*)pNmHdr;
	*pResult = 0;

	if ( pListInfo->uChanged & LVIF_STATE )		// item state has been changed
		if ( StateChanged( pListInfo->uNewState, pListInfo->uOldState, LVIS_STATEIMAGEMASK ) )		// check state changed
			if ( HasCheckState( pListInfo->uNewState ) )
			{
				std::vector< const CFlagInfo* > flagSet;
				QueryCheckedFlagWorkingSet( flagSet, pListInfo->iItem );

				bool checked = IsCheckedState( pListInfo->uNewState );
				if ( AnyFlagCheckConflict( flagSet, checked ) )
				{
					*pResult = 1;				// prevent conflict
					ui::BeepSignal();
					return TRUE;
				}
				SetFlagsChecked( flagSet, checked );
			}

	return FALSE;
}

BOOL CFlagsListCtrl::OnGroupTaskClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
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
				int pos = static_cast< int >( utl::LookupPos( pFlagGroup->GetFlags(), pValueOn ) );
				pos = utl::CircularAdvance( pos, static_cast< int >( pFlagGroup->GetFlags().size() ), !ui::IsKeyPressed( VK_SHIFT ) );
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

BOOL CFlagsListCtrl::OnNmCustomDraw_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLVCUSTOMDRAW* pDraw = (NMLVCUSTOMDRAW*)pNmHdr;
	BOOL handled = CReportListControl::OnNmCustomDraw_Reflect( pNmHdr, pResult );

	switch ( pDraw->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			if ( const CFlagInfo* pFlag = AsPtr< CFlagInfo >( pDraw->nmcd.lItemlParam ) )
				if ( pFlag->IsReadOnly() || pFlag->IsSeparator() )
				{
					pDraw->clrText = color::DarkGrey;
					*pResult |= CDRF_NEWFONT;
					return TRUE;		// handled
				}
				else if ( pFlag->IsOn( GetFlags() ) )
				{
					pDraw->clrText = HotFieldColor;
					*pResult |= CDRF_NEWFONT;
					return TRUE;		// handled
				}
			break;
	}
	return handled;
}

void CFlagsListCtrl::OnCopy( void )
{
	CClipboard::CopyText( Format(), this );
}

void CFlagsListCtrl::OnUpdateCopy( CCmdUI* )
{
}

void CFlagsListCtrl::OnCopySelected( void )
{
	std::vector< CFlagInfo* > selFlags;
	QuerySelectionAs( selFlags );

	if ( !selFlags.empty() )
	{
		std::tstring text; text.reserve( 1024 );

		if ( 1 == selFlags.size() )
			text = selFlags.front()->GetName().c_str();		// no line-end
		else
			for ( std::vector< CFlagInfo* >::const_iterator itSelFlag = selFlags.begin(); itSelFlag != selFlags.end(); ++itSelFlag )
			{
				static const TCHAR lineEnd[] = _T("\r\n");
				text += ( *itSelFlag )->GetName();
				text += lineEnd;
			}

		CClipboard::CopyText( text, this );
	}
}

void CFlagsListCtrl::OnUpdateCopySelected( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( FindItemWithState( LVNI_SELECTED ) != -1 );
}

void CFlagsListCtrl::OnExpandGroups( void )
{
	ExpandAllGroups();
}

void CFlagsListCtrl::OnUpdateExpandGroups( CCmdUI* pCmdUI )
{
	bool anyCollapsed = false;

	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
		if ( HasGroupState( GetGroupId( i ), LVGS_COLLAPSED ) )
		{
			anyCollapsed = true;
			break;
		}

	pCmdUI->Enable( anyCollapsed );
	pCmdUI->SetCheck( !anyCollapsed );
}

void CFlagsListCtrl::OnCollapseGroups( void )
{
	CollapseAllGroups();
}

void CFlagsListCtrl::OnUpdateCollapseGroups( CCmdUI* pCmdUI )
{
	bool anyExpanded = false;

	for ( int i = 0, groupCount = GetGroupCount(); i != groupCount; ++i )
		if ( !HasGroupState( GetGroupId( i ), LVGS_COLLAPSED ) )
		{
			anyExpanded = true;
			break;
		}

	pCmdUI->Enable( anyExpanded );
	pCmdUI->SetCheck( !anyExpanded );
}
