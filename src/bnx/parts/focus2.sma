/*
 * /opt0/bn/script/parts/focus2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A focus frame driven by its public variables: a 3x3 plane that moves onto the object in focused, takes its size
 * (or the size of its text, for class "LABL") and pulses there; it declares its natives itself instead of including
 * bn.inc.
 */

#pragma dynamic 1280

native FrameRate();
native swapchars(c);
native floatcmp(Float:fOne, Float:fTwo);
native Self();
native SetOptionP(obj, option, const Float:data[], cells);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native CreateNull(parent);
native Hide(obj);
native HideChildren(obj);
native AttachScript(obj, const func[], const file[] = "", const arg[] = "");
native RecvMessage(&msg, &a=0, &b=0, &c=0, &d=0);
native AnimScale(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native Destroy(obj);
native Float:floatadd(Float:dividend, Float:divisor);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native SetPubVar(obj, const name[], value);
native AnimModMove(obj, Float:x, Float:y, Float:z);
native Float:floatdiv(Float:dividend, Float:divisor);
native Float:floatsub(Float:oper1, Float:oper2);
native Scene(obj);
native GetOffPos(obj, const Float:x[], &Float:y, &Float:z, Float:off_x, Float:off_y, Float:off_z, ref);
native GetOptionP(obj, option, const data[], cells);
native Float:float(value);
native Float:floatmul(Float:oper1, Float:oper2);
native Parent(obj);
native GetPubVar(obj, const name[]);
native GetScale(obj, &Float:x, &Float:y, &Float:z);
native GetPos(obj, &Float:x, &Float:y, &Float:z, ref);
native random(max);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native GetRot(obj, &Float:x, &Float:y, &Float:z, ref, order);
native SetRot(obj, Float:x, Float:y, Float:z, ref, order);
native SetScale(obj, Float:x, Float:y, Float:z);
native AnimRotate(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, order);
native floatround(Float:value, method=0);
native GetLocalScale(obj, &Float:x, &Float:y, &Float:z);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator/(Float:oper1, Float:oper2) = floatdiv;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

/* As in bn.inc. */
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_SLOWER = 2;
const ANIM_FUNC_FASTER = 3;
const ANIM_FUNC_ANGULER = 5;

public focused;                 /* the object to frame, 0 for none */
public class;                   /* "LABL" to frame the text of a label */
public Float:zoffs; /* depth offset of the frame from the object */
public Float:sharpness = 0.5;   /* margin of the corners of the frame */
public Float:offs_width = 60.0; /* added to the size of the object */
public Float:offs_height = 60.0; /* added to the height of the object */
new Float:g_shrink_time = 85.0; /* first half of a move, in ms */
new Float:g_grow_time = 85.0;   /* second half of a move, in ms */
new Float:g_shrunk_size = 30.0; /* size of the frame halfway through a move */
new Float:g_base_alpha = 0.4;

/* Same as wait_ms of wait.inc. */
wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

stock bool:operator>(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) > 0;

main()
{
    {
        new Float:frame_margin[4] = {0.2, 0.2, 0.2, 0.2};
        SetOptionP(Self(), TAG("MGN"), frame_margin, 4);
        new Float:tex_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(Self(), TAG("MGNT"), tex_margin, 4);
    }
    SetColor(Self(), 1.0, 1.0, 1.0, 0.0);
    new animator = CreateNull(Self());
    Hide(animator);
    HideChildren(animator);
    new margin_obj = CreateNull(Self());
    Hide(margin_obj);
    HideChildren(margin_obj);
    while (!focused)
    {
        sleep;
    }
    place_on_focused();
    sleep;
    AttachScript(animator, "idle");
    AttachScript(margin_obj, "margin");
    new last_focused = focused;
    for (; ; )
    {
        new msg;
        while (RecvMessage(msg))
        {
            /* "KILL": the frame shrinks, fades out and destroys itself. */
            if (msg == TAG("KILL"))
            {
                AnimScale(Self(), 10.0, 10.0, 10.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, 1200.0, 0.0, ANIM_FUNC_ANGULER);
                wait_ms(1200);
                Destroy(Self());
            }
        }
        if (focused != last_focused)
        {
            if (focused)
            {
                new Float:pos[3];
                get_focus_pos(focused, pos, pos[1], pos[2]);
                AnimMoveHermite(Self(), pos[0], pos[1], pos[2], g_shrink_time + g_grow_time, 0.0, ANIM_FUNC_ANGULER,
                        0.2);
                if (class == TAG("LABL"))
                {
                    AttachScript(animator, "move_to_label");
                    SetPubVar(animator, "class", class);
                }
                else
                {
                    AttachScript(animator, "move_to");
                    SetPubVar(animator, "class", class);
                }
            }
            else
            {
                AttachScript(animator, "floating");
            }
        }
        last_focused = focused;
        if (focused)
        {
            new Float:pos[3];
            get_focus_pos(focused, pos, pos[1], pos[2]);
            AnimModMove(Self(), pos[0], pos[1], pos[2]);
        }
        sleep;
    }
}

/* Gets where the frame goes on target, in the scene: its origin, or the center of the text of a label. */
get_focus_pos(target, Float:pos[], &Float:y, &Float:z)
{
    if (!target)
    {
        return -1;
    }
    if (class == TAG("LABL"))
    {
        new Float:rect[4];
        get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
        GetOffPos(target, pos, y, z, rect[0] + rect[2] / 2.0, rect[1] - rect[3] / 2.0, zoffs, Scene(0));
    }
    else
    {
        GetOffPos(target, pos, y, z, 0.0, 0.0, zoffs, Scene(0));
    }
    return 0;
}

/* Waits until the text of obj is laid out and gets its rectangle, scaled like obj. */
get_text_rect(obj, &Float:x, &Float:y, &Float:w, &Float:h)
{
    new rect[5];
    for (; ; )
    {
        GetOptionP(obj, TAG("RECT"), rect, 5);
        if (rect[0])
        {
            break;
        }
        sleep;
    }
    x = float(rect[1]) / 64.0;
    y = float(rect[2]) / 64.0;
    w = float(rect[3]) / 64.0;
    h = float(rect[4]) / 64.0;
    new Float:scale[3];
    get_total_scale(obj, scale[0], scale[1], scale[2]);
    x = x * scale[0];
    y = y * scale[1];
    w = w * scale[0];
    h = h * scale[1];
}

/* Keeps the corners of the parent frame round while it is resized. */
public margin()
{
    for (; ; )
    {
        new Float:sharp = GetPubVar(Parent(0), "sharpness");
        new Float:scale[3];
        GetScale(Parent(0), scale[0], scale[1], scale[2]);
        new Float:frame_margin[4];
        if (scale[0] > scale[1])
        {
            frame_margin[0] = frame_margin[1] = sharp * scale[1] / scale[0];
            frame_margin[2] = frame_margin[3] = sharp;
        }
        else
        {
            frame_margin[2] = frame_margin[3] = sharp * scale[0] / scale[1];
            frame_margin[0] = frame_margin[1] = sharp;
        }
        SetOptionP(Parent(0), TAG("MGN"), frame_margin, 4);
        sleep;
    }
}

/* Pulses the parent frame on the focused object. */
public idle()
{
    AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.5 * g_base_alpha, 100.0, 0.0, ANIM_FUNC_ANGULER);
    wait_ms(300);
    for (; ; )
    {
        AnimColor(Parent(0), 1.0, 1.0, 1.0, 0.9 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
        wait_ms(1200);
        AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.2 * g_base_alpha, 1200.0, 0.0, ANIM_FUNC_ANGULER);
        wait_ms(1200);
    }
}

