#ifndef Serialization_hxx
#define Serialization_hxx

#include "Serialization.h"
#include "ContainerOwnership.h"


namespace serial
{
	// serialize container of pointers to CObject-based SERIALIZABLE DYNAMIC objects
	//

	template< typename PtrContainerT >
	void Save_CObjects_Dynamic( CArchive& archive, const PtrContainerT& objects )
	{
		archive.WriteCount( static_cast<DWORD_PTR>( rObjects.size() ) );			// WriteCount() for backwards compatibility

		for ( typename PtrContainerT::iterator itPtr = objects.begin(); itPtr != objects.end(); ++itPtr )
			archive << *itPtr;
	}

	template< typename PtrContainerT >
	void Load_CObjects_Dynamic( CArchive& archive, PtrContainerT& rObjects )
	{
		utl::ClearOwningContainer( rObjects );			// delete existing owned objects
		rObjects.resize( archive.ReadCount() );			// ReadCount() for backwards compatibility

		for ( typename PtrContainerT::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
			archive >> *itPtr;
	}


	// serialize container of heterogeneous pointers:
	//	- some of which are CObject-based serializable dynamic objects
	//	- some are not serializable
	//

	template< typename PtrContainerT >
	void Save_CObjects_Mixed( CArchive& archive, const PtrContainerT& objects )
	{
		size_t mfcSerialCount = std::count_if( objects.begin(), objects.end(), pred::IsA< CObject >() );
		archive.WriteCount( static_cast<DWORD_PTR>( mfcSerialCount ) );			// WriteCount() for backwards compatibility

		for ( typename PtrContainerT::const_iterator itPtr = objects.begin(); itPtr != objects.end(); ++itPtr )
			if ( CObject* pSerialObject = dynamic_cast<CObject*>( *itPtr ) )
				archive << pSerialObject;
	}

	template< typename PtrContainerT >
	void Load_CObjects_Mixed( CArchive& archive, PtrContainerT& rObjects )
	{
		utl::ClearOwningContainer( rObjects );			// delete existing owned objects
		rObjects.resize( archive.ReadCount() );			// ReadCount() for backwards compatibility

		for ( typename PtrContainerT::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
		{
			CObject* pSerialObject = NULL;
			archive >> pSerialObject;

			*itPtr = dynamic_cast<typename PtrContainerT::value_type>( pSerialObject );
		}
	}
}


namespace serial
{
	// serialize ContainerT<Type> - container of scalar objects with method: void Type::Stream( CArchive& archive )

	template< typename PtrContainerT >
	void LoadOwningPtrs( CArchive& archive, PtrContainerT& rItemPtrs )
	{
		::portable::size_t count = 0;

		archive >> count;
		utl::CreateOwningContainerObjects( rItemPtrs, count );

		// load each new item in container
		for ( typename PtrContainerT::const_iterator itItemPtr = rItemPtrs.begin(); itItemPtr != rItemPtrs.end(); ++itItemPtr )
			(*itItemPtr)->Stream( archive );
	}
}


#endif // Serialization_hxx
