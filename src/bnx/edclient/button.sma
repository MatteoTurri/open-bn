/*
 * /opt0/bn/script/edclient/button.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A row of push buttons of an edclient page, one for each "data" child of its parts element. Circle on a
 * button tells the manager "CUSR" with its index, then "PUSH".
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

#include "background.inc"

public g_objManager; /* the page: gets "CUSR" and "PUSH", and "FOCS" when the focus leaves the row */
public g_objFocus; /* the focus object, moved onto the current button */
public g_tagParts; /* the element of the row: "current", and a "data" child with a "text_id" for each button */
new g_bActive; /* the row has the focus and takes the pad */
new g_iFontSize = 15;
new Float:g_flSpacing = 16.0; /* between two buttons */
new g_iCurrent;
new g_iCount;
new g_objLabels[10];
new g_objPlanes[10];
new g_objPushed; /* the lit plane that flashes on a pushed button */

main()
{
    Init();
    for (; ; )
    {
        Update();
        sleep;
    }
}

/* Creates the buttons, all as wide and as high as the largest label, centered in a row. */
Init()
{
    new i;
    new text[64];
    new node;
    new len;
    len = domGetData(g_tagParts, "current", text, 64);
    if (len == 1)
    {
        g_iCurrent = atoi(text);
    }
    g_iCount = 0;
    node = domFirstChild(g_tagParts, "");
    for (; node; node = domNext(node, ""))
    {
        domGetTagName(node, text, 64);
        if (0 != strcmp("data", text))
        {
            continue;
        }
        len = domGetData(node, "text_id", text, 64);
        if (0 == len)
        {
            continue;
        }
        get_tag_attr("TEXT", text, "value", text, 64);
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        new obj;
        obj = CreatePlane(Self());
        SetColor(obj, 0.0, 0.54, 0.78, 0.0);
        SetOptionI(obj, TAG("ALPH"), BLEND_ADD);
        SetOptionI(obj, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(obj, TAG("ZOFS"), 1.0);
        SetOptionP(obj, TAG("MGNT"), tex_margins, 4);
        SetTexture(obj, imgGetResource(RES_BUTTON0), 0, -1);
        g_objPlanes[g_iCount] = obj;
        obj = CreateLabel(Self());
        SetPos(obj, 0.0, 0.0, 0.0, 0);
        SetColor(obj, 0.215, 0.708, 0.999, 0.0);
        SetOptionI(obj, TAG("ALPH"), BLEND_ADD);
        SetOptionI(obj, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(obj, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(obj, TAG("FSIZ"), g_iFontSize);
        SetOptionI(obj, TAG("AP64"), 1 * 64);
        SetOptionI(obj, TAG("KP64"), 1 * 64);
        SetOptionI(obj, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionS(obj, TAG("UTF8"), text);
        g_objLabels[g_iCount] = obj;
        g_iCount++;
    }
    g_iCurrent = clamp(g_iCurrent, 0, g_iCount);
    new button_w;
    new button_h;
    i = 0;
    for (; i < g_iCount; i++)
    {
        for (; ; )
        {
            new rect[5];
            GetOptionP(g_objLabels[i], TAG("RECT"), rect, 5);
            if (rect[0])
            {
                new label_w = rect[3] >> 6;
                new label_h = rect[4] >> 6;
                button_w = button_w < label_w ? label_w : button_w;
                button_h = button_h < label_h ? label_h : button_h;
                break;
            }
            sleep;
        }
    }
    button_w = button_w + g_iFontSize * 2;
    button_h = button_h + g_iFontSize;
    new Float:row_w;
    row_w = (float(button_w) + g_flSpacing) * float(g_iCount) - g_flSpacing;
    new Float:x;
    x = 0.5 * (-1.0 * row_w) + 0.5 * float(button_w);
    i = 0;
    for (; i < g_iCount; i++)
    {
        SetPos(g_objLabels[i], x, 0.0, 0.0, 0);
        SetPos(g_objPlanes[i], x, 0.0, 0.0, 0);
        new Float:margins[4];
        if (float(button_w) > float(button_h))
        {
            margins[0] = margins[1] = 0.5 * float(button_h) / float(button_w);
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * float(button_w) / float(button_h);
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(g_objPlanes[i], TAG("MGN"), margins, 4);
        SetScale(g_objPlanes[i], float(button_w), float(button_h), 1.0);
        x = x + (float(button_w) + g_flSpacing);
    }
    i = 0;
    for (; i < g_iCount; i++)
    {
        AnimColor(g_objLabels[i], 0.215, 0.708, 0.999, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_objPlanes[i], 0.0, 0.54, 0.78, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    }
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        new plane;
        plane = CreatePlane(Self());
        SetColor(plane, 0.0, 0.54, 0.78, 0.0);
        SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
        SetOptionI(plane, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(plane, TAG("ZOFS"), 1.0);
        SetOptionP(plane, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        if (float(button_w) > float(button_h))
        {
            margins[0] = margins[1] = 0.5 * float(button_h) / float(button_w);
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * float(button_w) / float(button_h);
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(plane, TAG("MGN"), margins, 4);
        SetScale(plane, float(button_w), float(button_h), 1.0);
        SetTexture(plane, imgGetResource(RES_BUTTON1), 0, -1);
        g_objPushed = plane;
    }
}

/*
 * Handles the messages, then the pad while the row has the focus: circle pushes the current button, left and right move
 * between the buttons and leave the row at its ends.
 */
Update()
{
    ProcessMessages();
    if (0 == g_bActive)
    {
        return;
    }
    new pressed;
    PadState(_, pressed, _, _, -1);
    if (pressed & PAD_CIRCLE)
    {
        new Float:x;
        new Float:y;
        new Float:z;
        play_se(ED_SE_DECIDE);
        GetPos(g_objPlanes[g_iCurrent], x, y, z, 0);
        SetPos(g_objPushed, x, y, z, 0);
        AnimColor(g_objPlanes[g_iCurrent], 0.0, 0.54, 0.78, 1.0 * 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_objPushed, 0.0, 0.54, 0.78, 1.0 * 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(floatround(100.0, 0));
        AnimColor(g_objPlanes[g_iCurrent], 0.0, 0.54, 0.78, 1.0 * 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_objPushed, 0.0, 0.54, 0.78, 1.0 * 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(floatround(100.0, 0));
        SendMessage(g_objManager, TAG("CUSR"), g_iCurrent, 0, 0, 0);
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RRGT"), 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_CROSS)
    {
        SendMessage(g_objManager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_RIGHT)
    {
        if (g_iCurrent < g_iCount - 1)
        {
            g_iCurrent = g_iCurrent < g_iCount - 1 ? ++g_iCurrent : g_iCurrent; /* redundant as written */
            send_focus(g_objFocus, g_objPlanes[g_iCurrent], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            play_se(ED_SE_CURSOR);
        }
        else
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
            g_bActive = 0;
        }
    }
    else if (pressed & PAD_LEFT)
    {
        if (g_iCurrent > 0)
        {
            g_iCurrent = g_iCurrent > 0 ? --g_iCurrent : g_iCurrent; /* redundant as written */
            send_focus(g_objFocus, g_objPlanes[g_iCurrent], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            play_se(ED_SE_CURSOR);
        }
        else
        {
            SendMessage(g_objManager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
            g_bActive = 0;
        }
    }
    else if (pressed & PAD_UP)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
        g_bActive = 0;
    }
    else if (pressed & PAD_DOWN)
    {
        SendMessage(g_objManager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
        g_bActive = 0;
    }
}

ProcessMessages()
{
    new msg;
    new a;
    new Float:b;
    /* "FOCS": a the direction; "FADE": a the alpha, b the duration; "PUSH" and "CHNG" give the row the pad back. */
    while (RecvMessage(msg, a, b))
    {
        if (msg == TAG("FOCS"))
        {
            HandleFocus(a);
        }
        else if (msg == TAG("FADE"))
        {
            new Float:alpha = a;
            new Float:duration = b;
            for (new i; i < g_iCount; i++)
            {
                AnimColor(g_objLabels[i], 0.215, 0.708, 0.999, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_objPlanes[i], 0.0, 0.54, 0.78, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else if (msg == TAG("PUSH"))
        {
            g_bActive = 1;
        }
        else if (msg == TAG("CHNG"))
        {
            g_bActive = 1;
        }
    }
}

/* "FOUT" takes the focus away; any other dir gives it to the current button. */
HandleFocus(dir)
{
    if (dir == TAG("FOUT"))
    {
        g_bActive = 0;
        return;
    }
    if (dir == TAG("FRST"))
    {
    }
    else if (dir == TAG("RTRN"))
    {
    }
    send_focus(g_objFocus, g_objPlanes[g_iCurrent], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    g_bActive = 1;
}
