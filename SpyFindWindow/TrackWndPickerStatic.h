#ifndef TrackWndPickerStatic_h
#define TrackWndPickerStatic_h
#pragma once

#include "utl/TrackStatic.h"


class CWndSpot;
class CWndHighlighter;


#define ON_TSWN_FOUNDWINDOW( id, memberFxn ) ON_CONTROL( CTrackWndPickerStatic::TSWN_FOUNDWINDOW, id, memberFxn )


class CTrackWndPickerStatic : public CTrackStatic
{
public:
	enum NotifyCode { TSWN_FOUNDWINDOW = _TSN_LAST };

	CTrackWndPickerStatic( void );
	virtual ~CTrackWndPickerStatic();

	const CWndSpot& GetSelectedWnd( void ) const;
protected:
	// base overrides
	virtual void EndTracking( void );
	virtual void OnTrack( void );
private:
	std::auto_ptr< CWndHighlighter > m_pWndHighlighter;
};


#endif // TrackWndPickerStatic_h
