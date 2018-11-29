#ifndef RegistryOptions_h
#define RegistryOptions_h
#pragma once

#include "IRegistrySection.h"


#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )					// skip past "m_"; it doesn't work well when expanded from other macros

#define MAKE_OPTION( pDataMember )  reg::MakeOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )		// also skip the leading "&" operator for address of data-member
#define MAKE_ENUM_OPTION( pDataMember )  reg::MakeEnumOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )
#define MAKE_ENUM_TAG_OPTION( pDataMember, pTags )  reg::MakeEnumOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1, (pTags) )
#define MAKE_MULTIPLE_OPTION( pDataMember )  reg::MakeMultipleOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )


namespace reg
{
	class CBaseOption;

	template< typename ValueT >
	class COption;
}


// container of serializable options (to registry)
//
class CRegistryOptions : public CCmdTarget
{
public:
	CRegistryOptions( const std::tstring& section, bool saveOnModify );
	CRegistryOptions( IRegistrySection* pRegSection, bool saveOnModify );		// takes ownership
	~CRegistryOptions();

	// protect on assignment; let derived class assign options' state (e.g. default values)
	CRegistryOptions( const CRegistryOptions& right ) { right; }
	CRegistryOptions& operator=( const CRegistryOptions& right ) { right; return *this; }

	IRegistrySection* GetSection( void ) const { return m_pRegSection.get(); }

	void AddOption( reg::CBaseOption* pOption, UINT ctrlId = 0 );
	reg::CBaseOption& LookupOption( const void* pDataMember ) const;
	reg::COption< bool >* FindBoolOptionByID( UINT ctrlId ) const;

	// all options
	void LoadAll( void );
	void SaveAll( void ) const;

	bool AnyNonDefaultValue( void ) const;
	size_t RestoreAllDefaultValues( void );

	// single option
	void SaveOption( const void* pDataMember ) const;
	bool RestoreOptionDefaultValue( const void* pDataMember );

	template< typename ValueT >
	bool ModifyOption( ValueT* pDataMember, const ValueT& newValue );

	void ToggleOption( bool* pBoolDataMember ) { ModifyOption( pBoolDataMember, !*pBoolDataMember ); }

	void UpdateControls( CWnd* pTargetWnd );			// update check-box buttons
protected:
	bool RestoreOptionDefaultValue( reg::CBaseOption* pOption );

	// overrideables
	virtual void OnOptionChanged( const void* pDataMember );
protected:
	std::auto_ptr< IRegistrySection > m_pRegSection;
	bool m_saveOnModify;								// for individual options
	std::vector< reg::CBaseOption* > m_options;

