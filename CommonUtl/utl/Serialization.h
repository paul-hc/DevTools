#ifndef Serialization_h
#define Serialization_h
#pragma once

#include "ContainerUtilities.h"


namespace serial
{
	interface IStreamable
	{
		virtual void Save( CArchive& archive ) throws_( CException* ) = 0;
		virtual void Load( CArchive& archive ) throws_( CException* ) = 0;
	};


	// adapts a class that implements "void Serialize( CArchive& archive )" to a IStreamable interface
	//
	template< typename BaseClass >
	abstract class CStreamableAdapter : public BaseClass, public IStreamable
	{
	protected:
		CStreamableAdapter( void ) : BaseClass() {}
	public:
		void Serialize( CArchive& archive )
		{
			if ( archive.IsLoading() )
				Load( archive );
			else
				Save( archive );
		}
	};
}


namespace serial
{
	// for serializing pointers to types different than CObject-based, but are dynamically castable to CObject
	//

	template< typename ObjectType >
	void Save_CObject( CArchive& archive, const ObjectType* pObject )
	{
		const CObject* pSerialObject = dynamic_cast< const CObject* >( pObject );
		archive << pSerialObject;
	}

	template< typename ObjectType >
	void Load_CObject( CArchive& archive, ObjectType*& rpObject )
	{
		CObject* pSerialObject = NULL;
		archive >> pSerialObject;

		rpObject = dynamic_cast< ObjectType* >( pSerialObject );
	}

	template< typename ObjectType >
	inline void Serialize_CObject( CArchive& archive, ObjectType*& rpObject )
	{
		if ( archive.IsStoring() )
			Save_CObject( archive, rpObject );
		else
			Load_CObject( archive, rpObject );
	}


	// serialize a container of scalars with predefined CArchive& insertor and extractor
	//

	template< typename ContainerT >
	void SaveValues( CArchive& archive, const ContainerT& items )
	{
		archive << items.size();
		for ( typename ContainerT::const_iterator it = items.begin(); it != items.end(); ++it )
			archive << *it;			// save by value
	}

	template< typename ContainerT >
	void LoadValues( CArchive& archive, ContainerT& rItems )
	{
		ContainerT::size_type size = 0;
		archive >> size;

		rItems.resize( size );
		for ( typename ContainerT::iterator it = rItems.begin(); it != rItems.end(); ++it )
			archive >> *it;			// load by value
	}

	template< typename ContainerT >
	inline void SerializeValues( CArchive& archive, ContainerT& rItems )
	{
		if ( archive.IsStoring() )
			SaveValues( archive, rItems );
		else
			LoadValues( archive, rItems );
	}


	// serialize container of pointers to CObject-based serializable dynamic objects
	//

	template< typename ContainerT >
	void SaveObjects( CArchive& archive, const ContainerT& objects )
	{
		archive.WriteCount( static_cast< DWORD_PTR >( rObjects.size() ) );			// WriteCount() for backwards compatibility

		for ( typename ContainerT::iterator itPtr = objects.begin(); itPtr != objects.end(); ++itPtr )
			archive << *itPtr;
	}

