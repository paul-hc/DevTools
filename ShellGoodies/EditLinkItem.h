#ifndef EditLinkItem_h
#define EditLinkItem_h
#pragma once

#include "utl/UI/ShortcutItem.h"


class CEditLinkItem : public CShortcutItem
{
	CEditLinkItem( const fs::CPath& linkPath, IShellLink* pShellLink );
public:
	CEditLinkItem( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut );
	virtual ~CEditLinkItem();

	static CEditLinkItem* LoadLinkItem( const fs::CPath& linkPath );

	const shell::CShortcut& GetSrcShortcut( void ) const { return GetShortcut(); }
	const shell::CShortcut& GetDestShortcut( void ) const { return m_destShortcut; }

	bool IsModified( void ) const { return m_destShortcut.IsValidTarget() && m_destShortcut != GetSrcShortcut(); }

	shell::CShortcut& RefDestShortcut( void ) { return m_destShortcut; }
	void Reset( void ) { m_destShortcut = GetSrcShortcut(); }

	// UI helpers
	bool HasInvalidTarget( bool isSrc ) const;
	bool HasInvalidWorkDir( bool isSrc ) const;
	bool HasInvalidIcon( bool isSrc ) const;
private:
	shell::CShortcut m_destShortcut;
};


namespace func
{
	struct AsSrcShortcut
	{
		const shell::CShortcut& operator()( const CEditLinkItem* pItem ) const { return pItem->GetSrcShortcut(); }
	};


	// accessors for order predicates

	struct Dest_AsTargetPath
	{
		const fs::CPath& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetTargetPath(); }
	};

	struct Dest_AsTargetPidl
	{
		const shell::CPidlAbsolute& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetTargetPidl(); }
	};

	struct Dest_AsWorkDirPath
	{
		const fs::CPath& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetWorkDirPath(); }
	};

	struct Dest_AsArguments
	{
		const std::tstring& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetArguments(); }
	};

	struct Dest_AsDescription
	{
		const std::tstring& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetDescription(); }
	};

	struct Dest_AsIconLocation
	{
		const shell::CIconLocation& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetIconLocation(); }
	};
}


namespace fmt
{
	std::tstring FormatEditLinkEntry( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut, const shell::CShortcut& destShortcut );
}


class CEnumTags;
class CTextEdit;
class CPathItemEdit;
class CImageEdit;
class CEnumComboBox;
class CHotKeyCtrl;


namespace multi
{
	// accumulates common values among multiple items, starting with an invalid value (uninitialized)

	enum MultiValueState { NullValue, SharedValue, MultipleValue };

	const CEnumTags& GetTags_MultiValueState( void );
	const std::tstring& GetMultipleValueTag( void );


	template< typename ValueT >
	abstract class CValueState
	{
	protected:
		CValueState( const ValueT& invalidValue = ValueT() )
			: m_invalidValue( invalidValue )
			, m_value( m_invalidValue )
			, m_state( NullValue )
		{
		}
	public:
		bool IsNullValue( void ) const { return NullValue == m_state; }
		bool IsSharedValue( void ) const { return SharedValue == m_state; }
		bool IsMultipleValue( void ) const { return MultipleValue == m_state; }

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
	};


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
		CPathValue( CPathItemEdit* pCtrl ) : CCtrlValueState<shell::TPath, CPathItemEdit>( pCtrl ) {}

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	};


	class CEnumValue : public CCtrlValueState<int, CEnumComboBox>
	{
	public:
		CEnumValue( CEnumComboBox* pCtrl ) : CCtrlValueState<int, CEnumComboBox>( pCtrl, CB_ERR ) {}

		bool UpdateCtrl( void ) const;
		bool InputCtrl( void );
	};


	class CHotKeyValue : public CCtrlValueState<WORD, CHotKeyCtrl>
	{
	public:
		CHotKeyValue( CHotKeyCtrl* pCtrl ) : CCtrlValueState<WORD, CHotKeyCtrl>( pCtrl ) {}

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


#include "utl/UI/Icon.h"


namespace single
{
	class CIconLocationValue : private utl::noncopyable
	{
	public:
		CIconLocationValue( CImageEdit* pImageEdit, CStatic* pLargeIconStatic ) : m_pImageEdit( pImageEdit ), m_pLargeIconStatic( pLargeIconStatic ) {}
		~CIconLocationValue();

		bool IsNullValue( void ) const { return m_iconLocation.IsEmpty() && m_linkPath.IsEmpty(); }

		void Clear( void );

		const shell::CIconLocation& Get( void ) const { return m_iconLocation; }
		bool Set( const shell::CIconLocation& iconLocation, const fs::CPath& linkPath );

		bool PickIcon( void );
	private:
		CImageEdit* m_pImageEdit;				// displays the small icon
		CStatic* m_pLargeIconStatic;			// displays the large icon

		shell::CIconLocation m_iconLocation;
		fs::CPath m_linkPath;					// fallback for the icon, if not customized

		CIcon m_smallIcon, m_largeIcon;
	};
}


#endif // EditLinkItem_h
