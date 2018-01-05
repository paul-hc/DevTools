#ifndef BaseStatic_h
#define BaseStatic_h
#pragma once


class CBaseStatic : public CStatic
{
protected:
	CBaseStatic( void );
public:
	virtual ~CBaseStatic();

	UINT GetDrawTextFlags( void ) const { return m_dtFlags; }
	void SetDrawTextFlags( UINT dtFlags ) { m_dtFlags = dtFlags; }
protected:
	bool UseMouseInput( void ) const { return HasFlag( GetStyle(), SS_NOTIFY ); }

	// overridables
	virtual bool HasCustomFacet( void ) const = 0;
	virtual void DrawBackground( CDC* pDC, const CRect& clientRect );
	virtual void Draw( CDC* pDC, const CRect& clientRect ) = 0;
protected:
	static CFont* GetMarlettFont( void );
private:
	void PaintImpl( CDC* pDC, const CRect& clientRect );
private:
	UINT m_dtFlags;
protected:
	// generated overrides
	public:
	virtual void PreSubclassWindow( void );
	protected:
	virtual void DrawItem( DRAWITEMSTRUCT* pDrawItem );
protected:
	// message map functions

	DECLARE_MESSAGE_MAP()
};


#endif // BaseStatic_h
