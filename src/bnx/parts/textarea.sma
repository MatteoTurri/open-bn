/*
 * /opt0/bn/script/parts/textarea.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A read-only text box with a scroll bar: the message of its "msg" element (or of its argument "msg"),
 * scrolled a line at a time by the focus messages UP and "DOWN".
 */

#include <core>
#include <bn>

#pragma dynamic 4096

#include "tag.inc"

#include <float>

public root; /* the element whose "msg" child holds the text, or 0 */
public manager; /* told "FOCS" "RTRN" after each focus message */
public focus; /* unused, but part of the layout */
public padhelp; /* the pad help buttons (PADHELP_* bits) */
public Float:width = 400.0;  /* in pixels */
public Float:height = 260.0; /* in pixels */
new g_text;
new Float:g_thumb_length;
new g_shown_line; /* the g_top_line the thumb shows */
new g_top_line;
new g_line_count; /* lines of the whole text */
new g_page_lines; /* lines in the box */
new g_bar_back;
new g_bar_track;
new g_bar_thumb;

create_text(parent, text_w, text_h)
{
    new edit = CreateEditML(parent);
    SetOptionI(edit, TAG("ALPH"), BLEND_ADD);
    SetOptionI(edit, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(edit, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionI(edit, TAG("TW64"), text_w << 6);
    SetOptionI(edit, TAG("TH64"), text_h << 6);
    SetOptionI(edit, TAG("LS64"), 4 * 64);
    new color[1] = {0xFFC8C8C8};
    SetOptionP(edit, TAG("COLR"), color, 1);
    new bg_color[4];
    SetOptionP(edit, TAG("BGC4"), bg_color, 4);
    return edit;
}

/*
 * Reads the text of tag, or with an "id" attribute the value of the TEXT element with that id; without a tag,
 * the argument "msg". Returns 0 when there is none.
 */
get_message(tag, text[], maxlength)
{
    if (tag)
    {
        if (domGetData(tag, "id", text, maxlength))
        {
            tag = domFindTag(domRoot(), "TEXT", text, 0);
            return domGetData(tag, "value", text, maxlength);
        }
        return domGetData(tag, "", text, maxlength);
    }
    return GetArg("msg", text, maxlength);
}

/* Creates the back, track and thumb of the scroll bar, bar_h pixels high, at x. */
create_scroll_bar(parent, Float:x, Float:bar_h)
{
    g_bar_back = CreatePlane(parent);
    SetPos(g_bar_back, 22.0 + x, 0.0, 0.0, 0);
    SetScale(g_bar_back, 48.0, bar_h, 1.0);
    SetColor(g_bar_back, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_back, imgGetResource(RES_SCRBAR1), 0, -1);
    SetOptionI(g_bar_back, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_back, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_back, TAG("ZOFS"), 1.0);
    g_bar_track = CreatePlane(parent);
    SetPos(g_bar_track, 2.0 + x, 0.0, 0.0, 0);
    SetScale(g_bar_track, 5.0, bar_h, 1.0);
    SetColor(g_bar_track, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_track, imgGetResource(RES_SCRBAR2), 0, -1);
    SetOptionI(g_bar_track, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_track, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_track, TAG("ZOFS"), 2.0);
    SetOptionF(g_bar_track, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    SetOptionF(g_bar_track, TAG("MGN"), 0.5, 0.5, 0.5, 0.5);
    g_bar_thumb = CreatePlane(parent);
    SetPos(g_bar_thumb, x, 0.0, 0.0, 0);
    SetScale(g_bar_thumb, 5.0, bar_h, 1.0);
    SetColor(g_bar_thumb, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_thumb, imgGetResource(RES_SCRBAR3), 0, -1);
    SetOptionI(g_bar_thumb, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_thumb, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_thumb, TAG("ZOFS"), 3.0);
    SetOptionF(g_bar_thumb, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    SetOptionF(g_bar_thumb, TAG("MGN"), 0.5, 0.5, 0.5, 0.5);
    g_thumb_length = height;
    g_shown_line = -1;
}

/* Moves and sizes the thumb for the lines in view, in duration ms. */
update_scroll_bar(Float:duration)
{
    new Float:thumb_h = float(g_page_lines) / float(g_line_count);
    if (thumb_h > 1.0)
    {
        thumb_h = 1.0;
    }
    thumb_h = thumb_h * height;
    if (g_thumb_length != thumb_h)
    {
        AnimScale(g_bar_thumb, 5.0, thumb_h, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
        g_thumb_length = thumb_h;
    }
    if (g_shown_line != g_top_line)
    {
        new Float:thumb_x;
        GetPos(g_bar_thumb, thumb_x, _, _, 0);
        AnimMoveLine(g_bar_thumb, thumb_x, height * (float(g_top_line) / float(g_line_count) - 0.5) + 0.5 * thumb_h,
                0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        g_shown_line = g_top_line;
    }
    new Float:margin;
    GetScale(g_bar_thumb, _, margin);
    margin = 0.5 * 20.0 / margin;
    if (margin > 0.5)
    {
        margin = 0.5;
    }
    SetOptionF(g_bar_thumb, TAG("MGN"), 0.5, 0.5, margin, margin);
}

main()
{
    if (root)
    {
        root = domFirstChild(root, "msg");
    }
    {
        new text[2048];
        if (get_message(root, text, 2048))
        {
            g_text = create_text(Self(), floatround(width - 16.0, 0), floatround(height, 0));
            SetPos(g_text, 16.0 - 0.5 * width, 0.5 * -height, 0.0, 0);
            SetOptionS(g_text, TAG("UTF8"), text);
            GetOptionI(g_text, TAG("CPDM"), g_line_count);
            GetOptionI(g_text, TAG("CPDL"), g_page_lines);
            SetOptionI(g_text, TAG("CPDY"), 0);
            SetOptionI(g_text, TAG("MDFY"), 0);
            SetOptionI(g_text, TAG("CRSW"), 0);
            SetOptionI(g_text, TAG("EDTE"), 0);
        }
    }
    create_scroll_bar(Self(), 0.5 * -width, height);
    for (; ; )
    {
        sleep;
        update_scroll_bar(100.0);
        new msg;
        new a;
        new Float:b;
        /* "FADE" (alpha, duration); "FOCS" (direction): "UP" or "DOWN" */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FADE"))
            {
                fade(a, b);
            }
            else if (msg == TAG("FOCS"))
            {
                if (a == TAG("UP"))
                {
                    if (g_top_line > 0)
                    {
                        g_top_line--;
                        SetOptionI(g_text, TAG("SCLY"), -1);
                    }
                }
                else if (a == TAG("DOWN"))
                {
                    if (g_top_line < g_line_count - g_page_lines)
                    {
                        g_top_line++;
                        SetOptionI(g_text, TAG("SCLY"), 1);
                    }
                }
                SendMessage(manager, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
    }
}

/* Fades to alpha in duration ms. */
fade(Float:alpha, Float:duration)
{
    AnimColor(g_text, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_bar_back, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_bar_track, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_bar_thumb, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}
