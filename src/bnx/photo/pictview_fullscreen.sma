/*
 * /opt0/bn/script/photo/pictview_fullscreen.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A photo on the whole screen, fit to it or pixel by pixel (the "mode" argument), drawn as four strips that the left
 * stick scrolls over the image. The buttons pressed go to its manager.
 */

#include <core>
#include <bn>

#pragma dynamic 512

/* Values of the "mode" argument. */
const FIT_TO_SCREEN = 0;
const PIXEL_BY_PIXEL = 1;

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
public manager; /* gets "REDY" (1 if the image cannot be shown), "PUSH" (0, button) and "FNSH" */

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

main()
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new mode;
    {
        new arg[16];
        if (GetArg("mode", arg, 16))
        {
            if (!strcmp(arg, "PIXEL_BY_PIXEL"))
            {
                mode = PIXEL_BY_PIXEL;
            }
            else if (!strcmp(arg, "FIT_TO_SCREEN"))
            {
                mode = FIT_TO_SCREEN;
            }
        }
    }
    new strips[4];
    for (new i; i < 4; i++)
    {
        strips[i] = CreatePlane(Self());
        SetColor(strips[i], 1.0, 1.0, 1.0, 0.0);
    }
    new decoding;
    {
        new image[64];
        if (GetArg("image", image, 64))
        {
            if (strlen(image))
            {
                if (mode == PIXEL_BY_PIXEL)
                {
                    imgDecStart(image, 0, 0, -1, 100, 5);
                }
                else if (FIT_TO_SCREEN == mode)
                {
                    imgDecStart(image, floatround(screen_w, 0), floatround(screen_h, 0), -1, 100, 7);
                }
                decoding = 1;
            }
            else
            {
                SendMessage(manager, TAG("REDY"), 1, 0, 0, 0);
            }
        }
    }
    new view_w;
    new view_h;
    new x;
    new y;
    new shown_x;
    new shown_y;
    new started;
    new pushed;
    new focused;
    new faded_in;
    /*
     * "FOCS" (a: "FRST" or "FOUT") takes or leaves the focus; "STRT" fades the photo in, "FNSH" fades it out and
     * ends; after a "PUSH" the buttons wait for a "PUSH" back from the manager.
     */
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        if (RecvMessage(msg, a))
        {
            if (msg == TAG("FOCS"))
            {
                if (a == TAG("FRST"))
                {
                    focused = 1;
                }
                else if (a == TAG("FOUT"))
                {
                    focused = 0;
                }
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
            else if (msg == TAG("FNSH"))
            {
                if (started)
                {
                    for (new i; i < 4; i++)
                    {
                        AnimColor(strips[i], 1.0, 1.0, 1.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                    wait_ms(1000);
                }
                break;
            }
            else if (msg == TAG("STRT"))
            {
                if (!decoding)
                {
                    for (new i; i < 4; i++)
                    {
                        AnimColor(strips[i], 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
                    }
                    x = y = 0;
                    shown_x = shown_y = -1;
                    started = 1;
                }
            }
        }
        if (focused && !pushed)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_L1)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("L1  "), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_R1)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("R1  "), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("STRT"), 0, 0);
                pushed = 1;
            }
        }
        if (decoding)
        {
            new ret = imgDecSync();
            if (ret == IMG_DONE)
            {
                view_w = min(imgWidth(), floatround(screen_w, 0));
                view_h = min(imgHeight(), floatround(screen_h, 0));
                printf("image size %d, %d\n", imgWidth(), imgHeight());
                for (new i; i < 4; i++)
                {
                    SetPos(strips[i], 0.0, float(view_h) * (-0.375 + 0.25 * i), 0.0, 0);
                    SetLocalScale(strips[i], float(view_w), 0.25 * float(view_h), 1.0);
                }
                SendMessage(manager, TAG("REDY"), 0, 0, 0, 0);
                decoding = 0;
            }
            else if (ret == IMG_FAILED)
            {
                SendMessage(manager, TAG("REDY"), 1, 0, 0, 0);
                decoding = 0;
            }
        }
        else if (started)
        {
            if (!faded_in)
            {
                new Float:alpha;
                GetColor(strips[0], _, _, _, alpha);
                if (1.0 == alpha)
                {
                    faded_in = 1;
                }
            }
            else if (focused && !pushed)
            {
                new stick_x;
                new stick_y;
                PadAnalog(stick_y, stick_x, _, _, _, _, _, _, _, _, _, _, _, _, _, _, -1);
                x = clamp(x + stick_x / 2, 0, imgWidth() - view_w);
                y = clamp(y + stick_y / 2, 0, imgHeight() - view_h);
            }
            if (x != shown_x || y != shown_y)
            {
                for (new i; i < 4; i++)
                {
                    imgTrimStart(x, y + view_h * i / 4, view_w, view_h / 4);
                    if (wait_image_trim() == IMG_DONE)
                    {
                        SetTexture(strips[i], imgTexID(), 0, -1);
                    }
                }
                shown_x = x;
                shown_y = y;
            }
        }
    }
    SendMessage(manager, TAG("FNSH"), 0, 0, 0, 0);
    Destroy(Self());
}