	// generated overrides
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void OnToggle_BoolOption( UINT cmdId );
	virtual void OnUpdate_BoolOption( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


class CEnumTags;


namespace reg
{
	// abstract base for options that hold reference to external data members
	//
	abstract class CBaseOption
	{
	protected:
		CBaseOption( const TCHAR* pEntry );
	public:
		virtual ~CBaseOption();

		void SetSection( IRegistrySection* pRegSection ) { ASSERT_PTR( pRegSection ); m_pRegSection = pRegSection; }

		UINT GetCtrlId( void ) const { return m_ctrlId; }
		void SetCtrlId( UINT ctrlId ) { m_ctrlId = ctrlId; }

		bool HasDataMember( const void* pDataMember ) const { return GetDataMember() == pDataMember; }

		virtual void Load( void ) = 0;
		virtual void Save( void ) const = 0;
		virtual const void* GetDataMember( void ) const = 0;
		virtual bool HasDefaultValue( void ) const = 0;
		virtual void SetDefaultValue( void ) = 0;				// returns true if value has changed
	protected:
		std::tstring m_entry;
		IRegistrySection* m_pRegSection;
		UINT m_ctrlId;
	};


	template< typename ValueT >
	class COption : public CBaseOption
	{
	public:
		COption( ValueT* pValue, const TCHAR* pEntry ) : CBaseOption( pEntry ), m_pValue( pValue ), m_defaultValue( *m_pValue ) { ASSERT_PTR( m_pValue ); }

		const ValueT& GetValue( void ) const { return *m_pValue; }
		ValueT& RefValue( void ) { return *m_pValue; }

		// base overrides
		virtual void Load( void )
		{
			*m_pValue = static_cast< ValueT >( m_pRegSection->GetIntParameter( m_entry.c_str(), (int)*m_pValue ) );
		}

		virtual void Save( void ) const
		{
			m_pRegSection->SaveParameter( m_entry.c_str(), static_cast< int >( *m_pValue ) );
		}

		virtual const void* GetDataMember( void ) const { return m_pValue; }
		virtual bool HasDefaultValue( void ) const { return m_defaultValue == *m_pValue; }
		virtual void SetDefaultValue( void ) { *m_pValue = m_defaultValue; }
	protected:
		ValueT* m_pValue;
		ValueT m_defaultValue;
	};


	template< typename ValueT >
	class CMultipleOption : public CBaseOption			// ValueT must define stream insertors and extractors for string conversion
	{
	public:
		CMultipleOption( std::vector< ValueT >* pValues, const TCHAR* pEntry, const TCHAR* pDelimiter = _T("|") )
			: CBaseOption( pEntry )
			, m_pValues( safe_ptr( pValues ) )
			, m_defaultValues( *m_pValues )
			, m_pDelimiter( pDelimiter )
		{
			ASSERT( !str::IsEmpty( m_pDelimiter ) );
		}

		// base overrides
		virtual void Load( void )
		{
			str::Split( *m_pValues, m_pRegSection->GetStringParameter( m_entry.c_str() ).c_str(), m_pDelimiter );
		}

		virtual void Save( void ) const
		{
			m_pRegSection->SaveParameter( m_entry.c_str(), str::Join( *m_pValues, m_pDelimiter ) );
		}

		virtual const void* GetDataMember( void ) const { return m_pValues; }
		virtual bool HasDefaultValue( void ) const { return m_defaultValues == *m_pValues; }
		virtual void SetDefaultValue( void ) { *m_pValues = m_defaultValues; }
	private:
		std::vector< ValueT >* m_pValues;
		std::vector< ValueT > m_defaultValues;
		const TCHAR* m_pDelimiter;
	};


	class CEnumOption : public COption< int >
	{
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
}


// CRegistryOptions inline code

inline void CRegistryOptions::SaveOption( const void* pDataMember ) const
{
	LookupOption( pDataMember ).Save();
}

inline bool CRegistryOptions::RestoreOptionDefaultValue( const void* pDataMember )
{
	return RestoreOptionDefaultValue( &LookupOption( pDataMember ) );
}

template< typename ValueT >
inline bool CRegistryOptions::ModifyOption( ValueT* pDataMember, const ValueT& newValue )
{
	ASSERT_PTR( pDataMember );
	if ( *pDataMember == newValue )
		return false;

	*pDataMember = newValue;
	OnOptionChanged( pDataMember );
	return true;
}


namespace reg
{
	// COption< std::tstring > specialization

	template<>
	inline void COption< std::tstring >::Load( void )
	{
		*m_pValue = m_pRegSection->GetStringParameter( m_entry.c_str(), m_pValue->c_str() );
	}

	template<>
	inline void COption< std::tstring >::Save( void ) const
	{
		m_pRegSection->SaveParameter( m_entry.c_str(), *m_pValue );
	}


	// COption< bool > specialization

	template<>
	inline void COption< bool >::Load( void )
	{
		*m_pValue = m_pRegSection->GetIntParameter( m_entry.c_str(), *m_pValue ) != FALSE;
	}

	template<>
	inline void COption< bool >::Save( void ) const
	{
		m_pRegSection->SaveParameter( m_entry.c_str(), *m_pValue );
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


#endif // RegistryOptions_h
