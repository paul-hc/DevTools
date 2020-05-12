#ifndef ResourcePool_h
#define ResourcePool_h
#pragma once

#include <vector>
#include "ContainerUtilities.h"


namespace utl
{
	// owns a number of shared resources, and controls their destruction; a composite itself
	// used in app::ExitInstance to release resources automatically

	class CResourcePool : public utl::IMemoryManaged
	{
	public:
		CResourcePool( void ) {}
		virtual ~CResourcePool() { std::for_each( m_pResources.rbegin(), m_pResources.rend(), func::Delete() ); }		// delete in reverse order of registration

		template< typename Type >
		void AddPointer( Type* pointer ) { m_pResources.push_back( new std::auto_ptr< Type >( pointer ) ); }

		template< typename Type >
		void AddAutoPtr( std::auto_ptr< Type >* pPtr ) { m_pResources.push_back( new CAutoPtrResource< Type >( pPtr ) ); }

		template< typename ComPtrType >
		void AddComPtr( ComPtrType& rComPtr ) { m_pResources.push_back( new CComPtrResource< ComPtrType >( rComPtr ) ); }

		template< typename ObjectType >
		void AddAutoClear( ObjectType* pClearable ) { m_pResources.push_back( new CAutoClearResource< ObjectType >( pClearable ) ); }
	private:
		std::vector< utl::IMemoryManaged* > m_pResources;
	};


	template< typename Type >
	class CAutoPtrResource : public utl::IMemoryManaged
	{
	public:
		CAutoPtrResource( std::auto_ptr< Type >* pPtr ) : m_pPtr( pPtr ) { ASSERT_PTR( m_pPtr ); }
		virtual ~CAutoPtrResource() { m_pPtr->reset(); }
	private:
		std::auto_ptr< Type >* m_pPtr;
	};


	template< typename ComPtrType >
	class CComPtrResource : public utl::IMemoryManaged
	{
	public:
		CComPtrResource( ComPtrType& rComPtr ) : m_rComPtr( rComPtr ) {}
		virtual ~CComPtrResource() { m_rComPtr = NULL; }
	private:
		ComPtrType& m_rComPtr;
	};


	template< typename ObjectType >
	class CAutoClearResource : public utl::IMemoryManaged
	{
	public:
		CAutoClearResource( ObjectType* pClearable ) : m_pClearable( pClearable ) { ASSERT_PTR( pClearable ); }
		virtual ~CAutoClearResource() { m_pClearable->Clear(); }
	private:
		ObjectType* m_pClearable;
	};
}


#endif // ResourcePool_h
