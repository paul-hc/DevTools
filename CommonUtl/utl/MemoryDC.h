#ifndef MemoryDC_h
#define MemoryDC_h
#pragma once


// double-buffer painting support, can be conditionally disabled;
// compatible interface with CMemDC defined in VC12\atlmfc\include\afxcontrolbarutil.h

class CMemoryDC
{
public:
	CMemoryDC( CDC& dc, bool useMemDC = true );							// uses ClipBox
	CMemoryDC( CDC& dc, const CRect& rect, bool useMemDC = true );
	virtual ~CMemoryDC();

	bool IsMemDC( void ) const { return m_memDC.GetSafeHdc() != NULL; }
	CDC& GetDC( void ) { return IsMemDC() ? m_memDC : m_rDC; }
private:
	void Construct( void );
protected:
	CDC& m_rDC;
	CRect m_sourceRect;			// rect for source bitmap in memory DC
	CDC m_memDC;
	CBitmap m_bitmap;
	CBitmap* m_pOldBitmap;
};


#endif // MemoryDC_h
