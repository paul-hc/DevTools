#ifndef ISubject_h
#define ISubject_h
#pragma once


/**
	Implemented by managed objects or inherited by managed interfaces.
	It only provides object distruction through the virtual destructor.
*/

interface IMemoryManaged
{
	virtual ~IMemoryManaged() = 0;
};

inline IMemoryManaged::~IMemoryManaged()
{
}


namespace utl
{
	interface ISubject
	{
		virtual std::tstring GetCode( void ) const = 0;
		virtual std::tstring GetDisplayCode( void ) const { return GetCode(); }
	};
}


namespace utl
{
	// safe code access for algorithms - let it work on any type (even if not based on utl::ISubject)

	template< typename ObjectType >
	inline std::tstring GetSafeCode( const ObjectType* pObject )
	{
		return pObject != NULL ? pObject->GetCode() : std::tstring();
	}
}


#endif // ISubject_h
