/*
 * Result codes of the image library (filters, RawImg, zoom operations): 0 is
 * success. The high part is the class, theImageDecoder maps each class to
 * its own result.
 */

#ifndef EICLERROR_H
#define EICLERROR_H

#define EICL_ERR_NO_MEMORY    0x10      /* out of memory, or cancelled */
#define EICL_ERR_STREAM       0x100     /* no stream, or the codec could not open it */
#define EICL_ERR_STREAM_WRITE 0x103     /* the encoder could not write the stream */
#define EICL_ERR_NO_DATA      0x1000    /* no stream (PNG) or no bitmap data */
#define EICL_ERR_BAD_DATA     0x1001    /* the codec failed or was cancelled, or a bad mode */
#define EICL_ERR_BAD_HEADER   0x1002    /* the header is not accepted */
#define EICL_ERR_CODEC        0x100000  /* the codec cannot be set up, or is not implemented */
#define EICL_ERR_FORMAT       0x1000000 /* format or size not handled: another decoder is tried */

#endif
