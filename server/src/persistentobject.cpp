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
//	Wolfpack Homepage: http://wpdev.sf.net/
//========================================================================================

#include "persistentobject.h"
#include "dbdriver.h"

PersistentObject::PersistentObject() : isPersistent(false)
{
}

void PersistentObject::save( const QString& /* = QString::null  */ )
{
	isPersistent = true;
}

void PersistentObject::load( const QString& /* = QString::null  */ )
{
	isPersistent = true;
}

bool PersistentObject::del( const QString& /* = QString::null */ )
{
	isPersistent = false;
	return true;
}

void PersistentObject::save( QStringList *tables, QStringList *fields, QStringList *conditions )
{
	if( !fields || !conditions )
		return;

	// Otherwise check if we are updating or inserting
	if( isPersistent )
	{
		// Update
		QString sql( "UPDATE " + tables->join(",") + " SET " + fields->join( "," ) + " WHERE " + conditions->join( "," ) );
		cDBDriver query;
		if( !query.execute( sql ) )
			throw query.error().latin1();
	}
	else
	{
		// Insert
		QString sql( "INSERT INTO " + tables->join(",") + " SET " + fields->join( "," ) );
		cDBDriver query;
		if( !query.execute( sql ) )
			throw query.error().latin1();
	}

	delete fields;
	delete conditions;
}

