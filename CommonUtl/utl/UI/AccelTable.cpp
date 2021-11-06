
#include "stdafx.h"
#include "AccelTable.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAccelKeys implementation

size_t CAccelKeys::FindPos( UINT cmdId )
{
	for ( size_t pos = 0; pos != m_keys.size(); ++pos )
		if ( cmdId == m_keys[ pos ].cmd )
			return pos;

	return utl::npos;
}

void CAccelKeys::Augment( UINT cmdId, WORD vkKey, BYTE fVirtFlag /*= FVIRTKEY*/ )
{
	size_t foundPos = FindPos( cmdId );

	ACCEL key = { fVirtFlag, vkKey, static_cast<WORD>( cmdId ) };

	if ( foundPos != utl::npos )
		m_keys[ foundPos ] = key;
	else
		m_keys.push_back( key );
}

bool CAccelKeys::Remove( UINT cmdId )
{
	size_t foundPos = FindPos( cmdId );
	if ( utl::npos == foundPos )
		return false;

	m_keys.erase( m_keys.begin() + foundPos );
	return true;
}

bool CAccelKeys::ReplaceCmdId( UINT cmdId, UINT newCmdId )
{
	size_t foundPos = FindPos( cmdId );
	if ( utl::npos == foundPos )
		return false;

	m_keys[ foundPos ].cmd = static_cast<WORD>( newCmdId );
	return true;
}


// CAccelTable implementation

CAccelTable::CAccelTable( UINT accelId )
	: m_hAccel( ::LoadAccelerators( CScopedResInst::Get(), MAKEINTRESOURCE( accelId ) ) )
{
	ASSERT_PTR( m_hAccel );
}

CAccelTable::CAccelTable( ACCEL keys[], int count )
	: m_hAccel( ::CreateAcceleratorTable( keys, count ) )
{
	ASSERT_PTR( m_hAccel );
}

CAccelTable::~CAccelTable()
{
	if ( m_hAccel != NULL )
		::DestroyAcceleratorTable( m_hAccel );
}

void CAccelTable::QueryKeys( std::vector< ACCEL >& rKeys ) const
{
	if ( m_hAccel != NULL )
	{
		int count = ::CopyAcceleratorTable( m_hAccel, NULL, 0 );
		int oldCount = static_cast<int>( rKeys.size() );

		rKeys.resize( rKeys.size() + count );			// allocate stoarge keys
		VERIFY( count == ::CopyAcceleratorTable( m_hAccel, &rKeys[ oldCount ], count ) );
	}
}

void CAccelTable::Load( UINT accelId )
{
	if ( m_hAccel != NULL )
		::DestroyAcceleratorTable( m_hAccel );

	m_hAccel = ::LoadAccelerators( CScopedResInst::Get(), MAKEINTRESOURCE( accelId ) );
	ASSERT_PTR( m_hAccel );
}

bool CAccelTable::LoadOnce( UINT accelId )
{
	if ( m_hAccel != NULL )
		return false;				// already loaded

	Load( accelId );
	return true;
}

void CAccelTable::Create( ACCEL keys[], int count )
{
	if ( m_hAccel != NULL )
		::DestroyAcceleratorTable( m_hAccel );

	m_hAccel = ::CreateAcceleratorTable( const_cast<ACCEL*>( keys ), count );
	ASSERT_PTR( m_hAccel );
}

void CAccelTable::Augment( UINT accelId )
{
	if ( NULL == m_hAccel )
		Load( accelId );
	else
	{
		std::vector< ACCEL > keys;
		QueryKeys( keys );
		CAccelTable( accelId ).QueryKeys( keys );

		Create( ARRAY_PAIR_V( keys ) );
	}
}

void CAccelTable::Augment( ACCEL keys[], int count )
{
	if ( NULL == m_hAccel )
		Create( keys, count );
	else
	{
		std::vector< ACCEL > keys;
		QueryKeys( keys );
		keys.insert( keys.end(), &keys[ 0 ], &keys[ count ] );

		Create( ARRAY_PAIR_V( keys ) );
	}
}

bool CAccelTable::Translate( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus /*= NULL*/ ) const
{
	if ( m_hAccel != NULL && IsKeyMessage( pMsg ) )
		if ( NULL == hCondFocus || GetFocus() == hCondFocus )
			if ( ::TranslateAccelerator( hTargetWnd, m_hAccel, pMsg ) )
				return true;

	return false;
}

bool CAccelTable::TranslateIfOwnsFocus( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus ) const
{
	if ( m_hAccel != NULL && IsKeyMessage( pMsg ) )
		if ( NULL == hCondFocus || ui::OwnsFocus( hCondFocus ) )
			if ( ::TranslateAccelerator( hTargetWnd, m_hAccel, pMsg ) )
				return true;

	return false;
}
