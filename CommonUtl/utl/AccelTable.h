#ifndef AccelTable_h
#define AccelTable_h
#pragma once


class CAccelTable
{
public:
	CAccelTable( void ) : m_hAccel( NULL ) {}
	CAccelTable( HACCEL hAccel ) : m_hAccel( hAccel ) { ASSERT_PTR( m_hAccel ); }
	CAccelTable( UINT accelId );
	CAccelTable( ACCEL keys[], int count );
	~CAccelTable();

	HACCEL GetAccel( void ) const { return m_hAccel; }
	void QueryKeys( std::vector< ACCEL >& rKeys ) const;		// additive

	void Load( UINT accelId );
	bool LoadOnce( UINT accelId );								// convenient for static data member initialization
	void Create( ACCEL keys[], int count );

	void Augment( UINT accelId );
	void Augment( ACCEL keys[], int count );

	bool Translate( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus = NULL ) const;
	bool TranslateIfOwnsFocus( MSG* pMsg, HWND hTargetWnd, HWND hCondFocus ) const;

	static bool IsKeyMessage( const MSG* pMsg ) { ASSERT_PTR( pMsg ); return pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST; }
private:
	HACCEL m_hAccel;
};


#endif // AccelTable_h
