/*
 * /opt0/bn/script/parts/focus_box.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The captions of a browser page: up to three header lines with their icons at the top left, a footer at the
 * bottom right, a video icon and a scrolling image, each set, colored and faded by messages from the page.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "text.inc"

#include "imgdec.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width6;
new Float:g_screen_height6;
new Float:g_screen_depth6;

/* Same as get_screen_size of text.inc, with globals of its own. */
get_screen_size6(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width6, g_screen_height6, g_screen_depth6);
    screen_w = g_screen_width6;
    screen_h = g_screen_height6;
}

#include "dom.inc"

/* Same as send_label_focus of sysconf.inc. */
send_label_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG("LABL"), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public Float:line_alpha = 1.0; /* set by "FAD0"; the box does not use it */
public Float:head_alpha = 1.0; /* of the head captions, set by "FAD1" */
public Float:foot_alpha = 1.0; /* also of the video icon and the image */
public Float:icon_alpha = 1.0; /* of the icons, set by "FAD3" */
public focus; /* the focus frame */
new Float:g_tint_r = 1.0; /* multiplies the colors below */
new Float:g_tint_g = 1.0;
new Float:g_tint_b = 1.0;
new Float:g_head_r = 0.7843;
new Float:g_head_g = 0.7843;
new Float:g_head_b = 0.7843;
new Float:g_foot_r = 0.7843;
new Float:g_foot_g = 0.7843;
new Float:g_foot_b = 0.7843;
new Float:g_icon_r = 1.0;
new Float:g_icon_g = 1.0;
new Float:g_icon_b = 1.0;
new g_heads[3];
new g_icons[3];
new g_foot;
new Float:g_screen_w;
new Float:g_screen_h;
new g_video_icon;
new g_image;
new g_video_icon_len; /* length of the url of the video icon, 0 for none */
new g_image_len;      /* length of the name of the image, 0 for none */
new Float:g_head_pos[3][3];
new g_head_font_sizes[3] = {15, 15, 15};

