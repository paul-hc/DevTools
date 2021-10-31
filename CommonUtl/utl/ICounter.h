#ifndef ICounter_h
#define ICounter_h
#pragma once


namespace utl
{
	interface ICounter
	{
		virtual size_t GetCount( void ) const = 0;

		virtual void AddCount( void ) = 0;
		virtual void ReleaseCount( void ) = 0;
	};


	class CCounter : public ICounter
	{
	public:
		CCounter( size_t count = 0 ) : m_count( count ) {}

		// ICounter interface
		virtual size_t GetCount( void ) const { return m_count; }
	protected:
		virtual void AddCount( void ) { ++m_count; }
		virtual void ReleaseCount( void ) { --m_count; }
	private:
		size_t m_count;
	};


	class CScopedIncrement
	{
	public:
		CScopedIncrement( ICounter* pCounter )		// optional counter
			: m_pCounter( pCounter )
		{
			if ( m_pCounter != NULL )
				m_pCounter->AddCount();
		}

		CScopedIncrement( ICounter& rCounter )		// mandatory counter
			: m_pCounter( &rCounter )
		{
			m_pCounter->AddCount();
		}

		~CScopedIncrement()
		{
			if ( m_pCounter != NULL )
				m_pCounter->ReleaseCount();
		}
	private:
		ICounter* m_pCounter;
	};
}


#endif // ICounter_h
