/*
 * /opt0/bn/script/mailer/mailaline.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A line of the address book of the mailer: the name and address of an entry, filled and faded by the list.
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

public g_manager; /* unused, but part of the layout */
public g_focus; /* unused, but part of the layout */
new g_objName;
new g_objAddress;

main()
{
    new text[32];
    g_objName = CreateLabel(Self());
    SetPos(g_objName, -249.0, 2.0, 0.0, 0);
    SetColor(g_objName, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objName, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objName, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objName, TAG("FSIZ"), 15);
    SetOptionI(g_objName, TAG("W64 "), floatround(210.0, 0) * 64); /* the name column, 210 pixels */
    SetOptionI(g_objName, TAG("ZSRT"), ZSORT_OVERLAY);
    g_objAddress = CreateLabel(Self());
    SetPos(g_objAddress, -24.0, 2.0, 0.0, 0);
    SetColor(g_objAddress, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionI(g_objAddress, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_objAddress, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objAddress, TAG("FSIZ"), 15);
    SetOptionI(g_objAddress, TAG("W64 "), floatround(285.0, 0) * 64); /* the address column, 285 pixels */
    SetOptionI(g_objAddress, TAG("ZSRT"), ZSORT_OVERLAY);
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        /*
         * "FADE" IN1 (fade in lit), IN2, OUT or DIM with the time in ms, "COL" NOR/MAT (normal or lit text, at the
         * cursor); then the texts "ADDR" and "NAME".
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
        while (RecvMessageStr(msg, text, 32))
        {
            if (msg == TAG("ADDR"))
            {
                SetOptionS(g_objAddress, TAG("UTF8"), text);
            }
            else if (msg == TAG("NAME"))
            {
                SetOptionS(g_objName, TAG("UTF8"), text);
            }
        }
        sleep;
    }
    return;
}

/* Fades the texts to alpha. */
FadeLine(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    GetColor(g_objName, color[0], color[1], color[2], color[3]);
    AnimColor(g_objName, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
    GetColor(g_objAddress, color[0], color[1], color[2], color[3]);
    AnimColor(g_objAddress, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Fades the line in, with the text in the color of the cursor line. */
FadeInLit(Float:duration, Float:delay)
{
    new Float:alpha = 1.0;
    AnimColor(g_objName, g_flLitRed, g_flLitGreen, g_flLitBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objAddress, g_flLitRed, g_flLitGreen, g_flLitBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Fades the line in, with the text in the normal color. */
FadeIn(Float:duration, Float:delay)
{
    new Float:alpha = 1.0;
    AnimColor(g_objName, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_objAddress, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, delay, ANIM_FUNC_LINEAR);
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
        AnimColor(g_objName, r, g, b, 1.0, duration, delay, ANIM_FUNC_LINEAR);
        AnimColor(g_objAddress, r, g, b, 1.0, duration, delay, ANIM_FUNC_LINEAR);
    }
    else
    {
        new Float:color[4];
        GetColor(g_objName, color[0], color[1], color[2], color[3]);
        AnimColor(g_objName, r, g, b, color[3], duration, delay, ANIM_FUNC_LINEAR);
        GetColor(g_objAddress, color[0], color[1], color[2], color[3]);
        AnimColor(g_objAddress, r, g, b, color[3], duration, delay, ANIM_FUNC_LINEAR);
    }
}
