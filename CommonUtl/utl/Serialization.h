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
	// serialize a container of scalars with predefined CArchive& insertor and extractor
	//
	template< typename Container >
	void SerializeValues( CArchive& archive, Container& rItems )
	{
		if ( archive.IsStoring() )
		{
			archive << rItems.size();
			// save each element in the vector
			for ( Container::const_iterator it = rItems.begin(); it != rItems.end(); ++it )
				archive << *it;
		}
		else
		{
			Container::size_type size = 0;

			archive >> size;
			rItems.resize( size );
			// load each element in the vector
			for ( Container::iterator it = rItems.begin(); it != rItems.end(); ++it )
				archive >> *it;
		}
	}


	// serialize container of pointers to CObject-based serializable dynamic objects
	//
	template< typename Container >
	void SerializeObjects( CArchive& archive, Container& rObjects )
	{
		// NOTE: must use WriteCount() and ReadCount() for backwards compatibility
		if ( archive.IsStoring() )
		{
			archive.WriteCount( static_cast< DWORD_PTR >( rObjects.size() ) );

			for ( Container::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
				archive << *itPtr;
		}
		else
		{
			utl::ClearOwningContainer( rObjects );			// delete existing owned objects
			rObjects.resize( archive.ReadCount() );

			for ( Container::iterator itPtr = rObjects.begin(); itPtr != rObjects.end(); ++itPtr )
				archive >> *itPtr;
		}
	}


	// serialize a container of scalar objects with method: void Type::Stream( CArchive& archive )
	//
	template< typename Container >
	void StreamItems( CArchive& archive, Container& rItems )
	{
		if ( archive.IsStoring() )
			archive << rItems.size();
		else
		{
			Container::size_type size = 0;

			archive >> size;
			rItems.resize( size );
		}
		// serialize (store/load) each element in the vector
		for ( Container::iterator it = rItems.begin(); it != rItems.end(); ++it )
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


inline CArchive& operator<<( CArchive& archive, const std::string& ansiStr )
{
	// as CStringA
	return archive << CStringA( ansiStr.c_str() );
}

inline CArchive& operator>>( CArchive& archive, std::string& rAnsiStr )
{
	// as CStringA
	CStringA ansiStr;
	archive >> ansiStr;
	rAnsiStr = ansiStr.GetString();
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


#endif // Serialization_h