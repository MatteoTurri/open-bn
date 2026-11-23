/*
 * /opt0/bn/script/music/progress_window.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A window with up to three lines of message (arguments "msg", "msg2", "msg3") and a progress bar, optionally a
 * percent text (argument "percent", a format of progress and progress2) and a cancel with the cross button
 * (argument "cancel"). The script ends when progress reaches 100 or the cross is pressed.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* The pad help of set_pad_help. */
const HELP_CANCEL = 0; /* the cross cancels */
const HELP_BACK = 1;   /* the cross goes back */
const HELP_NONE = 2;

#include "wait.inc"

#include "tag.inc"

#include <float>

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr2(tag[], id[], attr[], value[], maxlen)
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

public manager; /* unused, but part of the layout */
public focus; /* the focus object */
public progress; /* percent done, written by the caller; below 0 while it has not started */
public progress2; /* second value of the percent format */
public Float:width = 400.0; /* the width of the window */
public Float:height = 260.0; /* the height of the window */
public Float:red; /* the red of the color of the window */
public Float:green = 0.2353; /* the green of the color of the window */
public Float:blue = 0.3922; /* the blue of the color of the window */
new g_defocus;
new g_base;
new g_window;
new g_glow;
new g_progress_bar;
public Float:border_left; /* the window extends this much to the left */
new g_message;
new g_message2;
new g_message3;
new g_percent;
new g_has_percent = 1;
new g_has_cancel;
new g_break_flag; /* the cross does not end the script, it sets ProgressCancelFlag in the aval file */

#include "string.inc"

