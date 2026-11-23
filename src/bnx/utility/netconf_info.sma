/*
 * /opt0/bn/script/utility/netconf_info.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information line at the bottom of the network settings pages: "CHNG" (text) shows a new text, which
 * scrolls from right to left, over and over, when it is wider than the screen.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

main()
{
    new text_pending; /* a new text waits for its layout */
    new Float:x;
    new Float:text_w;
    new Float:step;
    SetOptionI(Self(), TAG("ALPH"), BLEND_ADD);
    SetOptionI(Self(), TAG("FONT"), FontID(0));
    SetOptionI(Self(), TAG("VALN"), ALIGN_CENTER);
    SetOptionI(Self(), TAG("FSIZ"), 15);
    SetOptionI(Self(), TAG("ZSRT"), ZSORT_OVERLAY);
    for (; ; )
    {
        if (text_pending)
        {
            new rect[5];
            GetOptionP(Self(), TAG("RECT"), rect, 5);
            if (rect[0])
            {
                AnimColor(Self(), 0.7843, 0.7843, 0.7843, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                x = 0.0;
                text_w = float(rect[3]) / 64.0;
                step = text_w > 640.0 ? 4.0 : 0.0;
                text_pending = 0;
            }
        }
        else
        {
            x = x - step;
            if (-text_w > x)
            {
                x = 640.0;
            }
            SetPos(Self(), x, 368.0, 0.0, 0);
        }
        new msg;
        new text[64];
        if (RecvMessageStr(msg, text, 64))
        {
            if (msg == TAG("CHNG"))
            {
                SetColor(Self(), 0.7843, 0.7843, 0.7843, 0.0);
                SetOptionS(Self(), TAG("UTF8"), text);
                text_pending = 1;
            }
        }
        sleep;
    }
}
