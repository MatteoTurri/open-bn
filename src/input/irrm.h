/*
 * IRrm: the IR remote control of the BB Navigator (not polymorphic).
 */

#ifndef IRRM_H
#define IRRM_H

class IRrm {
public:
	/* 0x00 */ int m_stat;
	/* 0x04 */ unsigned short m_deviceId;  /* remote model: 2362 or 7002 */
	/* 0x06 */ char m_index;  /* scancode, indexes the code tables */
	/* 0x07 */ char m_pad7;
	/* 0x08 */ int m_bypass;
	/* 0x0C */ int m_rmCondition;

	IRrm();
	~IRrm();

	int BypassSwitch(char enable);
	int GetStat();
	int GetSignal();
	int GetRMCode();
	int GetRMCondition();
	unsigned short GetPadCode();
}; /* size = 0x10 */

#endif
