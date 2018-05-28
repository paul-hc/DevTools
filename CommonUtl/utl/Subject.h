#ifndef Subject_h
#define Subject_h
#pragma once

#include "ISubject.h"


// standard implementation for utl::ISubject
abstract class CSubject : public utl::ISubject
{
protected:
	CSubject( void ) {}
public:
	virtual ~CSubject();

	// utl::ISubject interface (partial)
	virtual void AddObserver( utl::IObserver* pObserver );
	virtual void RemoveObserver( utl::IObserver* pObserver );
	virtual void UpdateAllObservers( utl::IMessage* pMessage );
private:
	std::vector< utl::IObserver* > m_observers;
};


#endif // Subject_h
