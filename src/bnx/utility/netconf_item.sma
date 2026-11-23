/*
 * /opt0/bn/script/utility/netconf_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of netconf_list: the object is the label with the name of a setting, and this script adds the label of
 * its value ("TEXT" text) on the right and fades both ("FADE" alpha, duration).
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

new g_value_label;

main()
{
    g_value_label = CreateLabel(Self());
    SetColor(g_value_label, 0.7843, 0.7843, 0.7843, 0.0);
    SetPos(g_value_label, 16.0, 0.0, 0.0, 0);
    SetOptionI(g_value_label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_value_label, TAG("FONT"), FontID(0));
    SetOptionI(g_value_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_value_label, TAG("FSIZ"), 15);
    SetOptionI(g_value_label, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_value_label, TAG("W64 "), 168 * 64);
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        if (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                fade(alpha, duration);
            }
        }
        new text[64];
        if (RecvMessageStr(msg, text, 64))
        {
            if (msg == TAG("TEXT"))
            {
                SetOptionS(g_value_label, TAG("UTF8"), text);
            }
        }
        sleep;
    }
}

/* Fades to alpha in duration ms. */
fade(Float:alpha, Float:duration)
{
    AnimColor(Self(), 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_value_label, 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}
