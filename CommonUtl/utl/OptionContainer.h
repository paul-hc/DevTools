#ifndef OptionContainer_h
#define OptionContainer_h
#pragma once

#include "IRegistrySection.h"


class CEnumTags;


#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )					// skip past "m_"; it doesn't work well when expanded from other macros

#define MAKE_OPTION( pDataMember )  reg::MakeOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )		// skips the leading "&" operator for address of data-member
#define MAKE_ENUM_OPTION( pDataMember )  reg::MakeEnumOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )
#define MAKE_ENUM_TAG_OPTION( pDataMember, pTags )  reg::MakeEnumOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1, (pTags) )
#define MAKE_MULTIPLE_OPTION( pDataMember )  reg::MakeMultipleOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )


namespace reg
{
	class CBaseOption;


	// container of serializable options (to registry)
	//
	class COptionContainer : private utl::noncopyable
	{
	public:
		COptionContainer( const std::tstring& section );
		COptionContainer( IRegistrySection* pRegSection );		// takes ownership
		~COptionContainer();

		CBaseOption& GetOptionAt( size_t index ) const { ASSERT( index < m_options.size() ); return *m_options[ index ]; }

		void AddOption( CBaseOption* pOption );
		CBaseOption& LookupOption( const void* pDataMember ) const;

		// all options
		void LoadOptions( void );
		void SaveOptions( void ) const;

		// single option
		void SaveOption( const void* pDataMember ) const;

		template< typename ValueT >
		bool ModifyOption( ValueT* pDataMember, const ValueT& newValue, bool save = true );

		void ToggleOption( bool* pBoolDataMember, bool save = true ) { ModifyOption( pBoolDataMember, !*pBoolDataMember, save ); }
	public:
		std::auto_ptr< IRegistrySection > m_pRegSection;
	protected:
		std::vector< CBaseOption* > m_options;
	};


	// abstract base for options that hold reference to external data members
	//
	abstract class CBaseOption
	{
	protected:
		CBaseOption( const TCHAR* pEntry );
	public:
		virtual ~CBaseOption();

		void SetContainer( COptionContainer* pContainer ) { ASSERT_PTR( pContainer ); m_pContainer = pContainer; }

		virtual void Load( void ) = 0;
		virtual void Save( void ) const = 0;
		virtual bool HasDataMember( const void* pDataMember ) const = 0;
	protected:
		std::tstring m_entry;
		COptionContainer* m_pContainer;
	};


	template< typename ValueT >
	class COption : public CBaseOption
	{
	public:
		COption( ValueT* pValue, const TCHAR* pEntry ) : CBaseOption( pEntry ), m_pValue( safe_ptr( pValue ) ) {}

		// base overrides
		virtual void Load( void )
		{
			*m_pValue = static_cast< ValueT >( m_pContainer->m_pRegSection->GetIntParameter( m_entry.c_str(), (int)*m_pValue ) );
		}

		virtual void Save( void ) const
		{
			m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), static_cast< int >( *m_pValue ) );
		}

		virtual bool HasDataMember( const void* pDataMember ) const
		{
			return pDataMember == m_pValue;
		}
	protected:
		ValueT* m_pValue;
	};


	template< typename ValueT >
	class CMultipleOption : public CBaseOption			// ValueT must define stream insertors and extractors
	{
	public:
		CMultipleOption( std::vector< ValueT >* pValues, const TCHAR* pEntry, const TCHAR* pDelimiter = _T("|") )
			: CBaseOption( pEntry )
			, m_pValues( safe_ptr( pValues ) )
			, m_pDelimiter( pDelimiter )
		{
			ASSERT( !str::IsEmpty( m_pDelimiter ) );
		}

		// base overrides
		virtual void Load( void )
		{
			str::Split( *m_pValues, m_pContainer->m_pRegSection->GetStringParameter( m_entry.c_str() ).c_str(), m_pDelimiter );
		}

		virtual void Save( void ) const
		{
			m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), str::Join( *m_pValues, m_pDelimiter ) );
		}

		virtual bool HasDataMember( const void* pDataMember ) const
		{
			return pDataMember == m_pValues;
		}
	private:
		std::vector< ValueT >* m_pValues;
		const TCHAR* m_pDelimiter;
	};


	class CEnumOption : public COption< int >
	{
		typedef COption< int > BaseClass;
	public:
		template< typename EnumType >
		CEnumOption( EnumType* pValue, const TCHAR* pEntry, const CEnumTags* pTags = NULL )
			: COption< int >( (int*)pValue, pEntry )
			, m_pTags( pTags )
		{
		}

		// base overrides
		virtual void Load( void );
		virtual void Save( void ) const;
	private:
		const CEnumTags* m_pTags;
	};


	// COptionContainer template code

	inline void COptionContainer::SaveOption( const void* pDataMember ) const
	{
		LookupOption( pDataMember ).Save();
	}

	template< typename ValueT >
	inline bool COptionContainer::ModifyOption( ValueT* pDataMember, const ValueT& newValue, bool save /*= true*/ )
	{
		ASSERT_PTR( pDataMember );
		if ( *pDataMember == newValue )
			return false;

		*pDataMember = newValue;

		if ( save )
			SaveOption( pDataMember );		// save right away the changed option
		return true;
	}


	// COption< std::tstring > specialization

	template<>
	inline void COption< std::tstring >::Load( void )
	{
		*m_pValue = m_pContainer->m_pRegSection->GetStringParameter( m_entry.c_str(), m_pValue->c_str() );
	}

	template<>
	inline void COption< std::tstring >::Save( void ) const
	{
		m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), *m_pValue );
	}


	// COption< bool > specialization

	template<>
	inline void COption< bool >::Load( void )
	{
		*m_pValue = m_pContainer->m_pRegSection->GetIntParameter( m_entry.c_str(), *m_pValue ) != FALSE;
	}

	template<>
	inline void COption< bool >::Save( void ) const
	{
		m_pContainer->m_pRegSection->SaveParameter( m_entry.c_str(), *m_pValue );
	}


	// option factory template code

	template< typename ValueT >
	inline CBaseOption* MakeOption( ValueT* pDataMember, const TCHAR* pEntry )
	{
		return new COption< ValueT >( pDataMember, pEntry );
	}

	template< typename EnumT >
	inline CBaseOption* MakeEnumOption( EnumT* pDataMember, const TCHAR* pEntry, const CEnumTags* pTags = NULL )
	{
		return new CEnumOption( pDataMember, pEntry, pTags );
	}

	template< typename ValueT >
	inline CBaseOption* MakeMultipleOption( ValueT* pDataMember, const TCHAR* pEntry )
	{
		return new CMultipleOption< ValueT >( pDataMember, pEntry );
	}

} // namespace registry


#endif // OptionContainer_h
