/*
 * The inline members of gm_window, included at the end of gm_packet.cpp.
 */

#ifndef GM_WINDOW_INLINE_H
#define GM_WINDOW_INLINE_H
#pragma interface

inline gm_window::gm_window()
{
	m_gsFrame = 0;
	m_gsZbuf = 0;
	m_offsetX2 = 1728;
	m_offsetX1 = 1728;
	m_offsetY2 = 1824;
	m_offsetY1 = 1824;
	m_width2 = 640;
	m_width1 = 640;
	m_height2 = 448;
	m_height1 = 448;
	m_flags = 0;
	m_state = 0;
	m_frameAlign = 32;
	m_zbufAlign = 16;
}

inline gm_window::~gm_window()
{
	term();
}

inline int gm_window::init(int flags)
{
	m_flags = flags;
	m_state |= 1;
	update();
	return 1;
}

inline void gm_window::term()
{
	m_state &= ~1;
}

inline void gm_window::update()
{
	int width, height;
	fwGetScreenSize(&width, &height);

	int offX = 2048 - width / 2;
	m_offsetX2 = offX;
	m_offsetX1 = offX;
	int offY = 2048 - height / 2;
	m_offsetY2 = offY;
	m_offsetY1 = offY;
	m_width2 = width;
	m_width1 = width;
	m_height2 = height;
	m_height1 = height;

	if (fwGsGetValueFRAME(&m_gsFrame))
		m_frameAlign = (m_gsFrame & 0x02000000) ? 16 : 32;
	if (fwGsGetValueZBUF(&m_gsZbuf))
		m_zbufAlign = (m_gsZbuf & 0x02000000) ? 16 : 32;
}

inline void gm_window::get_gs_frame(int *out)
{
	out[0] = (int)m_gsFrame;
	out[1] = 0;
}

inline void gm_window::get_gs_zbuf(int *out)
{
	out[0] = (int)m_gsZbuf;
	out[1] = 0;
}

inline void gm_window::get_gs_tex0(int *out)
{
	/* TEX0 that samples the frame buffer as a 1024x1024 texture (TBP0 = FBP * 32,
	   TBW = FBW, same PSM). */
	int v = (int)m_gsFrame;
	unsigned fbp = (v & 0x1ff) << 5;
	unsigned fbw = (v >> 16) & 0x3f;
	unsigned psm = (v >> 24) & 0x3f;
	fbw <<= 14;
	fbw |= 0xa8000000u;
	fbp |= fbw;
	psm <<= 20;
	fbp |= psm;
	out[0] = (int)fbp;
	out[1] = 2;
}

/* Screen size as a fraction of the 1024x1024 texture. */
inline void gm_window::get_nonpow2(float *out)
{
	out[0] = (float)m_width1 * 0.0009765625f;
	out[1] = (float)m_height1 * 0.0009765625f;
}

#endif
