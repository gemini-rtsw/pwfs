/* Id:$ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * testKit.c
 * 
 * PURPOSE
 * -------
 * Provide dummy source of wfs data to exercise the TCS gensubs. Also
 * provide dummy TCS receiving gensubs for ao data
 * 
 * FUNCTION NAME(S)
 * ----------------
 * 
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Sean Prior (srp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 
 * 28-Oct-1998: Original (srp)
 * 29-Oct-1998: array index from wfs sources run 1 -> np NOT 0 -> (np-1)
 * 05-Apr-2000: update routine to fit with synchroMap
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef MAX_WFS_SOURCES
#define MAX_WFS_SOURCES  5
#define TTF_ARRAY_SIZE   8
#define AO_ARRAY_SIZE   40
#define AO_ZERO_ARRAY_SIZE 24
#endif

/* specify include files */

#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <stdioLib.h>
#include <dbDefs.h>
#include <wdLib.h>
#include <msgQLib.h>
#include <subRecord.h>
#include <cadRecord.h>
#include <recSup.h>
#include <dbCommon.h>
#include <genSubRecord.h>
#include <timeLib.h>
#include <time.h>

#include "aoP1Lib.h"
#include "synchroMap.h"

typedef struct
{
   double  probeAngle; /* angle of guide probe supplied by Zeiss */
   double  tcsAngle;   /* rotation angle supplied by TCS */
   double  theta;
   double  sinTheta;
   double  cosTheta;
   double  null[AO_ZERO_ARRAY_SIZE];
   SEM_ID  access;
}frame;

/* declare global variables */

extern  double  ttfData[AO_ARRAY_SIZE+2];
extern  double  aoData[AO_ARRAY_SIZE+2];
extern  float   data[AO_ARRAY_SIZE+2];
extern  float   errors[AO_ARRAY_SIZE+2];
extern  frame   *ag2m2;
extern  frame   *ag2tcs;

/* declare prototypes */

char *ttfName[] =
{
    "Time         ",
    "number (np)  ",
    "z2           ",
    "z3           ",
    "z4           ",
    "e2           ",
    "e3           ",
    "e4           ",
    NULL
};

char *aoName[] =
{
    "Time         ",
    "number (np)  ",
    "z2           ",
    "z3           ",
    "z4           ",
    "z5           ",
    "z6           ",
    "z7           ",
    "z8           ",
    "z9           ",
    "z10          ",
    "z11          ",
    "z12          ",
    "z13          ",
    "z14          ",
    "z15          ",
    "z16          ",
    "z17          ",
    "z18          ",
    "z19          ",
    "z20          ",
    "e2           ",
    "e3           ",
    "e4           ",
    "e5           ",
    "e6           ",
    "e7           ",
    "e8           ",
    "e9           ",
    "e10          ",
    "e11          ",
    "e12          ",
    "e13          ",
    "e14          ",
    "e15          ",
    "e16          ",
    "e17          ",
    "e18          ",
    "e19          ",
    "e20          ",
    NULL
};

/* ===================================================================== */

void showTtfNull(void)
{

   printf("theta    = %f (rads)\n", ag2m2->theta);
   printf("sinTheta = %f\n", ag2m2->sinTheta);
   printf("cosTheta = %f\n", ag2m2->cosTheta);
   printf("null z2  = %f\n", ag2m2->null[5]);
   printf("null z3  = %f\n", ag2m2->null[6]);
   printf("null z4  = %f\n", ag2m2->null[7]);
}

void showAoNull(void)
{
    int index = 0;

    printf("theta    = %f (rads)\n", ag2tcs->theta);
    printf("sinTheta = %f\n", ag2tcs->sinTheta);
    printf("cosTheta = %f\n", ag2tcs->cosTheta);

    for(index = 0; index < 19; index++)
    {
       printf("null z%d  = %f\n", (index+2), ag2tcs->null[5 + index]);
    }
}

/* ===================================================================== */

void showTtf(void)
{
   int index = 0;

   for(index = 0; index < TTF_ARRAY_SIZE; index++)
   {
      printf("%s\t%f\n", ttfName[index], ttfData[index]);
   }
}

/* ===================================================================== */

void showAo(void)
{
   int index = 0;

   for(index = 0; index < AO_ARRAY_SIZE; index++)
   {
      printf("%s\t%f\n", aoName[index], aoData[index]);
   }
}

/* ===================================================================== */

void    showSynchro (const memMap * buffPtr)
{
   /* printout the memory locations of the specified buffer area */

   printf ("\nPage 8  - pwfs1 data\n");
   printf ("pwfs1      Addr = %x,           \n", 
           (unsigned int) &buffPtr->pwfs1.z1);

   printf ("z1      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.z1, buffPtr->pwfs1.z1);
   printf ("z2      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.z2, buffPtr->pwfs1.z2);
   printf ("z3      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.z3, buffPtr->pwfs1.z3);
   printf ("err1      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.err1, buffPtr->pwfs1.err1);
   printf ("err2      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.err2, buffPtr->pwfs1.err2);
   printf ("err3      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.err3, buffPtr->pwfs1.err3);
   printf ("interval   Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.interval, buffPtr->pwfs1.interval);
   printf ("time      Addr = %x, Value = %f\n", 
           (unsigned int) &buffPtr->pwfs1.time, buffPtr->pwfs1.time);
   printf ("name      Addr = %x, Value = %s\n", 
           (unsigned int) buffPtr->pwfs1.name, buffPtr->pwfs1.name);

   return;
}

/* ===================================================================== */

void    showTime (void)
{
   int     j, c[7];

   j = timeNowC (TAI, 3, c);
   printf ("status = %d time > %d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d (TAI)\n",
           j, c[0], c[1], c[2], c[3], c[4], c[5], c[6]);
}

/* ===================================================================== */

void    rawTime (void)
{
   int     j;
   double  rawt = 99;

   j = timeNow (&rawt);

   printf ("status = %d rawtime > %f\n", j, rawt);
}
