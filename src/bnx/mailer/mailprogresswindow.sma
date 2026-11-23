/*
 * /opt0/bn/script/mailer/mailprogresswindow.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A mailer message box with a progress bar (mailprogressbar) that follows progress, the percentage
 * (argument "percent", a format) and a cancel button (argument "cancel", its label) pressed with
 * the circle button; shown and hidden by the mailer page.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

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

new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flDimAlpha = 0.35;
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

public manager; /* the mailer page: told with "DGWT" that the dialog is ready */
public focus; /* the focus object, moved onto the cancel button */
public progress; /* from 0 to 100 */
public progress2; /* a second value for the percent format */
public Float:width = 400.0;  /* the size of the window, in pixels */
public Float:height = 260.0; /* the size of the window, in pixels */
public Float:red;            /* the color of the window */
public Float:green = 0.2353; /* the color of the window */
public Float:blue = 0.3922;  /* the color of the window */
new g_objBase;
new g_objWindow;
new g_objGlow;
new g_objBar; /* the mailprogressbar object */
new g_objMessage;
new g_objButton;
new g_button_label;
new g_objPercentLabel;
new g_bShowPercent = 1;
new g_bHasCancel = 1;
public dlg_active = 1; /* not changed here */
public dlg_cancel = 1; /* cleared when the cancel button is pressed */
public dlg_enable; /* 1 while the dialog is shown and takes the pad */

#include "string.inc"

#include "misc.inc"

main()
{
    new message[128];
    GetArg("msg", message, 128);
    new cancel_label[8];
    GetArg("cancel", cancel_label, 8);
    new percent_format[8];
    GetArg("percent", percent_format, 8);
    if (!strcmp(cancel_label, ""))
    {
        g_bHasCancel = 0;
    }
    if (!strcmp(percent_format, ""))
    {
        g_bShowPercent = 0;
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
    g_objBar = CreateNull(Self());
    SetOptionI(g_objBar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_objBar, _, "mailprogressbar");
    SetPubVar(g_objBar, "manager", Self());
    SetPubVar(g_objBar, "progress", 0);
    Hide(g_objBar);
    g_objMessage = create_button_label(Self(), 0.0, -30.0);
    SetColor(g_objMessage, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    SetOptionS(g_objMessage, TAG("UTF8"), message);
    Hide(g_objMessage);
    if (g_bHasCancel)
    {
        g_objButton = create_button_textured(Self(), 0.0, 90.0);
        g_button_label = create_button_label(Self(), 0.0, 90.0);
        SetColor(g_button_label, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionS(g_button_label, TAG("UTF8"), cancel_label);
        SetOptionI(g_button_label, TAG("ALPH"), BLEND_ADD);
        Hide(g_button_label);
    }
    if (g_bShowPercent)
    {
        new percent_text[8];
        sprintf(percent_text, percent_format, 0, 0);
        g_objPercentLabel = create_button_label(Self(), 0.0, 30.0);
        SetColor(g_objPercentLabel, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        SetOptionS(g_objPercentLabel, TAG("UTF8"), percent_text);
        Hide(g_objPercentLabel);
    }
    sleep;
    SendMessage(manager, TAG("DGWT"), 0, 0, 0, 0);
    sndSePlay(SE_DIALOG_OPEN);
    dlg_enable = 0;
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        /*
         * From the manager: "FADE" IN/OUT with the fade time in ms, "FOCS" IN/OUT, and "FOCS" DIM, which dims the
         * cancel button and disables it.
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
                    Show(g_objBar);
                    Show(g_objMessage);
                    Show(g_button_label);
                    Show(g_objPercentLabel);
                    FadeWindow(1.0, fade_time);
                    SendMessage(g_objBar, TAG("FADE"), TAG("IN"), fade_time, 0, 0);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 1;
                }
                else if (what == TAG("OUT"))
                {
                    FadeWindow(0.0, fade_time);
                    SendMessage(g_objBar, TAG("FADE"), TAG("OUT"), fade_time, 0, 0);
                    wait_ms(floatround(fade_time, 0));
                    dlg_enable = 0;
                }
            }
            else if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    if (g_bHasCancel)
                    {
                        send_focus(focus, g_objButton, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    }
                }
                else if (what == TAG("OUT"))
                {
                    if (g_bHasCancel)
                    {
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    }
                    g_bHasCancel = 0;
                }
                else if (what == TAG("DIM"))
                {
                    if (g_bHasCancel)
                    {
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    }
                    SetColor(g_button_label, g_flTextRed, g_flTextGreen, g_flTextBlue, g_flDimAlpha);
                    SetColor(g_objButton, 1.0, 1.0, 1.0, g_flDimAlpha);
                    g_bHasCancel = 0;
                }
            }
        }
        if (dlg_enable)
        {
            if (g_bHasCancel)
            {
                new pressed;
                PadState(_, pressed, _, _, -1);
                if (pressed & PAD_CIRCLE)
                {
                    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    sleep;
                    SetPubVar(g_objBar, "progress", progress);
                    dlg_cancel = 0;
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
            if (g_bShowPercent)
            {
                new percent_text[8];
                sprintf(percent_text, percent_format, progress, progress2);
                SetOptionS(g_objPercentLabel, TAG("UTF8"), percent_text);
            }
            SetPubVar(g_objBar, "progress", progress);
        }
        sleep;
    }
    return;
}

/* Fades the window, the message, the button and the percentage to alpha over duration ms. */
FadeWindow(Float:alpha, Float:duration)
{
    AnimColor(g_objWindow, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_objMessage, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (g_bHasCancel)
    {
        AnimColor(g_objButton, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_bHasCancel)
    {
        AnimColor(g_button_label, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_bShowPercent)
    {
        AnimColor(g_objPercentLabel, g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE);
}
