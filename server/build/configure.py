
#################################################################
#   )      (\_     # Wolfpack 13.0.0 Build Script               #
#  ((    _/{  "-;  # Created by: Wolfpack Development Team      #
#   )).-" {{ ;"`   # Revised by: Wolfpack Development Team      #
#  ( (  ;._ \\ ctr # Last Modification: check cvs logs          #
#################################################################

import os
import sys
import glob
import fnmatch
import dircache
import string

# These are the variables we are trying to figure out
py_libpath = ""
py_libfile = ""
py_incpath = ""
qt_qmake = ""

sys.stdout.write( "Wolfpack configure script\n"	)

def findFile( searchpath ):
	path = ""
	file = ""
	for entry in searchpath:
		pathexp, fileexp = os.path.split( entry )
		for path in glob.glob(pathexp):
			if os.path.exists(path):
				for file in dircache.listdir(path):
					if fnmatch.fnmatch( file, fileexp ):
						return (file, path)
	return (None, None)


def checkQt():

	if sys.platform == "win32":
		QMAKE_EXECUTABLE = "qmake.exe"
	else:
		QMAKE_EXECUTABLE = "qmake"

	sys.stdout.write( "Checking QTDIR enviroment variable..." )
	if ( len( os.environ["QTDIR"] ) > 0 and os.path.exists( os.environ["QTDIR"] ) ):
		sys.stdout.write( "ok\n" )
	else:
		sys.stdout.write( "failed\n" )
		sys.stdout.write( "You must properly setup QTDIR" )
		sys.exit();
	sys.stdout.write( "Searching for qmake..." )
	temp = ""
	
	QMAKESEARCHPATH = [ os.path.join(os.path.join(os.environ["QTDIR"], "bin"), QMAKE_EXECUTABLE) ]
	for dir in string.split( os.environ["PATH"], os.path.pathsep ):
		QMAKESEARCHPATH.append( os.path.join(dir, QMAKE_EXECUTABLE) )

	qmake_file, qmake_path = findFile(QMAKESEARCHPATH)
	global qt_qmake;
	qt_qmake = os.path.join(qmake_path, qmake_file)
	sys.stdout.write( "%s\n" % qt_qmake )
	
	return True

def checkPython():
	if sys.platform == "win32":
		PYTHONLIBSEARCHPATH = [ sys.prefix + "\Libs\python*.lib" ]
		PYTHONINCSEARCHPATH = [ sys.prefix + "\include\Python.h" ]
	elif sys.platform == "linux2":
		PYTHONLIBSEARCHPATH = [ "/usr/local/lib/libpython*.so", \
							 "/usr/local/lib/[Pp]ython*/libpython*.so", \
							 "/usr/lib/libpython*.so", \
							 "/usr/lib/[Pp]ython*/libpython*.so", \
							 "/usr/lib/[Pp]ython*/config/libpython*.so", \
							 "/usr/local/lib/[Pp]ython*/config/libpython*.so"]
		PYTHONINCSEARCHPATH = [ "/usr/local/include/[Pp]ython*/Python.h", \
							 "/usr/include/[Pp]ython*/Python.h"]

	else:
		sys.stdout.write("ERROR: Unknown platform %s to checkPython()" % sys.platform )
	sys.stdout.write( "Checking Python version... " )
	if sys.hexversion >= 0x020300F0:
		sys.stdout.write("ok\n")
	else:
		sys.stdout.write( "failed\n" )
		sys.stdout.write( "Wolfpack requires Python version greater than 2.3.0 " )
		sys.exit();
	sys.stdout.write( "Searching for Python library... " )
	
	global py_libpath
	global py_libfile

	py_libfile, py_libpath = findFile( PYTHONLIBSEARCHPATH )
	if ( py_libfile ):
		sys.stdout.write("%s\n" % os.path.join( py_libpath, py_libfile ) )
	else:
		sys.stdout.write("Not Found!\n")
		sys.exit()

	global py_incpath
	py_incfile = None
	sys.stdout.write( "Searching for Python includes... " )
	py_incfile, py_incpath = findFile( PYTHONINCSEARCHPATH )
	if ( py_incfile ):
		sys.stdout.write( "%s\n" % py_incpath )
	else:
		sys.stdout.write("Not Found!\n")
		sys.exit()
	
	return True

# Entry point
def main():
	checkPython()
	checkQt()

	# Create config.pri
	global py_libpath
	global py_libfile
	global py_incpath
	global qt_qmake
		
	config = file("config.pri", "w")
	config.write("# WARNING: This file was automatically generated by configure.py\n ")
	config.write("#          any changes to this file will be lost!\n")
	
	config.write("INCLUDEPATH += %s\n" % ( py_incpath ) )
	# Build LIBS
	LIBS = ""
	if sys.platform == "win32":
		LIBS = os.path.join( py_libpath, py_libfile )
	else:
		LIBS = "-l%s -L%s" % ( py_libfile, py_libpath )
	
	config.write("LIBS += %s\n" % LIBS)
	config.close()
	
	sys.stdout.write("Generating makefile...")
	os.execv(qt_qmake, [qt_qmake, "wolfpack.pro"])

	
if __name__ == "__main__":
    main()