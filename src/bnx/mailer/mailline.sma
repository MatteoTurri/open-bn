/*
 * /opt0/bn/script/mailer/mailline.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A line of the message list of the mailer: an icon with the state of the message, the sender, the
 * subject and the date, filled and faded by the list.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flLitRed = 1.0; /* the color of the text of the line at the cursor */
new Float:g_flLitGreen = 1.0;
new Float:g_flLitBlue = 1.0;
new Float:g_flDimAlpha = 0.2;
new g_iItemCount;
new g_szItemId[64];

/* Appends to ids, from g_iItemCount on, the id of every element below node with no children and no text. */
CollectItemIds(node, ids[][])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szItemId[0] = 0;
    domGetData(node, "id", g_szItemId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            sprintf(ids[g_iItemCount++], "%s", g_szItemId);
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            CollectItemIds(child, ids);
        }
    }
}

new g_iItemIndex;
new g_szFindId[64];

/* Counts the empty leaves below node like CollectItemIds; the one whose id is id sets g_iItemIndex. */
FindItemIndex(node, id[])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szFindId[0] = 0;
    domGetData(node, "id", g_szFindId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            if (!strcmp(g_szFindId, id))
            {
                g_iItemIndex = g_iItemCount;
            }
            g_iItemCount++;
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            FindItemIndex(child, id);
        }
    }
}

public g_manager; /* the mailer page, which holds the icon textures */
public g_focus; /* unused, but part of the layout */
new g_objFrom;
new g_objSubject;
new g_objDate;
new g_objIcon;

main()
{
    new Float:icon_w = GetPubVar(g_manager, "g_AWidth");
    new Float:icon_h = GetPubVar(g_manager, "g_AHeight");
    g_objIcon = CreatePlane(Self());
    SetPos(g_objIcon, -221.0, 0.0, 0.0, 0);
    SetScale(g_objIcon, icon_w, icon_h, 0.0);
    SetColor(g_objIcon, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objIcon, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objIcon, TAG("ALPH"), BLEND_ADD);
    Hide(g_objIcon);
    g_objFrom = CreateLabel(Self());
    SetPos(g_objFrom, -169.0, 2.0, 0.0, 0);
    SetColor(g_objFrom, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objFrom, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objFrom, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objFrom, TAG("FSIZ"), 15);
    SetOptionI(g_objFrom, TAG("W64 "), floatround(210.0 - 50.0 - 10.0, 0) * 64);
    SetOptionI(g_objFrom, TAG("ZSRT"), ZSORT_OVERLAY);
    g_objSubject = CreateLabel(Self());
    SetPos(g_objSubject, -0.7, 2.0, 0.0, 0);
    SetColor(g_objSubject, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objSubject, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objSubject, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objSubject, TAG("FSIZ"), 15);
    SetOptionI(g_objSubject, TAG("W64 "), floatround(360.0 - 210.0 - 10.0, 0) * 64);
    SetOptionI(g_objSubject, TAG("ZSRT"), ZSORT_OVERLAY);
    g_objDate = CreateLabel(Self());
    SetPos(g_objDate, 162.0, 2.0, 0.0, 0);
    SetColor(g_objDate, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objDate, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objDate, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objDate, TAG("FSIZ"), 14);
    SetOptionI(g_objDate, TAG("W64 "), floatround(100.0, 0) * 64);
    SetOptionI(g_objDate, TAG("ZSRT"), ZSORT_OVERLAY);
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        /*
         * "FADE" IN1 (fade in lit), IN2, OUT or DIM with the time in ms, "COL" NOR/MAT (normal or lit text, at the
         * cursor); then the texts "SUBJ", "FROM", "DATE" and "READ" (the state of the message, see ShowStatusIcon).
         */
        while (RecvMessage(msg, what, duration))
        {
            if (msg == TAG("FADE"))
            {
                if (what == TAG("IN1"))
                {
                    FadeInLit(duration, 0.0);
                }
                else if (what == TAG("IN2"))
                {
                    FadeIn(duration, 0.0);
                }
                else if (what == TAG("OUT"))
                {
                    FadeOut(duration);
                }
                else if (what == TAG("DIM"))
                {
                    DimLine(duration);
                }
            }
            else if (msg == TAG("COL"))
            {
                if (what == TAG("NOR"))
                {
                    SetTextColor(g_flTextRed, g_flTextGreen, g_flTextBlue, duration, 0.0);
                }
                else if (what == TAG("MAT"))
                {
                    SetTextColor(1.0, 1.0, 1.0, duration, 0.0);
                }
            }
        }
        new text[246];
        while (RecvMessageStr(msg, text, 246))
        {
            if (msg == TAG("SUBJ"))
            {
                SetOptionS(g_objSubject, TAG("UTF8"), text);
            }
            else if (msg == TAG("FROM"))
            {
                SetOptionS(g_objFrom, TAG("UTF8"), text);
            }
            else if (msg == TAG("DATE"))
            {
                SetOptionS(g_objDate, TAG("UTF8"), text);
            }
            else if (msg == TAG("READ"))
            {
                ShowStatusIcon(text);
            }
        }
        sleep;
    }
    return;
}

