
# Displays Players on a Map

import web.sessions
import web.template
import cgi
import sys
import re
import wolfpack
import wolfpack.sockets
import wolfpack.accounts
import wolfpack.time

form = cgi.FieldStorage()
session_id = form.getvalue( 'session', '' )

web.sessions.check_timeouts()
session = web.sessions.get_session( session_id )

if session == None:
	web.sessions.display_timeout()
	sys.exit()

username = session[ 'username' ]

XOFFSET = 0
YOFFSET = -254

content = """
<p>
<h1>Players on a Mala Map</h1>
Below you see the Players on the Map. Use your browsers refresh button to see the current output.<br>
<table width="640" height="511" border="0">
  <tr>
    <td background="map_malas.jpg">
"""

numero = 0
socket = wolfpack.sockets.first()
nextsocket = wolfpack.sockets.next()
while socket:
	if( socket.player and socket.player.pos.map == 3 and not socket.player.invisible ):
		
		numero += 1

		content += """
		<div id="
		"""

		content += 'layer' + unicode(numero)

		content += """
		" style="position:relative; width:8px; height:8px; z-index:0;
		"""

		content += 'left: ' + unicode(((socket.player.pos.x)/4) + XOFFSET) + 'px; '
		
		content += 'top: ' + unicode(((socket.player.pos.y)/4) + YOFFSET) + 'px;">'

		content += """
		<img src="marker.gif"
		"""

		content += ' alt="' + unicode(socket.player.name) + '" width="8" height="8">'

		content += """
		</div>
		"""
			
	socket = nextsocket
	nextsocket = wolfpack.sockets.next()

content += """
	</td>
  </tr>
</table>
</p>
"""

web.template.output( 	'&gt; <a href="map.py?session=%(session)s" class="header">Players on a Map</a>' % { 'session': session_id }, 
						'<i>Logged in as:</i> %(username)s<br /><a href="logout.py?session=%(session)s" class="header">Logout</a>' % { 'username': username, 'session': session_id }, 
						1, 
						content,
						session_id
					)

