/*
 * /opt0/bn/script/game/notice.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The notice of a game: an image (argument "image") seen through a 416x196 window that up and down scroll, with a
 * scroll bar when the image is taller than the window.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

#include "imgsync.inc"

new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
public fadein_finished; /* set once the image is shown */
new Float:g_screen_w;
new Float:g_screen_h;
new g_image;
new g_scroll_bar;

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

public notice()
{
    new self = Self();
    get_screen_size(g_screen_w, g_screen_h);
    g_image = CreatePlane(self);
    SetColor(g_image, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_image, TAG("ZSRT"), ZSORT_OVERLAY);
    new url[256];
    GetArg("image", url, 256);
    imgDecStart(url, 0, 0, -1, 100, 1);
    if (IMG_DONE != wait_image_decode())
    {
        return -1;
    }
    new view_w = 416;
    new view_h = 196;
    SetPos(g_image, 0.5 * g_screen_w, 0.5 * g_screen_h - 32.0, 0.0, 0);
    SetLocalScale(g_image, float(view_w), float(view_h), 1.0);
    if (imgHeight() > 196)
    {
        g_scroll_bar = CreateNull(self);
        SetPos(g_scroll_bar, 96.0 + 8.0, 224.0, 0.0, 0);
        SetOptionI(g_scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(g_scroll_bar, _, "scroll_bar");
        SetPubVar(g_scroll_bar, "manager", Self());
        SetPubVar(g_scroll_bar, "height", 224.0);
        SetPubVar(g_scroll_bar, "top", 0);
        SetPubVar(g_scroll_bar, "length", 0.5);
    }
    else
    {
        g_scroll_bar = 0;
    }
    new x;
    new y;
    new old_x = x;
    new old_y = y;
    show_image_part(x, y, view_w, view_h);
    fadein_finished = 1;
    for (; ; )
    {
        new held;
        new step;
        PadState(held, _, _, _, -1);
        new speed = 4;
        if (0 != held)
        {
            if (held == PAD_UP)
            {
                step = speed * -1;
            }
            if (held == PAD_DOWN)
            {
                step = speed;
            }
        }
        y = y + step / 2;
        x = clamp(x, 0, imgWidth() - view_w);
        y = clamp(y, 0, imgHeight() - view_h);
        if (x != old_x || y != old_y)
        {
            show_image_part(x, y, view_w, view_h);
            if (0 != g_scroll_bar)
            {
                SetPubVar(g_scroll_bar, "top", float(y) / (2 * float(imgHeight() - view_h)));
            }
        }
        sleep;
        old_x = x;
        old_y = y;
    }
    return 0;
}

/* Shows the w x h part of the decoded image at x, y. */
show_image_part(x, y, w, h)
{
    imgTrimStart(x, y, w, h);
    if (wait_image_trim() == IMG_DONE)
    {
        SetTexture(g_image, imgTexID(), 0, -1);
    }
    SetColor(g_image, 1.0, 1.0, 1.0, 1.0);
}
