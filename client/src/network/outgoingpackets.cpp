
#include "network/outgoingpackets.h"
#include "muls/speech.h"
#include "muls/maps.h"
#include "game/entity.h"
#include "game/mobile.h"
#include "game/statictile.h"
#include "game/groundtile.h"
#include "game/dynamicitem.h"
#include "profile.h"
#include "log.h"

#include <qglobal.h>

static uint getCapabilities() {
	uint result = 1; // Rennesaince flag

	if (Maps->facetEnabled(ILSHENAR)) {
		result |= 0x02 | 0x04; // Third dawn and Lord Blackthornes Revenge flag
	}

	if (Maps->facetEnabled(MALAS)) {
		result |= 0x08; // Age of shadows flag
	}

	if (Maps->facetEnabled(TOKUNO)) {
		result |= 0x10; // Samurai Empire flag
	}

	return result;
}

cLoginPacket::cLoginPacket(const QString &username, const QString &password) : cOutgoingPacket(0x80, 62) {
	writeFixedAscii(username, 30);
	writeFixedAscii(password, 30);
	m_Stream << (unsigned char)0;
}

cGameLoginPacket::cGameLoginPacket(unsigned int key, const QString &username, const QString &password) : cOutgoingPacket(0x91, 65) {
	m_Stream << key;
	writeFixedAscii(username, 30);
	writeFixedAscii(password, 30);	
}

cRequestRelayPacket::cRequestRelayPacket(unsigned short id) : cOutgoingPacket(0xa0, 3) {
	m_Stream << id;
}

cDeleteCharacter::cDeleteCharacter(unsigned int id) : cOutgoingPacket(0x83, 39) {
	fill(30, 0); // Password...
	m_Stream << id;
	fill(4, 0); // "client-ip"
}

cPlayMobilePacket::cPlayMobilePacket(unsigned char id) : cOutgoingPacket(0x5d, 73) {
    m_Stream << 0xedededed; // "pattern"
	fill(30, 0); // name+password are blank since unused
	fill(2, 0);
	m_Stream << getCapabilities(); // Client capabilities
	fill(24, 0);
	fill(3, 0); // Unknown. Maybe just the integer slot
	m_Stream << id;
	fill(4, 0); // "client-ip" but absolutly unneccesary
}

cDoubleClickPacket::cDoubleClickPacket(unsigned int serial) : cOutgoingPacket(0x06, 5) {
	m_Stream << serial;
}

cResyncPacket::cResyncPacket() : cOutgoingPacket(0x22, 3) {
	fill(2, 0);
}

unsigned int createOneMask(uint count) {
	uint result = 0;
	for (uint i = 0; i < count; ++i) {
		result |= 1 << i;
	}
	return result;
}

static void pushData(QByteArray &array, unsigned int &byteoffset, unsigned int &bitoffset, unsigned int data, unsigned char bit) {
    // We can at most save 8 - offset bit or bit bits
	while (bit > 0) {
		unsigned int bitCount = qMin<unsigned int>(8 - bitoffset, bit);
		unsigned int writeMask = (data >> (bit - bitCount)) & createOneMask(bitCount);
		
		data &= ~ (writeMask << (bit - bitCount)); // Clear out the data we write from the original data
		bit -= bitCount;

		array[byteoffset] = array[byteoffset] | (unsigned char)(writeMask << (8 - bitCount - bitoffset));
		bitoffset += bitCount;
		if (bitoffset > 7) {
			bitoffset = 0;
			++byteoffset;
		}
	}
}

