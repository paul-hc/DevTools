#ifndef InputOutput_h
#define InputOutput_h


namespace io
{
	char InputUserKey( bool skipWhitespace = true );
}


namespace func
{
	struct PtrStreamInserter : std::unary_function< void*, void >
	{
		PtrStreamInserter( std::tostream& os, const TCHAR* pSuffix = _T("") ) : m_os( os ), m_pSuffix( pSuffix ) {}

		template< typename ObjectType >
		void operator()( ObjectType* pObject ) const
		{
			m_os << *pObject << m_pSuffix;
		}
	private:
		std::tostream& m_os;
		const TCHAR* m_pSuffix;
	};
}


#endif // InputOutput_h
