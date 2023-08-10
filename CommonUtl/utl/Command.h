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
	// utl::IMessage interface (partial)
	virtual int GetTypeID( void ) const implement;

	std::tstring FormatExecTitle( utl::Verbosity verbosity = utl::Detailed ) const;

	utl::ISubject* GetSubject( void ) const { return m_pSubject; }
	void SetSubject( utl::ISubject* pSubject ) { m_pSubject = pSubject; }		// for serializable commands

	template< typename SubjectType >
	SubjectType* GetSubjectAs( void ) const { return dynamic_cast<SubjectType*>( m_pSubject ); }

	bool HasOriginCmd( void ) const { return m_pOriginCmd != nullptr; }
	CBaseCommand* GetOriginCmd( void ) const { return m_pOriginCmd; }
	void SetOriginCmd( CBaseCommand* pOriginCmd ) { m_pOriginCmd = pOriginCmd; }

	template< typename OriginCmdType >
	OriginCmdType* GetOriginCmdAs( void ) const { return checked_static_cast<OriginCmdType*>( m_pOriginCmd ); }

	// origin or this, whichever comes first
	template< typename CmdType >
	CmdType* GetReportingCmdAs( void ) const { return checked_static_cast<CmdType*>( m_pOriginCmd != nullptr ? m_pOriginCmd : const_cast<CBaseCommand*>( this ) ); }
private:
	persist int m_typeId;
	utl::ISubject* m_pSubject;		// no ownership
	CBaseCommand* m_pOriginCmd;		// command that implements Unexecute via 'this' temporary command - some formatting is redirected to the origin command
};


class CEnumTags;


abstract class CCommand : public CBaseCommand
{
protected:
	CCommand( int typeId, utl::ISubject* pSubject, const CEnumTags* pCmdTags = nullptr );
public:
	virtual ~CCommand();

	// utl::IMessage interface (partial)
	virtual std::tstring Format( utl::Verbosity verbosity ) const implement;			// override for special formatting

	// utl::ICommand interface (partial)
	virtual bool Unexecute( void ) implement;
	virtual bool IsUndoable( void ) const implement;
private:
	const CEnumTags* m_pCmdTags;
};


class CMacroCommand : public CBaseCommand
{
public:
	CMacroCommand( const std::tstring& userInfo = std::tstring(), int typeId = MacroCmdId );
	virtual ~CMacroCommand();

	enum { MacroCmdId = 0xFFFF };

	using CBaseCommand::SetTypeID;
	void SetUserInfo( const std::tstring& userInfo ) { m_userInfo = userInfo; }

	// composite command
	bool IsEmpty( void ) const { return m_subCommands.empty(); }
	utl::ICommand* GetMainCmd( void ) const { return m_pMainCmd; }
	const std::vector<utl::ICommand*>& GetSubCommands( void ) const { return m_subCommands; }

	void AddCmd( utl::ICommand* pSubCmd );
	void AddMainCmd( utl::ICommand* pMainCmd ) { m_pMainCmd = pMainCmd; AddCmd( pMainCmd ); }

	// utl::IMessage interface (partial)
	virtual std::tstring Format( utl::Verbosity verbosity ) const implement;

	// utl::ICommand interface
	virtual bool Execute( void ) implement;
	virtual bool Unexecute( void ) implement;
	virtual bool IsUndoable( void ) const implement;
protected:
	void Serialize( CArchive& archive );				// CObject-like serialization: called from the serializable derived class
protected:
	persist std::tstring m_userInfo;
	persist std::vector<utl::ICommand*> m_subCommands;	// with ownership
	persist utl::ICommand* m_pMainCmd;
};


// Concrete command classes must define a constructor, and override: IsUndoable(), DoExecute() and Unexecute().
// Note: SubjectType implements the utl::ISubject interface
//
template< typename SubjectType >
abstract class CObjectCommand : public CCommand
{
protected:
	CObjectCommand( int typeId, SubjectType* pObject, const CEnumTags* pCmdTags = nullptr )
		: CCommand( typeId, pObject, pCmdTags )
		, m_pObject( pObject )
	{
		ASSERT_PTR( m_pObject );
	}
public:
	SubjectType* GetObject( void ) const { return m_pObject; }

	// base overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const overrides( CCommand )			// standard implementation
	{
		std::tstring info = CCommand::Format( verbosity );
		if ( verbosity != utl::Brief )
			stream::Tag( info, m_pObject->GetDisplayCode(), _T(": ") );
		return info;
	}

	virtual bool Execute( void ) overrides( CCommand )
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
	SubjectType* m_pObject;
};


// Abstract base for command classes that change an object property.
// Concrete command class must implement IsUndoable(), DoExecute() and Unexecute().
//
template< typename SubjectType, typename PropertyType >
abstract class CObjectPropertyCommand : public CObjectCommand<SubjectType>
{
protected:
	CObjectPropertyCommand( int typeId, SubjectType* pObject, const PropertyType& rValue, const CEnumTags* pCmdTags = nullptr )
		: CObjectCommand<SubjectType>( typeId, pObject, pCmdTags )
		, m_value( rValue )
		, m_oldValue( rValue )
	{
	}
public:
	PropertyType m_value;
	PropertyType m_oldValue;
};


#endif // Command_h
