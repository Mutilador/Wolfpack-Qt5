//==================================================================================
//
//      Wolfpack Emu (WP)
//	UO Server Emulation Program
//
//	Copyright 1997, 98 by Marcus Rating (Cironian)
//      Copyright 2001-2003 by holders identified in authors.txt
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 59 Temple Palace - Suite 330, Boston, MA 02111-1307, USA.
//
//	* In addition to that license, if you are running this program or modified
//	* versions of it on a public system you HAVE TO make the complete source of
//	* the version used by you available or provide people with a location to
//	* download it.
//
//
//
//	Wolfpack Homepage: http://wpdev.sf.net/
//==================================================================================

#if !defined( __LOG_H__ )
#define __LOG_H__

// QT Includes
#include <qfile.h>

// Wolfpack Includes
#include "singleton.h"

// Log Levels
enum eLogLevel
{
	LOG_MESSAGE = 0,
	LOG_ERROR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_TRACE,
	LOG_DEBUG
};

class cUOSocket;

class cLog
{
protected:
	eLogLevel loglevel;
	QFile logfile;
	bool checkLogFile();
	unsigned char currentday; // Day of the month our current logfile is for

public:
	cLog(): loglevel( LOG_DEBUG ) {}
	virtual ~cLog();
	
	// Prints to the logfile only
	void log( eLogLevel, cUOSocket*, const QString&, bool timestamp = true );

	// Sends to the console and logs too
	void print( eLogLevel, const QString&, bool timestamp = true );
	void print( eLogLevel, cUOSocket*, const QString&, bool timestamp = true );
};

typedef SingletonHolder< cLog > Log;

#define DEBUG_LOG( value ) Log::instance()->log( LOG_DEBUG, QString( "%1 (%2:%3)" ).arg( value ).arg( __FILE__ ).arg( __LINE__ ) );

#endif
