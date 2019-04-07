#ifndef ISubject_h
#define ISubject_h
#pragma once


// Implemented by managed objects or inherited by managed interfaces.
//
interface IMemoryManaged
{
	virtual ~IMemoryManaged() = 0
	{
	}
};


namespace utl
{
	enum Verbosity { Brief, Detailed, DetailFields };

	interface IMessage
	{
		virtual int GetTypeID( void ) const = 0;
		virtual std::tstring Format( utl::Verbosity verbosity ) const = 0;
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
	inline const std::tstring& GetSafeCode( const ObjectType* pObject )
	{
		return pObject != NULL ? pObject->GetCode() : str::GetEmpty();
	}

	template< typename ObjectType >
	inline std::tstring GetSafeDisplayCode( const ObjectType* pObject )
	{
		return pObject != NULL ? pObject->GetDisplayCode() : str::GetEmpty();
	}

	inline int GetSafeTypeID( const IMessage* pMessage ) { return pMessage != NULL ? pMessage->GetTypeID() : 0; }
}


namespace func
{
	// adapters for comparison predicates

	struct AsCode
	{
		template< typename ObjectType >
		const std::tstring& operator()( const ObjectType* pObject ) const
		{
			return GetSafeCode( pObject );
		}
	};

	struct AsDisplayCode
	{
		template< typename ObjectType >
		std::tstring operator()( const ObjectType* pObject ) const
		{
			return GetSafeDisplayCode( pObject );
		}
	};
}


namespace utl
{
	template< typename CodeContainerT, typename ObjectContainerT >
	void QueryObjectCodes( CodeContainerT& rItemCodes, const ObjectContainerT& objects )
	{
		rItemCodes.reserve( rItemCodes.size() + objects.size() );

		for ( typename ObjectContainerT::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
			rItemCodes.push_back( ( *itObject )->GetCode() );		// works for containers of std::tstring, fs::CPath
	}


	template< typename IteratorT, typename GetTextFuncT >
	void QueryTextItems( std::vector< std::tstring >& rTextItems, IteratorT itFirst, IteratorT itLast, GetTextFuncT getTextFunc )
	{
		rTextItems.reserve( rTextItems.size() + std::distance( itFirst, itLast ) );

		for ( ; itFirst != itLast; ++itFirst )
			if ( *itFirst != NULL )
				rTextItems.push_back( getTextFunc( *itFirst ) );
	}

	template< typename IteratorT, typename GetTextFuncT >
	void MakeTextItemsList( std::tstring& rText, IteratorT itFirst, IteratorT itLast, GetTextFuncT getTextFunc, const TCHAR sep[] )
	{
		std::vector< std::tstring > textItems;
		QueryTextItems( textItems, itFirst, itLast, getTextFunc );

		rText = str::Join( textItems, sep );
	}

	template< typename ContainerT >
	std::tstring MakeCodeList( const ContainerT& objects, const TCHAR sep[] )
	{
		std::tstring text;
		MakeTextItemsList( text, objects.begin(), objects.end(), func::AsCode(), sep );
		return text;
	}

	template< typename ContainerT >
	std::tstring MakeDisplayCodeList( const ContainerT& objects, const TCHAR sep[] )
	{
		std::tstring text;
		MakeTextItemsList( text, objects.begin(), objects.end(), func::AsDisplayCode(), sep );
		return text;
	}
}


#endif // ISubject_h
