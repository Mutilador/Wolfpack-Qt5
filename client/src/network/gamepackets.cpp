
#include "network/uopacket.h"
#include "network/uosocket.h"
#include "dialogs/login.h"
#include "gui/worldview.h"
#include "game/mobile.h"
#include "game/dynamicitem.h"
#include "game/targetrequest.h"
#include "game/world.h"
#include "sound.h"
#include "log.h"
#include "enums.h"

#include <qlist.h>

class cLoginConfirmPacket : public cIncomingPacket {
protected:
	unsigned int serial;
	unsigned short body;
	unsigned short posx, posy;
	signed short posz;
	unsigned char direction;
public:
	cLoginConfirmPacket(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {
		safetyAssertSize(37);
		unsigned int unknown1;
		input >> serial >> unknown1 >> body >> posx >> posy >> posz >> direction;
	}

	virtual void handle(cUoSocket *socket) {
		Player->setSerial(serial);		
		Player->setBody(body);
		Player->setDirection(direction);
		World->moveCenter(posx, posy, posz, true);
		Player->move(posx, posy, posz);
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cLoginConfirmPacket(input, size);
	}
};


AUTO_REGISTER_PACKET(0x1b, cLoginConfirmPacket::creator);

class cResetGamePacket : public cIncomingPacket {
public:
	cResetGamePacket(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {		
	}

	virtual void handle(cUoSocket *socket) {
		LoginDialog->hide();
		WorldView->setVisible(true);
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cResetGamePacket(input, size);
	}
};


AUTO_REGISTER_PACKET(0x55, cResetGamePacket::creator);

class cUpdatePlayerPacket : public cIncomingPacket {
protected:
	unsigned int serial;
	unsigned short body;
	unsigned short hue;
	unsigned char flags;
	unsigned short posx, posy;
	signed char posz;
	unsigned char direction;
public:
	cUpdatePlayerPacket(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {
		unsigned char unknown1;
		unsigned short unknown2;
		input >> serial >> body >> unknown1 >> hue >> flags >> posx >> posy >> unknown2 >> direction >> posz;
	}

	virtual void handle(cUoSocket *socket) {
		if (Player->serial() != serial) {
			return; // Do nothing if serials dont match
		}

		// If the serial matches the player serial, update the player
		Player->setBody(body);
		Player->setHue(hue);
		Player->setDirection(direction);
			
		// Update position if applicable
		if (posx != Player->x() || posy != Player->y() || posz != Player->z()) {
			World->moveCenter(posx, posy, posz, false);
			Player->move(posx, posy, posz);
		}
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cUpdatePlayerPacket(input, size);
	}
};


AUTO_REGISTER_PACKET(0x20, cUpdatePlayerPacket::creator);

class cAddMobile : public cDynamicIncomingPacket {
protected:
	unsigned int serial;
	unsigned short body;
	unsigned short hue;
	unsigned char flags;
	unsigned short posx, posy;
	signed char posz;
	unsigned char direction;
	unsigned char notoriety;

	struct stEquipInfo {
		unsigned int serial;
		unsigned short id;
		unsigned short hue;
		unsigned char layer;
	};

	QList<stEquipInfo> equipment;
public:
	cAddMobile(QDataStream &input, unsigned short size) : cDynamicIncomingPacket(input, size) {
		input >> serial >> body;
		
		if (serial & 0x80000000) {
			unsigned short corpse; // or amount!
			input >> corpse;
			serial &= ~ 0x80000000; // Clear the flag
		}

		input >> posx >> posy;

		if (posx & 0x8000) {
			input >> direction;
			posx &= ~ 0x8000; // Clear the flag
		} else {
			direction = 0;
		}

		input >> posz >> direction >> hue >> flags >> notoriety;

		// Check for items
		stEquipInfo info;
		input >> info.serial;

		// Read until the serial is 0
		while (info.serial != 0) {
			input >> info.id >> info.layer;
			if (info.id & 0x8000) {
				input >> info.hue;
				info.id &= ~ 0x8000; // Clear the flag
			} else {
				info.hue = 0;
			}

            equipment.append(info);
			input >> info.serial; // Read next item serial
		}
	}

	virtual void handle(cUoSocket *socket) {
		// Normally we'd have to check if we have to create the mobile here
		cDynamicEntity *entity = World->findDynamic(serial);
		cMobile *mobile = 0;

		if (!entity) {
			return; // TODO: Object creation
		}

		if (entity->type() == MOBILE) {
			mobile = (cMobile*)entity;
			mobile->setHue(hue);
			mobile->setDirection(direction);
			mobile->setBody(body);

			if (posx != mobile->x() || posy != mobile->y() || posz != mobile->z()) {
				if (Player == mobile) {
					World->moveCenter(posx, posy, posz, false);
				}
				mobile->move(posx, posy, posz);
			}			
		}

		if (mobile) {
			QList<stEquipInfo>::const_iterator it;
			for (it = equipment.begin(); it != equipment.end(); ++it) {
				cDynamicItem *item = World->findItem(it->serial);

				if (!item) {
					item = new cDynamicItem(mobile, (enLayer)it->layer, it->serial);
				}

				item->setHue(it->hue);
				item->setId(it->id);
			}
		}
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cAddMobile(input, size);
	}
};


AUTO_REGISTER_PACKET(0x78, cAddMobile::creator);

class cAddGroundObject : public cDynamicIncomingPacket {
protected:
	unsigned int serial;
	unsigned short id;
	unsigned short hue;
	unsigned char flags;
	unsigned short posx, posy;
	signed char posz;
	unsigned char direction;
	unsigned short amount;
public:
	cAddGroundObject(QDataStream &input, unsigned short size) : cDynamicIncomingPacket(input, size) {
		safetyAssertSize(size);

        input >> serial >> id;
		
		if (serial & 0x80000000) {
			input >> amount;
			serial &= ~ 0x80000000; // Clear the flag
		}

		// Whatever this is for, it's in the docs
		if (id & 0x8000) {
			unsigned char counter;
			input >> counter;
			id &= ~0x8000;
			id += counter;
		}

		input >> posx >> posy;

		if (posx & 0x8000) {
			input >> direction;
			posx &= ~ 0x8000; // Clear the flag
		} else {
			direction = 0;
		}

		input >> posz;

		if (posy & 0x8000) {
			input >> hue;
			posy &= ~ 0x8000; // Clear the flag
		} else {
			hue = 0;
		}

		if (posy & 0x4000) {
			input >> flags;
			posy &= ~ 0x4000; // Clear the flag
		} else {
			flags = 0;
		}
	}

