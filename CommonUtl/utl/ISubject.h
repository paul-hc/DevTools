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
	interface IMessage
	{
		virtual int GetTypeID( void ) const = 0;
		virtual std::tstring Format( bool detailed ) const = 0;
	};

	interface ICommand : public IMemoryManaged
					   , public IMessage
	{
		virtual bool Execute( void ) = 0;
		virtual bool Unexecute( void ) = 0;
		virtual bool IsUndoable( void ) const = 0;
	};


	interface IObserver;

	interface ISubject
	{
		virtual const std::tstring& GetCode( void ) const = 0;
		virtual std::tstring GetDisplayCode( void ) const { return GetCode(); }

		virtual void AddObserver( IObserver* pObserver ) = 0;
		virtual void RemoveObserver( IObserver* pObserver ) = 0;
		virtual void UpdateAllObservers( IMessage* pMessage ) = 0;
	};

	interface IObserver
	{
		virtual void OnUpdate( ISubject* pSubject, IMessage* pMessage ) = 0;
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
