#ifndef WindowPlacement_h
#define WindowPlacement_h
#pragma once


class CWindowPlacement : public tagWINDOWPLACEMENT
{
public:
	CWindowPlacement( void );
	~CWindowPlacement();

	bool GetPlacement( const CWnd* pWnd );
	bool SetPlacement( CWnd* pWnd, bool restoreToMax = false, bool setMinPos = false ) const;

	int ChangeMaximizedShowCmd( UINT showCmd );

	// serialization
	void Stream( CArchive& archive );

	friend inline CArchive& operator>>( CArchive& archive, CWindowPlacement& rWp ) { rWp.Stream( archive ); return archive; }
	friend inline CArchive& operator<<( CArchive& archive, const CWindowPlacement& wp ) { const_cast< CWindowPlacement& >( wp ).Stream( archive ); return archive; }
};


#endif // WindowPlacement_h
