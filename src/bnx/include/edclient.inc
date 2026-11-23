/*
 * The natives of the edclient library (/opt0/bn/modules/edclient.so), in the order of its FuncTable, called by the
 * pages of edclient/. The module sends the requests to the server as https POSTs of the fields and reads its XML
 * answers.
 */

#pragma library "edclient"
/* Opens the client of the server host:port (https); returns 0, or a negative error. */
native edsClientInit(const host[], port);
native edsClientExit();
/* Copies field (EDS_FIELD_*) into buf; maxlength is not used. */
native edsClientGetParam(field, buf[], maxlength);
native edsClientSetStr(field, const value[]);
native edsClientSetInt(field, value);
/*
 * Starts request (EDS_REQ_*), or returns EDS_INVALID_PARAM or EDS_BUSY; edsClientSync gives its result, then the
 * answer is read with edsClientGetInt and edsClientGetStr.
 */
native edsClientRequest(request);
/*
 * device: 0 the hard disk, 1 and 2 the memory cards (also for edsClientRemove, edsClientLoad and edsClientSave);
 * edsClientFormat formats only a memory card. These start the operation, which edsClientSync waits for.
 */
native edsClientFormat(device);
native edsClientRemove(device);
/*
 * Reads the state of the memory cards; edsClientSync gives it: four bits for each card (bits 4-7 card 1, bits 0-3
 * card 2), the type of the card in the low two (2: a PS2 memory card) and bit 2 set when it is formatted.
 */
native edsClientPoll();
/* Reads the saved ID: EDS_FIELD_AUTH_ID and EDS_FIELD_AUTH_PASSWD. */
native edsClientLoad(device);
/* Saves EDS_FIELD_AUTH_ID and EDS_FIELD_AUTH_PASSWD; with bit 0x8000 in device it replaces an ID saved already. */
native edsClientSave(device);
/* Returns nonzero while the last operation runs; then eds_result is its result. */
native edsClientSync(&eds_result);
/* Cancels the request running. */
native edsClientCancel();
/* Opens entry index of a list of the answer, for edsClientGetStr and edsClientGetInt; edsClientLeave closes it. */
native edsClientEnter(index);
native edsClientLeave();
/* Copies the string field of the answer into buf (field -1: the date of the server, YYYYMMDD); returns 0, or -1. */
native edsClientGetStr(field, const buf[], maxlength);
/* Returns the number field of the answer, or -1. */
native edsClientGetInt(field);
#pragma library
