#ifndef AppService_h
#define AppService_h
#pragma once

#include <deque>
#include "Options.h"
#include "wnd/WndSpot.h"


interface IWndObserver;
interface IEventObserver;
class CEnumTags;
class CLogger;
namespace app { enum Event; }


enum { HotFieldColor = color::Blue, StaleWndColor = color::Red, MergeInsertWndColor = color::Green, SlowWndColor = color::html::PaleVioletRed2 };


class CAppService
{
	CAppService( void );
	~CAppService();
public:
	static CAppService& Instance( void );

	COptions* GetOptions( void ) { return &m_options; }

	CWndSpot& GetTargetWnd( void ) { return m_targetWnd; }
	void SetTargetWnd( const CWndSpot& targetWnd, IWndObserver* pSender = NULL );

	void AddObserver( CWnd* pObserver );
	void RemoveObserver( CWnd* pObserver );

	void PublishEvent( app::Event appEvent, IEventObserver* pSender = NULL );

	bool HasDirtyDetails( void ) const { return m_dirtyDetails; }
	bool SetDirtyDetails( bool dirtyDetails = true );
private:
	COptions m_options;
	CWndSpot m_targetWnd;			// no subclassing, no ownership
	bool m_dirtyDetails;

	std::deque< IWndObserver* > m_wndObservers;
	std::deque< IEventObserver* > m_eventObservers;
};


namespace app
{
	inline CAppService& GetSvc( void ) { return CAppService::Instance(); }
	inline COptions* GetOptions( void ) { return GetSvc().GetOptions(); }

	inline CWndSpot& GetTargetWnd( void ) { return GetSvc().GetTargetWnd(); }
	inline bool IsValidTargetWnd( void ) { return GetTargetWnd().IsValid(); }

	enum Feedback { Silent, Beep, Report };

	bool CheckValidTargetWnd( Feedback feedback = Report );
	inline CWndSpot* GetValidTargetWnd( Feedback feedback = Silent ) { return CheckValidTargetWnd( feedback ) ? &GetTargetWnd() : NULL; }
}


#endif // AppService_h
