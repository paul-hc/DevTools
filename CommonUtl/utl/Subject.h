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

	// protect observers on assignment
	CSubject( const CSubject& right ) { right; }
	CSubject& operator=( const CSubject& right ) { right; return *this; }		// protect observers on assign

	// utl::ISubject interface (partial)
	virtual void AddObserver( utl::IObserver* pObserver );
	virtual void RemoveObserver( utl::IObserver* pObserver );
	virtual void UpdateAllObservers( utl::IMessage* pMessage );
private:
	std::vector< utl::IObserver* > m_observers;
};


#endif // Subject_h
