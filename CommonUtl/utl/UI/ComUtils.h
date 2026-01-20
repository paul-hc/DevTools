#ifndef ComUtils_UTL_UI_h
#define ComUtils_UTL_UI_h
#pragma once

#include <propidl.h>		// for PROPVARIANT in <combaseapi.h>
#include <propvarutil.h>


namespace com
{
	struct CPropKeyName;	// FWD


	// IPropertyStore utils

	CComPtr<IPropertyStore> OpenFileProperties( const TCHAR* pFilePath, GETPROPERTYSTOREFLAGS flags = GPS_DEFAULT );	// could also pass GPS_BESTEFFORT
	bool QueryProperties( OUT std::vector<PROPERTYKEY>& rPropKeys, IPropertyStore* pPropertyStore, bool sortByCanName = true );
	bool QueryProperties( OUT std::vector<CPropKeyName>& rPropKeyNames, IPropertyStore* pPropertyStore, bool sortByCanName = true );

	void SortProperties( IN OUT std::vector<PROPERTYKEY>& rPropKeys );			// by Canonical Name
	void SortProperties( IN OUT std::vector<CPropKeyName>& rPropKeyNames );		// by Canonical Name


	// property utils

	std::tstring GetPropCanonicalName( const PROPERTYKEY& propKey );			// e.g. "System.DateAccessed"
	bool GetPropKeyFromCanonicalName( OUT PROPERTYKEY* pOutPropKey, const TCHAR* pCanonicalName );
	std::tstring FormatPropDisplayValue( const PROPERTYKEY& propKey, const PROPVARIANT& propValue, PROPDESC_FORMAT_FLAGS fmtFlags = PDFF_DEFAULT );		// e.g. "11-Jan-2026 12:50 AM"

	CComPtr<IPropertyDescription> GetPropertyDescription( const TCHAR* pPropCanonicalName );
	std::tstring GetPropertyLabel( IPropertyDescription* pPropDescr );			// e.g. "Date accessed"

	template< typename T >
	void CoTaskMemFreeArray( T** ppArray, ULONG cElem )
	{	// free memory allocated for VT_VECTOR/VT_ARRAY property variants
		ASSERT_PTR( ppArray );
		for ( ULONG i = 0; i != cElem; ++i )
			::CoTaskMemFree( ppArray[i] );		// free each element

		::CoTaskMemFree( ppArray );				// free the array of pointers
	}


	struct CPropKeyName		// pair of property key and canonical name
	{
		CPropKeyName( const PROPERTYKEY& propKey )
			: m_propKey( propKey )
			, m_canonicalName( com::GetPropCanonicalName( propKey ) )
		{
		}
	public:
		PROPERTYKEY m_propKey;
		std::tstring m_canonicalName;
	};
}


namespace fs { class CPath; }


namespace com
{
	// wrapper for a PROPVARIANT value
	//
	class CPropVariant
	{
	public:
		CPropVariant( void ) { ::PropVariantInit( &m_prop ); }
		~CPropVariant() { Clear(); }

		const PROPVARIANT& Get( void ) const { return m_prop; }
		PROPVARIANT* Clear( void ) { ::PropVariantClear( &m_prop ); return &m_prop; }		// also initializes the variant to zero, as does ::PropVariantInit()

		PROPVARIANT* operator&( void ) { return Clear(); }		// clear before passing variant to get new value

		bool IsEmpty( void ) const { return VT_EMPTY == m_prop.vt; }
		bool IsVector( void ) const { return HasFlag( m_prop.vt, VT_VECTOR ); }
		bool IsVectorOrArray( void ) const { return HasFlag( m_prop.vt, VT_VECTOR | VT_ARRAY ); }

		bool GetBool( OUT bool* pValue ) const;

		bool GetString( OUT std::tstring* pValue ) const;
		bool QueryStrings( OUT std::vector<std::tstring>* pStrItems ) const;

		bool GetFilePath( OUT fs::CPath* pFilePath ) const;
		bool QueryFilePaths( OUT std::vector<fs::CPath>* pOutFilePaths ) const;		// efficient via swap()

		template< typename IntT >
		bool GetInt( OUT IntT* pValue ) const
		{
			ASSERT_PTR( pValue );
			LONGLONG value;
			if ( IsEmpty() || !HR_OK( ::PropVariantToInt64( m_prop, &value ) ) )
				return false;

			*pValue = static_cast<IntT>( m_prop.bVal );
			return true;
		}

		template< typename UIntT >
		bool GetUInt( OUT UIntT* pValue ) const
		{
			ASSERT_PTR( pValue );
			ULONGLONG value;
			if ( IsEmpty() || !HR_OK( ::PropVariantToUInt64( m_prop, &value ) ) )
				return false;

			*pValue = static_cast<UIntT>( m_prop.bVal );
			return true;
		}
	private:
		PROPVARIANT m_prop;
	};
}


#include "utl/StringCompare.h"


namespace func
{
	struct PropKeyToNamePtr		// character-ptr translator
	{
		template< typename PropKeyNameT >
		const TCHAR* operator()( const PropKeyNameT& value ) const { return value.m_canonicalName.c_str(); }
	};
}


namespace pred
{
	// natural string compare
	typedef CompareAdapter<CompareIntuitiveCharPtr, func::PropKeyToNamePtr> TPropKeyNameCompareIntuitive;
	typedef LessValue<TPropKeyNameCompareIntuitive> TLess_PropKeyNameIntuitive;
}


#endif // ComUtils_UTL_UI_h
