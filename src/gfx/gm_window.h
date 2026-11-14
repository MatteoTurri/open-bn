/*
 * gm_window: the GS render target of the gm_* layer (screen offsets and size,
 * stored twice and always identical, and the FRAME/ZBUF register values).
 */

#pragma once

class gm_window {
public:
	/* 0x00 */ int m_flags;
	/* 0x04 */ int m_state;  /* bit 0: initialized */
	/* 0x08 */ int m_offsetX1, m_offsetY1, m_width1, m_height1;  /* offset = 2048 - size / 2 */
	/* 0x18 */ int m_offsetX2, m_offsetY2, m_width2, m_height2;
	/* 0x28 */ int m_frameAlign;  /* 16 for 16-bit pixel formats, else 32 */
	/* 0x2C */ int m_zbufAlign;
	/* 0x30 */ unsigned long long m_gsFrame;  /* GS FRAME register */
	/* 0x38 */ unsigned long long m_gsZbuf;  /* GS ZBUF register */

	gm_window();
	~gm_window();

	int init(int flags);
	void term();
	void update();
	void get_gs_frame(int *out);
	void get_gs_zbuf(int *out);
	void get_gs_tex0(int *out);
	void get_nonpow2(float *out);

	int get_offset_x() const { return m_offsetX1; }
	int get_offset_y() const { return m_offsetY1; }
	int get_width() const { return m_width1; }
	int get_height() const { return m_height1; }
}; /* size = 0x40 */
