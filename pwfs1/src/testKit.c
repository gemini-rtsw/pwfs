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
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef MAX_WFS_SOURCES
#define MAX_WFS_SOURCES	5
#define	TTF_ARRAY_SIZE	8
#define AO_ARRAY_SIZE	40
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

#include "osp.h"
#include "synchroMap.h"

typedef struct
{
	double  probeAngle;	/* angle of guide probe supplied by Zeiss */
	double  tcsAngle;	/* rotation angle supplied by TCS */
	double	theta;
	double	sinTheta;
	double	cosTheta;
	double	null[AO_ZERO_ARRAY_SIZE];
	SEM_ID	access;
}frame;

/* declare global variables */

extern	double	ttfData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	double	aoData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	float	data[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	float	errors[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern  frame	*ag2m2[MAX_WFS_SOURCES];
extern  frame	*ag2tcs[MAX_WFS_SOURCES];

struct	OSP_CONTEXT *testp1, *testp2, *testp3;

/* declare prototypes */

STATUS writeWfsToTcs(struct OSP_CONTEXT *pWfs);
STATUS writeWfsToSynchro(struct OSP_CONTEXT *pWfs);

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

long    ttfReceiver (struct genSubRecord * pgsub)
{
	int     index = 0;
	double	rxData[TTF_ARRAY_SIZE];
	double *ptr;

	ptr = (double *) pgsub->j;

	/* read in the array */

	for (index = 0; index < TTF_ARRAY_SIZE; index++)
	{
	    rxData[index] = *(ptr++);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = rxData[0];	/* TAI time */ 
	*(double *) pgsub->valb = rxData[1];	/* Number of coefficients */ 
	*(double *) pgsub->valc = rxData[2];	/* z2 */
	*(double *) pgsub->vald = rxData[3];	/* z3 */
	*(double *) pgsub->vale = rxData[4];	/* z4 */
	*(double *) pgsub->valf = rxData[5];	/* e2 */
	*(double *) pgsub->valg = rxData[6];	/* e3 */
	*(double *) pgsub->valh = rxData[7];	/* e4 */

	return (OK);
}

/* ===================================================================== */

long    aoReceiver (struct genSubRecord * pgsub)
{
	int     index = 0;
	double	rxData[AO_ARRAY_SIZE];
	double *ptr;

	ptr = (double *) pgsub->j;

	/* read in the array */

	for (index = 0; index < AO_ARRAY_SIZE; index++)
	{
	    rxData[index] = *(ptr++);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = rxData[0];	/* TAI time */ 
	*(double *) pgsub->valb = rxData[1];	/* Number of coefficients */ 
	*(double *) pgsub->valc = rxData[2];	/* z2 */
	*(double *) pgsub->vald = rxData[3];	/* z3 */
	*(double *) pgsub->vale = rxData[4];	/* z4 */
	*(double *) pgsub->valf = rxData[5];	/* e2 */
	*(double *) pgsub->valg = rxData[6];	/* e3 */
	*(double *) pgsub->valh = rxData[7];	/* e4 */

	return (OK);
}

/* ===================================================================== */

struct OSP_CONTEXT * createWfsSource (int wfsSource)
{
	struct	OSP_CONTEXT *ptest;
	int i = 0;

	if(wfsSource > (MAX_WFS_SOURCES - 1))
	{
		printf("wfsSource %d out of range\n", wfsSource);
		return(NULL);
	}

	ptest = (struct OSP_CONTEXT *) malloc (sizeof (struct OSP_CONTEXT));

	if(ptest == NULL)
	{
		printf("malloc failure\n");
		return(NULL);
	}

	ptest->z = (float *)data[wfsSource];
	ptest->err = (float *)errors[wfsSource];

	ptest->time = 666.0;
	ptest->np = 19;

	for(i = 1; i <= ptest->np; i++)
	{
		ptest->z[i] = 199.0;
		ptest->err[i] = 299.00;
	}

	ptest->wfsSource = wfsSource;

	/* trace message */

	printf("in createWfsSource ptest = %p, wfsSource = %d\n", ptest, ptest->wfsSource);

	return(ptest);
}

/* ===================================================================== */

STATUS fillWfsSource (struct OSP_CONTEXT *ptest, double value)
{
	int i = 0;

	if(ptest == NULL)
	{
		printf("passed pointer is null\n");
		return(ERROR);
	}

	ptest->time = value;
	ptest->np = 19;

	for(i = 1; i <= 19; i++)
	{
		ptest->z[i] = (float)(value+i-1);
		ptest->err[i] = (float)(value+i + 18) ;
	}

	return(OK);
}

/* ===================================================================== */

STATUS wfsDummy(void)
{
	/* pretend to be steven's wfs processing */
	
	if(testp1 == NULL)
		testp1 = createWfsSource(PWFS1);

	if(testp2 == NULL)
		testp2 = createWfsSource(PWFS2);

	if(testp3 == NULL)
		testp3 = createWfsSource(OIWFS);

	/* at each pass increment the data values */

	fillWfsSource(testp1, 1.0);
	fillWfsSource(testp2, 10.0);
	fillWfsSource(testp3, 100.0);

	/* write updated data to Tcs */

	writeWfsToTcs(testp1);
	writeWfsToTcs(testp2);
	writeWfsToTcs(testp3);

	/* write updated data to Synchro */

	writeWfsToSynchro(testp1);
	writeWfsToSynchro(testp2);
	writeWfsToSynchro(testp3);

	return(OK);
}
/* ===================================================================== */

void showTtfNull(int source)
{
	if(source >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", source);
		return;
	}

	printf("wfs %d\n\n", source);
	printf("theta    = %f (rads)\n", ag2m2[source]->theta);
	printf("sinTheta = %f\n", ag2m2[source]->sinTheta);
	printf("cosTheta = %f\n", ag2m2[source]->cosTheta);
	printf("null z2  = %f\n", ag2m2[source]->null[5]);
	printf("null z3  = %f\n", ag2m2[source]->null[6]);
	printf("null z4  = %f\n", ag2m2[source]->null[7]);
}

void showAoNull(int source)
{
    int index = 0;

	if(source >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", source);
		return;
	}

	printf("wfs %d\n\n", source);
	printf("theta    = %f (rads)\n", ag2tcs[source]->theta);
	printf("sinTheta = %f\n", ag2tcs[source]->sinTheta);
	printf("cosTheta = %f\n", ag2tcs[source]->cosTheta);

    for(index = 0; index < 19; index++)
    {
	printf("null z%d  = %f\n", (index+2), ag2tcs[source]->null[5 + index]);
    }
}

/* ===================================================================== */

void showTtf(int wfsNumber)
{
	int index = 0;

	if(wfsNumber >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", wfsNumber);
		return;
	}
	printf("wfs %d\n\n", wfsNumber);

	for(index = 0; index < TTF_ARRAY_SIZE; index++)
	{
		printf("%s\t%f\n", ttfName[index], ttfData[wfsNumber][index]);
	}
}

/* ===================================================================== */

void showAo(int wfsNumber)
{
	int index = 0;

	if(wfsNumber >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", wfsNumber);
		return;
	}

	printf("wfs %d\n\n", wfsNumber);

	for(index = 0; index < AO_ARRAY_SIZE; index++)
	{
		printf("%s\t%f\n", aoName[index], aoData[wfsNumber][index]);
	}
}

/* ===================================================================== */

void    testMem (const memMap * buffPtr)
{
	/* printout the memory locations of the specified buffer area */

	printf ("\nPage 0 - SCS to M2 commands\n");

	printf ("checksum       Addr = %x, Value = %d\n", (unsigned int)&buffPtr->page0.checksum, (int)buffPtr->page0.checksum);
	printf ("NS             Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.NS, (int)buffPtr->page0.NS);
	printf ("command        Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.commandCode, (int)buffPtr->page0.commandCode);
	printf ("xtiltguide     Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xTiltGuide, buffPtr->page0.xTiltGuide);
	printf ("ytiltguide     Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yTiltGuide, buffPtr->page0.yTiltGuide);
	printf ("zfocusguide    Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.zFocusGuide, buffPtr->page0.zFocusGuide);
	printf ("axtilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.AxTilt, buffPtr->page0.AxTilt);
	printf ("aytilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.AyTilt, buffPtr->page0.AyTilt);
	printf ("bxtilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.BxTilt, buffPtr->page0.BxTilt);
	printf ("bytilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.ByTilt, buffPtr->page0.ByTilt);
	printf ("cxtilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.CxTilt, buffPtr->page0.CxTilt);
	printf ("cytilt         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.CyTilt, buffPtr->page0.CyTilt);
	printf ("actuator1      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.actuator1, buffPtr->page0.actuator1);
	printf ("actuator2      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.actuator2, buffPtr->page0.actuator2);
	printf ("actuator3      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.actuator3, buffPtr->page0.actuator3);
	printf ("heartbeat      Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.heartbeat, (int)buffPtr->page0.heartbeat);
	printf ("xDemand        Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xDemand, buffPtr->page0.xDemand);
	printf ("yDemand        Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yDemand, buffPtr->page0.yDemand);
	printf ("central        Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.centralBaffle, (int)buffPtr->page0.centralBaffle);
	printf ("deployable     Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.deployBaffle, (int)buffPtr->page0.deployBaffle);
	printf ("profile        Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page0.chopProfile, (int)buffPtr->page0.chopProfile);
	printf ("frequency      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.chopFrequency, buffPtr->page0.chopFrequency);
	printf ("dutycycle      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.chopDutyCycle, buffPtr->page0.chopDutyCycle);
	printf ("xtilttol       Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xTiltTolerance, buffPtr->page0.xTiltTolerance);
	printf ("ytilttol       Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yTiltTolerance, buffPtr->page0.yTiltTolerance);
	printf ("zfocustol      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.zFocusTolerance, buffPtr->page0.zFocusTolerance);
	printf ("xpostol        Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xPositionTolerance, buffPtr->page0.xPositionTolerance);
	printf ("ypostol        Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xPositionTolerance, buffPtr->page0.xPositionTolerance);
	printf ("bandwidth      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.bandwidth, buffPtr->page0.bandwidth);
	printf ("xtiltgain      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xTiltGain, buffPtr->page0.xTiltGain);
	printf ("ytiltgain      Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yTiltGain, buffPtr->page0.yTiltGain);
	printf ("zfocusgain     Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.zFocusGain, buffPtr->page0.zFocusGain);
	printf ("xtiltshift     Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xTiltShift, buffPtr->page0.xTiltShift);
	printf ("ytiltshift     Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yTiltShift, buffPtr->page0.yTiltShift);
	printf ("zfocusshift    Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.zFocusShift, buffPtr->page0.zFocusShift);
	printf ("xtiltsmooth    Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.xTiltSmooth, buffPtr->page0.xTiltSmooth);
	printf ("ytiltsmooth    Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.yTiltSmooth, buffPtr->page0.yTiltSmooth);
	printf ("zfocussmooth   Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page0.zFocusSmooth, buffPtr->page0.zFocusSmooth);

	printf ("\nPage 1 - M2 to SCS responses\n");

	printf ("checksum	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.checksum, (int)buffPtr->page1.checksum);
	printf ("NR		Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.NR, (int)buffPtr->page1.NR);
	printf ("xtilt		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.xTilt, buffPtr->page1.xTilt);
	printf ("ytilt		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.yTilt, buffPtr->page1.yTilt);
	printf ("zfocus		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.zFocus, buffPtr->page1.zFocus);
	printf ("actuator1	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.actuator1, buffPtr->page1.actuator1);
	printf ("actuator2	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.actuator2, buffPtr->page1.actuator2);
	printf ("actuator3	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.actuator3, buffPtr->page1.actuator3);
	printf ("inPosition	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.inPosition, (int)buffPtr->page1.inPosition);
	printf ("chopTrans	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.chopTransition, (int)buffPtr->page1.chopTransition);
	printf ("statusword	Addr = %x, Value = %x\n", (unsigned int) &buffPtr->page1.statusWord.all, buffPtr->page1.statusWord.all);
	printf ("heartbeat	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.heartbeat, (int)buffPtr->page1.heartbeat);
	printf ("beamPosition	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.beamPosition, (int)buffPtr->page1.beamPosition);
	printf ("xposition	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.xPosition, buffPtr->page1.xPosition);
	printf ("yposition	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.yPosition, buffPtr->page1.yPosition);
	printf ("deployable	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.deployBaffle, (int)buffPtr->page1.deployBaffle);
	printf ("central	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.centralBaffle, (int)buffPtr->page1.centralBaffle);
	printf ("encoderA	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.baffleEncoderA, buffPtr->page1.baffleEncoderA);
	printf ("encoderB	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.baffleEncoderB, buffPtr->page1.baffleEncoderB);
	printf ("encoderC	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.baffleEncoderC, buffPtr->page1.baffleEncoderC);
	printf ("topEnd		Addr = %x, Value = %d\n", (unsigned int) &buffPtr->page1.topEnd, (int)buffPtr->page1.topEnd);
	printf ("temperature	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->page1.enclosureTemp, buffPtr->page1.enclosureTemp);

	printf ("\nPage 2 - M2 Diagnostics Data\n");
	printf ("Diagnostics	Addr = %x,           \n", (unsigned int) &buffPtr->testResults.checksum);

	printf ("\nPage 7 - Event System Data\n");
	printf ("currentBeam	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->eventData.currentBeam, (int)buffPtr->eventData.currentBeam);
	printf ("inPosition	Addr = %x, Value = %d\n", (unsigned int) &buffPtr->eventData.inPosition, (int)buffPtr->eventData.inPosition);
	printf ("xTilt		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.xTilt, buffPtr->eventData.xTilt);
	printf ("yTilt		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.yTilt, buffPtr->eventData.yTilt);
	printf ("zFocus		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.zFocus, buffPtr->eventData.zFocus);
	printf ("xPosition	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.xPosition, buffPtr->eventData.xPosition);
	printf ("yPosition	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.yPosition, buffPtr->eventData.yPosition);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->eventData.time, buffPtr->eventData.time);

	printf ("\nPage 8  - pwfs1 data\n");
	printf ("pwfs1		Addr = %x,           \n", (unsigned int) &buffPtr->pwfs1.z1);

	printf ("z1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.z1, buffPtr->pwfs1.z1);
	printf ("z2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.z2, buffPtr->pwfs1.z2);
	printf ("z3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.z3, buffPtr->pwfs1.z3);
	printf ("err1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.err1, buffPtr->pwfs1.err1);
	printf ("err2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.err2, buffPtr->pwfs1.err2);
	printf ("err3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.err3, buffPtr->pwfs1.err3);
	printf ("interval	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.interval, buffPtr->pwfs1.interval);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs1.time, buffPtr->pwfs1.time);
	printf ("name		Addr = %x, Value = %s\n", (unsigned int) buffPtr->pwfs1.name, buffPtr->pwfs1.name);

	printf ("\nPage 9  - pwfs2 data\n");
	printf ("pwfs2		Addr = %x,           \n", (unsigned int) &buffPtr->pwfs2.z1);
	printf ("z1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.z1, buffPtr->pwfs2.z1);
	printf ("z2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.z2, buffPtr->pwfs2.z2);
	printf ("z3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.z3, buffPtr->pwfs2.z3);
	printf ("err1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.err1, buffPtr->pwfs2.err1);
	printf ("err2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.err2, buffPtr->pwfs2.err2);
	printf ("err3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.err3, buffPtr->pwfs2.err3);
	printf ("interval	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.interval, buffPtr->pwfs2.interval);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->pwfs2.time, buffPtr->pwfs2.time);
	printf ("name		Addr = %x, Value = %s\n", (unsigned int) buffPtr->pwfs2.name, buffPtr->pwfs2.name);

	printf ("\nPage 10 - oiwfs data\n");
	printf ("oiwfs		Addr = %x,           \n", (unsigned int) &buffPtr->oiwfs.z1);
	printf ("z1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.z1, buffPtr->oiwfs.z1);
	printf ("z2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.z2, buffPtr->oiwfs.z2);
	printf ("z3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.z3, buffPtr->oiwfs.z3);
	printf ("err1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.err1, buffPtr->oiwfs.err1);
	printf ("err2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.err2, buffPtr->oiwfs.err2);
	printf ("err3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.err3, buffPtr->oiwfs.err3);
	printf ("interval	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.interval, buffPtr->oiwfs.interval);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->oiwfs.time, buffPtr->oiwfs.time);
	printf ("name		Addr = %x, Value = %s\n", (unsigned int) buffPtr->oiwfs.name, buffPtr->oiwfs.name);

	printf ("\nPage 11 - gaos data\n");
	printf ("gaos		Addr = %x,           \n", (unsigned int) &buffPtr->gaos.z1);
	printf ("z1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.z1, buffPtr->gaos.z1);
	printf ("z2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.z2, buffPtr->gaos.z2);
	printf ("z3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.z3, buffPtr->gaos.z3);
	printf ("err1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.err1, buffPtr->gaos.err1);
	printf ("err2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.err2, buffPtr->gaos.err2);
	printf ("err3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.err3, buffPtr->gaos.err3);
	printf ("interval	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.interval, buffPtr->gaos.interval);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gaos.time, buffPtr->gaos.time);
	printf ("name		Addr = %x, Value = %s\n", (unsigned int) buffPtr->gaos.name, buffPtr->gaos.name);

	printf ("\nPage 12 - gyro data\n");
	printf ("gyro		Addr = %x,           \n", (unsigned int) &buffPtr->gyro.z1);
	printf ("z1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.z1, buffPtr->gyro.z1);
	printf ("z2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.z2, buffPtr->gyro.z2);
	printf ("z3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.z3, buffPtr->gyro.z3);
	printf ("err1		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.err1, buffPtr->gyro.err1);
	printf ("err2		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.err2, buffPtr->gyro.err2);
	printf ("err3		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.err3, buffPtr->gyro.err3);
	printf ("interval	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.interval, buffPtr->gyro.interval);
	printf ("time		Addr = %x, Value = %f\n", (unsigned int) &buffPtr->gyro.time, buffPtr->gyro.time);
	printf ("name		Addr = %x, Value = %s\n", (unsigned int) buffPtr->gyro.name, buffPtr->gyro.name);

	printf ("\nPage 13 - M2 Engineering Data\n");

	printf ("follow1	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.follow1, buffPtr->m2Eng.follow1);
	printf ("follow2	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.follow2, buffPtr->m2Eng.follow2);
	printf ("follow3	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.follow3, buffPtr->m2Eng.follow3);
	printf ("current1	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.current1, buffPtr->m2Eng.current1);
	printf ("current2	Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.current2, buffPtr->m2Eng.current2);
	printf ("current3       Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.current3, buffPtr->m2Eng.current3);
	printf ("kaman1         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.kaman1, buffPtr->m2Eng.kaman1);
	printf ("kaman2         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.kaman2, buffPtr->m2Eng.kaman2);
	printf ("kaman3         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.kaman3, buffPtr->m2Eng.kaman3);
	printf ("integ1         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.integ1, buffPtr->m2Eng.integ1);
	printf ("integ2         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.integ2, buffPtr->m2Eng.integ2);
	printf ("integ3         Addr = %x, Value = %f\n", (unsigned int) &buffPtr->m2Eng.integ3, buffPtr->m2Eng.integ3);
}

/* ===================================================================== */

void    showTime (void)
{
	int     j, c[7];

	j = timeNowC (TAI, 3, c);
	printf ("status = %d time > %d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d (TAI)\n", j, c[0], c[1], c[2], c[3], c[4], c[5], c[6]);
}

/* ===================================================================== */

void    rawTime (void)
{
	int     j;
	double  rawt = 99;

	j = timeNow (&rawt);

	printf ("status = %d rawtime > %f\n", j, rawt);
}





