	template< typename ContainerT >
	void LoadObjects( CArchive& archive, ContainerT& rObjects )
	{
		utl::ClearOwningContainer( rObjects );			// delete existing owned objects
		rObjects.resize( archive.ReadCount() );			// ReadCount() for backwards compatibility

		for ( typename ContainerT::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
			archive >> *itPtr;
	}

	template< typename ContainerT >
	inline void SerializeObjects( CArchive& archive, ContainerT& rObjects )
	{
		if ( archive.IsStoring() )
			SaveObjects( archive, rObjects );
		else
			LoadObjects( archive, rObjects );
	}


	// serialize container of pointers, some of which are CObject-based serializable dynamic objects and some are not serializable
	//

	template< typename ContainerT >
	void Save_CObjects( CArchive& archive, const ContainerT& objects )
	{
		size_t mfcSerialCount = std::count_if( objects.begin(), objects.end(), pred::IsA< CObject >() );
		archive.WriteCount( static_cast< DWORD_PTR >( mfcSerialCount ) );			// WriteCount() for backwards compatibility

		for ( typename ContainerT::const_iterator itPtr = objects.begin(); itPtr != objects.end(); ++itPtr )
			if ( CObject* pSerialObject = dynamic_cast< CObject* >( *itPtr ) )
				archive << pSerialObject;
	}

	template< typename ContainerT >
	void Load_CObjects( CArchive& archive, ContainerT& rObjects )
	{
		utl::ClearOwningContainer( rObjects );			// delete existing owned objects
		rObjects.resize( archive.ReadCount() );			// ReadCount() for backwards compatibility

		for ( typename ContainerT::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
		{
			CObject* pSerialObject = NULL;
			archive >> pSerialObject;

			*itPtr = dynamic_cast< typename ContainerT::value_type >( pSerialObject );
		}
	}

	template< typename ContainerT >
	inline void Serialize_CObjects( CArchive& archive, ContainerT& rObjects )
	{
		if ( archive.IsStoring() )
			Save_CObjects( archive, rObjects );
		else
			Load_CObjects( archive, rObjects );
	}


	// serialize a container of scalar objects with method: void Type::Stream( CArchive& archive )
	//
	template< typename ContainerT >
	void StreamItems( CArchive& archive, ContainerT& rItems )
	{
		if ( archive.IsStoring() )
			archive << rItems.size();
		else
		{
			ContainerT::size_type size = 0;

			archive >> size;
			rItems.resize( size );
		}
		// serialize (store/load) each element in the vector
		for ( typename ContainerT::iterator it = rItems.begin(); it != rItems.end(); ++it )
			it->Stream( archive );
	}


	// serialize a container of scalar objects with method: void Type::Stream( CArchive& archive )
	//
	template< typename Type >
	void StreamPtr( CArchive& archive, std::auto_ptr< Type >& rPtr )
	{
		if ( archive.IsStoring() )
		{
			if ( rPtr.get() != NULL )
			{
				archive << true;				// has ptr
				rPtr->Stream( archive );
			}
			else
				archive << false;				// null ptr
		}
		else
		{
			bool hasPtr;
			archive >> hasPtr;

			if ( hasPtr )
			{
				if ( NULL == rPtr.get() )
					rPtr.reset( new Type );
				rPtr->Stream( archive );
			}
			else
				rPtr.reset();
		}
	}
}


// standard archive insertors/extractors


inline CArchive& operator<<( CArchive& archive, const std::string& narrowStr )
{
	// as CStringA
	return archive << CStringA( narrowStr.c_str() );
}

inline CArchive& operator>>( CArchive& archive, std::string& rNarrowStr )
{
	// as CStringA
	CStringA narrowStr;
	archive >> narrowStr;
	rNarrowStr = narrowStr.GetString();
	return archive;
}


inline CArchive& operator<<( CArchive& archive, const std::wstring& wideStr )
{
	// as CStringW
	return archive << CStringW( wideStr.c_str() );
}

inline CArchive& operator>>( CArchive& archive, std::wstring& rWideStr )
{
	// as CStringW
	CStringW wideStr;
	archive >> wideStr;
	rWideStr = wideStr.GetString();
	return archive;
}

inline CArchive& operator<<( CArchive& archive, const std::wstring* pWideStr )		// Unicode string as UTF8 (for better readability in archive stream)
{
	ASSERT_PTR( pWideStr );
	return archive << CStringA( str::ToUtf8( pWideStr->c_str() ).c_str() );
}

inline CArchive& operator>>( CArchive& archive, std::wstring* pOutWideStr )			// Unicode string as UTF8 (for better readability in archive stream)
{
	ASSERT_PTR( pOutWideStr );
	CStringA utf8Str;
	archive >> utf8Str;
	*pOutWideStr = str::FromUtf8( utf8Str.GetString() );
	return archive;
}


template< typename Type1, typename Type2 >
inline CArchive& operator<<( CArchive& archive, const std::pair< Type1, Type2 >& srcPair )
{
	return archive << srcPair.first << srcPair.second;
}

template< typename Type1, typename Type2 >
inline CArchive& operator>>( CArchive& archive, std::pair< Type1, Type2 >& destPair )
{
	return archive >> destPair.first >> destPair.second;
}


namespace serial
{
	inline void StreamUtf8( CArchive& archive, std::wstring& rString )
	{
		if ( archive.IsStoring() )
			archive << &rString;
		else
			archive >> &rString;
	}
}


#endif // Serialization_h