main()
{
    new message[64];
    GetArg("msg", message, 64);
    new cancel[8];
    GetArg("cancel", cancel, 8);
    new percent_format[8];
    GetArg("percent", percent_format, 8);
    new break_flag[8];
    GetArg("breakflag", break_flag, 8);
    new message2[64];
    GetArg("msg2", message2, 64);
    new message3[64];
    GetArg("msg3", message3, 64);
    if (strcmp(cancel, ""))
    {
        g_has_cancel = 1;
    }
    if (!strcmp(percent_format, ""))
    {
        g_has_percent = 0;
    }
    if (strcmp(break_flag, ""))
    {
        g_break_flag = 1;
    }
    g_defocus = CreateDefocus(Self());
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_defocus, TAG("ZOFS"), -3.0);
    g_base = CreatePlane(g_defocus);
    SetLocalScale(g_base, 2.0 * width, 2.0 * height, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane3x3(g_base);
    SetScale(g_window, width + border_left, height, 1.0);
    SetPos(g_window, 0.5 * -border_left, 0.0, 0.0, 0);
    SetColor(g_window, red, green, blue, 0.0);
    SetTexture(g_window, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_window, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / width;
        margins[2] = margins[3] = 0.5 * 128.0 / height;
        SetOptionP(g_window, TAG("MGN"), margins, 4);
    }
    g_glow = CreatePlane(g_window);
    SetPos(g_glow, 0.3, -0.2, 0.0, 0);
    SetScale(g_glow, 2.0, 2.0, 1.0);
    SetColor(g_glow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_glow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_glow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_glow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    g_progress_bar = CreateNull(g_defocus);
    SetOptionI(g_progress_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_progress_bar, _, "progress_bar");
    SetPubVar(g_progress_bar, "manager", Self());
    SetPubVar(g_progress_bar, "progress", 0);
    g_message = create_button_label(g_defocus, 0.0, -70.0);
    SetColor(g_message, 1.0, 1.0, 1.0, 0.0);
    SetOptionS(g_message, TAG("UTF8"), message);
    g_message2 = create_button_label(g_defocus, 0.0, -50.0);
    SetColor(g_message2, 1.0, 1.0, 1.0, 0.0);
    SetOptionS(g_message2, TAG("UTF8"), message2);
    g_message3 = create_button_label(g_defocus, 0.0, -30.0);
    SetColor(g_message3, 1.0, 1.0, 1.0, 0.0);
    SetOptionS(g_message3, TAG("UTF8"), message3);
    if (g_has_percent)
    {
        new percent_text[8];
        sprintf(percent_text, percent_format, 0, 0);
        g_percent = create_button_label(g_defocus, 0.0, 30.0);
        SetColor(g_percent, 1.0, 1.0, 1.0, 0.0);
        SetOptionS(g_percent, TAG("UTF8"), percent_text);
    }
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    if (g_has_cancel == 1)
    {
        set_pad_help(HELP_CANCEL);
    }
    else
    {
        set_pad_help(HELP_NONE);
    }
    new cancelled;
    avalSet("ProgressCancelFlag", "0");
    new new_message[64];
    new msg;
    new Float:alpha;
    new Float:duration;
    progress = -1;
    for (; ; )
    {
        /* "FADE" (alpha, duration): fades the window; "MESG" (string): a message in place of the three lines. */
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                printf("============= FADE\n");
                fade_window(alpha, duration);
                AnimColor(g_defocus, 0.35, 0.35, 0.35, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(300);
            }
        }
        if (g_has_cancel == 1)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (FindMessageStr(TAG("MESG"), new_message, 64))
            {
                SetOptionS(g_message, TAG("UTF8"), "    ");
                SetOptionS(g_message2, TAG("UTF8"), "    ");
                SetOptionS(g_message3, TAG("UTF8"), new_message);
            }
            if (pressed & PAD_CROSS)
            {
                if (!g_break_flag)
                {
                    sndSePlay(SE_DECIDE);
                    SetPubVar(g_progress_bar, "progress", progress);
                    break;
                }
                else if (!cancelled && progress >= 0)
                {
                    sndSePlay(SE_DECIDE);
                    printf("============= CANCEL\n");
                    cancelled = 1;
                    avalSet("ProgressCancelFlag", "1");
                }
            }
        }
        /* The glow on the window mirrors the position of the focus and follows its brightness. */
        if (focus)
        {
            new Float:focus_pos[3];
            GetPos(focus, focus_pos[0], focus_pos[1], focus_pos[2], g_window);
            SetPos(g_glow, -focus_pos[0], focus_pos[1], 0.0, 0);
            new Float:focus_alpha;
            GetColor(focus, _, _, _, focus_alpha);
            AnimColor(g_glow, 0.5 * focus_alpha, 0.5 * focus_alpha, 0.5 * focus_alpha, 1.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
        }
        if (g_has_percent)
        {
            new percent_text[8];
            if (progress < 0)
            {
                sprintf(percent_text, percent_format, 0, progress2);
            }
            else
            {
                sprintf(percent_text, percent_format, progress, progress2);
            }
            SetOptionS(g_percent, TAG("UTF8"), percent_text);
        }
        if (progress < 0)
        {
            SetPubVar(g_progress_bar, "progress", 0);
        }
        else
        {
            SetPubVar(g_progress_bar, "progress", progress);
        }
        if (progress >= 100)
        {
            break;
        }
        sleep;
    }
    set_pad_help(HELP_BACK);
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    Destroy(g_progress_bar);
    fade_window(0.0, 300.0);
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    return;
}

/* Fades the window, the messages and the percent text to alpha over duration ms. */
fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_message, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_message2, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_message3, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (g_has_percent)
    {
        AnimColor(g_percent, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Shows no pad help for HELP_NONE, else the cross labeled cancel for HELP_CANCEL or back. */
set_pad_help(mode)
{
    new label[64];
    if (mode == HELP_NONE)
    {
        SetSystemParam(SYS_PADHELP, 0);
    }
    else
    {
        if (HELP_CANCEL == mode)
        {
            get_tag_attr2("TEXT", "msgmusic_common_cancel", "value", label, 64);
        }
        else
        {
            get_tag_attr2("TEXT", "msgmusic_btn_back", "value", label, 64);
        }
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
}
