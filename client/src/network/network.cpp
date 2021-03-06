
#include <Python.h>

#include "log.h"
#include "scripts.h"
#include "game/mobile.h"
#include "network/network.h"
#include "network/outgoingpackets.h"
#include "network/uosocket.h"
#include "python/utilities.h"
#include "gui/gui.h"
#include "gui/statuswindow.h"

PyObject *cNetwork::createCharacter(PyObject *args) {
	cCharacterCreationInfo info;

	if (!Scripts->parseArguments(args, "QTbbbbbbbbbHHHHHbIHH", &info.name, &info.female, &info.strength, &info.dexterity, &info.intelligence,
		&info.skill1, &info.skill1Value, &info.skill2, &info.skill2Value, &info.skill3, &info.skill3Value,
		&info.skinColor, &info.hairColor, &info.beardColor,
		&info.hairStyle, &info.beardStyle,
		&info.startLocation, &info.characterSlot,
		&info.shirtHue, &info.pantsHue)) {
		return 0;
	}

	UoSocket->send(cCharacterCreationPacket(info));
	Py_RETURN_TRUE;
}

void cNetwork::requestHelp() {
	UoSocket->send(cRequestHelpPacket());
}

void cNetwork::changeWarmode(bool atwar) {
	UoSocket->send(cWarmodeChangeRequest(atwar));
}

void cNetwork::requestStatus(cMobile *mobile) {
	UoSocket->send(cRequestStatusPacket(mobile->serial()));
}

void cNetwork::showSmallStatus() {
	// Close the large one if it's open
	cControl *other = Gui->findByName("PlayerStatus");
	if (other) {
		Gui->queueDelete(other);
	}

	// This is called "SmallPlayerStatus"
	cStatusWindow *dialog = dynamic_cast<cStatusWindow*>(Gui->findByName("SmallPlayerStatus"));

	if (!dialog) {
		dialog = dynamic_cast<cStatusWindow*>(Gui->createDialog("SmallPlayerStatus"));

		if (!dialog) {
			Log->print(LOG_ERROR, tr("Unable to create status window from SmallPlayerStatus template.\n"));
			return;
		}

		dialog->setObjectName("SmallPlayerStatus");
		Gui->addControl(dialog);
	}

	dialog->setMobile(Player);
	requestStatus(Player);
}

void cNetwork::showLargeStatus() {
	// Close the large one if it's open
	cControl *other = Gui->findByName("SmallPlayerStatus");
	if (other) {
		Gui->queueDelete(other);
	}

	// This is called "PlayerStatus"
	cStatusWindow *dialog = dynamic_cast<cStatusWindow*>(Gui->findByName("PlayerStatus"));

	if (!dialog) {
		if (UoSocket->isAos()) {
			dialog = dynamic_cast<cStatusWindow*>(Gui->createDialog("PlayerStatusAos"));

			if (!dialog) {
				Log->print(LOG_ERROR, tr("Unable to create status window from PlayerStatusAos template.\n"));
				return;
			}
		} else {
			// TODO: Create old status dialog
			return;
		}
		dialog->setObjectName("PlayerStatus");
		Gui->addControl(dialog);
	}

	dialog->setMobile(Player);	
	requestStatus(Player);
}

void cNetwork::setStatLock(uchar stat, uchar lock) {
	UoSocket->send(cSetStatLockPacket(stat, lock));
	
	const cExtendedStatus *status = Player->status();

	if (!status) {
		Player->setStatus(new cExtendedStatus);
		status = Player->status();
	}

	switch (stat) {
		default:
		case 0:
			Player->setStatLocks(lock, status->dexterityLock(), status->intelligenceLock());
			break;
		case 1:
			Player->setStatLocks(status->strengthLock(), lock, status->intelligenceLock());
			break;
		case 2:
			Player->setStatLocks(status->dexterityLock(), status->dexterityLock(), lock);
			break;
	}
}

cNetwork *Network = 0;
