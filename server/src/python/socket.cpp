//==================================================================================
//
//      Wolfpack Emu (WP)
//	UO Server Emulation Program
//
//  Copyright 2001-2003 by holders identified in authors.txt
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

#include "utilities.h"
#include "../network/uosocket.h"
#include "../network/uotxpackets.h"
#include "target.h"
#include "gump.h"

/*!
	Struct for WP Python Sockets
*/
typedef struct {
    PyObject_HEAD;
	cUOSocket *pSock;
} wpSocket;

// Forward Declarations
static PyObject *wpSocket_getAttr( wpSocket *self, char *name );
static int wpSocket_setAttr( wpSocket *self, char *name, PyObject *value );

/*!
	The typedef for Wolfpack Python chars
*/
static PyTypeObject wpSocketType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "wpsocket",
    sizeof(wpSocketType),
    0,
    wpDealloc,
    0,
    (getattrfunc)wpSocket_getAttr,
    (setattrfunc)wpSocket_setAttr,
};

PyObject* PyGetSocketObject( cUOSocket *socket )
{
	if( !socket )
	{
		Py_INCREF( Py_None );
		return Py_None;
	}

	wpSocket *rVal = PyObject_New( wpSocket, &wpSocketType );
	rVal->pSock = socket;

	if( rVal )
		return (PyObject*)rVal;

	Py_INCREF( Py_None );
	return Py_None;
}

/*!
	Disconnects the socket.
*/
static PyObject* wpSocket_disconnect( wpSocket* self, PyObject* args )
{
	Q_UNUSED(args);
	if( !self->pSock )
		return PyFalse;

	self->pSock->socket()->close();
	return PyTrue;
}

/*!
	Sends a system message to the socket
*/
static PyObject* wpSocket_sysmessage( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	QString message;
	PyObject* param;
	if( PyTuple_Size( args ) > 0 )
		param = PyTuple_GetItem( args, 0 );

	if( checkArgStr( 0 ) )
	{
		message = PyString_AsString( param );
	}
	else if( checkArgUnicode( 0 ) )
	{
		message.setUnicodeCodes( (unsigned short*)getArgUnicode( 0 ), getUnicodeSize( 0 ) ) ;
	}
	else
	{
		PyErr_BadArgument();
		return NULL;
	}

	UINT16 color = 0x3b2;
	UINT16 font = 3;

	if( PyTuple_Size( args ) > 1 && checkArgInt( 1 ) )
		color = PyInt_AsLong( PyTuple_GetItem( args, 1 ) );

	if( PyTuple_Size( args ) > 2 && checkArgInt( 2 ) )
		font = PyInt_AsLong( PyTuple_GetItem( args, 2 ) );

	self->pSock->sysMessage( message, color, font );

	return PyTrue;
}

