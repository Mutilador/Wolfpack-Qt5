#################################################################
#   )      (\_     # WOLFPACK 13.0.0 Scripts                    #
#  ((    _/{  "-;  # Created by: DarkStorm                      #
#   )).-' {{ ;'`   # Revised by:                                #
#  ( (  ;._ \\ ctr # Last Modification: Created                 #
#################################################################

import wolfpack

# 0x1070 Facing South/North (Swinging: 0x1071)
# 0x1074 Facing East/West   (Swinging: 0x1075)

def onUse( char, item ):
	# Either the dummy is swinging or we aren't assigned to a dummy
	if( item.id != 0x1070 and item.id != 0x1074  ):
		return 1

	# Distance & Direction checks
	if( char.distanceto( item ) > 1 ):
		char.message( 'You must be standing in front of or behind the dummy to use it.' )
		return 1

	# Calculates the direction we'll have to look
	# to focus the dummy
	direction = char.directionto( item )
	
	# For a n/s dummy we need to either face north or south
	if( item.id == 0x1070 and direction != 0 and direction != 4 ):
		char.message( 'You must be standing in front of or behind the dummy to use it.' )
		return 1

	# For a e/w dummy we need to either face eath or west
	elif( item.id == 0x1074 and direction != 2 and direction != 6 ):
		char.message( 'You must be standing in front of or behind the dummy to use it.' )
		return 1
	
	# Turn to the correct direction
	if( char.direction != direction ):
		char.direction = direction
		char.update()

	skill = char.combatskill() # Determine the combat skill used by the character

	# We can only train FENCING+MACEFIGHTING+SWORDSMANSHIP+WRESTLING
	if( skill != wolfpack.FENCING and skill != wolfpack.MACEFIGHTING and skill != wolfpack.SWORDSMANSHIP and skill != wolfpack.WRESTLING ):
		char.message( "You can't train with this weapon on this dummy." )
		return 1
	
	# If we've already learned all we can > cancel.
	if( char.baseskill[ skill ] >= 300 ):
		char.message( "You can learn much from a dummy but you have already learned it all." )
		return 1
	
	# This increases the users skill
	char.checkskill( skill, 0, 1000 )
	
	# Display the char-action
	# (combat swing 1handed)
	char.action( 0x09 )

	if( item.id == 0x1070 or item.id == 0x1074 ):
		item.id += 1

	# Resend the item to surrounding clients after
	# changing the id and play the soundeffect
	# originating from the dummy
	item.update()
	item.soundeffect( 0x33 )
			
	# Add a timer to reset the id
	wolfpack.addtimer( "training_dummy.resetid", 3000, (item.serial,) )
		
	return 1
	
# Reset the id of a swinging dummy
def resetid( iSerial ):
	item = wolfpack.finditem( iSerial )
	
	if( item and ( item.id == 0x1070 or item.id == 0x1074 ) ):
		item.id += 1
		item.update()