/* Fades the texts and the icon to alpha. */
FadeLine(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objFrom, color[0], color[1], color[2], color[3]);
    AnimColor(g_objFrom, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objSubject, color[0], color[1], color[2], color[3]);
    AnimColor(g_objSubject, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objDate, color[0], color[1], color[2], color[3]);
    AnimColor(g_objDate, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objIcon, color[0], color[1], color[2], color[3]);
    AnimColor(g_objIcon, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Fades the line in, with the text in the color of the cursor line. */
FadeInLit(Float:duration, Float:delay)
{
    new Float:alpha = 1.0;
    new Float:color[4];
    AnimColor(g_objFrom, g_flLitRed, g_flLitGreen, g_flLitBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objSubject, g_flLitRed, g_flLitGreen, g_flLitBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objDate, g_flLitRed, g_flLitGreen, g_flLitBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objIcon, color[0], color[1], color[2], color[3]);
    AnimColor(g_objIcon, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Fades the line in, with the text in the normal color. */
FadeIn(Float:duration, Float:delay)
{
    new Float:alpha = 1.0;
    new Float:color[4];
    AnimColor(g_objFrom, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objSubject, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objDate, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objIcon, color[0], color[1], color[2], color[3]);
    AnimColor(g_objIcon, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

FadeOut(Float:duration)
{
    FadeLine(0.0, duration, 0.0);
}

DimLine(Float:duration)
{
    FadeLine(g_flDimAlpha, duration, 0.0);
}

/* Changes the color of the texts; a duration over 1 ms also makes them opaque. */
SetTextColor(Float:r, Float:g, Float:b, Float:duration, Float:delay)
{
    if (duration > 1.0)
    {
        AnimColor(g_objFrom, r, g, b, 1.0, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_objSubject, r, g, b, 1.0, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_objDate, r, g, b, 1.0, duration, delay, ANIM_FUNC_LINEAR);
    }
    else
    {
        new Float:color[4];
        GetColor(g_objFrom, color[0], color[1], color[2], color[3]);
        AnimColor(g_objFrom, r, g, b, color[3], duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objSubject, color[0], color[1], color[2], color[3]);
        AnimColor(g_objSubject, r, g, b, color[3], duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objDate, color[0], color[1], color[2], color[3]);
        AnimColor(g_objDate, r, g, b, color[3], duration, delay, ANIM_FUNC_LINEAR);
    }
}

/*
 * Shows the icon of status "R<unread>L<locked>C<partial>T0", each flag 0 or 1 (C: a multipart mail or a header cut
 * to fit); any other status hides it.
 */
ShowStatusIcon(status[])
{
    new tex;
    Show(g_objIcon);
    if (!strcmp(status, "R1L0C0T0"))
    {
        tex = GetPubVar(g_manager, "g_txA01");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R0L0C0T0"))
    {
        tex = GetPubVar(g_manager, "g_txA09");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R1L1C0T0"))
    {
        tex = GetPubVar(g_manager, "g_txA05");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R0L1C0T0"))
    {
        tex = GetPubVar(g_manager, "g_txA13");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R1L0C1T0"))
    {
        tex = GetPubVar(g_manager, "g_txA03");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R0L0C1T0"))
    {
        tex = GetPubVar(g_manager, "g_txA11");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R1L1C1T0"))
    {
        tex = GetPubVar(g_manager, "g_txA07");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else if (!strcmp(status, "R0L1C1T0"))
    {
        tex = GetPubVar(g_manager, "g_txA15");
        SetTexture(g_objIcon, tex, 0, -1);
    }
    else
    {
        Hide(g_objIcon);
    }
}
