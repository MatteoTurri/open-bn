/*
 * /opt0/bn/script/utility/browser_confirm_d.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A message box of the file browser closed with the cross button (argument "msg"). With "mcchk" or "mcchk2" it
 * also watches a memory card ("mcport", "mcslot", "mctype" and the same with 2) and flags it when it is removed.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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

#include "imgdec.inc"

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

#include "misc.inc"

#include "string.inc"

new g_card_types[4]; /* the type of the card in each port, as last seen */
public device_mode; /* unused, but part of the layout */
public mc_port; /* unused, but part of the layout */
public mc_slot; /* unused, but part of the layout */
public manager; /* unused, but part of the layout */
public focus; /* the focus object, told that nothing is focused */
public mc_eject_flag;  /* the card of mcport was removed or changed */
public mc_eject_flag2; /* the card of mcport2 was removed or changed */
public Float:width = 400.0; /* of the window, in pixels */
public Float:height = 260.0; /* of the window, in pixels */
public Float:red; /* color of the window */
public Float:green = 0.2353; /* color of the window */
public Float:blue = 0.3922; /* color of the window */
new g_defocus;
new g_base;
new g_window;
new g_glow;
public option_mode; /* the look of the browser: alpha blending, its own window texture, a brighter background */
public alpha_blend = BLEND_ADD; /* blend mode of the window and the message */
public window_texture; /* the texture of the window */
public Float:bg_brightness = 0.35; /* color of the defocused background */
new g_message;
new g_message_shown;

/* Like create_message_text of misc.inc, with the blend mode alpha_blend. */
create_message_text_blend(parent, text_w, text_h)
{
    new textbox = CreateTextbox(parent);
    SetOptionI(textbox, TAG("ALPH"), alpha_blend);
    SetOptionI(textbox, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(textbox, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(textbox, TAG("FSIZ"), 15);
    SetOptionI(textbox, TAG("TW64"), text_w << 6);
    SetOptionI(textbox, TAG("TH64"), text_h << 6);
    SetOptionI(textbox, TAG("LS64"), 4 * 64);
    return textbox;
}

main()
{
    new message[64];
    GetArg("msg", message, 64);
    new ok_label[8]; /* read, but the box has no button */
    if (!GetArg("ok", ok_label, 8))
    {
        copy_string(ok_label, "OK", 8);
    }
    mc_eject_flag = 0;
    mc_eject_flag2 = 0;
    new check_card;
    new check_card2;
    new check_mode = 1; /* argument "mcchkmode": 1 closes the box when a card is removed */
    new port;
    new slot;
    new port2;
    new slot2;
    new value[8];
    if (GetArg("mcchk", value, 8))
    {
        check_card = atoi(value);
    }
    else
    {
        check_card = 0;
    }
    if (GetArg("mcchk2", value, 8))
    {
        check_card2 = atoi(value);
    }
    else
    {
        check_card2 = 0;
    }
    if (check_card)
    {
        if (GetArg("mcport", value, 8))
        {
            port = atoi(value);
        }
        else
        {
            check_card = 0;
        }
        if (GetArg("mcslot", value, 8))
        {
            slot = atoi(value);
        }
        else
        {
            check_card = 0;
        }
        if (GetArg("mctype", value, 8))
        {
            g_card_types[port] = atoi(value);
        }
        else
        {
            check_card = 0;
        }
        if (GetArg("mcchkmode", value, 8))
        {
            check_mode = atoi(value);
        }
        else
        {
            check_mode = 1;
        }
    }
    if (check_card2)
    {
        if (GetArg("mcport2", value, 8))
        {
            port2 = atoi(value);
        }
        else
        {
            check_card2 = 0;
        }
        if (GetArg("mcslot2", value, 8))
        {
            slot2 = atoi(value);
        }
        else
        {
            check_card2 = 0;
        }
        if (GetArg("mctype2", value, 8))
        {
            g_card_types[port2] = atoi(value);
        }
        else
        {
            check_card2 = 0;
        }
        if (GetArg("mcchkmode", value, 8))
        {
            check_mode = atoi(value);
        }
        else
        {
            check_mode = 1;
        }
    }
    window_texture = imgGetResource(RES_WINDOW1);
    if (option_mode)
    {
        alpha_blend = BLEND_ALPHA;
        window_texture = load_kept_texture("file:/opt0/bn/script/utility/texture/window1.tm2");
        bg_brightness = 0.9;
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
    SetScale(g_window, width, height, 1.0);
    SetColor(g_window, red, green, blue, 0.0);
    SetTexture(g_window, window_texture, 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), alpha_blend);
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
    g_message = create_message_text_blend(g_defocus, floatround(width - 64.0, 0), floatround(height - 48.0, 0));
    SetOptionS(g_message, TAG("UTF8"), message);
    SetColor(g_message, 0.7843, 0.7843, 0.7843, 0.0);
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, bg_brightness, bg_brightness, bg_brightness, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    show_pad_help();
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    new frames;
    for (; ; )
    {
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_CROSS)
            {
                sndSePlay(SE_CANCEL);
                break;
            }
        }
        /* Centers the message once the textbox has laid it out, then fades it in. */
        if (!g_message_shown)
        {
            new rect[5];
            GetOptionP(g_message, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                g_message_shown = 1;
                SetPos(g_message, 0.5 * -float(rect[3] >> 6), 0.5 * -float(rect[4] >> 6), 0.0, 0);
                AnimColor(g_message, 0.7843, 0.7843, 0.7843, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        /* Every 30 frames: a card that is gone, of another type or in error (-1, -2) is flagged. */
        if (check_card || check_card2)
        {
            if (frames > 30)
            {
                new unused; /* unused, but part of the layout */
                update_card_info(unused, 0, 0, 0);
                new type;
                new free_blocks;
                new formatted;
                new status;
                if (check_card)
                {
                    get_card_info(port, slot, type, free_blocks, formatted, status);
                    if (g_card_types[port] != type || 0 == type || status == MC_CHANGED
                            || status == MC_CHANGED_UNFORMATTED)
                    {
                        mc_eject_flag = 1;
                        if (check_mode == 1)
                        {
                            break;
                        }
                    }
                    g_card_types[port] = type;
                }
                if (check_card2)
                {
                    get_card_info(port2, slot2, type, free_blocks, formatted, status);
                    if (g_card_types[port2] != type || 0 == type || status == MC_CHANGED
                            || status == MC_CHANGED_UNFORMATTED)
                    {
                        mc_eject_flag2 = 1;
                        if (check_mode == 1)
                        {
                            break;
                        }
                    }
                    g_card_types[port2] = type;
                }
                frames = 0;
            }
            else
            {
                frames++;
            }
        }
        sleep;
    }
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    fade_window(0.0, 300.0);
    AnimColor(g_message, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    return;
}

/* Fades the window to alpha over duration ms. */
fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Shows the cross in the pad help. */
show_pad_help()
{
    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
}

/* Decodes the image url and keeps its texture; returns it, 0 on failure. */
load_kept_texture(url[])
{
    start_image_decode2(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}
