#ifndef WndHighlighter_h
#define WndHighlighter_h
#pragma once

#include "WndSpot.h"
#include "utl/WindowTimer.h"


namespace opt { enum FrameStyle; }


// highlights using XOR inversion; destructor will redraw the entire desktop

class CWndHighlighter : public ISequenceTimerCallback
{
public:
	CWndHighlighter( bool finalRedraw = m_redrawAtEnd ) : m_finalRedraw( finalRedraw ), m_drawCount( 0 ) {}
	~CWndHighlighter();

	const CWndSpot& GetSelected( void ) const { return m_wndSpot; }
	bool SetSelected( const CWndSpot& wndSpot );

	bool IsDirty( void ) const { return m_drawCount != 0; }
	bool IsDrawn( void ) const { return IsDirty() && ( m_drawCount % 2 ) != 0; }
	static void DrawWindowFrame( CDC* pDC, const CWndSpot& wndSpot );		// draws using XOR inversion; subsequent calls will invert back the previously drawn window

	// flash highlight
	enum { FlashTimerEvent = 7654 };

	void FlashWnd( HWND hOwnerWnd, const CWndSpot& wndSpot, unsigned int count = 4, int elapse = 150 );

	// ISequenceTimerCallback interface
	virtual void OnSequenceStep( void );
private:
	enum { FillColor = RGB( 255, 0, 0 ) };

	void DrawWindowFrame( void );
	void RedrawDirtyWindows( void );
private:
	bool m_finalRedraw;
	CWndSpot m_wndSpot;
	std::vector< HWND > m_dirtyWnds;		// keeps track of highlighted windows (to be repainted at the end)
	std::auto_ptr< CDC > m_pScreenDC;		// lazy create
	unsigned int m_drawCount;
public:
	static bool m_cacheDesktopDC;			// keep desktop DC alive during a tracking transaction
	static bool m_redrawAtEnd;				// redraw desktop at the end of a tracking transaction
};


#endif // WndHighlighter_h
