
import wolfpack

def onDropOnItem(keyring, key):
	if not keyring.hasscript( 'keyring' ) or not key.hasscript( 'key' ):
		return 0

	char = key.container

	# Only non-blank keys may be put on a keyring...
	if not key.hastag('lock'):
		char.socket.clilocmessage(501689)
		return 0

	# Keyring is full
	keycount = len(keyring.content)

	if keycount >= 20:
		char.socket.clilocmessage(1008138)
		return 0

	# Add the key to the keyring
	key.removefromview(0)
	key.container = keyring
	key.layer = 0

	# Id?
	if keycount >= 5:
		newid = 0x176b
	elif keycount >= 3:
		newid = 0x176a
	else:
		newid = 0x1769

	if keyring.id != newid:
		keyring.id = newid
		keyring.update()

	char.socket.clilocmessage(501691)
	return 1

def solve_keys(char, keyring):
	# Put all rings into the backpack of the user
	backpack = char.getbackpack()

	for key in keyring.content:
		backpack.additem(key, 1, 1, 0)
		key.update()

	keyring.id = 0x1011
	keyring.update()

	char.socket.clilocmessage(501685)
	return

def lock_response(char, args, target):
	if len(args) != 1:
		return

	keyring = wolfpack.finditem(args[0])
	if not keyring or not char.canreach(keyring,5):
		char.socket.clilocmessage(500312)
		return

	# Check for an item target.
	if not target.item or ( not char.canreach(target.item,5) and not target.item.hasscript( 'boats.plank' ) ):
		char.socket.clilocmessage(501666)
		return

	# Targetted the keyring itself??
	if target.item == keyring:
		# Put all rings into the backpack of the user
		solve_keys(char, keyring)
		return

	if not target.item.hasscript( 'lock' ) or not target.item.hastag( 'lock' ):
		char.socket.clilocmessage(501666)
		return

	target_lock = str(target.item.gettag('lock'))

	# Search for a valid key
	keys = keyring.content

	for key in keys:
		if key.hastag('lock') and key.hasscript( 'key' ):
			key_lock = str(key.gettag('lock'))

			if key_lock == target_lock:
				if target.item.hastag('locked') and int(target.item.gettag('locked')) == 1:
					target.item.deltag('locked')
					target.item.say(1048001, "", "", False, 0x3b2, char.socket)
				else:
					target.item.settag('locked',1)
					target.item.say(1048000, "", "", False, 0x3b2, char.socket)
				char.soundeffect(0x241)
				return

	char.socket.clilocmessage(501668)

# The behaviour is similar to keys
def onUse(char, keyring):
	# Does this key open a lock?
	char.socket.clilocmessage(501680)
	char.socket.attachtarget('keyring.lock_response',[keyring.serial])
	return 1
