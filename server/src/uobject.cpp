//==================================================================================
//
//      Wolfpack Emu (WP)
//	UO Server Emulation Program
//
//	Copyright 1997, 98 by Marcus Rating (Cironian)
//  Copyright 2001 by holders identified in authors.txt
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
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//	* In addition to that license, if you are running this program or modified
//	* versions of it on a public system you HAVE TO make the complete source of
//	* the version used by you available or provide people with a location to
//	* download it.
//
//
//
//	Wolfpack Homepage: http://www.wpdev.sf.net/
//========================================================================================

// Platform Specifics
#include "platform.h"

// Wolfpack includes
#include "coord.h"
#include "uobject.h"
#include "iserialization.h"
#include "defines.h"

// Debug includes and defines
#undef  DBGFILE
#define DBGFILE "uobject.cpp"

cUObject::cUObject()
{
	init();
}

cUObject::cUObject( cUObject &src )
{
	this->serial = src.serial;
	this->multis = src.multis;
	this->name = src.name;
	this->free = src.free;
}

cUObject::~cUObject()
{
}

void cUObject::init()
{
	this->serial = INVALID_SERIAL;
	this->multis = INVALID_SERIAL;
	this->free = false;
}

void cUObject::Serialize(ISerialization &archive)
{
	if (archive.isReading())
	{
	//	archive.read("name", name);
		archive.read("serial", serial);
		archive.read("multis", multis);
	}
	else if (archive.isWritting())
	{
	//	archive.write("name", name);
		archive.write("serial", serial);
		archive.write("multis", multis);
	}
	archive.doneWritting();
}