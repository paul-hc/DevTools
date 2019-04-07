#ifndef AppCmdService_h
#define AppCmdService_h
#pragma once

#include "utl/CommandService.h"
#include "Application_fwd.h"


class CAppCmdService : public CCommandService
{
public:
	CAppCmdService( void );
	~CAppCmdService();

	bool SaveCommandModel( void );
	bool LoadCommandModel( void );

	bool UndoRedoAt( svc::StackType stackType, size_t topPos );
private:
	bool UndoAt( size_t topPos );
	bool RedoAt( size_t topPos );
private:
	enum { MaxCommands = 60 };
};


#endif // CommandService_h