	virtual void handle(cUoSocket *socket) {
		cDynamicItem *item = World->findItem(serial);

		if (!item) {
			item = new cDynamicItem(posx, posy, posz, Player->facet(), serial);
		} else {
			item->move(posx, posy, posz);
		}

		item->setHue(hue);
		item->setId(id);

		// TODO: Flags/Amount
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cAddGroundObject(input, size);
	}
};

AUTO_REGISTER_PACKET(0x1a, cAddGroundObject::creator);

class cDeleteObject : public cIncomingPacket {
protected:
	unsigned int serial;
public:
	cDeleteObject(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {
		input >> serial;
	}

	virtual void handle(cUoSocket *socket) {
		cDynamicItem *item = World->findItem(serial);

		if (item) {
			item->cleanPosition();
			item->decref();
		}
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cDeleteObject(input, size);
	}
};

AUTO_REGISTER_PACKET(0x1d, cDeleteObject::creator);

// Incoming unicode message
class cUnicodeMessagePacket : public cDynamicIncomingPacket {
protected:
	unsigned int serial; // Serial of source object
	unsigned short model; // Model of source object (whatever thats for..)
	unsigned char type; // Type of incoming message
	unsigned short hue;
	unsigned short font;
	QString language; // Pretty much unused
	QString name; // Name of source object
	QString message; // Message text
public:
	cUnicodeMessagePacket(QDataStream &input, unsigned short size) : cDynamicIncomingPacket(input, size) {
		safetyAssertSize(50);
		input >> serial >> model >> type >> hue >> font;

		// Language
		char strLang[5];
		input.readRawBytes(strLang, 4);
		strLang[4] = 0;
		language = strLang;

		// Object Name
		char strName[31];
		input.readRawBytes(strName, 30);
		strName[30] = 0;
		name = strName;

		// Unicode Stuff
		unsigned int length = ((size - 48) / 2) + 1;
		ushort *strMessage = new unsigned short[length];
		
		// We need to swap the single characters
		for (unsigned int i = 0; i < (length - 1); ++i) {
			input >> strMessage[i];
		}

		strMessage[length-1] = 0;
        
		message = QString::fromUtf16(strMessage);
	}

	virtual void handle(cUoSocket *socket) {
		if (!WorldView || !WorldView->isVisible()) {
			return; // Only process messages when ingame
		}

		// System Message
		if (serial == -1) {
			WorldView->addSysMessage(message, hue, font);
		}
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cUnicodeMessagePacket(input, size);
	}
};

AUTO_REGISTER_PACKET(0xae, cUnicodeMessagePacket::creator);

class cSoundEffectPacket : public cIncomingPacket {
protected:
	unsigned short model; // Sound id
	unsigned short posx, posy; // x,y position of source
	short posz; // z source
	unsigned short unknown;
	unsigned char mode; // Number of repeats?
public:
	cSoundEffectPacket(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {
		input >> mode >> model >> unknown >> posx >> posy >> posz;
	}

	virtual void handle(cUoSocket *socket) {
        Sound->playSound(model);
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cSoundEffectPacket(input, size);
	}
};

AUTO_REGISTER_PACKET(0x54, cSoundEffectPacket::creator);

// Incoming target request
class cRequestTargetPacket : public cIncomingPacket {
protected:
	unsigned int targetId; // Serial for the target command
	unsigned char targetType; // 0: Select Dynamic, 1: Select ground (allowed, not mandatory)
	unsigned char cursorType; // Criminal action?
public:
	cRequestTargetPacket(QDataStream &input, unsigned short size) : cIncomingPacket(input, size) {
		input >> targetType >> targetId >> cursorType;

		if (targetType > 1) {
			targetType = 1; // Only supported targettype for now
		}
	}

	virtual void handle(cUoSocket *socket) {
		if (!WorldView || !WorldView->isVisible()) {
			return; // Only process targets when ingame
		}

		// System Message
		Log->print(LOG_TRACE, tr("Received target request from server with serial 0x%1, type %2, cursor %3.\n").arg(targetId).arg(targetType).arg(cursorType));
		WorldView->requestTarget(new cServerTargetRequest(targetId, targetType, cursorType));
	}

	static cIncomingPacket *creator(QDataStream &input, unsigned short size) {
		return new cRequestTargetPacket(input, size);
	}
};

AUTO_REGISTER_PACKET(0x6c, cRequestTargetPacket::creator);