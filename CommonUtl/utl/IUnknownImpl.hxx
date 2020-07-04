#ifndef IUnknownImpl_hxx
#define IUnknownImpl_hxx


namespace utl
{
	template< typename InterfaceT >
	ULONG STDMETHODCALLTYPE IUnknownImpl< InterfaceT >::Release( void )
	{
		ULONG refCount = ::InterlockedDecrement( &m_refCount );
		if ( 0 == m_refCount )
			delete this;

		return refCount;
	}
}


#endif // IUnknownImpl_hxx
