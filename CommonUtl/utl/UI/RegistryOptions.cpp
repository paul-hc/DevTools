
#include "StdAfx.h"
#include "RegistryOptions.h"
#include "RegistrySection.h"
#include "CmdUpdate.h"
#include "EnumTags.h"
#include "Path.h"
#include "RuntimeException.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"
#include "ui_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRegistryOptions implementation

CRegistryOptions::CRegistryOptions( const std::tstring& sectionName, AutoSave autoSave )
	: m_autoSave( autoSave )
{
	SetSectionName( sectionName );
}

CRegistryOptions::CRegistryOptions( IRegistrySection* pRegSection, AutoSave autoSave )
	: m_pRegSection( pRegSection )
	, m_autoSave( autoSave )
{
}

CRegistryOptions::~CRegistryOptions()
{
	utl::ClearOwningContainer( m_options );
}

const std::tstring& CRegistryOptions::GetSectionName( void ) const
{
	IRegistrySection* pSection = GetSection();
	return pSection != NULL ? pSection->GetSectionName() : str::GetEmpty();
}

void CRegistryOptions::SetSectionName( const std::tstring& sectionName )
{
	if ( !sectionName.empty() )
		m_pRegSection.reset( new CAppRegistrySection( sectionName ) );
	else
		m_pRegSection.reset();
}

void CRegistryOptions::AddOption( reg::CBaseOption* pOption, UINT ctrlId /*= 0*/ )
{
	ASSERT_PTR( pOption );

	m_options.push_back( pOption );
	pOption->SetParent( this );
	pOption->SetCtrlId( ctrlId );
}

void CRegistryOptions::LoadAll( void )
{
	if ( IsPersistent() )
		for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
			( *itOption )->Load();
}

void CRegistryOptions::SaveAll( void ) const
{
	if ( IsPersistent() )
		for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
			( *itOption )->Save();
}

bool CRegistryOptions::AnyNonDefaultValue( void ) const
{
	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( !( *itOption )->HasDefaultValue() )
			return true;

	return false;
}

size_t CRegistryOptions::RestoreAllDefaultValues( void )
{
	size_t changedCount = 0;

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( RestoreOptionDefaultValue( *itOption ) )
			++changedCount;

	return changedCount;
}

bool CRegistryOptions::RestoreOptionDefaultValue( reg::CBaseOption* pOption )
{
	ASSERT_PTR( pOption );
	if ( pOption->HasDefaultValue() )
		return false;

	pOption->SetDefaultValue();
	OnOptionChanged( pOption->GetDataMember() );
	return true;
}

reg::CBaseOption& CRegistryOptions::LookupOption( const void* pDataMember ) const
{
	ASSERT_PTR( pDataMember );

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( ( *itOption )->HasDataMember( pDataMember ) )
			return **itOption;

	ASSERT( false );
	throw CRuntimeException( "Data-member not found in option container" );
}

reg::CBaseOption* CRegistryOptions::FindOptionByID( UINT ctrlId ) const
{
	ASSERT( ctrlId != 0 );

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( ( *itOption )->HasCtrlId( ctrlId ) )
			return *itOption;

	return NULL;
}

void CRegistryOptions::UpdateControls( CWnd* pTargetWnd )
{
	ASSERT_PTR( pTargetWnd->GetSafeHwnd() );

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( ( *itOption )->GetCtrlId() != 0 )
			if ( CWnd* pCtrl = pTargetWnd->GetDlgItem( ( *itOption )->GetCtrlId() ) )
				ui::UpdateControlUI( pCtrl->GetSafeHwnd(), pTargetWnd );
}

void CRegistryOptions::OnOptionChanged( const void* pDataMember )
{
	switch ( m_autoSave )
	{
		case SaveOnModify:
			SaveOption( pDataMember );		// save right away the changed option
			break;
		case SaveAllOnModify:
			SaveAll();
			break;
	}
}


// message handlers

BEGIN_MESSAGE_MAP( CRegistryOptions, CCmdTarget )
	ON_COMMAND_EX_RANGE( ui::MinCmdId, ui::MaxCmdId, OnToggleOption )
	ON_UPDATE_COMMAND_UI_RANGE( ui::MinCmdId, ui::MaxCmdId, OnUpdateOption )
END_MESSAGE_MAP()

