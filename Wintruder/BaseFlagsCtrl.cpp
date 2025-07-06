
#include "pch.h"
#include "BaseFlagsCtrl.h"
#include "resource.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaseFlagsCtrl::CBaseFlagsCtrl( CWnd* pCtrl )
	: m_pCtrl( pCtrl )
	, m_flagsMask( 0 )
	, m_flags( 0 )
	, m_accel( IDR_FLAGS_CONTROL_ACCEL )
{
}

void CBaseFlagsCtrl::SetFlagStore( const CFlagStore* pFlagStore )
{
	m_flagsMask = 0;

	std::vector<const CFlagStore*> flagStores;
	if ( pFlagStore != nullptr )
	{
		flagStores.push_back( pFlagStore );
		m_flagsMask = pFlagStore->GetMask();
	}

	m_flags = 0;

	if ( flagStores != m_flagStores )		// prevent flags list from updating when store hasn't changed
	{
		m_flagStores.swap( flagStores );
		InitControl();
	}
}

void CBaseFlagsCtrl::SetMultiFlagStores( const std::vector<const CFlagStore*>& flagStores, DWORD flagsMask /*= UINT_MAX*/ )
{
	m_flagsMask = flagsMask;

	if ( 0 == m_flagsMask )
		for ( std::vector<const CFlagStore*>::const_iterator itFlagStore = flagStores.begin(); itFlagStore != flagStores.end(); ++itFlagStore )
			m_flagsMask |= ( *itFlagStore )->GetMask();

	m_flags = 0;

	if ( flagStores != m_flagStores )		// prevent flags list from updating when store hasn't changed
	{
		m_flagStores = flagStores;
		InitControl();
	}
}

void CBaseFlagsCtrl::SetFlagsMask( DWORD flagsMask )
{
	m_flagsMask = flagsMask;

	m_flags = 0;
	InitControl();
}

void CBaseFlagsCtrl::SetFlags( DWORD flags )
{
	m_flags = ( flags & m_flagsMask );
	OutputFlags();
}

bool CBaseFlagsCtrl::UserSetFlags( DWORD flags )
{
	if ( flags == m_flags )
		return false;

	SetFlags( flags );

	if ( m_pCtrl->GetSafeHwnd() != nullptr )
		ui::SendCommandToParent( m_pCtrl->m_hWnd, FN_FLAGSCHANGED );
	return true;
}

std::tstring CBaseFlagsCtrl::Format( void ) const
{
	std::tstring text; text.reserve( 256 );

	stream::Tag( text, str::Format( _T("0x%08X"), m_flags ), nullptr );
	stream::Tag( text, FormatTooltip(), _T(": ") );
	return text;
}

std::tstring CBaseFlagsCtrl::FormatTooltip( void ) const
{
	std::tstring text; text.reserve( 256 );

	switch ( m_flagStores.size() )
	{
		case 0:
			CFlagStore::StreamMask( text, m_flagsMask );
			break;
		case 1:
			m_flagStores.front()->StreamFormatFlags( text, m_flags );
			m_flagStores.front()->StreamFormatMask( text );
			break;
		default:
			for ( std::vector<const CFlagStore*>::const_iterator itFlagStore = m_flagStores.begin(); itFlagStore != m_flagStores.end(); ++itFlagStore )
				( *itFlagStore )->StreamFormatFlags( text, m_flags );

			CFlagStore::StreamMask( text, m_flagsMask );
	}
	return text;
}