/*!
	Sends a localized message to the socket
*/
static PyObject* wpSocket_clilocmessage( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	if( !checkArgInt( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	UINT16 color = 0x37;
	UINT16 font = 3;

	QString params( "" );

	if( checkArgStr( 1 ) )
	{
		params = getArgStr( 1 );
	}
	else if( checkArgUnicode( 1 ) )
	{
		params.setUnicodeCodes( (unsigned short*)getArgUnicode( 1 ), getUnicodeSize( 1 ) ) ;
	}

	if( checkArgInt( 2 ) )
		color = getArgInt( 2 );

	if( checkArgInt( 3 ) )
		font = getArgInt( 3 );

	// Object
	cUObject *object = 0;

	if( checkArgChar( 4 ) )
		object = getArgChar( 4 );
	else if( checkArgItem( 4 ) )
		object = getArgItem( 4 );

	if( checkArgStr( 5 ) )
	{
		bool dontmove = false;
		bool prepend = false;

		if( checkArgInt( 6 ) && getArgInt( 6 ) == 1 )
			dontmove = true;

		if( checkArgInt( 7 ) && getArgInt( 7 ) == 1 )
			prepend = true;

		// Message Affix
		QString affix = getArgStr( 5 );
		self->pSock->clilocMessageAffix( getArgInt( 0 ), params, affix, color, font, object, dontmove, prepend );
	}
	else
	{
		self->pSock->clilocMessage( getArgInt( 0 ), params, color, font, object );
	}

	return PyTrue;
}

/*!
	Sends speech of a given object to the socket
*/
static PyObject* wpSocket_showspeech( wpSocket* self, PyObject* args )
{
	// Needed/Allowed arugments:
	// First Argument: Source
	// Second Argument: Speech
	// optional:
	// Third Argument: Color
	// Fourth Argument: SpeechType
	if( !self->pSock || !checkArgStr( 1 ) || !checkArgObject( 0 ) ) 
	{
		PyErr_BadArgument();
		return NULL;
	}

	cUObject *object = NULL;
	QString speech( PyString_AsString( PyTuple_GetItem( args, 1 ) ) );
	UINT16 color = 0x3b2;
	UINT16 font = 3;
	UINT8 type = 0;
	cUOTxUnicodeSpeech::eSpeechType eType;

	object = getWpItem( PyTuple_GetItem( args, 0 ) );

	if( !object )
		object = getWpChar( PyTuple_GetItem( args, 0 ) );

	if( !object )
		return PyFalse;

	if( PyTuple_Size( args ) > 2 && checkArgInt( 2 ) )
		color = getArgInt( 2 );

	if( PyTuple_Size( args ) > 3 && checkArgInt( 3 ) )
		font = getArgInt( 3 );

	if( PyTuple_Size( args ) > 4 && checkArgInt( 4 ) )
		type = getArgInt( 4 );

	switch( type )
	{
	case 1:		eType = cUOTxUnicodeSpeech::Broadcast;		break;
	case 2:		eType = cUOTxUnicodeSpeech::Emote;			break;
	case 6:		eType = cUOTxUnicodeSpeech::System;			break;
	case 8:		eType = cUOTxUnicodeSpeech::Whisper;		break;
	case 9:		eType = cUOTxUnicodeSpeech::Yell;			break;
	case 0:
	default:
				eType = cUOTxUnicodeSpeech::Regular;		break;
	};

	self->pSock->showSpeech( object, speech, color, font, eType );

	return PyTrue;
}

/*!
	Attachs a target request to the socket.
*/
static PyObject* wpSocket_attachtarget( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	if( !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	// Collect Data
	QString responsefunc = getArgStr( 0 );
	QString cancelfunc, timeoutfunc;
	UINT16 timeout = 0;
	PyObject *targetargs = 0;

	// If Second argument is present, it has to be a tuple
	if( PyTuple_Size( args ) > 1 && PyList_Check( PyTuple_GetItem( args, 1 ) ) )
		targetargs = PyList_AsTuple( PyTuple_GetItem( args, 1 ) );

	if( !targetargs )
		targetargs = PyTuple_New( 0 );

	if( checkArgStr( 2 ) )
		cancelfunc = getArgStr( 2 );

	if( checkArgStr( 3 ) && checkArgInt( 4 ) )
	{
		timeoutfunc = getArgStr( 3 );
		timeout = getArgInt( 4 );
	}

	cPythonTarget *target = new cPythonTarget( responsefunc, timeoutfunc, cancelfunc, targetargs );
	if( timeout != 0 )
		target->setTimeout( uiCurrentTime + timeout );
	self->pSock->attachTarget( target );

	return PyTrue;
}
/*!
	Attachs a multi target request to the socket.
*/
static PyObject* wpSocket_attachmultitarget( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	if( !checkArgStr( 0 ) && !checkArgInt( 1 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	// Collect Data
	QString responsefunc = getArgStr( 0 );
	UINT16 multiid = getArgInt( 1 );
	QString cancelfunc, timeoutfunc;
	UINT16 timeout = 0;
	PyObject *targetargs = 0;

	// If Third argument is present, it has to be a tuple
	if( PyTuple_Size( args ) > 2 && PyList_Check( PyTuple_GetItem( args, 2 ) ) )
		targetargs = PyList_AsTuple( PyTuple_GetItem( args, 2 ) );

	if( !targetargs )
		targetargs = PyTuple_New( 0 );

	if( checkArgStr( 2 ) )
		cancelfunc = getArgStr( 2 );

	if( checkArgStr( 3 ) && checkArgInt( 4 ) )
	{
		timeoutfunc = getArgStr( 3 );
		timeout = getArgInt( 4 );
	}

	cPythonTarget *target = new cPythonTarget( responsefunc, timeoutfunc, cancelfunc, targetargs );
	if( timeout != 0 )
		target->setTimeout( uiCurrentTime + timeout );
	self->pSock->attachTarget( target, multiid + 0x4000 );

	return PyTrue;
}

/*!
	Begins CH customization
*/
static PyObject* wpSocket_customize( wpSocket* self, PyObject* args )
{
	Q_UNUSED(args);
	if( !self->pSock )
		return PyFalse;

	if( !checkArgItem( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}
	P_ITEM signpost = getArgItem( 0 );

	cUOTxStartCustomHouse custom;
	custom.setSerial( signpost->getTag( "house" ).toInt() ); // Morex of signpost contain serial of house
	self->pSock->send( &custom );
	return PyTrue;
}


/*!
	Sends a gump to the socket. This function is used internally only.
*/
static PyObject* wpSocket_sendgump(wpSocket* self, PyObject* args) {
	// Parameters:
	// x, y, nomove, noclose, nodispose, serial, type, layout, text, callback, args
	int x, y;
	bool nomove, noclose, nodispose;
	unsigned int serial, type;
	PyObject *layout, *texts, *py_args;
	char *callback;

	if (!PyArg_ParseTuple(args, "iiBBBIIO!O!sO!:socket.sendgump", &x, &y, &nomove, 
		&noclose, &nodispose, &serial, &type, &PyList_Type, &layout, &PyList_Type, &texts, 
		&callback, &PyList_Type, &py_args)) {
		return 0;
	}

	// Convert py_args to a tuple
	py_args = PyList_AsTuple(py_args);

	cPythonGump *gump = new cPythonGump( callback, py_args );
	if( serial )
		gump->setSerial( serial );

	if( type )
		gump->setType( type );

	gump->setX( x );
	gump->setY( y );
	gump->setNoClose( noclose );
	gump->setNoMove( nomove );
	gump->setNoDispose( nodispose );

	INT32 i;
	for (i = 0; i < PyList_Size( layout ); ++i) {
        PyObject *item = PyList_GetItem(layout, i);

		if (PyString_Check(item)) {
			gump->addRawLayout(PyString_AsString(item));
		} else if (PyUnicode_Check(item)) {
			gump->addRawLayout(QString::fromUcs2((ushort*)PyUnicode_AS_UNICODE(item)));
		} else {
			gump->addRawLayout("");
		}
	}

	for (i = 0; i < PyList_Size(texts); ++i) {
        PyObject *item = PyList_GetItem(texts, i);

		if (PyString_Check(item)) {
			gump->addRawText(PyString_AsString(item));
		} else if (PyUnicode_Check(item)) {
			gump->addRawText(QString::fromUcs2((ushort*)PyUnicode_AS_UNICODE(item)));
		} else {
			gump->addRawText("");
		}
	}

	self->pSock->send( gump );

	return PyInt_FromLong( gump->serial() );
}

/*!
	Closes a gump that has been sent to the client using it's
	serial.
*/
static PyObject* wpSocket_closegump( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;
	
	if( !checkArgInt( 0 ) )
	{
		PyErr_BadArgument();
		return 0;
	}

	cUOTxCloseGump closeGump;
	closeGump.setButton( 0 );
	closeGump.setType( getArgInt( 0 ) );
	self->pSock->send( &closeGump );

	return PyTrue;
}

/*!
	Resends the world around this socket.
*/
static PyObject* wpSocket_resendworld( wpSocket* self, PyObject* args )
{
	Q_UNUSED(args);
	if( !self->pSock )
		return PyFalse;
	self->pSock->resendWorld( false );
	return PyTrue;
}

/*!
	Resends the player only.
*/
static PyObject* wpSocket_resendplayer( wpSocket* self, PyObject* args )
{
	Q_UNUSED(args);
	if( !self->pSock )
		return PyFalse;
	self->pSock->resendPlayer( false );
	return PyTrue;
}

/*!
	Sends a container and it's content to a socket.
*/
static PyObject* wpSocket_sendcontainer( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;
	
	if( !checkArgItem( 0 ) )
	{
		PyErr_BadArgument();
		return 0;
	}

	if( !getArgItem( 0 ) )
		return PyFalse;

	self->pSock->sendContainer( getArgItem( 0 ) );

	return PyTrue;
}

/*!
	Sends a packet to this socket.
*/
static PyObject* wpSocket_sendpacket( wpSocket* self, PyObject* args )
{
	if( PyTuple_Size( args ) != 1 )
	{
		PyErr_BadArgument();
		return 0;
	}

	PyObject *list = PyTuple_GetItem( args, 0 );

	if( !PyList_Check( list ) )
	{
		PyErr_BadArgument();
		return 0;
	}

	// Build a packet
	int packetLength = PyList_Size( list );

	QByteArray buffer( packetLength );

	for( int i = 0; i < packetLength; ++i )
		buffer[i] = PyInt_AsLong( PyList_GetItem( list, i ) );

	cUOPacket packet( buffer );
	self->pSock->send( &packet );

	return PyTrue;
}

static PyObject* wpSocket_sendpaperdoll( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;
	
	if( !checkArgChar( 0 ) )
	{
		PyErr_BadArgument();
		return 0;
	}

	if( !getArgChar( 0 ) )
		return PyFalse;

	self->pSock->sendPaperdoll( getArgChar( 0 ) );

	return PyTrue;
}

/*!
	Returns the custom tag passed
*/
static PyObject* wpSocket_gettag( wpSocket* self, PyObject* args )
{
	if( PyTuple_Size( args ) < 1 || !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	QString key = PyString_AsString( PyTuple_GetItem( args, 0 ) );
	cVariant value = self->pSock->tags().get(key);

	if( value.type() == cVariant::String )
		return PyUnicode_FromUnicode((Py_UNICODE*)value.toString().ucs2(), value.toString().length());
	else if( value.type() == cVariant::Int )
		return PyInt_FromLong( value.asInt() );
	else if( value.type() == cVariant::Double )
		return PyFloat_FromDouble( value.asDouble() );

	Py_INCREF( Py_None );
	return Py_None;
}

/*!
	Sets a custom tag
*/
static PyObject* wpSocket_settag( wpSocket* self, PyObject* args )
{
	char *key;
	PyObject *object;

	if (!PyArg_ParseTuple( args, "sO:char.settag( name, value )", &key, &object ))
		return 0;

	if (PyString_Check(object)) {
		self->pSock->tags().set(key, cVariant(PyString_AsString(object)));
	} else if (PyUnicode_Check(object)) {
		self->pSock->tags().set(key, cVariant(QString::fromUcs2((ushort*)PyUnicode_AsUnicode(object))));
	} else if (PyInt_Check(object)) {
		self->pSock->tags().set(key, cVariant((int)PyInt_AsLong(object)));
	} else if (PyFloat_Check(object)) {
		self->pSock->tags().set(key, cVariant((double)PyFloat_AsDouble(object)));
	}

	return PyTrue;
}

/*!
	Checks if a certain tag exists
*/
static PyObject* wpSocket_hastag( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	if( PyTuple_Size( args ) < 1 || !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	QString key = getArgStr( 0 );
	
	return self->pSock->tags().has( key ) ? PyTrue : PyFalse;
}

/*!
	Deletes a given tag
*/
static PyObject* wpSocket_deltag( wpSocket* self, PyObject* args )
{
	if( !self->pSock )
		return PyFalse;

	if( PyTuple_Size( args ) < 1 || !checkArgStr( 0 ) )
	{
		PyErr_BadArgument();
		return NULL;
	}

	QString key = getArgStr( 0 );
	self->pSock->tags().remove( key );

	return PyTrue;
}

/*!
	Resends the status window to this client.
*/
static PyObject *wpSocket_resendstatus( wpSocket *self, PyObject *args )
{
	Q_UNUSED(args);
	self->pSock->sendStatWindow();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *wpSocket_questarrow( wpSocket *self, PyObject *args )
{
	int show;
	int x = 0;
	int y = 0;

	if( !PyArg_ParseTuple( args, "i|ii:socket.questarrow( show, x, y )", &show, &x, &y ) )
		return 0;

	self->pSock->sendQuestArrow( show, x, y );
	return PyTrue;
}

static PyObject *wpSocket_log( wpSocket *self, PyObject *args )
{
	char loglevel;
	char *text;

	if( !PyArg_ParseTuple( args, "bs:socket.log( loglevel, text )", &loglevel, &text ) )
		return 0;

	self->pSock->log( (eLogLevel)loglevel, text );
	return PyTrue;
}

static PyMethodDef wpSocketMethods[] = 
{
	{ "questarrow",			(getattrofunc)wpSocket_questarrow, METH_VARARGS, NULL },
    { "sysmessage",			(getattrofunc)wpSocket_sysmessage, METH_VARARGS, "Sends a system message to the char." },
    { "clilocmessage",		(getattrofunc)wpSocket_clilocmessage, METH_VARARGS, "Sends a localized message to the socket." },
	{ "showspeech",			(getattrofunc)wpSocket_showspeech, METH_VARARGS, "Sends raw speech to the socket." },
	{ "disconnect",			(getattrofunc)wpSocket_disconnect, METH_VARARGS, "Disconnects the socket." },
	{ "attachtarget",		(getattrofunc)wpSocket_attachtarget,  METH_VARARGS, "Adds a target request to the socket" },
	{ "attachmultitarget",	(getattrofunc)wpSocket_attachmultitarget,  METH_VARARGS, "Adds a multi target request to the socket" },
	{ "sendgump",			(getattrofunc)wpSocket_sendgump,	METH_VARARGS, "INTERNAL! Sends a gump to this socket." },
	{ "closegump",			(getattrofunc)wpSocket_closegump,	METH_VARARGS, "Closes a gump that has been sent to the client." },
	{ "resendworld",		(getattrofunc)wpSocket_resendworld,  METH_VARARGS, "Sends the surrounding world to this socket." },
	{ "resendplayer",		(getattrofunc)wpSocket_resendplayer,  METH_VARARGS, "Resends the player only." },
	{ "sendcontainer",		(getattrofunc)wpSocket_sendcontainer,  METH_VARARGS, "Sends a container to the socket." },
	{ "sendpacket",			(getattrofunc)wpSocket_sendpacket,		METH_VARARGS, "Sends a packet to this socket." },
	{ "sendpaperdoll",		(getattrofunc)wpSocket_sendpaperdoll,	METH_VARARGS,	"Sends a char's paperdool to this socket."	},
	{ "gettag",				(getattrofunc)wpSocket_gettag,	METH_VARARGS,	"Gets a tag from a socket." },
	{ "settag",				(getattrofunc)wpSocket_settag,	METH_VARARGS,	"Sets a tag to a socket." },
	{ "hastag",				(getattrofunc)wpSocket_hastag,	METH_VARARGS,	"Checks if a socket has a specific tag." },
	{ "deltag",				(getattrofunc)wpSocket_deltag,	METH_VARARGS,	"Delete specific tag." },
	{ "resendstatus",		(getattrofunc)wpSocket_resendstatus, METH_VARARGS,	"Resends the status windows to this client." },
	{ "resendstatus",		(getattrofunc)wpSocket_resendstatus, METH_VARARGS,	"Resends the status windows to this client." },
	{ "customize",			(getattrofunc)wpSocket_customize, METH_VARARGS,	"Begin house customization." },
	{ "log",				(getattrofunc)wpSocket_log, METH_VARARGS, NULL },
    { NULL, NULL, 0, NULL }
};

// Getters & Setters
static PyObject *wpSocket_getAttr( wpSocket *self, char *name )
{
	if( !strcmp( name, "player" ) )
		return PyGetCharObject( self->pSock->player() );
	else if (!strcmp(name, "screenwidth")) {
		return PyInt_FromLong(self->pSock->screenWidth());
	} else if (!strcmp(name, "screenheight")) {
		return PyInt_FromLong(self->pSock->screenHeight());
	} else {
		return Py_FindMethod( wpSocketMethods, (PyObject*)self, name );
	}
}

static int wpSocket_setAttr( wpSocket *self, char *name, PyObject *value )
{
	Q_UNUSED(self);
	Q_UNUSED(name);
	Q_UNUSED(value);
	return 0;
}

int PyConvertSocket( PyObject *object, cUOSocket** sock )
{
	if( object->ob_type != &wpSocketType )
	{
		PyErr_BadArgument();
		return 0;
	}

	*sock = ( (wpSocket*)object )->pSock;
	return 1;
}
