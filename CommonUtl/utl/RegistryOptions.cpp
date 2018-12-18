
#include "StdAfx.h"
#include "RegistryOptions.h"
#include "RegistrySection.h"
#include "CmdUpdate.h"
#include "EnumTags.h"
#include "Path.h"
#include "RuntimeException.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRegistryOptions implementation

CRegistryOptions::CRegistryOptions( const std::tstring& section, bool saveOnModify )
	: m_saveOnModify( saveOnModify )
{
	if ( !section.empty() )
		m_pRegSection.reset( new CAppRegistrySection( section ) );
}

CRegistryOptions::CRegistryOptions( IRegistrySection* pRegSection, bool saveOnModify )
	: m_pRegSection( pRegSection )
	, m_saveOnModify( saveOnModify )
{
}

CRegistryOptions::~CRegistryOptions()
{
	utl::ClearOwningContainer( m_options );
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

reg::COption< bool >* CRegistryOptions::FindBoolOptionByID( UINT ctrlId ) const
{
	ASSERT( ctrlId != 0 );

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( ( *itOption )->GetCtrlId() == ctrlId )
			return checked_static_cast< reg::COption< bool >* >( *itOption );

	return NULL;
}

void CRegistryOptions::UpdateControls( CWnd* pTargetWnd )
{
	ASSERT_PTR( pTargetWnd );

	for ( std::vector< reg::CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
		if ( ( *itOption )->GetCtrlId() != 0 )
			if ( CWnd* pCtrl = pTargetWnd->GetDlgItem( ( *itOption )->GetCtrlId() ) )
				ui::UpdateControlUI( pCtrl, pTargetWnd );
}

void CRegistryOptions::OnOptionChanged( const void* pDataMember )
{
	if ( m_saveOnModify )
		SaveOption( pDataMember );		// save right away the changed option
}

BOOL CRegistryOptions::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( CN_COMMAND == code || CN_UPDATE_COMMAND_UI == code )		// boolean check toggle or update?
	{
		reg::COption< bool >* pBoolOption = FindBoolOptionByID( id );
		if ( NULL == pBoolOption )
			return false;											// not a bool option registered as such
	}

	return
		CCmdTarget::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CRegistryOptions, CCmdTarget )
	ON_COMMAND_RANGE( 1, 0x8FFF, OnToggle_BoolOption )
	ON_UPDATE_COMMAND_UI_RANGE( 1, 0x8FFF, OnUpdate_BoolOption )
END_MESSAGE_MAP()

void CRegistryOptions::OnToggle_BoolOption( UINT cmdId )
{
	reg::COption< bool >* pBoolOption = FindBoolOptionByID( cmdId );
	ASSERT_PTR( pBoolOption );

	ToggleOption( &pBoolOption->RefValue() );
}

void CRegistryOptions::OnUpdate_BoolOption( CCmdUI* pCmdUI )
{
	const reg::COption< bool >* pBoolOption = FindBoolOptionByID( pCmdUI->m_nID );
	ASSERT_PTR( pBoolOption );
	pCmdUI->SetCheck( pBoolOption->GetValue() );
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
		, m_ctrlId( 0 )
		, m_pParent( NULL )
	{
		ASSERT( !m_entry.empty() );

		m_entry[ 0 ] = func::ToUpper()( m_entry[ 0 ] );		// capitalize first letter of the entry name
	}

	CBaseOption::~CBaseOption()
	{
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
			COption< int >::Load();
	}

	void CEnumOption::Save( void ) const
	{
		if ( m_pTags != NULL )
			GetSection()->SaveParameter( m_entry.c_str(), m_pTags->FormatKey( *m_pValue ).c_str() );
		else
			COption< int >::Save();
	}

} // namespace reg
