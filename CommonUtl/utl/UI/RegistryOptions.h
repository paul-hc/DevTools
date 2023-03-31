#ifndef RegistryOptions_h
#define RegistryOptions_h
#pragma once

#include "IRegistrySection.h"
#include "Range.h"


#define ENTRY_MEMBER( value ) ( _T( #value ) + 2 )					// skip past "m_"; it doesn't work well when expanded from other macros

#define MAKE_OPTION( pDataMember )  reg::MakeOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )		// also skip the leading "&" operator for address of data-member
#define MAKE_ENUM_OPTION( pDataMember )  reg::MakeEnumOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )
#define MAKE_MULTIPLE_OPTION( pDataMember )  reg::MakeMultipleOption( pDataMember, ENTRY_MEMBER( pDataMember ) + 1 )


namespace reg
{
	class CBaseOption;

	template< typename ValueT >
	class COption;
}


// container of registry-persistent options
//
class CRegistryOptions : public CCmdTarget
{
public:
	enum AutoSave { NoAutoSave, SaveOnModify, SaveAllOnModify };

	CRegistryOptions( const std::tstring& sectionName, AutoSave autoSave );
	CRegistryOptions( IRegistrySection* pRegSection, AutoSave autoSave );		// takes ownership
	virtual ~CRegistryOptions();

	// protect on assignment; let derived class assign options' state (e.g. default values)
	CRegistryOptions( const CRegistryOptions& right ) { right; }
	CRegistryOptions& operator=( const CRegistryOptions& right ) { right; return *this; }

	bool IsPersistent( void ) const { return m_pRegSection.get() != nullptr; }

	IRegistrySection* GetSection( void ) const { return m_pRegSection.get(); }
	void SetSection( IRegistrySection* pRegSection ) { m_pRegSection.reset( pRegSection ); }

	const std::tstring& GetSectionName( void ) const;
	void SetSectionName( const std::tstring& sectionName );

	void AddOption( reg::CBaseOption* pOption, UINT ctrlId = 0 );

	reg::CBaseOption& LookupOption( const void* pDataMember ) const;

	template< typename OptionT >
	OptionT* LookupOptionAs( const void* pDataMember ) const { return checked_static_cast<OptionT*>( &LookupOption( pDataMember ) ); }

	reg::CBaseOption* FindOptionByID( UINT ctrlId ) const;

	// all options
	virtual void LoadAll( void );
	virtual void SaveAll( void ) const;

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
	bool IsHandledInternallyID( UINT ctrlId ) const { return FindOptionByID( ctrlId ) != nullptr; }
	bool RestoreOptionDefaultValue( reg::CBaseOption* pOption );

	// overrideables
	virtual void OnOptionChanged( const void* pDataMember );
protected:
	std::auto_ptr<IRegistrySection> m_pRegSection;
	AutoSave m_autoSave;
	std::vector<reg::CBaseOption*> m_options;

	// generated overrides
protected:
	virtual BOOL OnToggleOption( UINT cmdId );
	virtual void OnUpdateOption( CCmdUI* pCmdUI );

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

		void SetParent( const CRegistryOptions* pParent ) { ASSERT_NULL( m_pParent ); m_pParent = pParent; ASSERT_PTR( m_pParent ); }

		UINT GetCtrlId( void ) const { return m_ctrlId; }
		void SetCtrlId( UINT ctrlId ) { m_ctrlId = ctrlId; }
		virtual bool HasCtrlId( UINT ctrlId ) const;

		bool HasDataMember( const void* pDataMember ) const { return GetDataMember() == pDataMember; }

