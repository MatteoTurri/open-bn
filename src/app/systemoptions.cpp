/*
 * SystemOptions: constructor defaults and setters, and the global SysOption.
 */

#pragma implementation "systemoptions.h"
#include <stdio.h>
#include "ime/textim.h"
#include "systemoptions.h"

SystemOptions::SystemOptions()
{
	m_xgaMode = 0;
	m_newConsoleMode = 0;
	m_frameRate = 60;
	m_resWidth = 640;
	m_resHeight = 448;
	m_language = 0;
	unk_20 = 1;
	m_keyboardType = 0;
	m_keyboardRepeatWait = 1;
	m_keyboardRepeatSpeed = 1;
	m_mouseSpeed = 1;
	m_mouseDClick = 1;
	m_mouseChangeLR = 1;
	m_atokMode = 0;
	m_atokKeyBind = 0;
	m_softkbOnOff = 0;
	m_softkbQwerty = 0;
	m_spdif = 0;
	m_aspect = 0;
	m_video = 0;
	m_sysLanguage = 0;
	m_timeZone = 540;
	m_timeZoneCity = 51;
	m_summerTime = 0;
	m_timeNotation = 0;
	m_dateNotation = 0;
	m_autoDiag = 0;
}

void SystemOptions::setXGAMode()
{
	m_xgaMode = 1;
	m_frameRate = 60;
	m_resWidth = 1024;
	m_resHeight = 768;
}

int SystemOptions::getXGAMode()
{
	return m_xgaMode;
}

void SystemOptions::setSPECIALMode()
{
	m_specialMode = 1;
	m_frameRate = 60;
	m_resWidth = 896;
	m_resHeight = 512;
}

int SystemOptions::getSPECIALMode()
{
	return m_specialMode;
}

void SystemOptions::setNewConsoleMode()
{
	m_newConsoleMode = 1;
}

int SystemOptions::getNewConsoleMode()
{
	return m_newConsoleMode;
}

void SystemOptions::setKeyboardType(int type)
{
	if (type != m_keyboardType) {
		m_keyboardType = type;
		TextIM::UpdateKeyboardType();
	}
}

void SystemOptions::setKeyboardRepeatWait(int wait)
{
	if (wait != m_keyboardRepeatWait) {
		m_keyboardRepeatWait = wait;
		TextIM::UpdateKbdRate(m_keyboardRepeatSpeed, wait);
	}
}

void SystemOptions::setKeyboardRepeatSpeed(int speed)
{
	if (speed != m_keyboardRepeatSpeed) {
		m_keyboardRepeatSpeed = speed;
		TextIM::UpdateKbdRate(speed, m_keyboardRepeatWait);
	}
}

void SystemOptions::setMouseSpeed(int speed)
{
	if (speed != m_mouseSpeed)
		m_mouseSpeed = speed;
}

void SystemOptions::setMouseDClick(int dclick)
{
	if (dclick != m_mouseDClick)
		m_mouseDClick = dclick;
}

void SystemOptions::setMouseChangeLR(int changeLR)
{
	if (changeLR != m_mouseChangeLR)
		m_mouseChangeLR = changeLR;
}

/* NOTE: bug: the error message names setAtokKeyBind. */
void SystemOptions::setAtokMode(int mode)
{
	int value = mode;
	if ((unsigned int)value >= 2) {
		fprintf(stderr, "ERROR : setAtokKeyBind - bind no[%d]\n", value);
	} else {
		if (value != m_atokMode) {
			m_atokMode = value;
			TextIM::UpdateAtokMode();
		}
	}
}

void SystemOptions::setAtokKeyBind(int keyBind)
{
	int value = keyBind;
	if ((unsigned int)value >= 2) {
		fprintf(stderr, "ERROR : setAtokKeyBind - bind no[%d]\n", value);
	} else {
		if (value != m_atokKeyBind) {
			m_atokKeyBind = value;
			TextIM::UpdateAtokKeyBind();
		}
	}
}

void SystemOptions::setSoftkbOnOff(int onOff)
{
	if (onOff != m_softkbOnOff)
		m_softkbOnOff = onOff;
}

void SystemOptions::setSoftkbQwerty(int qwerty)
{
	if (qwerty != m_softkbQwerty)
		m_softkbQwerty = qwerty;
}

SystemOptions SysOption;
