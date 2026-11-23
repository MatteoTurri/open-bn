/*
 * /opt0/bn/script/parts/infotext.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A text box with the text of its argument "text", which fades in once the text is laid out and follows
 * width, height and meta_alpha.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
new g_textbox;
public Float:width = 200.0;  /* in pixels */
public Float:height = 200.0; /* in pixels */
public Float:meta_alpha = 1.0; /* alpha of the text, set by the "FADE" message */
new g_laying_out; /* the text is not laid out yet: it stays hidden */

public main()
{
    g_textbox = CreateTextbox(Self());
    SetColor(g_textbox, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_textbox, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_textbox, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_textbox, TAG("VALN"), 0); /* not a vertical alignment: the text keeps its baseline */
    SetOptionI(g_textbox, TAG("FSIZ"), 15);
    SetOptionI(g_textbox, TAG("TW64"), floatround(64.0 * width, 0));
    SetOptionI(g_textbox, TAG("TH64"), floatround(64.0 * height, 0));
    SetOptionI(g_textbox, TAG("LS64"), 4 * 64);
    {
        new text[1024];
        GetArg("text", text, 1024);
        SetOptionS(g_textbox, TAG("UTF8"), text);
        g_laying_out = 1;
    }
    new Float:shown_width = width;
    new Float:shown_height = height;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = alpha;
                fade(duration, 0.0);
            }
        }
        if (g_laying_out)
        {
            new rect[5];
            GetOptionP(g_textbox, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                g_laying_out = 0;
                fade(100.0, 0.0);
            }
        }
        if (shown_width != width || shown_height != height)
        {
            SetOptionI(g_textbox, TAG("TW64"), floatround(64.0 * width, 0));
            SetOptionI(g_textbox, TAG("TH64"), floatround(64.0 * height, 0));
        }
        shown_width = width;
        shown_height = height;
        sleep;
    }
}

/* Fades the text to meta_alpha, once it is laid out. */
fade(Float:duration, Float:delay)
{
    if (!g_laying_out)
    {
        AnimColor(g_textbox, 1.0, 1.0, 1.0, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
}
