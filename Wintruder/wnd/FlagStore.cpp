
#include "stdafx.h"
#include "FlagStore.h"
#include "AppService.h"
#include "utl/ContainerUtilities.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace stream
{
	const TCHAR flagSep[] = _T("+");
	const TCHAR maskSep[] = _T(" >> ");
}


// CFlagInfo implementation

bool CFlagInfo::IsOn( DWORD flags ) const
{
	ASSERT( !IsSeparator() );
	if ( IsBitFlag() )
		return ( m_value & flags ) != 0;
	else
		return m_value == ( flags & m_mask );
}

bool CFlagInfo::SetTo( DWORD* pFlags, bool on /*= true*/ ) const
{
	ASSERT_PTR( pFlags );
	ASSERT( !IsSeparator() );

	DWORD oldFlags = *pFlags;

	if ( IsBitFlag() )
		SetFlag( *pFlags, m_value, on );
	else if ( IsValue() )
	{
		if ( on )				// radio button checked?
			SetMaskedValue( *pFlags, m_mask, m_value );
	}
	else
	{
		ASSERT( false );	// TODO: is this ever hit?
		*pFlags &= ~m_mask;
		if ( on )
			*pFlags |= m_value;
	}

	return *pFlags != oldFlags;
}

void CFlagInfo::LazyInit( void ) const
{
	if ( m_name.empty() && m_pRawTag != NULL )
	{
		m_name = m_pRawTag;
		size_t sepPos = m_name.find( _T('/') );
		if ( sepPos != std::tstring::npos )
		{
			m_aliases = m_name.substr( sepPos + 1 );
			m_name = m_name.substr( 0, sepPos - 1 );
		}
	}
}

bool CFlagInfo::StreamFlagName( std::tstring& rOutput, DWORD flags, const TCHAR* pSep, bool withAliases /*= true*/ ) const
{
	if ( IsSeparator() || !IsOn( flags ) )
		return false;

	if ( 0 == m_value && !app::GetOptions()->m_displayZeroFlags )
		return false;

	stream::Tag( rOutput, withAliases ? m_pRawTag : GetName(), pSep );
	return true;
}


// CFlagGroup implementation

bool CFlagGroup::CanGroup( const CFlagInfo* pFlagInfo ) const
{
	ASSERT( !pFlagInfo->IsSeparator() );
	return m_flagInfos.empty() || pFlagInfo->m_mask == GetMask();
}

void CFlagGroup::AddToGroup( CFlagInfo* pFlagInfo )
{
	ASSERT( CanGroup( pFlagInfo ) );
	pFlagInfo->m_pGroup = this;
	m_flagInfos.push_back( pFlagInfo );
}

const CFlagInfo* CFlagGroup::FindOnFlag( DWORD flags ) const
{
	for ( std::vector< const CFlagInfo* >::const_iterator itFlagInfo = m_flagInfos.begin();
		  itFlagInfo != m_flagInfos.end(); ++itFlagInfo )
		if ( ( *itFlagInfo )->IsOn( flags ) )
			return *itFlagInfo;

	return NULL;
}

bool CFlagGroup::IsReadOnlyGroup( void ) const
{
	for ( std::vector< const CFlagInfo* >::const_iterator itFlagInfo = m_flagInfos.begin();
		  itFlagInfo != m_flagInfos.end(); ++itFlagInfo )
		if ( !( *itFlagInfo )->IsReadOnly() )
			return false;

	return !m_flagInfos.empty();
}


// CFlagStore implementation

CFlagStore::CFlagStore( const TCHAR* pWndClassAliases, CFlagInfo flagInfos[], unsigned int count, GetWindowFieldFunc pGetFunc, SetWindowFieldFunc pSetFunc )
	: m_mask( 0 )
	, m_editableMask( 0 )
	, m_pGetFunc( pGetFunc )
	, m_pSetFunc( pSetFunc )
{
	if ( !str::IsEmpty( pWndClassAliases ) )
		str::Split( m_wndClasses, pWndClassAliases, _T("|") );
	else
		m_wndClasses.push_back( std::tstring() );

	m_flagInfos.reserve( count );

	// go through all flags to detect if it needs any groups
	bool needsGroups = NeedsGroups( flagInfos, count );

	for ( unsigned int i = 0; i != count; ++i )
	{
		CFlagInfo* pFlagInfo = &flagInfos[ i ];
		pFlagInfo->m_pGroup = NULL;

		DWORD field = pFlagInfo->IsBitFlag() ? pFlagInfo->m_value : pFlagInfo->m_mask;
		m_mask |= field;

		if ( !pFlagInfo->IsReadOnly() )
			m_editableMask |= field;

		m_flagInfos.push_back( pFlagInfo );

		if ( pFlagInfo->IsSeparator() )
			m_groups.push_back( new CFlagGroup( pFlagInfo ) );
		else if ( needsGroups )
		{
			if ( m_groups.empty() || !m_groups.back()->CanGroup( pFlagInfo ) )
				m_groups.push_back( new CFlagGroup );				// add an anonymous group when encountering a new mask

			m_groups.back()->AddToGroup( pFlagInfo );
		}
	}
}

