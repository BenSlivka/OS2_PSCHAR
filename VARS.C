/*  vars.c - global variables
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Removed unneeded globals
*	13-Feb-1988 bws Add fColor flag
*/

#include "asrt.h"
#include "vio.h"
#include "ps.h"
#include "os2.h"
#include "queue.h"

int N_of_Cols;		    /* Number of columns on screen */
int N_of_Rows;		    /* Number of rows on screen */

int dataRows;		    /* Number of rows in data region */

int color[] = {
    attr(blue,black),	    /* border */
    attr(green,black),	    /* title */
    attr(red,black),	    /* header */
    attr(white,black),	    /* menu */
    attr(black,white),	    /* cursor */
    attr(blue,white),	    /* help */
    attr(white,black),	    /* data */
    attr(white,blue),	    /* option screen */
    attr(white,blue),	    /* option field */
    attr(blue,white),	    /* option cursor */
    attr(magenta,black)     /* debug text */
};
int cColors=sizeof(color)/sizeof(int);	/* Number of entries in color array */


int fDebug=FALSE;	    /* TRUE for debug output */
int fXDebug=FALSE;	    /* TRUE for eXtensive debug output */
int fFile=FALSE;	    /* TRUE if dump images to file */
int fProfile=FALSE;	    /* TRUE for profiling */
int fRecord=FALSE;	    /* TRUE if echo OS records */
int f43mode=TRUE;	    /* TRUE if 43 line mode desired */
int fColor=TRUE;	    /* TRUE if colors desired */

dword	refreshPeriod=300;  /* refresh period, in milliseconds */
