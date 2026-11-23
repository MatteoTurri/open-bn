/*
 * /opt0/bn/script/system/busy.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The busy indicator: two planes run through the 30 frames of texture/busy.png half a cycle apart,
 * while the system is busy and for a second after.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 64

#include "tag.inc"

#include "imgsync.inc"

main()
{
    new planes[2];
    {
        new i;
        planes[i] = CreatePlane(Self());
        SetColor(planes[i], 1.0, 1.0, 1.0, 0.0);
        SetOptionI(planes[i], TAG("ALPH"), BLEND_ADD);
    }
    {
        new i = 1;
        planes[i] = CreatePlane(Self());
        SetColor(planes[i], 1.0, 1.0, 1.0, 0.0);
        SetOptionI(planes[i], TAG("ALPH"), BLEND_ADD);
    }
    imgDecStart("texture/busy.png", 0, 0, -1, 100, 1);
    if (IMG_DONE != wait_image_decode())
    {
        return;
    }
    new width = imgWidth();
    new height = imgHeight();
    new frame_height = height / 30;
    new frame[2] = {0, 15};
    new remain_ms;
    SetLocalScale(planes[0], float(width), float(frame_height), 1.0);
    SetLocalScale(planes[1], float(width), float(frame_height), 1.0);
    new busy;
    for (; ; )
    {
        busy = GetSystemParam(SYS_BUSY);
        if (busy)
        {
            remain_ms = 1000;
        }
        if (remain_ms <= 0)
        {
            sleep;
            continue;
        }
        remain_ms = remain_ms - 1000 / FrameRate();
        if (remain_ms <= 0)
        {
            Hide(planes[0]);
            Hide(planes[1]);
            remain_ms = 0;
            frame[0] = 0;
            frame[1] = 15;
            sleep;
            continue;
        }
        for (new i; i < 2; i++)
        {
            imgTrimStart(0, frame_height * frame[i], width, frame_height);
            if (wait_image_trim() == IMG_DONE)
            {
                SetTexture(planes[i], imgTexID(), 0, -1);
            }
            frame[i]++;
            if (frame[i] == 15 && !busy)
            {
                AnimColor(planes[i], 1.0, 1.0, 1.0, 0.0, 750.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (frame[i] == 1 && busy)
            {
                Show(planes[i]);
                AnimColor(planes[i], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            }
            if (frame[i] >= 30)
            {
                frame[i] = 0;
            }
            sleep;
        }
        sleep;
    }
}
