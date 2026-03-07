#ifndef MultiValueState_h
#define MultiValueState_h
#pragma once

#include "MultiValueBase.h"
#include "utl/Path.h"


namespace multi
{
	// accumulates common values among multiple items, starting with an invalid value (uninitialized)
	//
	template< typename ValueT >
	abstract class CValueState
	{
	protected:
		CValueState( const ValueT& invalidValue = ValueT() )
			: m_invalidValue( invalidValue )
			, m_value( m_invalidValue )
			, m_state( NullValue )
			, m_modified( false )
		{
		}
	public:
		bool IsNullValue( void ) const { return NullValue == m_state; }
		bool IsSharedValue( void ) const { return SharedValue == m_state; }
		bool IsMultipleValue( void ) const { return MultipleValue == m_state; }

		bool IsModified( void ) const { return m_modified; }

		const ValueT& GetValue( void ) const { REQUIRE( IsSharedValue() ); return m_value; }
		MultiValueState GetState( void ) const { return m_state; }

		void Clear( void ) { m_value = ValueT(); m_state = NullValue; }

		void Accumulate( const ValueT& value )
		{
			if ( IsNullValue() )
			{	// NullValue -> SharedValue
				m_value = value;
				m_state = SharedValue;
			}
			else if ( IsSharedValue() )
			{	// SharedValue -> MultipleValue
				if ( value != m_value )
				{
					m_value = m_invalidValue;		// reset the value
					m_state = MultipleValue;
				}
			}
		}
	protected:
		const ValueT m_invalidValue;

		ValueT m_value;
		MultiValueState m_state;
		bool m_modified;			// m_value changed on input?
	};


	// common values accumulator, with associated control
	//
	template< typename ValueT, typename CtrlT >
	abstract class CCtrlValueState : public CValueState<ValueT>
	{
	protected:
		CCtrlValueState( CtrlT* pCtrl, const ValueT& invalidValue = ValueT() )
			: CValueState<ValueT>( invalidValue )
			, m_pCtrl( pCtrl )
		{
		}
	protected:
		CtrlT* m_pCtrl;
	};
}


class CTextEdit;
class CPathItemEdit;
class CEnumComboBox;
class CHotKeyCtrlEx;


namespace multi
{
	class CStringValue : public CCtrlValueState<std::tstring, CTextEdit>
	{
	public:
		CStringValue( CTextEdit* pCtrl ) : CCtrlValueState<std::tstring, CTextEdit>( pCtrl ) {}

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	};


	class CPathValue : public CCtrlValueState<shell::TPath, CPathItemEdit>
	{
	public:
		CPathValue( CPathItemEdit* pCtrl ) : CCtrlValueState<shell::TPath, CPathItemEdit>( pCtrl ), m_anyGuidPath( false ) {}

		void Clear( void ) { __super::Clear(); m_anyGuidPath = false; }
		void Accumulate( const shell::TPath& value ) { __super::Accumulate( value ); m_anyGuidPath |= value.IsGuidPath(); }

		bool AnyGuidPath( void ) const { return m_anyGuidPath; }
		void SetAnyGuidPath( void );		// inherit the GUID property from another field value

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	private:
		bool m_anyGuidPath;
	};


	class CEnumValue : public CCtrlValueState<int, CEnumComboBox>
	{
	public:
		CEnumValue( CEnumComboBox* pCtrl ) : CCtrlValueState<int, CEnumComboBox>( pCtrl, CB_ERR ) {}

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	};


	class CHotKeyValue : public CCtrlValueState<WORD, CHotKeyCtrlEx>
	{
	public:
		CHotKeyValue( CHotKeyCtrlEx* pCtrl ) : CCtrlValueState<WORD, CHotKeyCtrlEx>( pCtrl ) {}

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	};


	class CFlagCheckState : public CValueState<UINT>		// button checked state: BST_UNCHECKED/BST_CHECKED/BST_INDETERMINATE
	{
		using CValueState<UINT>::Accumulate;
	public:
		CFlagCheckState( UINT flag, CWnd* pDlg, UINT btnId, bool editable = true )
			: CValueState<UINT>( BST_INDETERMINATE )
			, m_flag( flag )
			, m_pDlg( pDlg )
			, m_btnId( btnId )
			, m_editable( editable )
		{
			ASSERT( flag != 0 );
		}

		bool HasValidValue( void ) const { return IsSharedValue() && m_value != BST_INDETERMINATE; }
		bool IsChecked( void ) const { ASSERT( HasValidValue() ); return BST_CHECKED == m_value; }

		void AccumulateFlags( UINT flags ) { __super::Accumulate( HasFlag( flags, m_flag ) ? BST_CHECKED : BST_UNCHECKED ); }

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	private:
		const UINT m_flag;
		CWnd* m_pDlg;
		UINT m_btnId;
		bool m_editable;
	};
}


#endif // MultiValueState_h