BOOL CRegistryOptions::OnToggleOption( UINT cmdId )
{
	if ( reg::CBaseOption* pOption = FindOptionByID( cmdId ) )
	{
		if ( reg::TBoolOption* pBoolOption = dynamic_cast<reg::TBoolOption*>( pOption ) )
			ToggleOption( &pBoolOption->RefValue() );
		else if ( reg::CEnumOption* pEnumOption = dynamic_cast<reg::CEnumOption*>( pOption ) )
			ModifyOption( &pEnumOption->RefValue(), pEnumOption->GetValueFromRadioId( cmdId ) );
		else
			return FALSE;		// continue routing (to derived class handlers)

		return TRUE;			// handled
	}
	return FALSE;				// continue routing (to derived class handlers)
}

void CRegistryOptions::OnUpdateOption( CCmdUI* pCmdUI )
{
	if ( const reg::CBaseOption* pOption = FindOptionByID( pCmdUI->m_nID ) )
	{
		if ( const reg::TBoolOption* pBoolOption = dynamic_cast<const reg::TBoolOption*>( pOption ) )
			pCmdUI->SetCheck( pBoolOption->GetValue() );
		else if ( const reg::CEnumOption* pEnumOption = dynamic_cast<const reg::CEnumOption*>( pOption ) )
			ui::SetRadio( pCmdUI, pEnumOption->GetValueFromRadioId( pCmdUI->m_nID ) == pEnumOption->GetValue() );
		else
			pCmdUI->ContinueRouting();		// continue routing (to derived class handlers)
	}
	else
		pCmdUI->ContinueRouting();			// continue routing (to derived class handlers)
}


namespace reg
{
	const TCHAR* SkipDataMemberPrefix( const TCHAR* pEntry )
	{
		const TCHAR* pSkipped = pEntry;
		pSkipped = str::SkipPrefix< str::Case >( pSkipped, _T("&") );		// strip "&" operator used to pass the address of data-member
		pSkipped = str::SkipPrefix< str::Case >( pSkipped, _T("m_") );
		pSkipped = str::SkipPrefix< str::Case >( pSkipped, _T("s_") );
		return pSkipped;
	}


	// CBaseOption implementation

	CBaseOption::CBaseOption( const TCHAR* pEntry )
		: m_entry( SkipDataMemberPrefix( pEntry ) )
		, m_pParent( NULL )
		, m_ctrlId( 0 )
	{
		ASSERT( !m_entry.empty() );

		m_entry[ 0 ] = func::ToUpper()( m_entry[ 0 ] );		// capitalize first letter of the entry name
	}

	CBaseOption::~CBaseOption()
	{
	}

	bool CBaseOption::HasCtrlId( UINT ctrlId ) const
	{
		return m_ctrlId == ctrlId;
	}


	// COption< fs::CPath > specialization

	template<>
	void COption< fs::CPath >::Load( void )
	{
		*m_pValue = GetSection()->GetStringParameter( m_entry.c_str(), m_pValue->GetPtr() );
	}

	template<>
	void COption< fs::CPath >::Save( void ) const
	{
		GetSection()->SaveParameter( m_entry.c_str(), m_pValue->GetPtr() );
	}


	// COption< double > serialization

	template<>
	void COption< double >::Load( void )
	{
		std::tstring text = GetSection()->GetStringParameter( m_entry.c_str(), num::FormatNumber( *m_pValue ).c_str() );
		num::ParseNumber( *m_pValue, text );
	}

	template<>
	void COption< double >::Save( void ) const
	{
		GetSection()->SaveParameter( m_entry.c_str(), num::FormatNumber( *m_pValue ) );
	}


	// CEnumOption implementation

	void CEnumOption::Load( void )
	{
		if ( m_pTags != NULL )
		{
			std::tstring keyTag = GetSection()->GetStringParameter( m_entry.c_str(), m_pTags->FormatKey( *m_pValue ).c_str() );
			int newValue;
			if ( m_pTags->ParseKeyAs( newValue, keyTag ) )
				*m_pValue = newValue;
			else
				TRACE( _T(" * CEnumOption::Load() - tag mismatch '%s' in enum option '%s'\n"), keyTag.c_str(), m_entry.c_str() );
		}
		else
			TBaseClass::Load();
	}

	void CEnumOption::Save( void ) const
	{
		if ( m_pTags != NULL )
			GetSection()->SaveParameter( m_entry.c_str(), m_pTags->FormatKey( *m_pValue ).c_str() );
		else
			TBaseClass::Save();
	}

	bool CEnumOption::HasCtrlId( UINT ctrlId ) const
	{
		return HasRadioId( ctrlId ) || TBaseClass::HasCtrlId( ctrlId );
	}

	int CEnumOption::GetValueFromRadioId( UINT radioId ) const
	{
		ASSERT( m_radioIds.Contains( radioId ) );
		return radioId - m_radioIds.m_start;
	}

} // namespace reg
