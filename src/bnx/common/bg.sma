/*
 * /opt0/bn/script/common/bg.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The common background: two layers of space and a cloud, behind a black cover that "STRT" fades out. Linked
 * to the camera of the main window, its camera follows it by a fraction of its moves.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

/*
 * The arrangements of the two layers: two rich ones of both textures and two poor ones of the second, each pair
 * alternating at every change; the negative of the next one while the layers fade out.
 */
const LAYOUT_RICH1 = 1;
const LAYOUT_RICH2 = 2;
const LAYOUT_POOR1 = 3;
const LAYOUT_POOR2 = 4;

#include "imgsync.inc"

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

#include "misc.inc"

#include "dom.inc"

new g_cloud;
new Float:g_layer_dist = 500.0; /* the distance at which a layer fills the screen */
new Float:g_rot_follow = 0.4; /* the fraction of the main camera's rotation that the camera follows */
new Float:g_pos_follow = 0.4; /* the same for its position */

main()
{
    new cloud_id;
    new Float:base_pos[3];
    base_pos[0] = 0.0;
    base_pos[1] = 0.0;
    base_pos[2] = 0.0;
    new Float:base_quat[4];
    base_quat[0] = 0.0;
    base_quat[1] = 1.0;
    base_quat[2] = 0.0;
    base_quat[3] = 0.0;
    new Float:home_quat[4];
    home_quat[0] = 0.0;
    home_quat[1] = 1.0;
    home_quat[2] = 0.0;
    home_quat[3] = 0.0;
    SetPos(Camera(0), 0.0, 0.0, 0.0, 0);
    SetQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 0);
    {
        new Float:screen_w;
        new Float:screen_h;
        new Float:x;
        new Float:y;
        new Float:z;
        get_screen_size2(screen_w, screen_h);
        map_screen_point(x, y, z, 0.5 * screen_w, 0.5 * screen_h, Camera(0));
        g_layer_dist = z;
        printf("bg best dist = z %f\n", z);
    }
    new cover;
    new cover_fading;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size2(screen_w, screen_h);
        cover = CreatePlane(Disp2D(0));
        SetColor(cover, 0.0, 0.0, 0.0, 1.0);
        SetPos(cover, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
        SetLocalScale(cover, screen_w, screen_h, 1.0);
        SetOptionI(cover, TAG("ZSRT"), ZSORT_OVERLAY);
        new edge;
        edge = CreatePlane(Disp2D(0));
        SetPos(edge, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
        SetLocalScale(edge, screen_w, screen_h, 1.0);
        SetOptionI(edge, TAG("COL4"), 0, 0, 0xFF000000, 0xFF000000);
        SetOptionF(edge, TAG("CLIP"), 0.995, 0, 0.005, 1.0);
    }
    g_cloud = CreateCloud(Scene(0));
    SetPos(g_cloud, 0.0, 0.0, -20000.0 * 1.0, 0);
    LookAt(g_cloud, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetLocalScale(g_cloud, 1.5 * 10000.0, 1.5 * 10000.0, 1.5 * 10000.0);
    {
        new Float:color_jitter[4] = {0.1, 0.1, 0.0, 0.5};
        SetOptionP(g_cloud, TAG("CDW "), color_jitter, 4);
    }
    SetOptionI(g_cloud, TAG("CLID"), cloud_id);
    SetOptionI(g_cloud, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_cloud, TAG("NSPR"), 50);
    SetOptionI(g_cloud, TAG("NDSP"), 25);
    SetColor(g_cloud, 0.3, 0.5, 1.0, 0.7);
    new textures[2];
    {
        new url[128];
        get_tag_attr("IMG", "space0", "src", url, 128);
        start_image_decode2(url);
        if (IMG_DONE != finish_image_decode())
        {
            printf("spaceimage loaded \n");
        }
        imgKeepTex(imgTexID());
        textures[0] = imgTexID();
        get_tag_attr("IMG", "space1", "src", url, 128);
        start_image_decode2(url);
        if (IMG_DONE != finish_image_decode())
        {
            printf("spaceimage loaded \n");
        }
        imgKeepTex(imgTexID());
        textures[1] = imgTexID();
    }
    new layer0;
    new layer1;
    new layout = LAYOUT_RICH1;
    new Float:fade_in_time = 1000.0;
    create_layer(layer0, textures[0], 0.0, 0.0, -g_layer_dist, 40.0 * 1.0, 0.0, 2500.0);
    SetRot(layer0, 0.0, 0.0, 3.141592, 0, 0);
    create_layer(layer1, textures[1], 0.0, 0.0, -g_layer_dist, 25.0 * 1.0, 0.0, 2500.0);
    new linked;
    new main_scene;
    new main_camera;
    for (; ; )
    {
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_R1)
            {
                cloud_id++;
                SetOptionI(g_cloud, TAG("CLID"), cloud_id);
            }
        }
        new last_scene = main_scene;
        main_scene = wndGetScene(WND_MAIN);
        main_camera = Camera(main_scene);
        if (main_scene != last_scene)
        {
            SendMessage(Self(), TAG("RELS"), 1, 500.0, 0, 0);
            SendMessage(Self(), TAG("FADE"), 0, 0, 500.0, 0);
        }
        new msg;
        new Float:a;
        new Float:b;
        new Float:c;
        /*
         * "FADE" (alpha0, alpha1, time), "POOR" and "RICH" (fade-in time, fade-out time), "CLDR" (cloud size), "STRT"
         * (time), "LINK" (camera, position fraction, rotation fraction), "AJST", "CRCT", "RELS" (animated, time),
         * "CPTN" (pattern).
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FADE"))
            {
                AnimColor(layer0, 1.0, 1.0, 1.0, a, c, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(layer1, 1.0, 1.0, 1.0, b, c, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (msg == TAG("POOR"))
            {
                if (layout < LAYOUT_POOR1)
                {
                    layout = -LAYOUT_POOR1;
                }
                else if (layout == LAYOUT_POOR1)
                {
                    layout = -LAYOUT_POOR2;
                }
                else if (layout == LAYOUT_POOR2)
                {
                    layout = -LAYOUT_POOR1;
                }
                AnimColor(layer0, 1.0, 1.0, 1.0, 0.0, b, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(layer1, 1.0, 1.0, 1.0, 0.0, b, 0.0, ANIM_FUNC_LINEAR);
                fade_in_time = a;
            }
            else if (msg == TAG("RICH"))
            {
                if (layout >= LAYOUT_POOR1)
                {
                    layout = -LAYOUT_RICH1;
                }
                else if (layout == LAYOUT_RICH1)
                {
                    layout = -LAYOUT_RICH2;
                }
                else if (layout == LAYOUT_RICH2)
                {
                    layout = -LAYOUT_RICH1;
                }
                AnimColor(layer0, 1.0, 1.0, 1.0, 0.0, b, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(layer1, 1.0, 1.0, 1.0, 0.0, b, 0.0, ANIM_FUNC_LINEAR);
                fade_in_time = a;
            }
            else if (msg == TAG("CLDR"))
            {
                SetOptionI(g_cloud, TAG("BSIZ"), a);
            }
            else if (msg == TAG("STRT"))
            {
                if (cover)
                {
                    new Float:duration = 2000.0;
                    if (a > 0.0)
                    {
                        duration = a;
                    }
                    AnimColor(cover, 0.0, 0.0, 0.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
                    cover_fading = 1;
                }
            }
            else if (msg == TAG("AJST"))
            {
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:w;
                base_pos[0] = 0.0;
                base_pos[1] = 0.0;
                base_pos[2] = 0.0;
                base_quat[0] = 0.0;
                base_quat[1] = 1.0;
                base_quat[2] = 0.0;
                base_quat[3] = 0.0;
                GetPos(main_camera, x, y, z, main_scene);
                AnimMoveHermite(Camera(0), 1.0 * x * g_pos_follow, 1.0 * y * g_pos_follow, 1.0 * z * g_pos_follow,
                        2000.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                GetQuat(main_camera, x, y, z, w, main_scene);
                printf("fg cam adj quat %f, %f, %f, %f \n", x, y, z, w);
                QuatInterp(x, y, z, w, home_quat[0], home_quat[1], home_quat[2], home_quat[3], x, y, z, w,
                        g_rot_follow);
                AnimQuat(Camera(0), x, y, z, w, 2000.0, 0.0, ANIM_FUNC_EXPONENT);
            }
            else if (msg == TAG("CRCT"))
            {
                GetPos(main_camera, base_pos[0], base_pos[1], base_pos[2], main_scene);
                GetQuat(main_camera, base_quat[0], base_quat[1], base_quat[2], base_quat[3], main_scene);
                AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 2000.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                AnimQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 2000.0, 0.0,
                        ANIM_FUNC_EXPONENT);
            }
            else if (msg == TAG("LINK"))
            {
                linked = 1;
                g_pos_follow = b;
                g_rot_follow = c;
                printf("pos %f, rot%f\n ", b, c);
            }
            else if (msg == TAG("RELS"))
            {
                g_pos_follow = 0.4;
                g_rot_follow = 0.4;
                base_pos[0] = 0.0;
                base_pos[1] = 0.0;
                base_pos[2] = 0.0;
                base_quat[0] = 0.0;
                base_quat[1] = 1.0;
                base_quat[2] = 0.0;
                base_quat[3] = 0.0;
                if (_:a == 1)
                {
                    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, b, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                    AnimQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], b, 0.0,
                            ANIM_FUNC_EXPONENT);
                }
                else
                {
                    SetPos(Camera(0), 0.0, 0.0, 0.0, 0);
                    SetQuat(Camera(0), home_quat[0], home_quat[1], home_quat[2], home_quat[3], 0);
                }
                linked = 0;
            }
            if (msg == TAG("CPTN"))
            {
                printf("cloud pattern changed to %d\n", a);
                SetOptionI(g_cloud, TAG("CLID"), a);
            }
        }
        if (cover && cover_fading)
        {
            if (ANIM_DONE == AnimGetState(cover, ANIM_COLOR))
            {
                Destroy(cover);
                cover = 0;
                cover_fading = 0;
            }
        }
        if (layer0 && layout < 0 && ANIM_DONE == AnimGetState(layer0, ANIM_COLOR))
        {
            Destroy(layer0);
            Destroy(layer1);
            if (layout == -LAYOUT_RICH1)
            {
                create_layer(layer0, textures[0], 0.0, 1.0, -g_layer_dist, 40.0 * 1.0, 0.8, fade_in_time);
                create_layer(layer1, textures[1], 0.0, 1.0, -g_layer_dist, 25.0 * 1.0, 1.0, fade_in_time);
                SetRot(layer0, 0.0, 0.0, 3.141592, 0, 0);
            }
            else if (layout == -LAYOUT_RICH2)
            {
                create_layer(layer0, textures[0], 0.0, 1.0, -g_layer_dist, 40.0 * 1.0, 0.8, fade_in_time);
                create_layer(layer1, textures[1], 0.0, 1.0, -g_layer_dist, 25.0 * 1.0, 1.0, fade_in_time);
                SetRot(layer1, 0.0, 0.0, 3.141592, 0, 0);
            }
            else if (layout == -LAYOUT_POOR1)
            {
                create_layer(layer0, textures[1], -80.0, -32.0, -g_layer_dist, 40.0 * 1.0, 0.7, fade_in_time);
                create_layer(layer1, textures[1], 50.0, 38.0, -g_layer_dist, 25.0 * 1.0, 1.0, fade_in_time);
                SetRot(layer0, 0.0, 0.0, 3.141592, 0, 0);
            }
            else if (layout == -LAYOUT_POOR2)
            {
                create_layer(layer0, textures[1], -80.0, 32.0, -g_layer_dist, 40.0 * 1.0, 0.7, fade_in_time);
                create_layer(layer1, textures[1], 50.0, -38.0, -g_layer_dist, 25.0 * 1.0, 1.0, fade_in_time);
                SetRot(layer1, 0.0, 0.0, 3.141592, 0, 0);
            }
            layout = -layout;
        }
        if (wndGetScene(WND_MAIN))
        {
            if (linked)
            {
                new Float:x;
                new Float:y;
                new Float:z;
                new Float:w;
                GetOffPos(main_camera, x, y, z, -base_pos[0], -base_pos[1], -base_pos[2], main_scene);
                AnimModMove(Camera(0), 1.0 * x * g_pos_follow, 1.0 * y * g_pos_follow, 1.0 * z * g_pos_follow);
                GetQuat(main_camera, x, y, z, w, main_scene);
                new Float:qx;
                new Float:qy;
                new Float:qz;
                new Float:qw;
                QuatMultiply(qx, qy, qz, qw, x, y, z, w, -base_quat[0], -base_quat[1], -base_quat[2], base_quat[3]);
                QuatMultiply(qx, qy, qz, qw, qx, qy, qz, qw, home_quat[0], home_quat[1], home_quat[2], home_quat[3]);
                QuatInterp(qx, qy, qz, qw, home_quat[0], home_quat[1], home_quat[2], home_quat[3], qx, qy, qz, qw,
                        g_rot_follow);
                AnimModQuat(Camera(0), qx, qy, qz, qw);
            }
        }
        sleep;
    }
}

/* Creates a layer plane with texture at (x, y, z) times scale, fading in to alpha. */
create_layer(&plane, texture, Float:x, Float:y, Float:z, Float:scale, Float:alpha, Float:duration)
{
    plane = CreatePlane(Scene(0));
    SetPos(plane, x * scale, y * scale, z * scale, 0);
    LookAt(plane, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    SetColor(plane, 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(plane, 1024.0 * scale, 512.0 * scale, 1.0 * scale);
    SetTexture(plane, texture, 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AnimColor(plane, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}
