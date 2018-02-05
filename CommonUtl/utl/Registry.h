#ifndef Registry_h
#define Registry_h
#pragma once


namespace reg
{
	enum SeekBound { SB_First, SB_Last };


	struct CValue;


	class CKey : private utl::noncopyable
	{
	public:
		CKey( HKEY hKey = NULL ) : m_hKey( hKey ) {}
		CKey( HKEY hParentKey, const TCHAR* pKeyName, bool forceCreate = false );
		CKey( const TCHAR* pKeyFullPath, bool forceCreate = false ) : m_hKey( CKey::OpenKeyFullPath( pKeyFullPath, forceCreate ) ) {}
		CKey( const CKey& src ) : m_hKey( ( (CKey&)src ).Detach() ) {}
		~CKey() { Close(); }

		static HKEY ExtractRootKey( std::tstring& rSubKeysPath, const std::tstring& srcKeyPath );
		static HKEY OpenKeySubPath( HKEY hParentKey, const TCHAR* pSubKeyPath, bool forceCreate = false );
		static HKEY OpenKeyFullPath( const TCHAR* pKeyFullPath, bool forceCreate = false );

		HKEY Get( void ) const { return m_hKey; }

		bool IsValid( void ) const { return this != NULL && m_hKey != NULL; }
		bool HasSubKeys( void ) const { return GetSubKeyCount() != 0; }
		bool HasValues( void ) const { return GetValueCount() != 0; }

		struct CInfo;
		int GetKeyInfo( CInfo& rKeyInfo ) const { VERIFY( rKeyInfo.Build( m_hKey ) ); return rKeyInfo.m_subKeyCount; }

		void Assign( HKEY hKey ) { Close(); m_hKey = hKey; }
		bool Flush( void ) { ASSERT( IsValid() ); return ERROR_SUCCESS == ::RegFlushKey( m_hKey ); }
		HKEY Detach( void );
		bool Close( void );

		void RemoveAllSubKeys( void );
		void RemoveAllValues( void );
		void RemoveAll( void ) { RemoveAllSubKeys(); RemoveAllValues(); }

		// sub-keys
		int GetSubKeyCount( void ) const { return CInfo( m_hKey ).m_subKeyCount; }
		bool HasSubKey( const TCHAR* pSubKeyName ) const { return CKey( m_hKey, pSubKeyName, false ).IsValid(); }

		CKey OpenSubKey( const TCHAR* pSubKeyName, bool forceCreate = false ) { return CKey( m_hKey, pSubKeyName, forceCreate ); }
		bool RemoveSubKey( const TCHAR* pSubKeyName ) { ASSERT( IsValid() && !str::IsEmpty( pSubKeyName ) ); return ERROR_SUCCESS == ::RegDeleteKey( m_hKey, pSubKeyName ); }

		// values access
		int GetValueCount( void ) const { return CInfo( m_hKey ).m_valueCount; }
		DWORD GetValueType( const TCHAR* pValueName, DWORD* pBufferSize = NULL ) const;
		bool HasValue( const TCHAR* pValueName ) const { return GetValueType( pValueName ) != REG_NONE; }
		bool GetValue( CValue& rValue, const TCHAR* pValueName ) const;
		bool SetValue( const TCHAR* pValueName, const CValue& value );
		bool RemoveValue( const TCHAR* pValueName );

		// direct values manipulation
		std::tstring ReadString( const TCHAR* pValueName, const TCHAR* pDefaultText = _T("") );
		bool WriteString( const TCHAR* pValueName, const TCHAR* pValue );

		long ReadNumber( const TCHAR* pValueName, long defaultNumber = 0L );
		bool WriteNumber( const TCHAR* pValueName, long value );
	protected:
		HKEY m_hKey;
	public:
		struct CInfo
		{
			CInfo( HKEY hKey ) { memset( this, 0, sizeof( CInfo ) ); VERIFY( NULL == hKey || Build( hKey ) ); }

			bool Build( HKEY hKey )
			{
				ASSERT_PTR( hKey );
				return ERROR_SUCCESS == ::RegQueryInfoKey( hKey, NULL, NULL, NULL, &m_subKeyCount, &m_subKeyNameMaxLen, NULL,
														   &m_valueCount, &m_valueNameMaxLen, &m_valueBufferMaxLen, NULL, NULL );
			}
		public:
			DWORD m_subKeyCount;
			DWORD m_subKeyNameMaxLen;
			DWORD m_valueCount;
			DWORD m_valueNameMaxLen;
			DWORD m_valueBufferMaxLen;
		};
	};


