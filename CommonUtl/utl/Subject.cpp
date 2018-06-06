
#include "stdafx.h"
#include "Subject.h"
#include "ContainerUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSubject::~CSubject()
{
}

void CSubject::AddObserver( utl::IObserver* pObserver )
{
	ASSERT_PTR( pObserver );
	ASSERT( !utl::Contains( m_observers, pObserver ) );

	m_observers.push_back( pObserver );
}

void CSubject::RemoveObserver( utl::IObserver* pObserver )
{
	ASSERT_PTR( pObserver );

	std::vector< utl::IObserver* >::iterator itFound = std::find( m_observers.begin(), m_observers.end(), pObserver );
	if ( itFound != m_observers.end() )
		m_observers.erase( itFound );
	else
		TRACE( _T(" # Observer %s already removed from subject %s.\n"), str::GetTypeName( typeid( *pObserver ) ).c_str(), str::GetTypeName( typeid( *this ) ).c_str() );
}

void CSubject::UpdateAllObservers( utl::IMessage* pMessage )
{
	for ( std::vector< utl::IObserver* >::const_iterator itObserver = m_observers.begin(); itObserver != m_observers.end(); ++itObserver )
		( *itObserver )->OnUpdate( this, pMessage );
}
