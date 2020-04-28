#ifndef Command_h
#define Command_h
#pragma once

#include "ICommand.h"


abstract class CBaseCommand : public utl::ICommand
							, private utl::noncopyable
{
protected:
	CBaseCommand( int typeId, utl::ISubject* pSubject );

	void SetTypeID( int typeId ) { m_typeId = typeId; }
	void NotifyObservers( void );

	void Serialize( CArchive& archive );				// CObject-like serialization: called from the serializable derived class
public:
	utl::ISubject* GetSubject( void ) const { return m_pSubject; }
	void SetSubject( utl::ISubject* pSubject ) { m_pSubject = pSubject; }		// for serializable commands

	template< typename ObjectType >
	ObjectType* GetSubjectAs( void ) const { return dynamic_cast< ObjectType* >( m_pSubject ); }

	// utl::IMessage interface (partial)
	virtual int GetTypeID( void ) const;
private:
	persist int m_typeId;
	utl::ISubject* m_pSubject;		// no ownership
};


class CEnumTags;


abstract class CCommand : public CBaseCommand
{
protected:
	CCommand( int typeId, utl::ISubject* pSubject, const CEnumTags* pCmdTags = NULL );
public:
	virtual ~CCommand();

	// utl::IMessage interface (partial)
	virtual std::tstring Format( utl::Verbosity verbosity ) const;			// override for special formatting

	// utl::ICommand interface (partial)
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
private:
	const CEnumTags* m_pCmdTags;
};


class CMacroCommand : public CBaseCommand
{
	enum { MacroCmdId = 0xFFFF };
public:
	CMacroCommand( const std::tstring& userInfo = std::tstring(), int typeId = MacroCmdId );
	virtual ~CMacroCommand();

	using CBaseCommand::SetTypeID;
	void SetUserInfo( const std::tstring& userInfo ) { m_userInfo = userInfo; }

	// composite command
	bool IsEmpty( void ) const { return m_subCommands.empty(); }
	const std::vector< utl::ICommand* >& GetSubCommands( void ) const { return m_subCommands; }

	void AddCmd( utl::ICommand* pSubCmd );
	void AddMainCmd( utl::ICommand* pMainCmd ) { m_pMainCmd = pMainCmd; AddCmd( pMainCmd ); }

	// utl::IMessage interface (partial)
	virtual std::tstring Format( utl::Verbosity verbosity ) const;

	// utl::ICommand interface
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
protected:
	void Serialize( CArchive& archive );				// CObject-like serialization: called from the serializable derived class
protected:
	persist std::tstring m_userInfo;
	persist std::vector< utl::ICommand* > m_subCommands;		// with ownership
	persist utl::ICommand* m_pMainCmd;
};


// Concrete command classes must define a constructor, and override: IsUndoable(), DoExecute() and Unexecute().
//
template< typename ObjectType >
abstract class CObjectCommand : public CCommand
{
protected:
	CObjectCommand( int typeId, ObjectType* pObject, const CEnumTags* pCmdTags = NULL )
		: CCommand( typeId, pObject, pCmdTags )
		, m_pObject( pObject )
	{
		ASSERT_PTR( m_pObject );
	}
public:
	ObjectType* GetObject( void ) const { return m_pObject; }

	// base overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const			// standard implementation
	{
		std::tstring info = CCommand::Format( verbosity );
		if ( verbosity != utl::Brief )
			stream::Tag( info, m_pObject->GetDisplayCode(), _T(": ") );
		return info;
	}

	virtual bool Execute( void )
	{
		if ( !DoExecute() )
			return false;
		NotifyObservers();
		return true;
	}
protected:
	// overridables
	virtual bool DoExecute( void ) = 0;
protected:
	ObjectType* m_pObject;
};


// Abstract base for command classes that change an object property.
// Concrete command class must implement IsUndoable(), DoExecute() and Unexecute().
//
template< typename ObjectType, typename PropertyType >
class CObjectPropertyCommand : public CObjectCommand< ObjectType >
{
protected:
	CObjectPropertyCommand( int typeId, ObjectType* pObject, const PropertyType& rValue, const CEnumTags* pCmdTags = NULL )
		: CObjectCommand< ObjectType >( typeId, pObject, pCmdTags )
		, m_value( rValue )
		, m_oldValue( rValue )
	{
	}
public:
	PropertyType m_value;
	PropertyType m_oldValue;
};


#endif // Command_h
