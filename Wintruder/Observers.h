#ifndef Observers_h
#define Observers_h
#pragma once


class CWndSpot;


interface IBaseObserver
{
	virtual bool CanNotify( void ) const
	{
		const CWnd* pObserverWnd = dynamic_cast< const CWnd* >( this );
		return NULL == pObserverWnd || pObserverWnd->GetSafeHwnd() != NULL;		// observer is not a window or window is initialized
	}
};


interface IWndObserver : public IBaseObserver
{
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd ) = 0;
	virtual void OutputTargetWnd( void );
};


interface IWndDetailObserver : public IWndObserver
{
	virtual bool IsDirty( void ) const = 0;
};


namespace app
{
	enum Event { RefreshWndTree, UpdateTarget, DirtyChanged, OptionChanged, WndStateChanged };
}


interface IEventObserver : public IBaseObserver
{
	virtual void OnAppEvent( app::Event appEvent ) = 0;
};


#endif // Observers_h
