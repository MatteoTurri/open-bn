/*
 * /opt0/bn/script/mailer/maildlgnobtn.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A mailer message box without buttons, shown and hidden by the mailer page; while the page lets it
 * take the pad, the cross button closes it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new g_iItemCount;
new g_szItemId[64];

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

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

public manager; /* the mailer page: told with "DGWT" that the dialog is ready */
public focus; /* the focus object, whose position and brightness the glow follows */
public Float:width = 400.0;  /* the size of the window, in pixels */
public Float:height = 260.0; /* the size of the window, in pixels */
public Float:red;            /* the color of the window */
public Float:green = 0.2353; /* the color of the window */
public Float:blue = 0.3922;  /* the color of the window */
new g_objBase;
new g_objWindow;
new g_objGlow;
new g_objMessage;
new g_bMessageShown;
public dlg_active; /* set by the manager; cleared when the user answers */
public dlg_enable; /* 1 while the dialog is shown and takes the pad */
public pad_active; /* 1 while the cross button may close the box ("FOCS" IN) */

#include "misc.inc"

main()
{
    new message[128];
    GetArg("msg", message, 128);
    new ok_label[8];
    if (!GetArg("ok", ok_label, 8))
    {
        copy_string(ok_label, "OK", 8);
    }
    g_objBase = CreatePlane(Self());
    SetLocalScale(g_objBase, 2.0 * width, 2.0 * height, 1.0);
    SetColor(g_objBase, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objBase, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_objBase, TAG("ZOFS"), -2.0);
    SetOptionI(g_objBase, TAG("ALPH"), BLEND_KEEP);
    Hide(g_objBase);
    g_objWindow = CreatePlane3x3(g_objBase);
    SetScale(g_objWindow, width, height, 1.0);
    SetColor(g_objWindow, red, green, blue, 0.0);
    SetTexture(g_objWindow, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_objWindow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_objWindow, TAG("ZOFS"), -1.0);
    SetOptionI(g_objWindow, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_objWindow, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / width;
        margins[2] = margins[3] = 0.5 * 128.0 / height;
        SetOptionP(g_objWindow, TAG("MGN"), margins, 4);
        Hide(g_objWindow);
    }
    g_objGlow = CreatePlane(g_objWindow);
    SetPos(g_objGlow, 0.3, -0.2, 0.0, 0);
    SetScale(g_objGlow, 2.0, 2.0, 1.0);
    SetColor(g_objGlow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_objGlow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_objGlow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_objGlow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    Hide(g_objGlow);
    g_objMessage = create_message_text(Self(), floatround(width - 64.0, 0), floatround(height - 48.0, 0));
    SetOptionS(g_objMessage, TAG("UTF8"), message);
    SetColor(g_objMessage, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    Hide(g_objMessage);
    sleep;
    SendMessage(manager, TAG("DGWT"), 0, 0, 0, 0);
    sndSePlay(SE_DIALOG_OPEN);
    pad_active = dlg_enable = 0;
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        /*
         * From the manager: "FADE" IN/IN2/OUT with the fade time in ms (IN2 hides the pad help), "FOCS" IN/OUT to
         * let the box take the pad or not.
         */
        while (RecvMessage(msg, what, duration))
        {
            if (msg == TAG("FADE"))
            {
                new Float:fade_time = duration;
                if (what == TAG("IN"))
                {
                    ShowPadHelp();
                    Show(g_objBase);
                    Show(g_objWindow);
                    Show(g_objGlow);
                    Show(g_objMessage);
                    FadeWindow(1.0, fade_time);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 1;
                }
                else if (what == TAG("IN2"))
                {
                    SetSystemParam(SYS_PADHELP, 0);
                    Show(g_objBase);
                    Show(g_objWindow);
                    Show(g_objGlow);
                    Show(g_objMessage);
                    FadeWindow(1.0, fade_time);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 1;
                }
                else if (what == TAG("OUT"))
                {
                    FadeWindow(0.0, fade_time);
                    AnimColor(g_objMessage, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0, fade_time, 0.0,
                            ANIM_FUNC_LINEAR);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 0;
                }
            }
            else if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    pad_active = 1;
                }
                else if (what == TAG("OUT"))
                {
                    pad_active = 0;
                }
            }
        }
        if (dlg_enable)
        {
            {
                new pressed;
                if (pad_active)
                {
                    PadState(_, pressed, _, _, -1);
                }
                if (pressed & PAD_CROSS)
                {
                    sndSePlay(SE_CANCEL);
                    dlg_active = 0;
                }
            }
            /* The glow on the window mirrors the position of the focus and follows its brightness. */
            if (focus)
            {
                new Float:focus_pos[3];
                GetPos(focus, focus_pos[0], focus_pos[1], focus_pos[2], g_objWindow);
                SetPos(g_objGlow, -focus_pos[0], focus_pos[1], 0.0, 0);
                new Float:focus_alpha;
                GetColor(focus, _, _, _, focus_alpha);
                AnimColor(g_objGlow, 0.5 * focus_alpha, 0.5 * focus_alpha, 0.5 * focus_alpha, 1.0, 100.0, 0.0,
                        ANIM_FUNC_LINEAR);
            }
            /* Centers the message once the textbox has laid it out, then fades it in. */
            if (!g_bMessageShown)
            {
                new rect[5];
                GetOptionP(g_objMessage, TAG("RECT"), rect, 5);
                if (rect[0])
                {
                    g_bMessageShown = 1;
                    SetPos(g_objMessage, 0.5 * -float(rect[3] >> 6), -8.0 - 0.5 * float(rect[4] >> 6), 0.0, 0);
                    AnimColor(g_objMessage, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, 100.0, 0.0,
                            ANIM_FUNC_LINEAR);
                }
            }
        }
        sleep;
    }
    return;
}

/* Fades the window to alpha over duration ms. */
FadeWindow(Float:alpha, Float:duration)
{
    AnimColor(g_objWindow, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
}
