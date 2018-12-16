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
private:
	enum { MaxCommands = 60 };
};


#endif // CommandService_h
