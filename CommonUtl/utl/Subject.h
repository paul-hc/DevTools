#ifndef Subject_h
#define Subject_h
#pragma once

#include "ISubject.h"


// standard implementation of utl::ISubject or a utl::ISubject-derived interface
//
template< typename ISubjectT >
abstract class CSubjectImpl : public ISubjectT
{
protected:
	CSubjectImpl( void ) {}
public:
	virtual ~CSubjectImpl()
	{
	}

	// protect observers on assignment
	CSubjectImpl( const CSubjectImpl& right ) { right; }
	CSubjectImpl& operator=( const CSubjectImpl& right ) { right; return *this; }

	// utl::ISubject interface (partial)

	virtual void AddObserver( utl::IObserver* pObserver )
	{
		ASSERT_PTR( pObserver );
		ASSERT( std::find( m_observers.begin(), m_observers.end(), pObserver ) == m_observers.end() );

		m_observers.push_back( pObserver );
	}

	virtual void RemoveObserver( utl::IObserver* pObserver )
	{
		ASSERT_PTR( pObserver );

		std::vector< utl::IObserver* >::iterator itFound = std::find( m_observers.begin(), m_observers.end(), pObserver );
		if ( itFound != m_observers.end() )
			m_observers.erase( itFound );
		else
			TRACE( _T(" # Observer %s already removed from subject %s.\n"), str::GetTypeName( typeid( *pObserver ) ).c_str(), str::GetTypeName( typeid( *this ) ).c_str() );
	}

	virtual void UpdateAllObservers( utl::IMessage* pMessage )
	{
		for ( std::vector< utl::IObserver* >::const_iterator itObserver = m_observers.begin(); itObserver != m_observers.end(); ++itObserver )
			( *itObserver )->OnUpdate( this, pMessage );
	}
private:
	std::vector< utl::IObserver* > m_observers;
};


typedef CSubjectImpl<utl::ISubject> TSubject;


#endif // Subject_h
