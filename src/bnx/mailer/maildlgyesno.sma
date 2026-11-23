/*
 * /opt0/bn/script/mailer/maildlgyesno.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A mailer question box with two buttons (labels: arguments "yes" and "no"), shown and hidden by the
 * mailer page: left and right choose, the circle button presses, the cross button cancels.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* As in mailer.inc: the answers, and the buttons of g_objButtons. */
const ANSWER_YES = 0;
const ANSWER_NO = 1;
const ANSWER_CANCEL = -1;

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "text.inc"

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
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
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
public focus; /* the focus object, moved onto the selected button */
public select; /* in: the button selected first; out: ANSWER_YES, ANSWER_NO or ANSWER_CANCEL (mailer.inc) */
public Float:width = 400.0;  /* the size of the window, in pixels */
public Float:height = 260.0; /* the size of the window, in pixels */
public Float:red;            /* the color of the window */
public Float:green = 0.2353; /* the color of the window */
public Float:blue = 0.3922;  /* the color of the window */
new g_objBase;
new g_objWindow;
new g_objGlow;
new g_objMessage;
new g_objButtonLabels[2];
new g_objButtons[2];
new g_objButtonLit;
new g_bMessageShown;
public dlg_active; /* set by the manager; cleared when the user answers */
public dlg_enable; /* 1 while the dialog is shown and takes the pad */

#include "misc.inc"

#include "string.inc"

#include "button.inc"

/* Like fit_button_to_label, for the label of button index. */
FitButtonsToLabel(index, &Float:button_w, &Float:button_h)
{
    for (new rect[5]; ; )
    {
        GetOptionP(g_objButtonLabels[index], TAG("RECT"), rect, 5);
        if (rect[0])
        {
            new Float:extent = float(rect[3] >> 6) + float(30);
            if (button_w < extent)
            {
                button_w = extent;
            }
            extent = 20.0 + float(rect[4] >> 6);
            if (button_h < extent)
            {
                button_h = extent;
            }
            break;
        }
        sleep;
    }
}

