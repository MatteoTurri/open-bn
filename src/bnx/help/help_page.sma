/*
 * /opt0/bn/script/help/help_page.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A page of the manual viewer: the image of its argument "image", scrolled with the directional buttons,
 * which slides in and out as the viewer turns the pages.
 */

#include <core>
#include <bn>

#pragma dynamic 512

/* The error code that help reports for an image that cannot be loaded. */
const ERR_IMAGE = -10010;

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

new Float:g_pi = 3.141592;

#include "text.inc"

#include "imgdec.inc"

new Float:g_screen_width6;
new Float:g_screen_height6;
new Float:g_screen_depth6;
public suicide; /* closes the page: 1, 2 sliding it out to the right, 3 to the left; or an error code */
public prepare = -1; /* 2 or 3 slide the page in from the left or the right; 0 once it is shown, or an error code */
public padhelp_lr; /* unused, but part of the layout */
public arrowStatus; /* show the scroll arrows */
new Float:g_screen_w;
new Float:g_screen_h;
new g_halves[2];
new g_arrow_up;
new g_arrow_down;
new g_arrow_left;
new g_arrow_right;
new g_arrow_tex;

/* Same as get_screen_size of text.inc, with globals of its own. */
get_screen_size6(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width6, g_screen_height6, g_screen_depth6);
    screen_w = g_screen_width6;
    screen_h = g_screen_height6;
}

