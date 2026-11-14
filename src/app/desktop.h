/*
 * Desktop: base class of the root window. A singleton: the constructor
 * registers the instance returned by GetDesktop().
 */

#ifndef DESKTOP_H
#define DESKTOP_H

class Desktop {
public:
	/* 0x00 */ int unk_00;
	/* 0x04 */ int unk_04;
	virtual ~Desktop();

	Desktop(int, int);
	static Desktop *GetDesktop();
	void DispatchMouseMessage(int, int, int, int) const;
	void Draw() const;

	static Desktop *desktop;
};

#endif
