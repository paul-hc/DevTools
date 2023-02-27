#ifndef ContainerOwnership_h
#define ContainerOwnership_h
#pragma once

#include <type_traits>		// for std::tr1::remove_pointer
#include <algorithm>


namespace func
{
	/*
		usage:
			std::vector<Element*> elements;
			std::for_each( elements.begin(), elements.end(), func::Delete() );
		or
			utl::for_each( elements, func::Delete() );
	*/
	struct Delete
	{
		template< typename Type >
		void operator()( Type* pObject ) const
		{
			delete pObject;
		}
	};

	struct DeleteFirst
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			delete const_cast<typename PairType::first_type>( rPair.first );
		}
	};


	struct DeleteSecond
	{
		template< typename PairType >
		void operator()( const PairType& rPair ) const
		{
			delete rPair.second;
		}
	};

	struct ReleaseCom
	{
		template< typename InterfaceT >
		void operator()( InterfaceT* pInterface ) const
		{
			pInterface->Release();
		}
	};


	// std::map aliases
	typedef DeleteFirst TDeleteKey;
	typedef DeleteSecond TDeleteValue;
}


namespace utl
{
	template< typename Type, typename SmartPtrType >
	inline bool ResetPtr( SmartPtrType& rPtr, Type* pObject ) { rPtr.reset( pObject ); return pObject != nullptr; }

	template< typename SmartPtrType >
	inline bool ResetPtr( SmartPtrType& rPtr ) { rPtr.reset( nullptr ); return false; }


	/**
		object ownership helpers - for containers of pointers with ownership
	*/

	template< typename PtrType >
	inline PtrType* ReleaseOwnership( PtrType*& rPtr )		// release item ownership in owning containers of pointers
	{
		PtrType* ptr = rPtr;
		rPtr = nullptr;				// mark detached item as NULL to prevent being deleted
		return ptr;
	}


	template< typename PtrContainerT >
	void ClearOwningContainer( PtrContainerT& rContainer )
	{
		std::for_each( rContainer.begin(), rContainer.end(), func::Delete() );
		rContainer.clear();
	}

	template< typename PtrContainerT >
	void CreateOwningContainerObjects( PtrContainerT& rItemPtrs, size_t count )		// using default constructor
	{
		ClearOwningContainer( rItemPtrs );		// delete existing items

		typedef typename std::tr1::remove_pointer<typename PtrContainerT::value_type>::type TItem;

		rItemPtrs.reserve( count );
		for ( size_t i = 0; i != count; ++i )
			rItemPtrs.push_back( new TItem() );
	}

	template< typename PtrContainerT >
	void CloneOwningContainerObjects( PtrContainerT& rItemPtrs, const PtrContainerT& srcItemPtrs )		// using copy constructor
	{
		ClearOwningContainer( rItemPtrs );		// delete existing items

		typedef typename std::tr1::remove_pointer<typename PtrContainerT::value_type>::type TItem;

		rItemPtrs.reserve( srcItemPtrs.size() );
		for ( typename PtrContainerT::const_iterator itSrcItem = srcItemPtrs.begin(); itSrcItem != srcItemPtrs.end(); ++itSrcItem )
			rItemPtrs.push_back( new TItem( **itSrcItem ) );
	}


	template< typename PtrContainer, typename ClearFunctor >
	void ClearOwningContainer( PtrContainer& rContainer, ClearFunctor clearFunctor )
	{
		std::for_each( rContainer.begin(), rContainer.end(), clearFunctor );
		rContainer.clear();
	}

	template< typename MapType >
	void ClearOwningMap( MapType& rObjectToObjectMap )		// i.e. map-like associative container
	{
		for ( typename MapType::iterator it = rObjectToObjectMap.begin(); it != rObjectToObjectMap.end(); ++it )
		{
			delete it->first;
			delete it->second;
		}

		rObjectToObjectMap.clear();
	}

	template< typename MapType >
	void ClearOwningMapKeys( MapType& rObjectToValueMap )	// i.e. map-like associative container
	{
		for ( typename MapType::iterator it = rObjectToValueMap.begin(); it != rObjectToValueMap.end(); ++it )
			delete it->first;

		rObjectToValueMap.clear();
	}

	template< typename MapType >
	void ClearOwningMapValues( MapType& rKeyToObjectMap )	// i.e. map-like associative container
	{
		for ( typename MapType::iterator it = rKeyToObjectMap.begin(); it != rKeyToObjectMap.end(); ++it )
			delete it->second;

		rKeyToObjectMap.clear();
	}


	// exception-safe owning container of pointers; use swap() at the end to exchange safely the new items (old items will be deleted by this).
	//
	template< typename ContainerT, typename DeleteFunc = func::Delete >
	class COwningContainer : public ContainerT
	{
		using ContainerT::clear;
	public:
		COwningContainer( void ) : ContainerT() {}
		~COwningContainer() { clear(); }

		void clear( void ) { std::for_each( ContainerT::begin(), ContainerT::end(), DeleteFunc() ); Release(); }
		void Release( void ) { ContainerT::clear(); }
	};
}


#endif // ContainerOwnership_h
