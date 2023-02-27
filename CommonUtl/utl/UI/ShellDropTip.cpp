
#include "stdafx.h"
#include "ShellDropTip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	// CDropTip implementation

	const std::wstring CDropTip::s_nullStr( L"--" );

	CDropTip::CDropTip( void )
	{
		#pragma warning( disable: 4996 )	// function call with parameters that may be unsafe

		// IMP: reset default values to logical "null"
		std::fill_n( &m_messages[ 0 ], COUNT_OF( m_messages ), s_nullStr );
		std::fill_n( &m_insertFmts[ 0 ], COUNT_OF( m_insertFmts ), s_nullStr );
	}

	/*
		Set text for specified image type.
		pMessage:  String to be used without szInsert.
		pInsertFmt: String to be used with szInsert.
				This may be NULL. Then pMessage is returned when requesting the string with szInsert.
		NOTES:
		 - Because the %1 placeholder is used to insert the szInsert text, a single percent character inside the strings must be escaped by another one.
		 - When passing NULL for pMessage (or just not calling this for a specific image type), the default system text is shown.
		   To avoid this, pass an empty string. However, this will result in an empty text area shown below and right of the new style cursor.
	*/
	bool CDropTip::StoreTypeField( DROPIMAGETYPE type, const wchar_t* pMessage, const wchar_t* pInsertFmt )
	{
		ASSERT( IsValidType( type ) );

		m_messages[ type ] = m_insertFmts[ type ] = s_nullStr;

		if ( pMessage != nullptr )
			m_messages[ type ] = pMessage;
		if ( pInsertFmt != nullptr )
			m_insertFmts[ type ] = pInsertFmt;

		return pMessage != nullptr || pInsertFmt != nullptr;
	}

	const wchar_t* CDropTip::GetTypeField( DROPIMAGETYPE type, Field field ) const
	{
		if ( !IsValidType( type ) )
			return nullptr;

		if ( InsertFmt == field && !IsNull( m_insertFmts[ type ] ) )
			return m_insertFmts[ type ].c_str();
		return !IsNull( m_insertFmts[ type ] ) ? m_messages[ type ].c_str() : nullptr;
	}

	bool CDropTip::MakeDescription( DROPDESCRIPTION* pDestDropTip, const wchar_t* pMessage ) const
	{
		bool changed = false;
		if ( CopyInsertFmt( pDestDropTip, m_insert.c_str() ) )
			changed = true;
		if ( CopyMessage( pDestDropTip, pMessage ) )
			changed = true;
		return changed;
	}

	bool CDropTip::CopyText( DROPDESCRIPTION* pDestDropTip, DROPIMAGETYPE type ) const
	{
		return CopyMessage( pDestDropTip, GetTypeField( type, HasInsertFmt( pDestDropTip ) ? InsertFmt : Message ) );
	}

	bool CDropTip::CopyMessage( DROPDESCRIPTION* pDestDropTip, const wchar_t* pMessage ) const
	{
		ASSERT_PTR( pDestDropTip );
		return _ModifyStr( pDestDropTip->szMessage, COUNT_OF( pDestDropTip->szMessage ), pMessage );
	}

	// Copy string to drop description szInsert member.
	// Returns true if strings has been changed.
	bool CDropTip::CopyInsertFmt( DROPDESCRIPTION* pDestDropTip, const wchar_t* pInsertFmt ) const
	{
		ASSERT_PTR( pDestDropTip );
		return _ModifyStr( pDestDropTip->szInsert, COUNT_OF( pDestDropTip->szInsert ), pInsertFmt );
	}

	bool CDropTip::Clear( DROPDESCRIPTION* pDestDropTip )
	{
		ASSERT_PTR( pDestDropTip );

		bool changed =
			pDestDropTip->type != DROPIMAGE_INVALID ||
			!str::IsEmpty( pDestDropTip->szMessage ) ||
			!str::IsEmpty( pDestDropTip->szInsert );

		pDestDropTip->type = DROPIMAGE_INVALID;
		pDestDropTip->szMessage[ 0 ] = pDestDropTip->szInsert[ 0 ] = L'\0';
		return changed;
	}

	bool CDropTip::_ModifyStr( wchar_t* pDest, size_t destSize, const wchar_t* pSrc )
	{
		// helper function for CopyMessage() and CopyInsertFmt()
		ASSERT_PTR( pDest );
		ASSERT( destSize != 0 );

		if ( 0 == wcscmp( pDest, pSrc ) )
			return false;						// string not changed

		wcsncpy_s( pDest, destSize, pSrc != nullptr ? pSrc : L"", _TRUNCATE );
		return true;
	}

} //namespace shell