/* Nothing focused: shrinks the parent frame and lets it drift around its place. */
public floating()
{
    AnimScale(Parent(0), 10.0, 10.0, 10.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    new Float:pos[3];
    new Float:home[3];
    new Float:velocity[3];
    GetPos(Parent(0), pos[0], pos[1], pos[2], 0);
    home[0] = pos[0];
    home[1] = pos[1];
    home[2] = pos[2];
    velocity[0] = float(random(15)) / 10.0;
    velocity[1] = float(random(15)) / 10.0;
    velocity[2] = float(random(15)) / 10.0;
    for (; ; )
    {
        if (home[0] > pos[0])
        {
            velocity[0] += 0.01;
        }
        else
        {
            velocity[0] -= 0.01;
        }
        if (home[1] > pos[1])
        {
            velocity[1] += 0.01;
        }
        else
        {
            velocity[1] -= 0.01;
        }
        if (home[2] > pos[2])
        {
            velocity[2] += 0.01;
        }
        else
        {
            velocity[2] -= 0.01;
        }
        pos[0] += velocity[0];
        pos[1] += velocity[1];
        pos[2] += velocity[2];
        AnimMoveLine(Parent(0), pos[0], pos[1], pos[2], 100.0, 0.0, ANIM_FUNC_LINEAR);
        sleep;
    }
}

/* Puts the frame on the focused object at once, at its size, and fades it in. */
place_on_focused()
{
    if (class == TAG("LABL"))
    {
        {
            new Float:pos[3];
            get_focus_pos(focused, pos, pos[1], pos[2]);
            SetPos(Self(), pos[0], pos[1], pos[2], 0);
        }
        {
            new Float:rot[3];
            GetRot(focused, rot[0], rot[1], rot[2], Parent(0), 0);
            SetRot(Self(), rot[0], rot[1], rot[2], 0, 0);
        }
        {
            new Float:rect[4];
            get_text_rect(focused, rect[0], rect[1], rect[2], rect[3]);
            SetScale(Self(), rect[2] + offs_width, rect[3] + offs_height, 1.0);
        }
        AnimColor(Self(), 1.0, 1.0, 1.0, g_base_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        {
            new Float:pos[3];
            get_focus_pos(focused, pos, pos[1], pos[2]);
            SetPos(Self(), pos[0], pos[1], pos[2], 0);
        }
        {
            new Float:rot[3];
            GetRot(focused, rot[0], rot[1], rot[2], Parent(0), 0);
            SetRot(Self(), rot[0], rot[1], rot[2], 0, 0);
        }
        {
            new Float:size[3];
            get_total_scale(focused, size[0], size[1], size[2]);
            SetScale(Self(), size[0] + offs_width, size[1] + offs_height, 1.0);
        }
        AnimColor(Self(), 1.0, 1.0, 1.0, g_base_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Shrinks the parent frame on the way to the focused object, then grows it to the size of the object. */
public move_to()
{
    new target = GetPubVar(Parent(0), "focused");
    new Float:extra_w = GetPubVar(Parent(0), "offs_width");
    new Float:extra_h = GetPubVar(Parent(0), "offs_height");
    {
        new Float:rot[3];
        GetRot(target, rot[0], rot[1], rot[2], Parent(Parent(0)), 0);
        AnimRotate(Parent(0), rot[0], rot[1], rot[2], g_shrink_time + g_grow_time, 0.0, ANIM_FUNC_LINEAR, 0);
    }
    AnimScale(Parent(0), g_shrunk_size, g_shrunk_size, 1.0, g_shrink_time, 0.0, ANIM_FUNC_SLOWER);
    AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.0, g_shrink_time, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(floatround(g_shrink_time, 0));
    {
        new Float:size[3];
        get_total_scale(target, size[0], size[1], size[2]);
        AnimScale(Parent(0), size[0] + extra_w, size[1] + extra_h, 1.0, g_grow_time, 0.0, ANIM_FUNC_FASTER);
        AnimColor(Parent(0), 1.0, 1.0, 1.0, g_base_alpha, g_grow_time, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(floatround(g_grow_time, 0));
    }
    AttachScript(Self(), "idle");
}

/* Same as move_to, to the size of the text of a label. */
public move_to_label()
{
    new target = GetPubVar(Parent(0), "focused");
    new Float:extra_w = GetPubVar(Parent(0), "offs_width");
    new Float:extra_h = GetPubVar(Parent(0), "offs_height");
    {
        new Float:rot[3];
        GetRot(target, rot[0], rot[1], rot[2], Parent(Parent(0)), 0);
        AnimRotate(Parent(0), rot[0], rot[1], rot[2], g_shrink_time + g_grow_time, 0.0, ANIM_FUNC_LINEAR, 0);
    }
    AnimScale(Parent(0), g_shrunk_size, g_shrunk_size, 1.0, g_shrink_time, 0.0, ANIM_FUNC_SLOWER);
    AnimColor(Parent(0), 1.0, 1.0, 1.0, 1.0, g_shrink_time, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(floatround(g_shrink_time, 0));
    {
        new Float:rect[4];
        get_text_rect(target, rect[0], rect[1], rect[2], rect[3]);
        AnimScale(Parent(0), rect[2] + extra_w, rect[3] + extra_h, 1.0, g_grow_time, 0.0, ANIM_FUNC_FASTER);
        AnimColor(Parent(0), 1.0, 1.0, 1.0, g_base_alpha, g_grow_time, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(floatround(g_grow_time, 0));
    }
    AttachScript(Self(), "idle");
}

/* Gets the scale of obj times its local scale. */
get_total_scale(obj, &Float:x, &Float:y, &Float:z)
{
    new Float:scale[3];
    new Float:local_scale[3];
    GetScale(obj, scale[0], scale[1], scale[2]);
    GetLocalScale(obj, local_scale[0], local_scale[1], local_scale[2]);
    x = scale[0] * local_scale[0];
    y = scale[1] * local_scale[1];
    z = scale[2] * local_scale[2];
}
