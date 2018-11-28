
#include "StdAfx.h"
#include "OptionContainer.h"
#include "RegistrySection.h"
#include "RuntimeException.h"
#include "EnumTags.h"
#include "Path.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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
		, m_pContainer( NULL )
	{
		ASSERT( !m_entry.empty() );

		m_entry[ 0 ] = func::ToUpper()( m_entry[ 0 ] );		// capitalize first letter of the entry name
	}

	CBaseOption::~CBaseOption()
	{
	}


	// COptionContainer implementation

	COptionContainer::COptionContainer( const std::tstring& section )
		: m_pRegSection( new CAppRegistrySection( section ) )
	{
	}

	COptionContainer::COptionContainer( IRegistrySection* pRegSection )
		: m_pRegSection( pRegSection )
	{
		ASSERT_PTR( m_pRegSection.get() );
	}

	COptionContainer::~COptionContainer()
	{
		utl::ClearOwningContainer( m_options );
	}

	void COptionContainer::AddOption( CBaseOption* pOption )
	{
		ASSERT_PTR( pOption );

		m_options.push_back( pOption );
		pOption->SetContainer( this );
	}

	void COptionContainer::LoadOptions( void )
	{
		for ( std::vector< CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
			( *itOption )->Load();
	}

	void COptionContainer::SaveOptions( void ) const
	{
		for ( std::vector< CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
			( *itOption )->Save();
	}

	CBaseOption& COptionContainer::LookupOption( const void* pDataMember ) const
	{
		ASSERT_PTR( pDataMember );

		for ( std::vector< CBaseOption* >::const_iterator itOption = m_options.begin(); itOption != m_options.end(); ++itOption )
			if ( ( *itOption )->HasDataMember( pDataMember ) )
				return **itOption;

		ASSERT( false );
		throw CRuntimeException( "Data-member not found in option container" );
	}


	// COption< fs::CPath > specialization

	template<>
	void COption< fs::CPath >::Load( void )
	{
		*m_pValue = m_pContainer->m_pRegSection->GetStringParameter( m_entry.c_str(), m_pValue->GetPtr() );
	}

	template<>
	void COption< fs::CPath >::Save( void ) const
	{
		m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), m_pValue->GetPtr() );
	}


	// COption< double > serialization

	template<>
	void COption< double >::Load( void )
	{
		std::tstring text = m_pContainer->m_pRegSection->GetStringParameter( m_entry.c_str(), num::FormatNumber( *m_pValue ).c_str() );
		num::ParseNumber( *m_pValue, text );
	}

	template<>
	void COption< double >::Save( void ) const
	{
		m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), num::FormatNumber( *m_pValue ) );
	}


	// CEnumOption implementation

	void CEnumOption::Load( void )
	{
		if ( m_pTags != NULL )
		{
			std::tstring keyTag = m_pContainer->m_pRegSection->GetStringParameter( m_entry.c_str(), m_pTags->FormatKey( *m_pValue ).c_str() );
			int newValue;
			if ( m_pTags->ParseKeyAs( newValue, keyTag ) )
				*m_pValue = newValue;
			else
				TRACE( _T(" * CEnumOption::Load() - tag mismatch '%s' in enum option '%s'\n"), keyTag.c_str(), m_entry.c_str() );
		}
		else
			BaseClass::Load();
	}

	void CEnumOption::Save( void ) const
	{
		if ( m_pTags != NULL )
			m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), m_pTags->FormatKey( *m_pValue ).c_str() );
		else
			BaseClass::Save();
	}

} // namespace reg
