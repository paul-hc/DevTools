#ifndef Command_h
#define Command_h
#pragma once

#include "ISubject.h"


class CEnumTags;


abstract class CCommand : public utl::ICommand
						, private utl::noncopyable
{
protected:
	CCommand( unsigned int cmdId, utl::ISubject* pSubject, const CEnumTags* pCmdTags = NULL );
public:
	virtual ~CCommand();

	utl::ISubject* GetSubject( void ) const { return m_pSubject; }

	// utl::IMessage interface implementation
	virtual unsigned int GetTypeID( void ) const;
	virtual std::tstring Format( bool detailed ) const;			// override for special formatting

	// utl::ICommand interface implementation
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
protected:
	void NotifyObservers( void );
private:
	unsigned int m_cmdId;
	utl::ISubject* m_pSubject;		// no ownership
	const CEnumTags* m_pCmdTags;
};


class CMacroCommand : public utl::ICommand
					, private utl::noncopyable
{
	enum { MacroCmdId = 0xFFFF };
public:
	CMacroCommand( const std::tstring& userInfo = std::tstring() );
	virtual ~CMacroCommand();

	// composite command
	bool IsEmpty( void ) const { return m_subCommands.empty(); }
	const std::vector< utl::ICommand* >& GetSubCommands( void ) const { return m_subCommands; }

	void AddCmd( utl::ICommand* pSubCmd );
	void AddMainCmd( utl::ICommand* pMainCmd ) { m_pMainCmd = pMainCmd; AddCmd( pMainCmd ); }

	// utl::IMessage interface implementation
	virtual unsigned int GetTypeID( void ) const;
	virtual std::tstring Format( bool detailed ) const;

	// utl::ICommand interface implementation
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
protected:
	std::tstring m_userInfo;
	std::vector< utl::ICommand* > m_subCommands;		// with ownership
	utl::ICommand* m_pMainCmd;
};


#include <deque>


class CCommandModel : private utl::noncopyable
{
public:
	CCommandModel( void ) {}
	~CCommandModel();

	void Clear( void );
	void RemoveExpiredCommands( size_t maxSize );

	bool Execute( utl::ICommand* pCmd );
	void Push( utl::ICommand* pCmd );					// already executed by the caller

	bool Undo( size_t stepCount = 1 );
	bool Redo( size_t stepCount = 1 );

	bool CanUndo( void ) const;
	bool CanRedo( void ) const;
private:
	// commands stored in UNDO and REDO must keep their objects alive
	std::deque< utl::ICommand* > m_undoStack;			// stack top at end
	std::deque< utl::ICommand* > m_redoStack;			// stack top at end
};


#endif // Command_h