	struct CValue
	{
		CValue( void );
		CValue( const CValue& src );
		~CValue();

		bool IsValid( void ) const { return m_pBuffer != NULL && m_type != REG_NONE; }

		CValue& operator=( const CValue& src );
		void AttachBuffer( void* extDestBuffer, DWORD extBufferSize );
		BYTE* AllocCopy( const void* srcBuffer, DWORD srcBufferSize, DWORD srcType = REG_BINARY );
		bool Destroy( void );
		void Detach( void );

		// registry IO
		bool Read( const CKey& key, const TCHAR* pValueName );
		bool Write( CKey& key, const TCHAR* pValueName ) const;

		DWORD GetDword( void ) const { ASSERT( IsValid() && REG_DWORD == m_type ); return *(DWORD*)m_pBuffer; }
		std::tstring GetString( void ) const { ASSERT( IsValid() && REG_SZ == m_type ); return reinterpret_cast< const TCHAR* >( m_pBuffer ); }

		// assignment
		void Assign( DWORD srcType, const void* srcBuffer, DWORD srcBufferSize );
		void SetDword( DWORD dword );
		void SetString( const TCHAR* pText ) { Assign( REG_SZ, (const BYTE*)pText, ( (DWORD)str::GetLength( pText ) + 1 ) * sizeof( TCHAR ) ); }
	protected:
		bool ensureBuffer( DWORD requiredSize );
	public:
		DWORD m_type;
		BYTE* m_pBuffer;
		DWORD m_size;
	protected:
		bool m_autoDelete;
	private:
		DWORD m_quickBuffer;
	};


	class CKeyIterator
	{
	public:
		CKeyIterator( const CKey* pParentKey, SeekBound bound = SB_First );
		~CKeyIterator();

		bool IsValid( void ) const;
		const TCHAR* GetName( void ) const { ASSERT( IsValid() ); return m_pSubKeyName; }		// was operator()
		int GetPos( void ) const { return m_pos; }
		int GetCount( void ) const { return m_keyInfo.m_subKeyCount; }

		CKeyIterator& operator++( void ) { ++m_pos; SeekToPos(); return *this; }
		CKeyIterator& operator--( void ) { --m_pos; SeekToPos(); return *this; }

		CKeyIterator& Seek( int pos ) { m_pos = pos; SeekToPos(); return *this; }
		CKeyIterator& Restart( SeekBound bound = SB_First ) { m_pos = bound == SB_First ? 0 : ( m_keyInfo.m_subKeyCount - 1 ); SeekToPos(); return *this; }
	protected:
		bool SeekToPos( void );
	protected:
		const CKey* m_pParentKey;
		CKey::CInfo m_keyInfo;
		TCHAR* m_pSubKeyName;
		int m_pos;
		LSTATUS m_lastResult;
	};


	class CValueIterator
	{
	public:
		CValueIterator( const CKey* pParentKey, SeekBound bound = SB_First );
		~CValueIterator();

		bool IsValid( void ) const;
		const TCHAR* GetName( void ) const { ASSERT( IsValid() ); return m_pValueName; }		// was operator()
		int GetPos( void ) const { return m_pos; }
		int GetCount( void ) const { return m_keyInfo.m_valueCount; }

		DWORD GetValueType( void ) const { ASSERT( IsValid() ); return m_valueType; }
		DWORD GetValueBuffSize( void ) const { ASSERT( IsValid() ); return m_valueBuffSize; }

		CValueIterator& operator++( void ) { ++m_pos; SeekToPos(); return *this; }
		CValueIterator& operator--( void ) { --m_pos; SeekToPos(); return *this; }

		CValueIterator& Seek( int valueIndex ) { m_pos = valueIndex; SeekToPos(); return *this; }
		CValueIterator& Restart( SeekBound bound = SB_First );
	protected:
		bool SeekToPos( void );
	protected:
		const CKey* m_pParentKey;
		CKey::CInfo m_keyInfo;

		TCHAR* m_pValueName;
		int m_pos;
		LSTATUS m_lastResult;

		DWORD m_valueType;
		DWORD m_valueBuffSize;
	};


	void QuerySubKeyNames( std::vector< std::tstring >& rSubKeyNames, const reg::CKey& key );
}


#endif // Registry_h
