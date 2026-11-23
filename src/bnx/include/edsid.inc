/*
 * A wait for the edclient server behind a dialog.
 */

#if !defined g_any_child2
new g_any_child2[1]; /* empty: any element */
#endif
#if !defined g_any_next2
new g_any_next2[1]; /* empty: any element */
#endif
new g_dialog_tag[] = "dialog";
new g_type_attr[4] = {0x74797065, 0, 0x64657669, 0x63650000}; /* "type", then "device" */
/*
 * Shows the dialog dialog_id (the texts of its text elements) while waiting for the server; returns the result of
 * the request.
 */
stock wait_eds_with_dialog(focus_obj, dialog_id[])
{
    new ret;
    new buf[128];
    new args[512];
    new dialog_node;
    new node;
    new len;
    dialog_node = domFindTag(domRoot(), g_dialog_tag, dialog_id, 0);
    len = domGetData(dialog_node, g_type_attr, buf, 128);
    args = "";
    append_string(args, "msg=", 512);
    node = domFirstChild(dialog_node, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 128);
        if (0 != strcmp("text", buf))
        {
            continue;
        }
        len = domGetData(node, "text_id", buf, 128);
        if (0 == len)
        {
            continue;
        }
        len = get_tag_attr("TEXT", buf, "value", buf, 128);
        if (0 == len)
        {
            continue;
        }
        append_string(args, buf, 512);
        append_string(args, "\n", 512);
    }
    append_string(args, ";", 512);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 260.0, 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_obj);
    SetPubVar(dialog, "se_open", ED_SE_WAIT_OPEN);
    SetPubVar(dialog, "se_ok", ED_SE_DECIDE);
    SetPubVar(dialog, "se_cancel", ED_SE_CANCEL);
    SetPubVar(dialog, "se_cursor", ED_SE_CURSOR);
    while (edsClientSync(ret))
    {
        sleep;
    }
    SetPubVar(dialog, "canceled", 1);
    for (; ; )
    {
        if (ScriptState(dialog) == SCRIPT_FINISHED)
        {
            break;
        }
        sleep;
    }
    Destroy(dialog);
    play_se(ED_SE_WAIT_CLOSE);
    return ret;
}