cSendUnicodeSpeechPacket::cSendUnicodeSpeechPacket(enSpeechType type, const QString &message, unsigned short color, unsigned char font, const QString &language) : cOutgoingPacket(0xad, 14) {
	if (color == 0xFFFF) {
		if (type == SPEECH_EMOTE) {
			color = Profile->emoteHue();
		} else {
			color = Profile->speechHue();
		}
	}

	QVector<unsigned short> keywords = Speech->match(message); // Speech.mul keywords

	/*for (int i = 0; i < keywords.size(); ++i) {
		Log->print(LOG_MESSAGE, QString("Text '%1' contains Keyword %2.\n").arg(message).arg(keywords[i]));
	}*/

	// Prepare the language string for the speech.
	char lang[4] = "enu";
	if (!language.isEmpty()) {
		QByteArray qbaLang = language.toLatin1();
		memcpy(lang, qbaLang.constData(), qMin<size_t>(3, qbaLang.length()));
	}

	// Two different packet formats
	if (keywords.size() > 0) {
		m_Stream << (unsigned char)(type | 0xc0) << color << (unsigned short)font;
		m_Stream.writeRawData(lang, 4);

		// Build the encoded packet list
		unsigned int bitoffset = 0, byteoffset = 0;
		QByteArray array((12 + 5 * keywords.size() + 7) / 8, 0);
		pushData(array, byteoffset, bitoffset, keywords.size(), 12); // Push the keyword count (12 bit)
		for (int i = 0; i < keywords.size(); ++i) {
			pushData(array, byteoffset, bitoffset, keywords[i], 12); // 12 bit for each keyword
		}

		m_Stream.writeRawData(array.data(), array.size());

		writeUtf8Terminated(message);
	} else {
		m_Stream << (unsigned char)type << color << (unsigned short)font;
		m_Stream.writeRawData(lang, 4);
		
		// Simply dump the unicode string in big endian
		writeBigUnicodeTerminated(message);		
	}

	writeDynamicSize();
}

cTargetResponsePacket::cTargetResponsePacket(uint targetId, uchar targetType, uchar cursorType, cEntity *target) : cOutgoingPacket(0x6c, 19) {
	if (target && (target->type() == MOBILE || target->type() == ITEM || target->type() == STATIC)) {
		targetType = 0;
	}

	m_Stream << targetType << targetId << cursorType;

	// Target cancelled
	if (!target) {
		m_Stream << (unsigned int)0 << (unsigned int)~0 << (unsigned short)0 << (unsigned short)0;
	} else {
		// For Mobiles write all
		if (target->type() == MOBILE) {
			cMobile *mobile = dynamic_cast<cMobile*>(target);
			m_Stream << mobile->serial() << mobile->x() << mobile->y() << (short)mobile->z() << (unsigned short)0;
		} else if (target->type() == ITEM) {
			cDynamicItem *item = dynamic_cast<cDynamicItem*>(target);
			m_Stream << item->serial() << item->x() << item->y() << (short)item->z() << (unsigned short)item->id();
		} else if (target->type() == GROUND) {
			cGroundTile *item = dynamic_cast<cGroundTile*>(target);
			m_Stream << (unsigned int)0 << item->x() << item->y() << (short)item->z() << (unsigned short)0;
		} else if (target->type() == STATIC) {
			cStaticTile *item = dynamic_cast<cStaticTile*>(target);
			m_Stream << (unsigned int)0 << item->x() << item->y() << (short)item->z() << (unsigned short)item->id();
		}
	}
}

cGenericGumpResponsePacket::cGenericGumpResponsePacket(uint serial, uint type, uint button, QVector<uint> switches, QMap<uint, QString> strings) : cOutgoingPacket(0xb1, 23) {
	m_Stream << serial << type << button << (uint)switches.size();

	for (int i = 0; i < switches.size(); ++i) {
		m_Stream << switches[i];
	}

	m_Stream << (unsigned int)strings.size();

	QMap<uint, QString>::const_iterator it;
	for (it = strings.begin(); it != strings.end(); ++it) {
		m_Stream << (unsigned short)it.key() << (unsigned short)(it.value().length() + 1);
		writeBigUnicodeTerminated(it.value());
	}	

	writeDynamicSize();
}

cMoveRequestPacket::cMoveRequestPacket(uchar direction, uchar sequence, uint fastwalkKey) : cOutgoingPacket(0x02, 7) {
	m_Stream << direction << sequence << fastwalkKey;
}

cSingleClickPacket::cSingleClickPacket(uint serial) : cOutgoingPacket(0x09, 5) {
	m_Stream << serial;
}

cPingPacket::cPingPacket(uchar sequence) : cOutgoingPacket(0x73, 2) {
	m_Stream << sequence;
}

cRequestMultipleTooltipsPacket::cRequestMultipleTooltipsPacket(QVector<uint> tooltips) : cOutgoingPacket(0xd6, 3 + tooltips.size() * 4) {
	foreach (uint key, tooltips) {
		m_Stream << key;
	}
}

