/*
 * SystemOptions: the system settings of the Broadband Navigator (keyboard,
 * mouse, ATOK, software keyboard, screen mode, time zone).
 */

#ifndef SYSTEMOPTIONS_H
#define SYSTEMOPTIONS_H

class SystemOptions {
public:
	SystemOptions();

	void setXGAMode();
	int getXGAMode();
	void setSPECIALMode();
	int getSPECIALMode();
	void setNewConsoleMode();
	int getNewConsoleMode();

	void setKeyboardType(int type);
	void setKeyboardRepeatWait(int wait);
	void setKeyboardRepeatSpeed(int speed);

	void setMouseSpeed(int speed);
	void setMouseDClick(int dclick);
	void setMouseChangeLR(int changeLR);

	void setAtokMode(int mode);
	void setAtokKeyBind(int keyBind);

	void setSoftkbOnOff(int onOff);
	void setSoftkbQwerty(int qwerty);

	int getResWidth() { return m_resWidth; }
	int getResHeight() { return m_resHeight; }
	int getKeyboardType() { return m_keyboardType; }
	int getKeyboardRepeatWait() { return m_keyboardRepeatWait; }
	int getKeyboardRepeatSpeed() { return m_keyboardRepeatSpeed; }
	int getMouseSpeed() { return m_mouseSpeed; }
	int getMouseChangeLR() { return m_mouseChangeLR; }
	int getAtokMode() { return m_atokMode; }
	int getAtokKeyBind() { return m_atokKeyBind; }
	int getSoftkbOnOff() { return m_softkbOnOff; }
	int getSoftkbQwerty() { return m_softkbQwerty; }
	int getTimeZone() { return m_timeZone; }
	int getSummerTime() { return m_summerTime; }

	/* the console settings, copied from /dev/ps2sysconf by syscnf.cpp */
	void setSpdif(int v) { m_spdif = v; }
	void setAspect(int v) { m_aspect = v; }
	void setVideo(int v) { m_video = v; }
	void setSysLanguage(int v) { m_sysLanguage = v; }
	void setTimeZone(int v) { m_timeZone = v; }
	void setTimeZoneCity(int v) { m_timeZoneCity = v; }
	void setSummerTime(int v) { m_summerTime = v; }
	void setTimeNotation(int v) { m_timeNotation = v; }
	void setDateNotation(int v) { m_dateNotation = v; }
	void setAutoDiag(int v) { m_autoDiag = v; }

public:
	/* 0x00 */ int m_xgaMode;
	/* 0x04 */ int unk_04;  /* not set by the constructor */
	/* 0x08 */ int m_specialMode;  /* not set by the constructor */
	/* 0x0C */ int m_newConsoleMode;
	/* 0x10 */ int m_frameRate;  /* frames per second (60) */
	/* 0x14 */ int m_resWidth;
	/* 0x18 */ int m_resHeight;
	/* 0x1C */ int m_language;  /* script parameter 700 */
	/* 0x20 */ int unk_20;  /* script parameter 800 */
	/* 0x24 */ int m_keyboardType;
	/* 0x28 */ int m_keyboardRepeatWait;
	/* 0x2C */ int m_keyboardRepeatSpeed;
	/* 0x30 */ int m_mouseSpeed;
	/* 0x34 */ int m_mouseDClick;
	/* 0x38 */ int m_mouseChangeLR;
	/* 0x3C */ int m_atokMode;
	/* 0x40 */ int m_atokKeyBind;
	/* 0x44 */ int m_softkbOnOff;
	/* 0x48 */ int m_softkbQwerty;
	/* 0x4C-0x6C: the console settings of /dev/ps2sysconf */
	/* 0x4C */ int m_spdif;
	/* 0x50 */ int m_aspect;
	/* 0x54 */ int m_video;
	/* 0x58 */ int m_sysLanguage;
	/* 0x5C */ int m_timeZone;  /* minutes east of UTC (540 = JST) */
	/* 0x60 */ int m_timeZoneCity;  /* city ID of the time zone */
	/* 0x64 */ int m_summerTime;
	/* 0x68 */ int m_timeNotation;
	/* 0x6C */ int m_dateNotation;
	/* 0x70 */ int m_autoDiag;

public:
	virtual ~SystemOptions() {}   // vptr at 0x74
};  /* size = 0x78 */

extern SystemOptions SysOption;

#endif
