#ifndef ScopedValue_h
#define ScopedValue_h
#pragma once


template< typename ValueType >
class CScopedValue
{
public:
	CScopedValue( ValueType* pValue, const ValueType& newValue )
		: m_pValue( pValue )
	{
		ASSERT_PTR( m_pValue );
		m_oldValue = *m_pValue;
		*m_pValue = newValue;
	}

	~CScopedValue()
	{
		*m_pValue = m_oldValue;
	}
private:
	ValueType* m_pValue;
	ValueType m_oldValue;
};


template< typename ValueType >
class CScopedFlag
{
public:
	CScopedFlag( ValueType* pValue, const ValueType& flags )
		: m_pValue( pValue )
		, m_flagsMask( flags )
		, m_oldFlags( *m_pValue & m_flagsMask )
	{
		ASSERT_PTR( m_pValue );
		*m_pValue |= flags;
	}

	~CScopedFlag()
	{
		*m_pValue &= ~m_flagsMask;
		*m_pValue |= m_oldFlags;
	}
private:
	ValueType* m_pValue;
	ValueType m_flagsMask;
	ValueType m_oldFlags;
};


template< typename ValueType = int >
class CScopedIncrement
{
public:
	CScopedIncrement( ValueType* pValue )
		: m_pValue( pValue )
	{
		ASSERT( m_pValue != NULL && *m_pValue >= 0 );
		++*m_pValue;
	}

	~CScopedIncrement()
	{
		ASSERT( *m_pValue > 0 );
		--*m_pValue;
	}
private:
	ValueType* m_pValue;
};


#endif // ScopedValue_h
