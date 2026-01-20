#ifndef IUnknownImpl_h
#define IUnknownImpl_h
#pragma once


namespace utl
{
	template< typename InterfaceT >
	class IUnknownImpl : public InterfaceT
	{
	protected:
		IUnknownImpl( void )
			: m_rIID( __uuidof( InterfaceT ) )
			, m_refCount( 1 )
		{
		}

		virtual ~IUnknownImpl()
		{
		}

		ULONG GetRefCount( void ) const { return m_refCount; }
	public:
		virtual HRESULT STDMETHODCALLTYPE QueryInterface( const IID& rIID, void** ppObject )
		{
			if ( nullptr == ppObject )
				return E_INVALIDARG;

			*ppObject = nullptr;			// always set out parameter to NULL, validating it first

			if ( m_rIID == rIID || IID_IUnknown == rIID )
			{
				*ppObject = (void*)this;
				AddRef();
				return NOERROR;
			}
			return E_NOINTERFACE;
		}

		virtual ULONG STDMETHODCALLTYPE AddRef( void )
		{
			::InterlockedIncrement( &m_refCount );
			return m_refCount;
		}

		virtual ULONG STDMETHODCALLTYPE Release( void )
		{
			ULONG refCount = ::InterlockedDecrement( &m_refCount );
			if ( 0 == m_refCount )
				delete this;

			return refCount;
		}
	private:
		const IID& m_rIID;
		ULONG m_refCount;
	};
}


#endif // IUnknownImpl_h
