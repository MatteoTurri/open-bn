/*
 * /opt0/bn/script/photo/progress_window.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The progress window of photo: up to three lines of message (arguments "msg", "msg2", "msg3"), a progress bar
 * and optionally a percent text ("percent", a format of progress and progress2). With "cancel" the cross button
 * cancels: it closes the window, or with "breakflag" sets the window variable ProgressCancelFlag for the caller.
 */

#include <core>
#include <bn>

#pragma dynamic 512

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

/* What show_cross_help shows on the cross button. */
enum
{
    CROSS_CANCEL,
    CROSS_BACK,
    CROSS_NONE
}

public manager;   /* unused, but part of the layout */
public focus;     /* the focus object, taken away when the window closes */
public progress;  /* percent done, written by the caller */
public progress2; /* second value of the percent format */
new Float:g_width = 400.0;
new Float:g_height = 260.0;
new Float:g_red;
new Float:g_green = 0.2353;
new Float:g_blue = 0.3922;
new g_defocus;
new g_base;
new g_window;
new g_glow;
new g_progress_bar;
public Float:border_left;
new g_messages[3];
new g_percent;
new g_has_percent = 1;
new g_has_cancel;     /* argument "cancel": the cross button cancels */
new g_cancel_by_flag; /* argument "breakflag": a cancel sets ProgressCancelFlag instead of closing */

#include "string.inc"

main()
{
    new msg_count;
    new messages[3][64];
    if (GetArg("msg", messages[msg_count], 64))
    {
        msg_count++;
    }
    if (GetArg("msg2", messages[msg_count], 64))
    {
        msg_count++;
    }
    if (GetArg("msg3", messages[msg_count], 64))
    {
        msg_count++;
    }
    new percent_format[8];
    GetArg("percent", percent_format, 8);
    {
        new cancel_arg[8];
        GetArg("cancel", cancel_arg, 8);
        new breakflag_arg[8];
        GetArg("breakflag", breakflag_arg, 8);
        if (strcmp(cancel_arg, ""))
        {
            g_has_cancel = 1;
        }
        if (!strcmp(percent_format, ""))
        {
            g_has_percent = 0;
        }
        if (strcmp(breakflag_arg, ""))
        {
            g_cancel_by_flag = 1;
        }
    }
    g_defocus = CreateDefocus(Self());
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_defocus, TAG("ZOFS"), -3.0);
    g_base = CreatePlane(g_defocus);
    SetLocalScale(g_base, 2.0 * g_width, 2.0 * g_height, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane3x3(g_base);
    SetScale(g_window, g_width + border_left, g_height, 1.0);
    SetPos(g_window, 0.5 * -border_left, 0.0, 0.0, 0);
    SetColor(g_window, g_red, g_green, g_blue, 0.0);
    SetTexture(g_window, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_window, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / g_width;
        margins[2] = margins[3] = 0.5 * 128.0 / g_height;
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
    {
        new Float:y = -30.0;
        for (new i = msg_count - 1; i >= 0; i--)
        {
            g_messages[i] = create_button_label(g_defocus, 0.0, y);
            SetColor(g_messages[i], 0.7843, 0.7843, 0.7843, 0.0);
            SetOptionS(g_messages[i], TAG("UTF8"), messages[i]);
            y = y - 20.0;
        }
        if (g_has_percent)
        {
            new percent_text[8];
            sprintf(percent_text, percent_format, 0, 0);
            g_percent = create_button_label(g_defocus, 0.0, 30.0);
            SetColor(g_percent, 1.0, 1.0, 1.0, 0.0);
            SetOptionS(g_percent, TAG("UTF8"), percent_text);
        }
    }
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    if (g_has_cancel == 1)
    {
        show_cross_help(CROSS_CANCEL);
    }
    else
    {
        show_cross_help(CROSS_NONE);
    }
    new cancelled;
    set_wnd_var_int("ProgressCancelFlag", 0);
    new msg;
    new Float:alpha;
    new Float:duration;
    for (; ; )
    {
        /* "FADE" (alpha, duration) */
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
            if (pressed & PAD_CROSS)
            {
                if (!g_cancel_by_flag)
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
                    set_wnd_var_int("ProgressCancelFlag", 1);
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
        if (progress <= 0)
        {
            SetPubVar(g_progress_bar, "progress", 2);
        }
        else
        {
            SetPubVar(g_progress_bar, "progress", progress);
        }
        if (progress >= 100 || cancelled)
        {
            break;
        }
        sleep;
    }
    wait_ms(300);
    show_cross_help(CROSS_BACK);
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
    AnimColor(g_window, g_red, g_green, g_blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (IsValidObject(g_messages[0]))
    {
        AnimColor(g_messages[0], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (IsValidObject(g_messages[1]))
    {
        AnimColor(g_messages[1], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (IsValidObject(g_messages[2]))
    {
        AnimColor(g_messages[2], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_has_percent)
    {
        AnimColor(g_percent, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Shows the cross button in the pad help with the label of help, or no pad help. */
show_cross_help(help)
{
    new label[64];
    if (help == CROSS_NONE)
    {
        SetSystemParam(SYS_PADHELP, 0);
    }
    else
    {
        if (CROSS_CANCEL == help)
        {
            get_tag_attr("TEXT", "msgphoto_common_cancel", "value", label, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgphoto_btn_back", "value", label, 64);
        }
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
}