		virtual void Load( void ) = 0;
		virtual void Save( void ) const = 0;
		virtual const void* GetDataMember( void ) const = 0;
		virtual bool HasDefaultValue( void ) const = 0;
		virtual void SetDefaultValue( void ) = 0;				// returns true if value has changed
	protected:
		// must only be called for persistent options
		IRegistrySection* GetSection( void ) const { ASSERT_PTR( m_pParent->GetSection() ); return m_pParent->GetSection(); }
	protected:
		std::tstring m_entry;
	private:
		const CRegistryOptions* m_pParent;
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
			*m_pValue = static_cast<ValueT>( GetSection()->GetIntParameter( m_entry.c_str(), (int)*m_pValue ) );
		}

		virtual void Save( void ) const
		{
			GetSection()->SaveParameter( m_entry.c_str(), static_cast<int>( *m_pValue ) );
		}

		virtual const void* GetDataMember( void ) const { return m_pValue; }
		virtual bool HasDefaultValue( void ) const { return m_defaultValue == *m_pValue; }
		virtual void SetDefaultValue( void ) { *m_pValue = m_defaultValue; }
	protected:
		ValueT* m_pValue;
		ValueT m_defaultValue;
	};


	typedef COption<bool> TBoolOption;


	class CEnumOption : public COption<int>
	{
		typedef COption<int> TBaseClass;
	public:
		template< typename EnumType >
		CEnumOption( EnumType* pValue, const TCHAR* pEntry ) : COption<int>( (int*)pValue, pEntry ), m_pTags( nullptr ), m_radioIds( 0 ) {}

		void SetEnumTags( const CEnumTags* pTags ) { m_pTags = pTags; }
		void SetRadioIds( UINT firstRadioId, UINT lastRadioId ) { m_radioIds.SetRange( firstRadioId, lastRadioId ); }

		bool HasRadioId( UINT radioId ) const { return radioId != 0 && !m_radioIds.IsEmpty() && m_radioIds.Contains( radioId ); }
		int GetValueFromRadioId( UINT radioId ) const;

		// base overrides
		virtual bool HasCtrlId( UINT ctrlId ) const;
		virtual void Load( void );
		virtual void Save( void ) const;
	private:
		const CEnumTags* m_pTags;
		Range<UINT> m_radioIds;
	};


	template< typename ValueT >
	class CMultipleOption : public CBaseOption			// ValueT must define stream insertors and extractors for string conversion
	{
	public:
		CMultipleOption( std::vector<ValueT>* pValues, const TCHAR* pEntry, const TCHAR* pDelimiter = _T("|") )
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
			str::Split( *m_pValues, GetSection()->GetStringParameter( m_entry.c_str() ).c_str(), m_pDelimiter );
		}

		virtual void Save( void ) const
		{
			GetSection()->SaveParameter( m_entry.c_str(), str::Join( *m_pValues, m_pDelimiter ) );
		}

		virtual const void* GetDataMember( void ) const { return m_pValues; }
		virtual bool HasDefaultValue( void ) const { return m_defaultValues == *m_pValues; }
		virtual void SetDefaultValue( void ) { *m_pValues = m_defaultValues; }
	private:
		std::vector<ValueT>* m_pValues;
		std::vector<ValueT> m_defaultValues;
		const TCHAR* m_pDelimiter;
	};
}


// CRegistryOptions inline code

inline void CRegistryOptions::SaveOption( const void* pDataMember ) const
{
	if ( IsPersistent() )
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
	// COption<std::tstring> specialization

	template<>
	inline void COption<std::tstring>::Load( void )
	{
		*m_pValue = GetSection()->GetStringParameter( m_entry.c_str(), m_pValue->c_str() );
	}

	template<>
	inline void COption<std::tstring>::Save( void ) const
	{
		GetSection()->SaveParameter( m_entry.c_str(), *m_pValue );
	}


	// COption<bool> specialization

	template<>
	inline void COption<bool>::Load( void )
	{
		*m_pValue = GetSection()->GetIntParameter( m_entry.c_str(), *m_pValue ) != FALSE;
	}

	template<>
	inline void COption<bool>::Save( void ) const
	{
		GetSection()->SaveParameter( m_entry.c_str(), *m_pValue );
	}


	// option factory template code

	template< typename ValueT >
	inline COption<ValueT>* MakeOption( ValueT* pDataMember, const TCHAR* pEntry )
	{
		return new COption<ValueT>( pDataMember, pEntry );
	}

	template< typename EnumT >
	inline CEnumOption* MakeEnumOption( EnumT* pDataMember, const TCHAR* pEntry )
	{
		return new CEnumOption( pDataMember, pEntry );
	}

	template< typename ValueT >
	inline CMultipleOption<ValueT>* MakeMultipleOption( ValueT* pDataMember, const TCHAR* pEntry )
	{
		return new CMultipleOption<ValueT>( pDataMember, pEntry );
	}

} // namespace registry


#endif // RegistryOptions_h
