#ifndef Serialization_h
#define Serialization_h
#pragma once

#include "Serialization_fwd.h"


namespace serial
{
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
	// *** MFC - using CObject based objects ***

	// for serializing pointers to types dynamically castable to CObject
	//

	template< typename ObjectType >
	void Save_CObject_Dynamic( CArchive& archive, const ObjectType* pObject )
	{
		const CObject* pSerialObject = dynamic_cast<const CObject*>( pObject );
		archive << pSerialObject;
	}

	template< typename ObjectType >
	void Load_CObject_Dynamic( CArchive& archive, ObjectType*& rpObject )
	{
		CObject* pSerialObject = nullptr;
		archive >> pSerialObject;

		rpObject = dynamic_cast<ObjectType*>( pSerialObject );
	}

	template< typename ObjectType >
	inline void Serialize_CObject_Dynamic( CArchive& archive, ObjectType*& rpObject )
	{
		if ( archive.IsStoring() )
			Save_CObject_Dynamic( archive, rpObject );
		else
			Load_CObject_Dynamic( archive, rpObject );
	}


	// serialize container of pointers to CObject-based SERIALIZABLE DYNAMIC objects
	//

	template< typename PtrContainerT >
	void Save_CObjects_Dynamic( CArchive& archive, const PtrContainerT& objects );

	template< typename PtrContainerT >
	void Load_CObjects_Dynamic( CArchive& archive, PtrContainerT& rObjects );

	template< typename PtrContainerT >
	inline void Serialize_CObjects_Dynamic( CArchive& archive, PtrContainerT& rObjects )
	{
		if ( archive.IsStoring() )
			Save_CObjects_Dynamic( archive, rObjects );
		else
			Load_CObjects_Dynamic( archive, rObjects );
	}


	// serialize container of heterogeneous pointers:
	//	- some of which are CObject-based serializable dynamic objects
	//	- some are not serializable
	//

	template< typename PtrContainerT >
	void Save_CObjects_Mixed( CArchive& archive, const PtrContainerT& objects );

	template< typename PtrContainerT >
	void Load_CObjects_Mixed( CArchive& archive, PtrContainerT& rObjects );

	template< typename PtrContainerT >
	inline void Serialize_CObjects_Mixed( CArchive& archive, PtrContainerT& rObjects )
	{
		if ( archive.IsStoring() )
			Save_CObjects_Mixed( archive, rObjects );
		else
			Load_CObjects_Mixed( archive, rObjects );
	}
}


namespace serial
{
	// serialize a container of scalars with predefined CArchive insertor and extractor
	//

	template< typename ContainerT >
	void SaveValues( CArchive& archive, const ContainerT& items )
	{
		archive << static_cast<::portable::size_t>( items.size() );
		for ( typename ContainerT::const_iterator it = items.begin(); it != items.end(); ++it )
			archive << *it;			// save by value
	}

	template< typename ContainerT >
	void LoadValues( CArchive& archive, ContainerT& rItems )
	{
		::portable::size_t count = 0;
		archive >> count;

		rItems.resize( count );
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
}


namespace serial
{
	// serialize ContainerT<Type> - container of scalar objects with method: void Type::Stream( CArchive& archive )
	//
	template< typename ContainerT >
	void StreamItems( CArchive& archive, ContainerT& rItems )
	{
		if ( archive.IsStoring() )
			archive << static_cast<::portable::size_t>( rItems.size() );
		else
		{
			::portable::size_t count = 0;

			archive >> count;
			rItems.resize( count );
		}

		// stream (store/load) each item in the vector
		for ( typename ContainerT::iterator it = rItems.begin(); it != rItems.end(); ++it )
			it->Stream( archive );
	}


	template< typename PtrContainerT >
	void SaveOwningPtrs( CArchive& archive, const PtrContainerT& rItemPtrs )
	{
		archive << static_cast<::portable::size_t>( rItemPtrs.size() );

		// store each item in container
		for ( typename PtrContainerT::const_iterator itItemPtr = rItemPtrs.begin(); itItemPtr != rItemPtrs.end(); ++itItemPtr )
			( *itItemPtr )->Stream( archive );
	}

	template< typename PtrContainerT >
	void LoadOwningPtrs( CArchive& archive, PtrContainerT& rItemPtrs );

	// serialize ContainerT<Type*> - owning container of pointers to objects with:
	//	- default constructor Type::Type()
	//	- method: void Type::Stream( CArchive& archive )
	// note: it has identical binary footprint as StreamItems()
	//
	template< typename PtrContainerT >
	inline void StreamOwningPtrs( CArchive& archive, PtrContainerT& rItemPtrs )
	{
		if ( archive.IsStoring() )
			SaveOwningPtrs( archive, rItemPtrs );
		else
			LoadOwningPtrs( archive, rItemPtrs );
	}


	// serialize a container of scalar objects with method: void Type::Stream( CArchive& archive )
	//
	template< typename Type >
	void StreamPtr( CArchive& archive, std::auto_ptr<Type>& rPtr )
	{
		if ( archive.IsStoring() )
		{
			if ( rPtr.get() != nullptr )
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
				if ( nullptr == rPtr.get() )
					rPtr.reset( new Type() );

				rPtr->Stream( archive );
			}
			else
				rPtr.reset();
		}
	}
}


namespace serial
{
	enum UnicodeEncoding { WideEncoding, Utf8Encoding };


	struct CPolicy
	{
		static void ToUtf8String( CStringA& rUtf8Str, const std::wstring& wideStr );
		static void FromUtf8String( std::wstring& rWideStr, const CStringA& utf8Str );
	public:
		static UnicodeEncoding s_strEncoding;
	};


	UnicodeEncoding InspectSavedStringEncoding( ::CArchive& rLoadArchive, size_t* pLength = nullptr );

	const BYTE* GetLoadingCursor( const ::CArchive& rLoadArchive );
	void UnreadBytes( ::CArchive& rLoadArchive, size_t bytes );

	template< typename ValueT >
	inline void UnreadValue( ::CArchive& rLoadArchive, ValueT& rValue ) { UnreadBytes( rLoadArchive, sizeof( rValue ) ); }
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


CArchive& operator<<( CArchive& archive, const std::wstring& wideStr );
CArchive& operator>>( CArchive& archive, std::wstring& rWideStr );

CArchive& operator<<( CArchive& archive, const std::wstring* pWideStr );		// Unicode string as UTF8 (for better readability in archive stream)
CArchive& operator>>( CArchive& archive, std::wstring* pOutWideStr );			// Unicode string as UTF8 (for better readability in archive stream)


template< typename FirstT, typename SecondT >
inline CArchive& operator<<( CArchive& archive, const std::pair<FirstT, SecondT>& srcPair )
{
	return archive << srcPair.first << srcPair.second;
}

template< typename FirstT, typename SecondT >
inline CArchive& operator>>( CArchive& archive, std::pair<FirstT, SecondT>& destPair )
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
