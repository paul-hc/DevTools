
#include "stdafx.h"
#include "ResourcePool.h"
#include "ContainerOwnership.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	CResourcePool::~CResourcePool()
	{
		std::for_each( m_pResources.rbegin(), m_pResources.rend(), func::Delete() );	// delete in reverse order of registration
	}
}
