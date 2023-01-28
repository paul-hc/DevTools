#ifndef WindowPlacement_h
#define WindowPlacement_h
#pragma once


class CWindowPlacement : public tagWINDOWPLACEMENT
{
public:
	CWindowPlacement( void );

	bool IsEmpty( void ) const;

	void Reset( void );
	void Setup( const CWnd* pWnd, const CRect& normalRect, UINT _showCmd, UINT _flags = 0 );
	void QueryCreateStruct( CREATESTRUCT* rCreateStruct ) const;

	bool ReadWnd( const CWnd* pWnd );
	bool CommitWnd( CWnd* pWnd, bool restoreToMax = false, bool setMinPos = false );

	bool IsRestoreToMaximized( void ) const { return HasFlag( this->flags, WPF_RESTORETOMAXIMIZED ); }
	int ChangeMaximizedShowCmd( UINT _showCmd );

	void RegSave( const TCHAR regSection[] ) const;
	bool RegLoad( const TCHAR regSection[], const CWnd* pWnd );

	const CRect& GetNormalPosition( void ) const { return (const CRect&)this->rcNormalPosition; }
	CRect& RefNormalPosition( void ) { return (CRect&)this->rcNormalPosition; }
	bool EnsureVisibleNormalPosition( const CWnd* pWnd );

	// serialization
	void Stream( CArchive& archive );

	friend inline CArchive& operator>>( CArchive& archive, CWindowPlacement& rWp ) { rWp.Stream( archive ); return archive; }
	friend inline CArchive& operator<<( CArchive& archive, const CWindowPlacement& wp ) { const_cast<CWindowPlacement&>( wp ).Stream( archive ); return archive; }
};


#endif // WindowPlacement_h
