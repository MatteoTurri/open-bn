/*
 * /opt0/bn/script/mailer/mailprogressbar.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A progress bar 300 pixels long that follows progress, for mailprogresswindow: it is shown and hidden
 * with "FADE" IN/OUT (with the fade time in ms) and follows progress while shown.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

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

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
public auto_fade_out = 1; /* unused, but part of the layout */
new g_objTrack;
new g_objHighlight;
new g_objFill;
public progress; /* from 0 to 100 */
public dlg_active; /* unused, but part of the layout */
public dlg_enable; /* 1 while the bar is shown */

public main()
{
    g_objTrack = CreatePlane3x3(Self());
    SetScale(g_objTrack, 300.0, 10.0, 1.0);
    SetPos(g_objTrack, 0.0, 0.0, 0.0, 0);
    SetColor(g_objTrack, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objTrack, imgGetResource(RES_BAR1), 0, -1);
    SetOptionI(g_objTrack, TAG("ALPH"), BLEND_ADD);
    {
        new Float:track_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_objTrack, TAG("MGNT"), track_margin_tex, 4);
        new Float:track_margin[4] = {0.2, 0.2, 0.5, 0.5};
        SetOptionP(g_objTrack, TAG("MGN"), track_margin, 4);
        Hide(g_objTrack);
    }
    g_objHighlight = CreatePlane3x3(Self());
    SetPos(g_objHighlight, 300.0 * -0.5, 0.5, 0.0, 0);
    SetScale(g_objHighlight, 0.0, 1.0, 1.0);
    SetColor(g_objHighlight, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objHighlight, imgGetResource(RES_BAR2), 0, -1);
    SetOptionI(g_objHighlight, TAG("ALPH"), BLEND_ADD);
    {
        new Float:highlight_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_objHighlight, TAG("MGNT"), highlight_margin_tex, 4);
        new Float:highlight_margin[4] = {0.5, 0.0, 0.5, 0.5};
        SetOptionP(g_objHighlight, TAG("MGN"), highlight_margin, 4);
        Hide(g_objHighlight);
    }
    g_objFill = CreatePlane3x3(Self());
    SetPos(g_objFill, 300.0 * -0.5, 0.0, 0.0, 0);
    SetScale(g_objFill, 0.0, 10.0, 1.0);
    SetColor(g_objFill, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_objFill, imgGetResource(RES_BAR3), 0, -1);
    SetOptionI(g_objFill, TAG("ALPH"), BLEND_ADD);
    {
        new Float:fill_margin_tex[4] = {0.0, 0.5, 0.5, 0.5};
        SetOptionP(g_objFill, TAG("MGNT"), fill_margin_tex, 4);
        new Float:fill_margin[4] = {0.0, 0.5, 0.5, 0.5};
        SetOptionP(g_objFill, TAG("MGN"), fill_margin, 4);
        Hide(g_objFill);
    }
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        while (RecvMessage(msg, what, duration))
        {
            if (msg == TAG("FADE"))
            {
                new Float:fade_time = duration;
                if (what == TAG("IN"))
                {
                    Show(g_objTrack);
                    Show(g_objHighlight);
                    Show(g_objFill);
                    AnimColor(g_objTrack, 1.0, 1.0, 1.0, 1.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
                    AnimColor(g_objHighlight, 1.0, 1.0, 1.0, 0.5, fade_time, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 1;
                }
                else if (what == TAG("OUT"))
                {
                    AnimColor(g_objTrack, 1.0, 1.0, 1.0, 0.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
                    AnimColor(g_objHighlight, 1.0, 1.0, 1.0, 0.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
                    AnimColor(g_objFill, 1.0, 1.0, 1.0, 0.0, fade_time, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 0;
                }
            }
        }
        if (dlg_enable)
        {
            progress = clamp(progress, 0, 100);
            new Float:move_time = 25.0;
            AnimScale(g_objHighlight, 300.0 * progress / 100, 1.0, 1.0, move_time, 0.0, ANIM_FUNC_LINEAR);
            AnimMoveLine(g_objHighlight, 300.0 * -0.5 - 300.0 * -0.5 * progress / 100, 0.5, 0.0, move_time, 0.0,
                    ANIM_FUNC_LINEAR);
            AnimScale(g_objFill, 300.0 * progress / 100, 10.0, 1.0, move_time, 0.0, ANIM_FUNC_LINEAR);
            /* the fill is drawn cap_w ahead; shorter than its cap it is hidden */
            static Float:cap_w = 10.0;
            AnimMoveLine(g_objFill, 300.0 * -0.5 - 300.0 * -0.5 * progress / 100 + cap_w, 0.0, 0.0, move_time, 0.0,
                    ANIM_FUNC_LINEAR);
            {
                new Float:fill_scale[3];
                GetScale(g_objFill, fill_scale[0], fill_scale[1], fill_scale[2]);
                if (fill_scale[0] < cap_w)
                {
                    AnimColor(g_objFill, 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else if (fill_scale[0] < 100.0)
                {
                    new Float:margin[4] = {0.0, 0.5, 0.5, 0.5};
                    margin[1] = cap_w / fill_scale[0];
                    SetOptionP(g_objFill, TAG("MGN"), margin, 4);
                    AnimColor(g_objFill, 1.0, 1.0, 1.0, 1.0, 10.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    new Float:margin[4] = {0.0, 0.5, 0.5, 0.5};
                    margin[1] = cap_w / fill_scale[0];
                    margin[0] = 1.0 - 90.0 / fill_scale[0] - margin[1];
                    SetOptionP(g_objFill, TAG("MGN"), margin, 4);
                    AnimColor(g_objFill, 1.0, 1.0, 1.0, 1.0, 10.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
        }
        sleep;
    }
}
