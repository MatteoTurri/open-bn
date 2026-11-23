/*
 * /opt0/bn/script/top_camera.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The camera of the top page: moves to the item of top_list it is told to look at, sways slowly in front of it,
 * shows its label, and flies into it with a glow when it is opened.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "topmenu.inc"

/* The slow sway of the camera once it has settled in front of an item. */
const SWAY_OFF = 0;
const SWAY_OUT = 2; /* the next move goes back to the full distance */
const SWAY_IN = 3; /* the next move goes nearer */

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &Float:b, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

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

#include "imgdec.inc"

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, Float:rot_follow)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("LINK"), Camera(0), pos_follow, rot_follow, 0);
    }
}

#include "pfs.inc"

/* Sends the camera of the background window back home ("RELS"), animated over duration ms if animate is 1. */
release_bg_camera(animate, duration)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("RELS"), animate, duration, 0, 0);
    }
}

#include "string.inc"

#include "dom.inc"

/* Runs on a plane of the camera: on "INTO" (target, duration) it flares up in front of target and fades. */
public channel_glowup()
{
    new self = Self();
    {
        new url[256];
        get_tag_attr("IMG", "channel_glowup", "src", url, 256);
        start_image_decode2(url);
    }
    new status;
    while (IMG_BUSY == status)
    {
        status = finish_image_decode();
    }
    SetTexture(self, imgTexID(), 0, -1);
    for (; ; )
    {
        SetColor(self, 1.0, 1.0, 1.0, 0.0);
        SetScale(self, 50.0, 50.0, 1.0);
        new target;
        new Float:duration;
        wait_message(TAG("INTO"), target, duration);
        if (!target)
        {
            return;
        }
        new Float:step_time = 0.8 * duration;
        {
            new Float:x;
            new Float:y;
            new Float:z;
            GetOffPos(target, x, y, z, 0.0, -5.0, -100.0, Parent(0));
            SetPos(self, x, y, z, 0);
            LookAt(self, Parent(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
            AnimMoveHermite(self, 0.0, 0.0, 20.0, step_time, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        }
        AnimScale(self, 400.0, 400.0, 1.0, step_time, 0.0, ANIM_FUNC_SLOWER);
        AnimColor(self, 1.0, 1.0, 1.0, 0.65, step_time, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(floatround(1.2 * step_time, 0));
        release_bg_camera(0, 0);
        AnimScale(self, 32.0, 32.0, 1.0, step_time, 0.0, ANIM_FUNC_SLOWER);
        AnimColor(self, 1.0, 1.0, 1.0, 0.0, step_time, 0.0, ANIM_FUNC_ANGULER);
        wait_ms(floatround(step_time, 0));
    }
}

public Float:best_dist; /* the distance of the 2D display from the camera */
public pad_enable; /* the camera has nearly arrived: the list may take the pad */

/* Runs on the camera: moves to the item it is told to look at, sways in front of it and shows its label. */
public camera_main()
{
    new sway;
    new target;
    new old_target;
    new look_at = CreateNull(Scene(0));
    new up = CreateNull(Scene(0));
    new preload;
    new label_received;
    new near;
    new settled;
    new Float:dist;
    new Float:look_dist;
    new view = VIEW_WIDE;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size5(screen_w, screen_h);
    {
        new Float:x;
        new Float:y;
        map_screen_point(x, y, best_dist, 0.5 * screen_w, 0.5 * screen_h, Self());
    }
    new glow;
    glow = CreatePlane(Self());
    SetColor(glow, 1.0, 1.0, 1.0, 0.0);
    AttachScript(glow, "channel_glowup", "", "");
    new colors[4];
    static white_bytes[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF};
    pack_color_bytes(colors, white_bytes);
    new label = CreateLabel(Disp2D(0));
    SetPos(label, 0.5 * screen_w, 0.78 * screen_h, 0.0, 0);
    SetColor(label, 0.0, 0.0, 0.0, 0.0);
    SetOptionI(label, TAG("FONT"), FontID(0));
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 24);
    SetOptionP(label, TAG("COL4"), colors, 4);
    SetPos(Self(), 0.0, 0.0, 0.0, 0);
    LookAt(Self(), Scene(0), 0.0, 0.0, -500.0, 0, 0.0, 0.0, 0.0);
    new Float:progress;
    new label_text[256];
    for (; ; )
    {
        new old_settled = settled;
        new old_near = near;
        new msg;
        new a;
        new Float:b;
        new Float:c;
        /*
         * "RSET" back home; "DIST" view; "LOOK" target: move in front of it; "FRST" target, duration and "BACK"
         * target, duration, start distance: the first look at the list and the return to it; "INTO" target,
         * duration: fly into it; "LOFF" hides the label; "FLON" turns the preload off.
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FLON"))
            {
                preload = 0;
            }
            if (msg == TAG("RSET"))
            {
                AnimMoveHermite(Self(), 0.0, 0.0, 0.0, 2000.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                new Float:x;
                new Float:y;
                new Float:z;
                x = 0.0;
                y = 0.0;
                z = -best_dist;
                AnimMoveHermite(look_at, x, y, z, 2000.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                AnimMoveHermite(up, x, 1000.0 + y, z, 1700.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                target = 0;
                old_target = 0;
                settled = 0;
                pad_enable = 0;
                near = 0;
            }
            if (msg == TAG("DIST"))
            {
                view = a;
                settled = 0;
                pad_enable = 0;
                if (target)
                {
                    if (VIEW_CLOSE == view)
                    {
                        printf("send preload message !!\n");
                        if (preload)
                        {
                            SendMessage(target, TAG("LOAD"), 0, 0, 0, 0);
                        }
                    }
                    else if (view == VIEW_WIDE)
                    {
                        SendMessage(target, TAG("SLEP"), 300.0, 0, 0, 0);
                    }
                }
            }
            if (msg == TAG("LOOK"))
            {
                old_target = target;
                target = a;
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:height = -5.0;
                if (view == VIEW_WIDE)
                {
                    GetPos(target, x, y, z, 0);
                    dist = 0.7 * sqrt(x * x + y * y + z * z);
                    look_dist = 0.0;
                }
                else
                {
                    dist = 0.9 * best_dist;
                    look_dist = 0.0;
                }
                if (target && target != old_target)
                {
                    if (old_target)
                    {
                        SendMessage(old_target, TAG("SLEP"), 300.0, 0, 0, 0);
                    }
                }
                if (target && VIEW_CLOSE == view)
                {
                    if (preload)
                    {
                        SendMessage(target, TAG("LOAD"), 0, 0, 0, 0);
                    }
                }
                GetOffPos(target, x, y, z, 0.0, height, dist, Parent(0));
                AnimMoveHermite(Self(), x, y, z, 1200.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                GetOffPos(target, x, y, z, 0.0, height, look_dist, Parent(0));
                AnimMoveHermite(look_at, x, y, z, 1200.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                GetOffPos(target, x, y, z, 0.0, 1200.0, 0.0, Parent(0));
                AnimMoveHermite(up, x, y, z, 1100.0, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                settled = 0;
                pad_enable = 0;
                near = 0;
            }
            if (msg == TAG("FRST"))
            {
                target = a;
                new Float:duration = b;
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:height = -5.0;
                if (view == VIEW_WIDE)
                {
                    dist = 4.0 * best_dist;
                    look_dist = 0.0;
                }
                else
                {
                    dist = best_dist;
                    look_dist = 0.0;
                }
                SetPos(Self(), 0.0, 0.0, 0.0, 0);
                GetOffPos(target, x, y, z, 0.0, height, dist, Parent(0));
                AnimMoveHermite(Self(), x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                GetOffPos(target, x, y, z, 0.0, height, look_dist, Parent(0));
                SetPos(look_at, x, y, z, 0);
                AnimMoveHermite(look_at, x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                GetOffPos(target, x, y, z, 0.0, 1200.0, 0.0, Parent(0));
                SetPos(up, x, y, z, 0);
                AnimMoveHermite(up, x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 0.6);
                settled = 0;
                pad_enable = 0;
                near = 0;
                link_bg_camera(0.4, 0.4);
                adjust_bg_camera();
            }
            if (msg == TAG("BACK"))
            {
                target = a;
                new Float:duration = b;
                new Float:start_dist = c;
                new Float:height = -5.0;
                new Float:x;
                new Float:y;
                new Float:z;
                dist = best_dist;
                look_dist = 0.0;
                GetOffPos(target, x, y, z, 0.0, height, start_dist, Parent(0));
                SetPos(Self(), x, y, z, 0);
                GetOffPos(target, x, y, z, 0.0, height, look_dist, Parent(0));
                SetPos(look_at, x, y, z, 0);
                GetOffPos(target, x, y, z, 0.0, 1000.0, 0.0, Parent(0));
                SetPos(up, x, y, z, 0);
                GetOffPos(target, x, y, z, 0.0, height, dist, Parent(0));
                AnimMoveHermite(Self(), x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                settled = 0;
                pad_enable = 0;
                near = 0;
                link_bg_camera(0.4, 0.4);
                adjust_bg_camera();
            }
            if (msg == TAG("INTO"))
            {
                old_target = target;
                target = a;
                new Float:duration = b;
                new Float:x;
                new Float:y;
                new Float:z;
                dist = -20.0;
                look_dist = -100.0;
                GetOffPos(target, x, y, z, 0.0, 0.0, dist, Parent(0));
                AnimMoveHermite(Self(), x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                GetOffPos(target, x, y, z, 0.0, 0.0, look_dist, Parent(0));
                AnimMoveHermite(look_at, x, y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                SendMessage(glow, TAG("INTO"), target, duration, 0, 0);
                settled = 0;
                pad_enable = 0;
                near = 0;
            }
            if (msg == TAG("LOFF"))
            {
                AnimColor(label, 0.0, 0.0, 0.0, 0.0, 250.0, 0.0, ANIM_FUNC_LINEAR);
                label_received = 0;
            }
        }
        new str[256];
        /* "LABL" text: the label of the item, shown when the camera is nearly there. */
        while (RecvMessageStr(msg, str, 256))
        {
            if (msg == TAG("LABL"))
            {
                printf("label %s\n", str);
                label_received = 1;
                copy_string(label_text, str, 256);
            }
        }
        label_received = 0;
        new Float:old_progress = progress;
        progress = AnimGetCurrTime(Self(), ANIM_MOVE);
        if ((progress > 0.8 || ANIM_DONE == AnimGetState(Self(), ANIM_MOVE)) && VIEW_CLOSE == view)
        {
            near = 1;
        }
        if (progress > 0.8 || ANIM_DONE == AnimGetState(Self(), ANIM_MOVE))
        {
            pad_enable = 1;
        }
        if (progress > 0.98 || ANIM_DONE == AnimGetState(Self(), ANIM_MOVE))
        {
            settled = 1;
        }
        if (label_received)
        {
            if ((progress > 0.7 && old_progress <= 0.7) || ANIM_DONE == AnimGetState(Self(), ANIM_MOVE))
            {
                SetOptionS(label, TAG("UTF8"), label_text);
                AnimColor(label, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (old_progress > progress)
            {
                AnimColor(label, 1.0, 1.0, 1.0, 0.0, 600.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        if (near && old_near != near)
        {
            if (target && VIEW_CLOSE == view)
            {
                printf("send busy message1 !!\n");
                SendMessage(target, TAG("BUSY"), target, 0, 0, 0);
            }
        }
        if (settled && old_settled != settled && target)
        {
            sway = SWAY_IN;
            new Float:x;
            new Float:y;
            new Float:z;
            GetPos(Self(), x, y, z, 0);
            AnimMoveLine(Self(), x, y, z, 0.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else if (!settled && old_settled != settled)
        {
            sway = SWAY_OFF;
        }
        if (sway >= SWAY_OUT && ANIM_DONE == AnimGetState(Self(), ANIM_MOVE))
        {
            new Float:x;
            new Float:y;
            new Float:z;
            if (sway == SWAY_IN)
            {
                GetOffPos(target, x, y, z, 0.0, -5.0, 0.88 * dist, Scene(0));
                AnimMoveLine(Self(), x, y, z, 16000.0, 0.0, ANIM_FUNC_ANGULER);
                sway = SWAY_OUT;
            }
            else if (sway == SWAY_OUT)
            {
                GetOffPos(target, x, y, z, 0.0, -5.0, dist, Scene(0));
                AnimMoveLine(Self(), x, y, z, 16000.0, 0.0, ANIM_FUNC_ANGULER);
                sway = SWAY_IN;
            }
        }
        /* Until it settles, the camera keeps up with the target, which may be moving. */
        if (target && !settled)
        {
            new Float:x;
            new Float:y;
            new Float:z;
            new Float:height = -5.0;
            GetOffPos(target, x, y, z, 0.0, height, dist, Parent(0));
            AnimModMove(Self(), x, y, z);
            GetOffPos(target, x, y, z, 0.0, height, look_dist, Parent(0));
            AnimModMove(look_at, x, y, z);
            GetOffPos(target, x, y, z, 0.0, 1000.0, 0.0, Parent(0));
            AnimModMove(up, x, y, z);
        }
        LookAt(Self(), look_at, 0.0, 0.0, 0.0, up, 0.0, 0.0, 0.0);
        sleep;
    }
}