cCharacterCreationInfo::cCharacterCreationInfo() {
	female = false;
	strength = 25;
	dexterity = 20;
	intelligence = 20;
	skill1 = 0;
	skill2 = 1;
	skill3 = 2;
	skill1Value = 50;
	skill2Value = 50;
	skill3Value = 0;
	skinColor = 1002;
	hairColor = 1102;
	beardColor = 0;
	hairStyle = 0x203C; // Long Hair
	beardStyle = 0;
	startLocation = 0;
	characterSlot = 0;
	shirtHue = 2;
	pantsHue = 2;
}

cCharacterCreationPacket::cCharacterCreationPacket(const cCharacterCreationInfo &info) : cOutgoingPacket(0, 104) {
	uint clientIp = 0;

	m_Stream << (uint)0xedededed << (uint)0xffffffff << (uchar)0;
	for (int i = 0; i < 30; ++i) {
		if (i < info.name.length()) {
			m_Stream << (uchar)info.name.at(i).toLatin1();
		} else {
			m_Stream << (uchar)0;
		}
	}
	fill(2, 0); // Password (Now used for profession et al)
	m_Stream << getCapabilities();
	fill(24, 0);
	m_Stream << (uchar)( info.female ? 1 : 0 ) << info.strength << info.dexterity << info.intelligence
		<< info.skill1 << info.skill1Value << info.skill2 << info.skill2Value << info.skill3 << info.skill3Value
		<< info.skinColor << info.hairStyle << info.hairColor << info.beardStyle << info.beardColor
		<< (uchar)0 << info.startLocation << info.characterSlot << clientIp << info.shirtHue << info.pantsHue;
}

cRequestContextMenu::cRequestContextMenu(uint serial) : cOutgoingPacket(0xbf, 9) {
	m_Stream << (ushort)0x13 << serial;
}

cContextMenuResponsePacket::cContextMenuResponsePacket(uint serial, ushort id) : cOutgoingPacket(0xbf, 13) {
	m_Stream << (ushort)0x15 << serial << id;
}

cWarmodeChangeRequest::cWarmodeChangeRequest(bool mode) : cOutgoingPacket(0x72, 5) {
	m_Stream << (uchar)(mode ? 1 : 0);
	fill(3, 0);
}

cVersionPacket::cVersionPacket(const QString &version) : cOutgoingPacket(0xbd, 3) {
	writeAscii(version);
	m_Stream << (uchar)0; // Null termination
	writeDynamicSize();
}

cLanguagePacket::cLanguagePacket(const QString &language) : cOutgoingPacket(0xbf, 9) {
	m_Stream << (ushort)0x0b; // packet id
	writeAscii(language.left(3).toUpper()); // Write first 3 characters of the language (everything else is ignored anyway)
	m_Stream << (uchar)0; // Null termination
	writeDynamicSize(); // Finalize packet
}

cRequestHelpPacket::cRequestHelpPacket() : cOutgoingPacket(0x9b, 258) {	
}

cRequestAttackPacket::cRequestAttackPacket(uint serial) : cOutgoingPacket(0x05, 5) {
	m_Stream << serial;
}

cGrabItemPacket::cGrabItemPacket(uint serial, ushort amount) : cOutgoingPacket(0x7, 7) {
	m_Stream << serial << amount;
}

cDropItemPacket::cDropItemPacket(uint item, ushort x, ushort y, signed char z, uint container) : cOutgoingPacket(0x8, 14) {
	m_Stream << item << x << y << z << container;
}

cWearItemPacket::cWearItemPacket(uint item, uchar layer, uint mobile) : cOutgoingPacket(0x13, 10) {
	m_Stream << item << layer << mobile;
}

cRequestStatusPacket::cRequestStatusPacket(uint serial) : cOutgoingPacket(0x34, 10) {
	m_Stream << 0xedededed << (uchar)0x04 << serial;
}

cRequestSkillsPacket::cRequestSkillsPacket(uint serial) : cOutgoingPacket(0x34, 10) {
	m_Stream << 0xedededed << (uchar)0x05 << serial;
}

cSetStatLockPacket::cSetStatLockPacket(uchar stat, uchar lock) : cOutgoingPacket(0xbf, 7) {
	m_Stream << (ushort)0x1a << stat << lock;
	writeDynamicSize();
}