main()
{
    new message[128];
    GetArg("msg", message, 128);
    new yes_label[8];
    GetArg("yes", yes_label, 8);
    new no_label[8];
    GetArg("no", no_label, 8);
    select = clamp(select, ANSWER_YES, ANSWER_NO);
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
    g_objButtons[ANSWER_YES] = create_button(Self(), -64.0, 78.0);
    SetTexture(g_objButtons[ANSWER_YES], imgGetResource(RES_BUTTON0), 0, -1);
    g_objButtonLabels[ANSWER_YES] = create_button_label(Self(), -64.0, 78.0);
    SetColor(g_objButtonLabels[ANSWER_YES], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
    SetOptionS(g_objButtonLabels[ANSWER_YES], TAG("UTF8"), yes_label);
    SetOptionI(g_objButtonLabels[ANSWER_YES], TAG("ALPH"), BLEND_ADD);
    g_objButtons[ANSWER_NO] = create_button(Self(), 64.0, 78.0);
    SetTexture(g_objButtons[ANSWER_NO], imgGetResource(RES_BUTTON0), 0, -1);
    g_objButtonLabels[ANSWER_NO] = create_button_label(Self(), 64.0, 78.0);
    SetColor(g_objButtonLabels[ANSWER_NO], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
    SetOptionS(g_objButtonLabels[ANSWER_NO], TAG("UTF8"), no_label);
    SetOptionI(g_objButtonLabels[ANSWER_NO], TAG("ALPH"), BLEND_ADD);
    g_objButtonLit = create_button(Self(), 0.0, 78.0);
    SetTexture(g_objButtonLit, imgGetResource(RES_BUTTON1), 0, -1);
    Hide(g_objButtonLit);
    {
        new Float:button_w;
        new Float:button_h;
        new Float:button_x;
        FitButtonsToLabel(0, button_w, button_h);
        FitButtonsToLabel(1, button_w, button_h);
        button_x = -12.0 - 0.5 * button_w;
        SetPos(g_objButtons[ANSWER_YES], button_x, 78.0, 0.0, 0);
        SetPos(g_objButtonLabels[ANSWER_YES], button_x, 78.0, 0.0, 0);
        SetScale(g_objButtons[ANSWER_YES], button_w, button_h, 1.0);
        button_x = 12.0 + 0.5 * button_w;
        SetPos(g_objButtons[ANSWER_NO], button_x, 78.0, 0.0, 0);
        SetPos(g_objButtonLabels[ANSWER_NO], button_x, 78.0, 0.0, 0);
        SetScale(g_objButtons[ANSWER_NO], button_w, button_h, 1.0);
        SetScale(g_objButtonLit, button_w, button_h, 1.0);
    }
    sleep;
    SendMessage(manager, TAG("DGWT"), 0, 0, 0, 0);
    sndSePlay(SE_DIALOG_OPEN);
    for (; ; )
    {
        new msg;
        new what;
        new Float:duration;
        /* From the manager: "FADE" IN/OUT with the fade time in ms, "FOCS" IN/OUT. */
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
                    send_focus(focus, g_objButtons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
                else if (what == TAG("OUT"))
                {
                    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
        }
        if (dlg_enable)
        {
            {
                new pressed;
                PadState(_, pressed, _, _, -1);
                if (pressed & PAD_RIGHT && !select)
                {
                    sndSePlay(SE_CURSOR);
                    select++;
                    send_focus(focus, g_objButtons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    FadeWindow(1.0, 100.0);
                }
                else if (pressed & PAD_LEFT && select)
                {
                    sndSePlay(SE_CURSOR);
                    select--;
                    send_focus(focus, g_objButtons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    FadeWindow(1.0, 100.0);
                }
                else if (pressed & PAD_CIRCLE && !is_animating(focus))
                {
                    sndSePlay(SE_DECIDE);
                    {
                        new Float:lit_x;
                        new Float:lit_y;
                        GetPos(g_objButtons[select], lit_x, lit_y, _, 0);
                        SetPos(g_objButtonLit, lit_x, lit_y, 0.0, 0);
                        Show(g_objButtonLit);
                        AnimColor(g_objButtons[select], 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_objButtonLit, 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(100);
                        AnimColor(g_objButtons[select], 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_objButtonLit, 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        wait_ms(100);
                        Hide(g_objButtonLit);
                    }
                    dlg_active = 0;
                }
                else if (pressed & PAD_CROSS && !is_animating(focus))
                {
                    sndSePlay(SE_CANCEL);
                    {
                        new Float:lit_x;
                        new Float:lit_y;
                        GetPos(g_objButtons[select], lit_x, lit_y, _, 0);
                        SetPos(g_objButtonLit, lit_x, lit_y, 0.0, 0);
                        Show(g_objButtonLit);
                        AnimColor(g_objButtons[select], 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_objButtonLit, 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(100);
                        AnimColor(g_objButtons[select], 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_objButtonLit, 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        wait_ms(100);
                        Hide(g_objButtonLit);
                    }
                    select = ANSWER_CANCEL;
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

/* Fades the window and the buttons to alpha over duration ms, the label of the selected button in the text color. */
FadeWindow(Float:alpha, Float:duration)
{
    AnimColor(g_objWindow, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (select)
    {
        AnimColor(g_objButtonLabels[ANSWER_YES], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, alpha, duration, 0.0,
                ANIM_FUNC_LINEAR);
        AnimColor(g_objButtonLabels[ANSWER_NO], g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, 0.0,
                ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_objButtonLabels[ANSWER_YES], g_flTextRed, g_flTextGreen, g_flTextBlue, alpha, duration, 0.0,
                ANIM_FUNC_LINEAR);
        AnimColor(g_objButtonLabels[ANSWER_NO], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, alpha, duration, 0.0,
                ANIM_FUNC_LINEAR);
    }
    AnimColor(g_objButtons[ANSWER_YES], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_objButtons[ANSWER_NO], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

ShowPadHelp()
{
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
}
