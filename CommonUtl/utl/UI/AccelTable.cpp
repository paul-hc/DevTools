
#include "stdafx.h"
#include "AccelTable.h"
#include "WndUtils.h"

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
	if ( m_hAccel != nullptr )
		::DestroyAcceleratorTable( m_hAccel );
}

void CAccelTable::QueryKeys( std::vector< ACCEL >& rKeys ) const
{
	if ( m_hAccel != nullptr )
	{
		int count = ::CopyAcceleratorTable( m_hAccel, nullptr, 0 );
		int oldCount = static_cast<int>( rKeys.size() );

		rKeys.resize( rKeys.size() + count );			// allocate stoarge keys
		VERIFY( count == ::CopyAcceleratorTable( m_hAccel, &rKeys[ oldCount ], count ) );
	}
}

void CAccelTable::Load( UINT accelId )
{
	if ( m_hAccel != nullptr )
		::DestroyAcceleratorTable( m_hAccel );

	m_hAccel = ::LoadAccelerators( CScopedResInst::Get(), MAKEINTRESOURCE( accelId ) );
	ASSERT_PTR( m_hAccel );
}

bool CAccelTable::LoadOnce( UINT accelId )
{
	if ( m_hAccel != nullptr )
		return false;				// already loaded

	Load( accelId );
	return true;
}

void CAccelTable::Create( ACCEL keys[], int count )
{
	if ( m_hAccel != nullptr )
		::DestroyAcceleratorTable( m_hAccel );

	m_hAccel = ::CreateAcceleratorTable( const_cast<ACCEL*>( keys ), count );
	ASSERT_PTR( m_hAccel );
}

void CAccelTable::Augment( UINT accelId )
{
	if ( nullptr == m_hAccel )
		Load( accelId );
	else
	{
		std::vector<ACCEL> allKeys;
		QueryKeys( allKeys );
		CAccelTable( accelId ).QueryKeys( allKeys );

		Create( ARRAY_SPAN_V( allKeys ) );
	}
}

void CAccelTable::Augment( ACCEL keys[], int count )
{
	if ( nullptr == m_hAccel )
		Create( keys, count );
	else
	{
		std::vector<ACCEL> allKeys;
		QueryKeys( allKeys );
		allKeys.insert( allKeys.end(), &keys[ 0 ], &keys[ count ] );

		Create( ARRAY_SPAN_V( allKeys ) );
	}
}

bool CAccelTable::Translate( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus /*= nullptr*/ ) const
{
	if ( m_hAccel != nullptr && IsKeyMessage( pMsg ) )
		if ( nullptr == hCondFocus || GetFocus() == hCondFocus )
			if ( ::TranslateAccelerator( hTargetWnd, m_hAccel, pMsg ) )
				return true;

	return false;
}

bool CAccelTable::TranslateIfOwnsFocus( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus ) const
{
	if ( m_hAccel != nullptr && IsKeyMessage( pMsg ) )
		if ( nullptr == hCondFocus || ui::OwnsFocus( hCondFocus ) )
			if ( ::TranslateAccelerator( hTargetWnd, m_hAccel, pMsg ) )
				return true;

	return false;
}
