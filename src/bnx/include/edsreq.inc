/*
 * A request to the edclient server, with its wait page.
 */

/*
 * Sends request to the server and waits for the answer, opening the wait page page_id after 5 seconds; with
 * cancelable the user can cancel it. Returns 0, EDS_CANCELED, EDS_NETWORK_ERROR or another negative error.
 */
stock run_eds_request(request, page_id[], focus_obj, arrows[], cancelable)
{
    new msg;
    new a;
    new b;
    new c;
    new start_tick;
    new ret;
    new page;
    new slide_in;
    new slide_dir;
    slide_in = SLIDE_IN_NONE; /* redundant as written */
    slide_in = 0; /* acts as SLIDE_IN_NONE */
    if (0 != arrows[PAGE_ARROW_NEXT])
    {
        update_page_arrows(arrows, 0, 0, 1, 1);
    }
    ret = edsClientRequest(request);
    if (ret < 0)
    {
        printf("@@@ failed edsClientRequest [%d]\n", ret);
        return ret;
    }
    SystemCommand(SYSCMD_BUSY, 1);
    if (cancelable == 1)
    {
        /* the label of the cross button: "Abort" */
        new label[16];
        label{0} = 0xE4;
        label{1} = 0xB8;
        label{2} = 0xAD;
        label{3} = 0xE6;
        label{4} = 0x96;
        label{5} = 0xAD;
        label{6} = 0;
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    else
    {
        SetSystemParam(SYS_PADHELP, 0);
    }
    start_tick = tickcount();
    new busy = 1;
    for (; ; )
    {
        while (RecvMessage(msg, a, b, c))
        {
        }
        new pressed;
        PadState(_, pressed, _, _, -1);
        if (pressed & PAD_CROSS)
        {
            if (cancelable == 1)
            {
                if (0 == busy)
                {
                    if (0 != page)
                    {
                        close_ed_page(page, focus_obj, slide_dir);
                    }
                    ret = EDS_CANCELED;
                    break;
                }
                else
                {
                    edsClientCancel();
                    printf("@@@@ edsClientCancel()\n");
                }
            }
        }
        if (busy == 1)
        {
            busy = edsClientSync(ret);
        }
        if (busy == 1)
        {
            if (0 == page)
            {
                start_tick = tickcount() < start_tick ? tickcount() : start_tick;
                if (tickcount() - start_tick > 5000)
                {
                    page = open_ed_page(page_id, focus_obj, slide_in);
                    SetPubVar(page, "g_iOpenSE", ED_SE_WAIT_OPEN);
                    SetPubVar(page, "g_iCloseSE", ED_SE_WAIT_CLOSE);
                    SendMessage(page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    start_tick = tickcount();
                }
            }
            else
            {
            }
        }
        else if (0 != page)
        {
            start_tick = tickcount() < start_tick ? tickcount() : start_tick;
            if (tickcount() - start_tick > 3000)
            {
                close_ed_page(page, focus_obj, slide_dir);
                break;
            }
        }
        else
        {
            break;
        }
        sleep;
    }
    while (RecvMessage(msg, a, b, c))
    {
    }
    SystemCommand(SYSCMD_BUSY, 0);
    if (ret < 0)
    {
        printf("@@@ request failed [%d]\n", ret);
        if (ret == EDS_CANCELED)
        {
        }
        else if (ret <= EDS_NETWORK_ERROR && ret > -3000)
        {
            ret = EDS_NETWORK_ERROR;
        }
        return ret;
    }
    return 0;
}

