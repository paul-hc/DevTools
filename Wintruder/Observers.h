#ifndef Observers_h
#define Observers_h
#pragma once


class CWndSpot;


interface IBaseObserver
{
	virtual bool CanNotify( void ) const
	{
		const CWnd* pObserverWnd = dynamic_cast<const CWnd*>( this );
		return nullptr == pObserverWnd || pObserverWnd->GetSafeHwnd() != nullptr;		// observer is not a window or window is initialized
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
	enum Event
	{
		RefreshWndTree,			// refresh all windows
		RefreshBranch,			// refresh branch of the current node + all child windows
		RefreshSiblings,		// refresh parent branch of the current node (all sibling windows)
		UpdateTarget,
		DirtyChanged,
		OptionChanged,
		ToggleAutoUpdate,
		WndStateChanged			// selected window changed
	};
}


interface IEventObserver : public IBaseObserver
{
	virtual void OnAppEvent( app::Event appEvent ) = 0;
};


#endif // Observers_h