main()
{
    get_screen_size6(g_screen_w, g_screen_h);
    for (new i; i < 3; i++)
    {
        g_head_pos[i][0] = -21.0 + 0.3593 * -g_screen_w / 2;
        g_head_pos[i][1] = -6.0 + 0.5133 * -g_screen_h / 2 + 25.0 * i;
        g_head_pos[i][2] = 0.0;
    }
    apply_colors(100.0, 0.0);
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        new Float:c;
        new Float:d;
        new str[256];
        /*
         * "FADE", "FAD1" to "FAD3" (alpha); "FAD0" (alpha, duration); "FCOL", "FCL1" to "FCL3" (r, g, b, duration);
         * "FOCS" (header index); "FOUT"; "OFS0" to "OFS2" (dx, dy, dz, duration); "FSZ0" to "FSZ2" (font size)
         */
        while (RecvMessage(msg, a, b, c, d))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = a;
            }
            else if (msg == TAG("FCOL"))
            {
                g_tint_r = a;
                g_tint_g = b;
                g_tint_b = c;
                apply_colors(d, 0.0);
            }
            else if (msg == TAG("FAD0"))
            {
                line_alpha = a;
                apply_colors(b, 0.0);
            }
            else if (msg == TAG("FAD1"))
            {
                head_alpha = a;
            }
            else if (msg == TAG("FCL1"))
            {
                g_head_r = a;
                g_head_g = b;
                g_head_b = c;
                apply_colors(d, 0.0);
            }
            else if (msg == TAG("FAD2"))
            {
                foot_alpha = a;
            }
            else if (msg == TAG("FCL2"))
            {
                g_foot_r = a;
                g_foot_g = b;
                g_foot_b = c;
                apply_colors(d, 0.0);
            }
            else if (msg == TAG("FAD3"))
            {
                icon_alpha = a;
            }
            else if (msg == TAG("FCL3"))
            {
                g_icon_r = a;
                g_icon_g = b;
                g_icon_b = c;
                apply_colors(d, 0.0);
            }
            else if (msg == TAG("FOCS"))
            {
                if (focus)
                {
                    send_label_focus(focus, g_heads[a], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else if (msg == TAG("FOUT"))
            {
                if (focus)
                {
                    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else if (msg == TAG("OFS0"))
            {
                move_head(0, a, b, c, d);
            }
            else if (msg == TAG("OFS1"))
            {
                move_head(1, a, b, c, d);
            }
            else if (msg == TAG("OFS2"))
            {
                move_head(2, a, b, c, d);
            }
            else if (msg == TAG("FSZ0"))
            {
                set_head_font_size(0, a);
            }
            else if (msg == TAG("FSZ1"))
            {
                set_head_font_size(1, a);
            }
            else if (msg == TAG("FSZ2"))
            {
                set_head_font_size(2, a);
            }
        }
        /* "HED0" to "HED2" (text), "FOOT" (text), "ICN0" to "ICN2" (url), "VICN" (url), "CPYI" (image name) */
        while (RecvMessageStr(msg, str, 256))
        {
            if (msg == TAG("HED0"))
            {
                set_head(0, str);
            }
            else if (msg == TAG("HED1"))
            {
                set_head(1, str);
            }
            else if (msg == TAG("HED2"))
            {
                set_head(2, str);
            }
            else if (msg == TAG("FOOT"))
            {
                set_foot(str);
            }
            if (msg == TAG("ICN0"))
            {
                set_icon(0, str);
            }
            if (msg == TAG("ICN1"))
            {
                set_icon(1, str);
            }
            if (msg == TAG("ICN2"))
            {
                set_icon(2, str);
            }
            if (msg == TAG("VICN"))
            {
                set_video_icon(str);
            }
            if (msg == TAG("CPYI"))
            {
                set_image(str);
            }
        }
        sleep;
    }
}

/* Sets the text of header line index, squeezed into 233 pixels (down to 60%, cut beyond). */
set_head(index, text[])
{
    index = clamp(index, 0, 2);
    if (!g_heads[index])
    {
        g_heads[index] = CreateLabelShadow(Self());
        SetOptionP(g_heads[index], TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_heads[index], TAG("SX64"), 2 * 64);
        SetOptionI(g_heads[index], TAG("SY64"), 2 * 64);
        SetPos(g_heads[index], g_head_pos[index][0], g_head_pos[index][1], g_head_pos[index][2], 0);
        SetColor(g_heads[index], 0.7843, 0.7843, 0.7843, 1.0 * 0.0);
        SetOptionI(g_heads[index], TAG("HALN"), ALIGN_LEFT);
        SetOptionI(g_heads[index], TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_heads[index], TAG("FSIZ"), g_head_font_sizes[index]);
        SetOptionI(g_heads[index], TAG("ALPH"), BLEND_ADD);
    }
    SetOptionS(g_heads[index], TAG("UTF8"), text);
    {
        new Float:color[4];
        GetColor(g_heads[index], color[0], color[1], color[2], color[3]);
        SetColor(g_heads[index], color[0], color[1], color[2], 0.0);
    }
    new rect[5];
    for (; ; )
    {
        GetOptionP(g_heads[index], TAG("RECT"), rect, 5);
        if (rect[0])
        {
            break;
        }
        sleep;
        continue;
    }
    if (float(rect[3] >> 6) > 233.0)
    {
        if (0.6 * float(rect[3] >> 6) >= 233.0)
        {
            SetOptionI(g_heads[index], TAG("W64 "), floatround(233.0 * 64.0 / 0.6, 0));
            SetLocalScale(g_heads[index], 0.6, 1.0, 1.0);
        }
        else
        {
            SetLocalScale(g_heads[index], 233.0 / float(rect[3] >> 6), 1.0, 1.0);
        }
        rect[3] = floatround(233.0 * 64.0, 0);
    }
    else
    {
        SetLocalScale(g_heads[index], 1.0, 1.0, 1.0);
    }
    apply_colors(100.0, 0.0);
}

set_foot(text[])
{
    if (!g_foot)
    {
        g_foot = CreateLabelShadow(Self());
        SetOptionP(g_foot, TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(g_foot, TAG("SX64"), 2 * 64);
        SetOptionI(g_foot, TAG("SY64"), 2 * 64);
        SetPos(g_foot, 0.3593 * g_screen_w / 2, 16.0 + 0.5133 * g_screen_h / 2, 0.0, 0);
        SetColor(g_foot, 0.7843, 0.7843, 0.7843, 1.0 * 0.0);
        SetOptionI(g_foot, TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_foot, TAG("FSIZ"), 12);
        SetOptionI(g_foot, TAG("ALPH"), BLEND_ADD);
    }
    SetOptionS(g_foot, TAG("UTF8"), text);
    apply_colors(100.0, 0.0);
}

/* Replaces the icon of header line index with the image at url, fading out the old one. */
set_icon(index, url[])
{
    if (g_icons[index])
    {
        AnimColor(g_icons[index], 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
        TimerDie(g_icons[index], ms_to_frames(1000));
    }
    g_icons[index] = CreatePlane(Self());
    SetPos(g_icons[index], g_head_pos[index][0] + (-35.0 - -21.0), g_head_pos[index][1], g_head_pos[index][2], 0);
    SetColor(g_icons[index], 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_icons[index], load_image(url, 0, 0, -1), 0, -1);
    SetLocalScale(g_icons[index], float(imgTexWidth()), float(imgTexHeight()), 1.0);
    apply_colors(100.0, 0.0);
}

/* Animates every caption to its color times the tint and to its alpha times meta_alpha. */
apply_colors(Float:duration, Float:delay)
{
    for (new i; i < 3; i++)
    {
        if (g_heads[i])
        {
            AnimColor(g_heads[i], g_head_r * g_tint_r, g_head_g * g_tint_g, g_head_b * g_tint_b,
                    meta_alpha * head_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
        if (g_icons[i])
        {
            AnimColor(g_icons[i], g_icon_r * g_tint_r, g_icon_g * g_tint_g, g_icon_b * g_tint_b,
                    0.8 * (meta_alpha * icon_alpha), duration, delay, ANIM_FUNC_LINEAR);
        }
    }
    if (g_foot)
    {
        AnimColor(g_foot, g_foot_r * g_tint_r, g_foot_g * g_tint_g, g_foot_b * g_tint_b, meta_alpha * foot_alpha,
                duration, delay, ANIM_FUNC_LINEAR);
    }
    if (g_video_icon_len > 0)
    {
        if (g_video_icon)
        {
            AnimColor(g_video_icon, g_foot_r * g_tint_r, g_foot_g * g_tint_g, g_foot_b * g_tint_b,
                    meta_alpha * foot_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
    }
    else if (g_video_icon)
    {
        AnimColor(g_video_icon, 0.0, 0.0, 0.0, 0.0, duration, delay, ANIM_FUNC_LINEAR);
    }
    if (g_image_len > 0)
    {
        if (g_image)
        {
            SendMessage(g_image, TAG("FADE"), meta_alpha * foot_alpha, duration, 0, 0);
        }
    }
    else if (g_image)
    {
        SendMessage(g_image, TAG("FADE"), 0, duration, 0, 0);
    }
}

/* Same as has_extension of string.inc. */
has_extension(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}

/* Decodes an image (a .tm2 at its own size) and returns its texture, 0 on failure. */
load_image(url[], tex_w, tex_h, dec_mode)
{
    if (0 == strlen(url))
    {
        return 0;
    }
    new path[256];
    sprintf(path, url);
    if (has_extension(path, "tm2"))
    {
        start_image_decode(path);
        new status = finish_image_decode();
        if (IMG_DONE != status)
        {
            return 0;
        }
    }
    else
    {
        imgDecStart(path, tex_w, tex_h, dec_mode, 100, 0);
        new status = wait_image_decode();
        if (IMG_DONE != status)
        {
            return 0;
        }
    }
    return imgTexID();
}

/* Replaces the video icon with the image at url; an empty url only hides it. */
set_video_icon(url[])
{
    g_video_icon_len = strlen(url);
    if (g_video_icon_len < 1)
    {
        apply_colors(100.0, 0.0);
        return;
    }
    if (g_video_icon)
    {
        AnimColor(g_video_icon, 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
        TimerDie(g_video_icon, ms_to_frames(1000));
        g_video_icon = 0;
    }
    g_video_icon = CreatePlane(Self());
    SetPos(g_video_icon, 8.0 + (-35.0 + g_screen_w * (-0.3593 / 2)), 4.0 + (224.0 + g_screen_h * (-0.5133 / 2)), 0.0,
            0);
    SetColor(g_video_icon, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_video_icon, load_image(url, 0, 0, -1), 0, -1);
    SetLocalScale(g_video_icon, float(imgTexWidth()), float(imgTexHeight()), 1.0);
    apply_colors(100.0, 0.0);
}

/* Replaces the scrolling image (a scroll_image object) with image; an empty name removes it. */
set_image(image[])
{
    g_image_len = strlen(image);
    if (g_image_len < 1)
    {
        if (g_image)
        {
            AnimColor(g_image, 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            TimerDie(g_image, ms_to_frames(1000));
            g_image = 0;
        }
        return;
    }
    if (g_image)
    {
        AnimColor(g_image, 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
        TimerDie(g_image, ms_to_frames(1000));
    }
    g_image = CreateNull(Self());
    SetPos(g_image, 96.0 + g_screen_w * (-0.3593 / 2), 2.0 + (272.0 + g_screen_h * (-0.5133 / 2)), 0.0, 0);
    {
        new args[32];
        sprintf(args, "image = %s", image);
        AttachScript(g_image, _, "scroll_image", args);
    }
    apply_colors(100.0, 0.0);
}

/* Moves header line index and its icon by dx, dy, dz from their place. */
move_head(index, Float:dx, Float:dy, Float:dz, Float:duration)
{
    g_head_pos[index][0] = -21.0 + 0.3593 * -g_screen_w / 2 + dx;
    g_head_pos[index][1] = -6.0 + 0.5133 * -g_screen_h / 2 + 25.0 * index + dy;
    g_head_pos[index][2] = 0.0 + dz;
    if (g_heads[index])
    {
        AnimMoveLine(g_heads[index], g_head_pos[index][0], g_head_pos[index][1], g_head_pos[index][2], duration, 0.0,
                ANIM_FUNC_LINEAR);
    }
    if (g_icons[index])
    {
        AnimMoveLine(g_icons[index], g_head_pos[index][0] + (-35.0 - -21.0), g_head_pos[index][1], g_head_pos[index][2],
                duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

set_head_font_size(index, font_size)
{
    g_head_font_sizes[index] = font_size;
    if (g_heads[index])
    {
        SetOptionI(g_heads[index], TAG("FSIZ"), font_size);
    }
}