CFlagStore::~CFlagStore()
{
	std::for_each( m_groups.begin(), m_groups.end(), func::Delete() );
}

bool CFlagStore::NeedsGroups( const CFlagInfo flagInfos[], unsigned int count ) const
{
	const CFlagInfo* pPrevFlag = NULL;
	for ( unsigned int i = 0; i != count; ++i )
	{
		const CFlagInfo* pFlagInfo = &flagInfos[ i ];
		if ( pFlagInfo->IsSeparator() )
			return true;
		else
		{
			if ( pPrevFlag != NULL && pFlagInfo->m_mask != pPrevFlag->m_mask )
				return true;				// different mask requires new group

			pPrevFlag = pFlagInfo;
		}
	}

	return false;
}

void CFlagStore::StreamFormatFlags( std::tstring& rOutput, DWORD flags, const TCHAR* pSep /*= stream::flagSep*/ ) const
{
	for ( std::vector< CFlagInfo* >::const_iterator itFlag = m_flagInfos.begin(); itFlag != m_flagInfos.end(); ++itFlag )
		( *itFlag )->StreamFlagName( rOutput, flags, pSep );		// conditional if flags are on
}

void CFlagStore::StreamFormatMask( std::tstring& rOutput, const TCHAR* pSep /*= stream::maskSep*/ ) const
{
	std::tstring tag = str::Format( _T("%s Mask=0x%X"),
		m_wndClasses.front().empty() ? _T("General") : wc::FormatClassName( m_wndClasses.front().c_str() ).c_str(),
		m_mask );

	stream::Tag( rOutput, tag, pSep );
}

void CFlagStore::StreamMask( std::tstring& rOutput, DWORD mask, const TCHAR* pSep /*= maskSep*/ )
{
	if ( mask != 0 && mask != UINT_MAX )
		stream::Tag( rOutput, str::Format( _T("Mask=0x%X"), mask ), pSep );
}

int CFlagStore::CheckFlagsTransition( DWORD newFlags, DWORD oldFlags ) const
// checks the validity of a bit-field transition, taking care of mutual exclusive and read-only fields;
// returns -1 if no error detected, else the index of element that generates the error
{
	const int count = static_cast<int>( m_flagInfos.size() );
	std::vector< CFlagInfo* >::const_iterator itFlag = m_flagInfos.begin();

	DWORD roMask = GetReadOnlyMask();
	DWORD roMismatch = ( newFlags & roMask ) ^ ( oldFlags & roMask );

	// first check for read-only mismatch
	if ( roMismatch != 0 )
	{	// error: atempt to modify a read-only field, seek for error index
		for ( int i = 0; i != count; ++i )
			if ( ( *itFlag )->IsOn( roMismatch ) )
				return i;

		return 0;		// for some reason error index couldn't be detected 
	}

	// second check for mutual exclusive fields mismatch
	for ( int i = 0; i != count; ++i, ++itFlag )
		if ( ( *itFlag )->IsValue() )
		{
			DWORD maskExclusive = ( *itFlag )->m_mask;
			bool foundMatchExclusive = false;

			while ( ( *itFlag )->IsValue() && ( *itFlag )->m_mask == maskExclusive )
			{
				if ( ( *itFlag )->IsOn( newFlags ) )
					if ( !foundMatchExclusive )
						foundMatchExclusive = true;
					else
						return utl::Distance< int >( m_flagInfos.begin(), itFlag );		// error: many mutual exclusive fields active?

				++itFlag;
			}
			if ( !foundMatchExclusive )
				return i;							// error: none of mutual exclusive group is active 

			i = utl::Distance< int >( m_flagInfos.begin(), --itFlag );					// restore increment context
		}

	return -1;
}
