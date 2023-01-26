#ifndef ShellDropTip_h
#define ShellDropTip_h
#pragma once

#include <vector>
#include <shlobj.h>


namespace shell
{
	// AKA drop description - stores user-defined drop descriptions displayed on the drop cursor (Vista+).
	// Used by ole::CDataSource, ole::CDropSource, ole::CDropTarget.
	//
	class CDropTip
	{
		enum { MaxDropImage = DROPIMAGE_NOIMAGE, _DropImageCount };
	public:
		CDropTip( void );

		bool HasInsert( void ) const { return !m_insert.empty(); }
		void SetInsert( const wchar_t* pInsert ) { m_insert = pInsert; }

		bool HasTypeField( DROPIMAGETYPE type ) const { return IsValidType( type ) && !IsNull( m_messages[ type ] ); }		// text has been stored for image type?
		const wchar_t* GetTypeField( DROPIMAGETYPE type ) const { return GetTypeField( type, HasInsert() ? InsertFmt : Message ); }
		bool StoreTypeField( DROPIMAGETYPE type, const wchar_t* pMessage, const wchar_t* pInsertFmt );

		bool MakeDescription( DROPDESCRIPTION* pDestDropTip, const wchar_t* pMessage ) const;
		bool MakeDescription( DROPDESCRIPTION* pDestDropTip, DROPIMAGETYPE type ) const { return MakeDescription( pDestDropTip, GetTypeField( type ) ); }

		bool CopyInsertFmt( DROPDESCRIPTION* pDestDropTip, const wchar_t* pInsertFmt ) const;
		bool CopyText( DROPDESCRIPTION* pDestDropTip, DROPIMAGETYPE type ) const;
		bool CopyMessage( DROPDESCRIPTION* pDestDropTip, const wchar_t* pMessage ) const;

		static bool IsValidType( DROPIMAGETYPE type ) { return type >= DROPIMAGE_NONE && type <= MaxDropImage; }
		static bool HasInsertFmt( const DROPDESCRIPTION* pDropTip ) { return pDropTip != NULL && !str::IsEmpty( pDropTip->szInsert ); }
		static bool Clear( DROPDESCRIPTION* pDropTip );
	private:
		enum Field { Message, InsertFmt };

		const wchar_t* GetTypeField( DROPIMAGETYPE type, Field field ) const;

		static bool IsNull( const std::wstring& field ) { return 0 == field.compare( s_nullStr.c_str() ); }	// workaround strange compile error with operator==
		static bool _ModifyStr( wchar_t* pDest, size_t destSize, const wchar_t* pSrc );			// returns true if text changed
	private:
		std::wstring m_insert;
		std::wstring m_messages[ _DropImageCount ];			// per type
		std::wstring m_insertFmts[ _DropImageCount ];		// insert

		static const std::wstring s_nullStr;				// placeholder for showing system default text
	};
}


#endif // ShellDropTip_h