public help_page()
{
    new result;
    new obj = Self();
    get_screen_size6(g_screen_w, g_screen_h);
    g_halves[0] = CreatePlane(obj);
    SetColor(g_halves[0], 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_halves[0], TAG("ZSRT"), ZSORT_OVERLAY);
    g_halves[1] = CreatePlane(obj);
    SetColor(g_halves[1], 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_halves[1], TAG("ZSRT"), ZSORT_OVERLAY);
    result = load_arrow_texture();
    if (result < 0)
    {
        prepare = ERR_IMAGE;
        goto done;
    }
    g_arrow_up = CreatePlane(obj);
    SetPos(g_arrow_up, 320.0, 60.0 + 14.0, 0.0, 0);
    SetLocalScale(g_arrow_up, 24.0, 24.0, 1.0);
    SetColor(g_arrow_up, 1.0, 1.0, 1.0, 0.0);
    SetRot(g_arrow_up, 0.0, 0.0, 0.0, 0, 0);
    SetOptionI(g_arrow_up, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_arrow_up, g_arrow_tex, 0, -1);
    g_arrow_down = CreatePlane(obj);
    SetPos(g_arrow_down, 320.0, 420.0 - 14.0 - 1.0, 0.0, 0);
    SetLocalScale(g_arrow_down, 24.0, 24.0, 1.0);
    SetColor(g_arrow_down, 1.0, 1.0, 1.0, 0.0);
    SetRot(g_arrow_down, 0.0, 0.0, g_pi, 0, 0);
    SetOptionI(g_arrow_down, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_arrow_down, g_arrow_tex, 0, -1);
    g_arrow_left = CreatePlane(obj);
    SetPos(g_arrow_left, 24.0, 240.0, 0.0, 0);
    SetLocalScale(g_arrow_left, 24.0, 24.0, 1.0);
    SetColor(g_arrow_left, 1.0, 1.0, 1.0, 0.0);
    SetRot(g_arrow_left, 0.0, 0.0, 0.5 * g_pi, 0, 0);
    SetOptionI(g_arrow_left, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_arrow_left, g_arrow_tex, 0, -1);
    g_arrow_right = CreatePlane(obj);
    SetPos(g_arrow_right, 616.0, 240.0, 0.0, 0);
    SetLocalScale(g_arrow_right, 24.0, 24.0, 1.0);
    SetColor(g_arrow_right, 1.0, 1.0, 1.0, 0.0);
    SetRot(g_arrow_right, 0.0, 0.0, 1.5 * g_pi, 0, 0);
    SetOptionI(g_arrow_right, TAG("ZSRT"), ZSORT_OVERLAY);
    SetTexture(g_arrow_right, g_arrow_tex, 0, -1);
    new url[256];
    GetArg("image", url, 256);
    printf("fullscreen image get %s\n", url);
    imgDecStart(url, 0, 0, -1, 100, 1);
    if (IMG_DONE != wait_image_decode())
    {
        prepare = ERR_IMAGE;
        goto done;
    }
    new view_w = get_smaller(imgWidth(), floatround(g_screen_w, 0));
    new view_h = get_smaller(imgHeight(), floatround(g_screen_h, 0));
    SetPos(g_halves[0], 0.5 * g_screen_w, 0.5 * g_screen_h - 0.25 * float(view_h), 0.0, 0);
    SetPos(g_halves[1], 0.5 * g_screen_w, 0.5 * g_screen_h + 0.25 * float(view_h), 0.0, 0);
    SetLocalScale(g_halves[0], float(view_w), 0.5 * float(view_h), 1.0);
    SetLocalScale(g_halves[1], float(view_w), 0.5 * float(view_h), 1.0);
    printf("fullscreen image size %d, %d\n", imgWidth(), imgHeight());
    update_arrows(0, 0);
    new x;
    new y;
    new last_x = x;
    new last_y = y;
    result = show_image_rect(x, y, view_w, view_h);
    if (result < 0)
    {
        prepare = ERR_IMAGE;
        goto done;
    }
    AnimColor(g_halves[0], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_halves[1], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    if (prepare == 2)
    {
        SetPos(Self(), 0.5 * -g_screen_w, 0.0, 0.0, 0);
        AnimMoveLine(Self(), 0.0, 0.0, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
    }
    else if (prepare == 3)
    {
        SetPos(Self(), 0.5 * g_screen_w, 0.0, 0.0, 0);
        AnimMoveLine(Self(), 0.0, 0.0, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
    }
    wait_ms(200);
    prepare = 0;
    for (; ; )
    {
        if (suicide)
        {
            new Float:duration = 200.0;
            AnimColor(g_halves[0], 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(g_halves[1], 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(g_arrow_up, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(g_arrow_down, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(g_arrow_right, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(g_arrow_left, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
            if (suicide == 2)
            {
                AnimMoveLine(Self(), 0.5 * g_screen_w, 0.0, 0.0, duration, 0.0, ANIM_FUNC_EXPONENT);
            }
            else if (suicide == 3)
            {
                AnimMoveLine(Self(), 0.5 * -g_screen_w, 0.0, 0.0, duration, 0.0, ANIM_FUNC_EXPONENT);
            }
            wait_ms(200);
            break;
        }
        new held;
        new pressed;
        new released;
        new repeated;
        new dx;
        new dy;
        PadState(held, pressed, released, repeated, -1);
        new step = 15;
        if (0 != held)
        {
            if (held == PAD_LEFT)
            {
                dx = step * -1;
            }
            if (held == PAD_RIGHT)
            {
                dx = step;
            }
            if (held == PAD_UP)
            {
                dy = step * -1;
            }
            if (held == PAD_DOWN)
            {
                dy = step;
            }
        }
        x = x + dx / 2;
        y = y + dy / 2;
        x = clamp(x, 0, imgWidth() - view_w);
        y = clamp(y, 0, imgHeight() - view_h);
        if (x != last_x || y != last_y)
        {
            result = show_image_rect(x, y, view_w, view_h);
            if (result < 0)
            {
                suicide = ERR_IMAGE;
            }
        }
        if (!arrowStatus)
        {
            hide_arrows();
        }
        else
        {
            update_arrows(x, y);
        }
        sleep;
        last_x = x;
        last_y = y;
    }
    done:
    if (g_halves[0])
    {
        Destroy(g_halves[0]);
    }
    if (g_halves[1])
    {
        Destroy(g_halves[1]);
    }
    if (g_arrow_up)
    {
        Destroy(g_arrow_up);
    }
    if (g_arrow_down)
    {
        Destroy(g_arrow_down);
    }
    if (g_arrow_left)
    {
        Destroy(g_arrow_left);
    }
    if (g_arrow_right)
    {
        Destroy(g_arrow_right);
    }
    return;
}

/* Shows the w by h rectangle of the image at (x, y) in the two halves; returns 0 or ERR_IMAGE. */
show_image_rect(x, y, w, h)
{
    imgTrimStart(x, y, w, h / 2);
    if (wait_image_trim() == IMG_DONE)
    {
        SetTexture(g_halves[0], imgTexID(), 0, -1);
    }
    else
    {
        return ERR_IMAGE;
    }
    imgTrimStart(x, y + h / 2, w, h / 2);
    if (wait_image_trim() == IMG_DONE)
    {
        SetTexture(g_halves[1], imgTexID(), 0, -1);
    }
    else
    {
        return ERR_IMAGE;
    }
    return 0;
}

get_smaller(a, b)
{
    if (a < b)
    {
        return a;
    }
    return b;
}

/* Shows the arrows of the directions the image can still be scrolled to from (x, y). */
update_arrows(x, y)
{
    if (0 == y)
    {
        AnimColor(g_arrow_up, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_arrow_up, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (y >= imgHeight() - g_screen_h)
    {
        AnimColor(g_arrow_down, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_arrow_down, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (0 == x)
    {
        AnimColor(g_arrow_left, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_arrow_left, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (x >= imgWidth() - g_screen_w)
    {
        AnimColor(g_arrow_right, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_arrow_right, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

hide_arrows()
{
    AnimColor(g_arrow_up, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_arrow_down, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_arrow_right, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_arrow_left, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, ANIM_FUNC_LINEAR);
}

load_arrow_texture()
{
    new url[64];
    get_tag_attr2("IMG", "arrow", "src", url, 64);
    start_image_decode2(url);
    new status = finish_image_decode();
    if (IMG_DONE != status)
    {
        return ERR_IMAGE;
    }
    g_arrow_tex = imgTexID();
    imgKeepTex(g_arrow_tex);
    return 0;
}
