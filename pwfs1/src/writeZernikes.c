/* Id:$ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * writeZernikes.c
 * 
 * PURPOSE
 * -------
 * Take zernike data from the wavefront processing section and make available
 * on the synchro bus and to the TCS
 * 
 * FUNCTION NAME(S)
 * ----------------
 * gensubToTcsInit   - Initialisation semaphores
 * gensubToTcsTtf    - Write ttf data from global array to port VALJ
 * gensubToTcsAo     - Write ao data from global array to port VALJ
 * writeWfsToTcs     - Write data from designated structure to global array
 *                     and low pass filter
 * writeWfsToSynchro - Write data from designated structure to synchro bus
 * dfilter           - low pass filter
 * ttfZero           - Receive ttfZero array from TCS
 * aoZero            - Receive aoZero array from TCS
 * showAoDiags       - Write diagnostic data from ao structure to gensub
 *                     outputs for display
 * showFgDiags       - Write diagnostic data from fg structure to gensub
 *                     outputs for display
 * gensubFanDouble   - receive array of doubles on port A, write elements to
 *                     individual output ports
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
 * 29-Oct-1998: Array indexing for wfs structures is 1 ->np, not 0 -> (np-1)
 * 11-Nov-1998: Add frame of reference conversion
 * 05-Jan-1999: Include nulling zernike values from ttfZero and aoZero
 * 12-Jan-1999: Add showAoDiags and showFgDiags function
 * 13-Jan-1999: Modify gensubToTcsTtf to write zernikes and errors to output 
 *              ports
 * 21-Jan-1999: Modify to continue initialisation even if synchro bus
 *              not present then inhibit synchro writes accordingly. Also 
 *              initialise time written to synchro to zero until format sorted 
 *              out, increment time count for each write to the synchro bus 
 *              as temporary measure.
 * 22-Jan-1999: Add function to read guide probe angle in ttfZero and aoZero.
 * 23-Jan-1999: Modify gensubToTcsAo to write arrays of zernikes and errors 
 *              to outputs a and b
 *              Add function gensubFanDoubles to read array of doubles and 
 *              write elements to the output ports
 * 24-Jan-1999: ttfZero and aoZero - read ports B and C for fudge factors 
 *              in polarity and rotation
 * 10-Feb-1999: cb - add check max/min TT and focus to writeWfsToSynchro()
 * 17-Feb-1999: cb - ttfZero change computation of theta for TCS and SCS
 * 26-Apr-1999: cb - replace in writeWfsToSynchro z[] by FGZernikes[]
 * 26-Apr-1999: cb - simplified version for split backplane PWFS1
 * 18-Nov-1999: cb - add in aoZero, contribution from the cass rotator angle
 * 26-Nov-1999: cb - modify writeWfsToSynchro to update interval as for P2
 * 24-Apr-2000: cb - Majpr modifications new aoP1Lib library
 *
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef PI
#define PI 3.14159265358979
#endif

#define MAX_WFS_SOURCES      5
#define TTF_ARRAY_SIZE       8
#define TTF_ZERO_ARRAY_SIZE  9
#define AO_ZERO_ARRAY_SIZE   24
#define AO_ARRAY_SIZE        40
#define MAX_FILTERS          (3 * MAX_WFS_SOURCES)
#define WFS_TIMEOUT          40
#define DEGS2RADS            ((double)(2.0*PI)/(double)360.0)   
                                 /* conversion factor for degrees to radians  */
#define DISCARD_THRESHOLD    60
#define LOW_PROBE_ANGLE      -360.0   
                                 /* high limit on guide probe angle (degrees) */
#define HIGH_PROBE_ANGLE     360.0   
                                 /* low limit on guide probe angle (degrees)  */
#define MAX_TT_M2            12.5   
                                 /* max tip/tilt for AO correction (arcsec)   */
#define MIN_TT_M2            -12.5   
                                 /* min tip/tilt for AO correction (arcsec)   */
#define MAX_FOCUS_M2         0.84   
                                 /* max focus for AO correction (microns)     */
#define MIN_FOCUS_M2         -0.84   
                                 /* min focus for AO correction (microns)     */
#define MICRON2MM            1.0e-3  
                                 /* conversion factor for microns to mm       */

/* specify include files */

#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <logLib.h>
#include <stdioLib.h>
#include <dbDefs.h>
#include <wdLib.h>
#include <msgQLib.h>
#include <string.h>
#include <logLib.h>
#include <subRecord.h>
#include <cadRecord.h>
#include <recSup.h>
#include <dbCommon.h>
#include <genSubRecord.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <gemTypes.h>

#include "aoP1Lib.h"
#include "synchroMap.h"

typedef struct
{
   double  probeAngle; /* angle of guide probe supplied by Zeiss */
   double  tcsAngle;   /* rotation angle supplied by TCS */
   double  theta;
   double  sinTheta;
   double  cosTheta;
   double  sin2Theta;
   double  cos2Theta;
   double  sin3Theta;
   double  cos3Theta;
   double  sin4Theta;
   double  cos4Theta;
   double  null[AO_ZERO_ARRAY_SIZE];
   SEM_ID  access;
}frame;

typedef struct
{
   double   z2;
   double   z3;
   double   z4;
   double   z5;
   double   z6;
   double   z7;
   double   z8;
   double   z9;
   double   z10;
   double   z11;
   double   z12;
   double   z13;
   double   z14;
   double   z15;
   double   z16;
   double   z17;
   double   z18;
   double   z19;
   double   z20;
}converted;

/* declare global variables */

frame   *ag2m2;
frame   *ag2tcs;
wfs     *ptrPwfs1;
double  ttfData[AO_ARRAY_SIZE+2];
double  aoData[AO_ARRAY_SIZE+2];
float   data[AO_ARRAY_SIZE+2];
float   errors[AO_ARRAY_SIZE+2];
SEM_ID  wfsLock;

WFS_VECT localCentroidsVect;
WFS_VECT localTotalCountsVect;
WFS_VECT localFgCentroidsVect;
WFS_VECT localFgTotalCountsVect;
SEM_ID   accessAoData;
SEM_ID   accessFgData;

AO_CCD_ID aoCcdIdP1;
AO_CB_CTRL_ID aoCbCtrlIdP1;
AO_CB_FG_CTRL_ID aoCbFgCtrlIdP1;
AO_CB_IM_ID   aoCbImIdP1;

/* declare prototypes */

long rmIntSend(int interrupt, int node);

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * dfilter
 *
 * INVOCATION:
 * double newSample
 * int Id
 *
 * double   dfilter(double newSample, int Id)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > double newSample       - latest data sample
 * > int    iD              - identification of filter bank. There are three 
 *                            filters per wfs source hence
 *
 *                            source     xtilt       ytilt        focus
 *                            -----------------------------------------
 *                            HRWFS        0           1            2
 *                            PWFS1        3           4            5
 *                            PWFS2        6           7            8
 *                            OIWFS        9           10           11
 *                            AOWFS        12          13           14
 *
 * FUNCTION VALUE:
 * double     returns current filtered value
 *
 * PURPOSE:
 * Filter the data in accordance with the IIR filter coefficients specified
 *
 * DESCRIPTION:
 * The function performs a two pole low pass butterworth filter on the supplied
 * data. A history array is maintained for each bank identified by the index Id.
 * The cutoff frequency is approximately 0.05*Fsample hence for a 200Hz update
 * the spectral content above 10Hz is substantially attenuated suitable for
 * sub-sampling at 20Hz.
 *
 * EXTERNAL VARIABLES:
 * 
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 *
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version - Sean Prior
 *-
 */

double dfilter
   (
   double newSample, 
   int Id
   )
{
   int i = 0;
   double sum = 0;
   static double sample[6][MAX_FILTERS];

   static double coeffs[] = {
      1.0,
      1.77863177782458,
      -0.80080264666571,
      0.00554271721028,
      0.01108543442056,
      0.00554271721028
   };

   /* put new sample into the array */

   sample[3][Id] = newSample;

   /* multiply samples by coefficients and accumulate */

   for(i=1; i < 6; i++)
      sum += sample[i][Id]*coeffs[i];

   /* ripple samples ready for next call */

   sample[5][Id] = sample[4][Id];
   sample[4][Id] = sample[3][Id];
   sample[2][Id] = sample[1][Id];
   sample[1][Id] = sum;

   return(sum);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsInit
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long gensubToTcsInit(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Initialise structures and semaphores for wfs processing
 *
 * DESCRIPTION:
 * Detects the presence of the synchro bus card at the expected address
 * Create array of mutex semaphores corresponding to each of the wfs sources
 * Create structures to hold coordinate conversion transformations
 * Assign pointers to synchro bus pages for each wfs source
 *
 * EXTERNAL VARIABLES:
 * wfsLock   - Global mutex semaphore
 * ag2m2     - pointer to coord conversion structures
 * ag2tcs    - pointer to coord conversion structures
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998: Original version (srp)
 * 05-Jan-1999: Put all initialisation and semaphore creation in this section 
 *              rather than creating as necessary during operation
 * 22-Jan-1999: Initialise time values on synchro bus to 0.0
 * 26-Apr-1999: Simplified version for split backplane PWFS1 (cb)
 * 26-Nov-1999: Update interval as for PWFS2 (cb)
 *-
 */

long gensubToTcsInit
   (
   struct genSubRecord * pgsub
   )
{
   memMap   *basePtr = (memMap *)SYNCHROBASE;
   static   int processedFlag = FALSE;
   char     junk;

   /* this initialisation routine only needs to be called once */

   if(processedFlag != FALSE)
      return(OK);
   else
      processedFlag = TRUE;


   /* create semaphore to prevent multiple access to wfs data */

   if(wfsLock == NULL)
   {
      if ((wfsLock = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create wfsLock sem\n");
      }
   }

   /* create semaphore to prevent multiple access to ao data */

   if(accessAoData == NULL)
   {
      if ((accessAoData = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessAoData sem\n");
      }
   }

   /* create semaphore to prevent multiple access to FG data */

   if(accessFgData == NULL)
   {
      if ((accessFgData = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessFgData sem\n");
      }
   }

   /* create structure holding angle and null values for ao data */

   if((ag2tcs = (frame *)calloc(1, sizeof(frame))) == NULL)
   {
      logMsg("Unable to calloc conversion frame \n", 0, 0, 0, 0, 0, 0);
   }
   else
   {
       if ((ag2tcs->access = 
           semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
           == NULL)
       {
          logMsg("Unable to create mutex for conversion frame\n", 
                 0, 0, 0, 0, 0 ,0);
       }
       else
       {
          /* initialise trig values */

          ag2tcs->probeAngle = 0.0;
          ag2tcs->tcsAngle = 0.0;
          ag2tcs->theta   = 0.0;
          ag2tcs->sinTheta = sin(0.0);
          ag2tcs->cosTheta = cos(0.0);
          ag2tcs->sin2Theta = sin(0.0);
          ag2tcs->cos2Theta = cos(0.0);
          ag2tcs->sin3Theta = sin(0.0);
          ag2tcs->cos3Theta = cos(0.0);
          ag2tcs->sin4Theta = sin(0.0);
          ag2tcs->cos4Theta = cos(0.0);
       }
   }

   /* create structure holding angle and null values for ttf data */

   if( (ag2m2 = (frame *)calloc(1, sizeof(frame))) == NULL)
   {
      logMsg("Unable to calloc conversion frame \n", 0, 0, 0, 0, 0, 0);
   }
   else
   {
       if ((ag2m2->access = 
            semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
           == NULL)
       {
          logMsg("Unable to create mutex for conversion frame\n", 
                 0, 0, 0, 0, 0 ,0);
       }
       else
       {
          /* initialise trig values */

          ag2m2->probeAngle = 0.0;
          ag2m2->tcsAngle = 0.0;
          ag2m2->theta   = 0.0;
          ag2m2->sinTheta = sin(0.0);
          ag2m2->cosTheta = cos(0.0);
          ag2m2->sin2Theta = sin(0.0);
          ag2m2->cos2Theta = cos(0.0);
          ag2m2->sin3Theta = sin(0.0);
          ag2m2->cos3Theta = cos(0.0);
          ag2m2->sin4Theta = sin(0.0);
          ag2m2->cos4Theta = cos(0.0);
       }
   }

   /* verify presence of 5588 synchro card */

   if (vxMemProbe ((void *)basePtr, VX_READ, 1, &junk) != OK)    
   {
      logMsg("synchro card not detected at address %p\n", 
             (int)basePtr, 0, 0, 0, 0, 0);
      basePtr = NULL;
      return(ERROR);
   }

   /* if synchro card present, initialise structure pointers */

   /* assign pointers and write ID strings for synchro bus */

   if(ptrPwfs1 == NULL)
   {
      ptrPwfs1 = (wfs*)&basePtr->pwfs1;
      strncpy(ptrPwfs1->name, "pwfs1", 15);
      ptrPwfs1->time = 0.0;
      ptrPwfs1->interval = 0.0;
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsTtf
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long gensubToTcsTtf(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy data from the global ao data arrays to the VALJ port for reading 
 * by the TCS
 *
 * DESCRIPTION:
 * Mutex access to the global ao data array is then attempted and data is 
 * copied out to the VALJ port. Status return will be bad if there is a timeout 
 * on mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock       - mutex semaphore
 * ttfData       - ttf data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version - Sean Prior
 * 13-Jan-1999: Write zernikes and errors to outputs for screen display (srp)
 * 26-Apr-1999: Simplified version for split backplane PWFS1 (cb)
 *-
 */

long gensubToTcsTtf 
   (
   struct genSubRecord * pgsub
   )
{
   /* write array to TCS system */

   if(semTake(wfsLock, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access wfsLock\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      memcpy (pgsub->valj, ttfData, TTF_ARRAY_SIZE * sizeof (double));

      /* also write values to gensub outputs for screen display */

      *(double *)pgsub->vala = ttfData[8];   /* z2 */
      *(double *)pgsub->valb = ttfData[9];   /* z3 */
      *(double *)pgsub->valc = ttfData[10];  /* z4 */
      *(double *)pgsub->vald = ttfData[5];   /* e2 */
      *(double *)pgsub->vale = ttfData[6];   /* e3 */
      *(double *)pgsub->valf = ttfData[7];   /* e4 */

      semGive(wfsLock);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsAo
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long gensubToTcsAo(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy data from the global ao data arrays to the VALJ port for reading by 
 * the TCS
 *
 * DESCRIPTION:
 * Mutex access to the global ao data is then attempted and data is copied 
 * out to the VALJ port. Status return will be returned bad if there is a 
 * timeout on mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock       - mutex semaphores
 * aoData        - ao data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version Sean Prior
 * 23-Jan-1999  Write arrays of zernikes and errors to vala and valb
 *              to be picked up and displayed by other gensubs (srp)
 * 26-Apr-1999  Simplified version for split backplane PWFS1 (cb)
 *-
 */

long gensubToTcsAo 
   (
   struct genSubRecord * pgsub
   )
{
   int index = 0;
   double zernikes[19];
   double errors[19];

   /* write array to TCS system */

   if(semTake(wfsLock, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access wfsLock\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      for(index = 0; index < 19; index++)
      {
         zernikes[index] = aoData[index+2];
         errors[index] = aoData[index+21];
      }

      /* write whole array to valj for the TCS to pick up */

      memcpy (pgsub->valj, aoData, AO_ARRAY_SIZE * sizeof (double));

      /* write Zernike values to vala for display */

      memcpy (pgsub->vala, zernikes, 19 * sizeof (double));

      /* write error values to valb for display */

      memcpy (pgsub->valb, errors, 19 * sizeof (double));

      semGive(wfsLock);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * writeWfsToTcs
 *
 * INVOCATION:
 * AO_CTRL_ID aoCtrlId
 * double *pAoVect
 * double *pAoErrorsVect
 * double *pTime
 * long   STATUS;
 *
 * STATUS writeWfsToTcs(AO_CTRL_ID aoCtrlId, double *pAoVect,
 *                      double *pAoErrorsVect, double *pTime
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId    - Pointer to the AO control context structure
 * > double * pAoVect       - Vector containing the zernike modes
 * > double * pAoErrorsVect - Vector containing the associated errors
 * > double * pTime         - Pointer to the associated time stamp value

 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Convert values to the TCS required frame of reference and write to global
 * arrays ready for the gensubs to pick up and transmit to the TCS.
 *
 * DESCRIPTION:
 * First the zernikes are retrieved and rotated where necessary to the 
 * TCS frame of reference.
 *
 * EXTERNAL VARIABLES:
 * wfsLock       - Global mutex semaphores
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * Pointer to the ao data are given.
 * The contents of these vectors is not protected by mutex so there is a 
 * requirement placed on the calling task that the contents of these vectors 
 * do not change for the duration of this routine.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version (srp)
 * 11-Nov-1998  Add frame of reference conversion
 * 05-Jan-1999  Add null zernike calculation
 * 23-Apr-1999  Simplified version for split backplane PWFS1 (cb)
 */

STATUS writeWfsToTcs
   (
   AO_CTRL_ID aoCtrlId,
   double     *pAoVect,
   double     *pAoErrorsVect,
   double     *pTime
   )
{
   int       i=0;
   frame     *f;
   converted result;
   double    *pz;

   /* check that array counts are within limits */

   if(aoCtrlId->aoModeNb > AO_MODE_NB)
   {
       logMsg("zernike count np = %d out of limits\n", 
              (int)aoCtrlId->aoModeNb, 0, 0, 0, 0, 0);
       return(ERROR);
   }

   /* access frame */

   pz = pAoVect;
   f = ag2tcs;

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* first rotate the tip and tilt values to the tcs frame of reference */

      /* tip and tilt: r * cos(t) and r * sin(t) */

      result.z2 = (f->cosTheta*(*pz) - f->sinTheta*(*(pz+1)));
      result.z3 = (f->sinTheta*(*pz) + f->cosTheta*(*(pz+1)));

      /* focus : 2*r^2 -1 */

      result.z4 = *(pz+2);

      /* astig0 and astig45: r^2 * cos(2t) and r^2 * sin(2t) */

      result.z5 = (f->cos2Theta*(*(pz+3)) - f->sin2Theta*(*(pz+4))) 
                  - (f->null[8])*1000.0;
      result.z6 = (f->sin2Theta*(*(pz+3)) + f->cos2Theta*(*(pz+4))) 
                  - (f->null[9])*1000.0;

      /* comaX and comaY: (3*r^2 - 2) * r * cos(t) and 
         (3*r^2 - 2) * r * sin(t) */

      result.z7 = (f->cosTheta*(*(pz+5)) - f->sinTheta*(*(pz+6)));
      result.z8 = (f->sinTheta*(*(pz+5)) + f->cosTheta*(*(pz+6)));

      /* spherical: 6*r^4 - 6*r^2 + 1 */
      result.z9 = (*(pz+7));

      /* trefoilX and trefoilY: r^3 * cos(3t) and r^3 * sin(3t) */
      result.z10 = (f->cos3Theta*(*(pz+8)) - f->sin3Theta*(*(pz+9)));
      result.z11 = (f->sin3Theta*(*(pz+8)) + f->cos3Theta*(*(pz+9)));

      /* (4*r^2-3) * r^2 * cos(2t) and (4*r^2-3) * r^2 * sin(2t) */
      result.z12 = (f->cos2Theta*(*(pz+10)) - f->sin2Theta*(*(pz+11)));
      result.z13 = (f->sin2Theta*(*(pz+10)) + f->cos2Theta*(*(pz+11)));

      /* (10*r^4 -12*r^3 + 3) * r * cos(t) and 
         (10*r^4 -12*r^3 + 3) * r * sin(t) */
      result.z14 = (f->cosTheta*(*(pz+12)) - f->sinTheta*(*(pz+13)));
      result.z15 = (f->sinTheta*(*(pz+12)) + f->cosTheta*(*(pz+13)));

      /* 20*r^6 - 30*r^4 + 12*r^2 - 1 */
      result.z16 = (*(pz+14));

      /* r^4 * cos(4t) and r^4 * sin(4t) */
      result.z17 = (f->cos4Theta*(*(pz+15)) - f->sin4Theta*(*(pz+16)));
      result.z18 = (f->sin4Theta*(*(pz+15)) + f->cos4Theta*(*(pz+16)));

      /* (5*r^2 - 4) * r^3 * cos(3t) and (5*r^2 - 4) * r^3 * cos(3t) */
      result.z19 = (f->cos3Theta*(*(pz+17)) - f->sin3Theta*(*(pz+18)));
      result.z20 = (f->sin3Theta*(*(pz+17)) + f->cos3Theta*(*(pz+18)));

      semGive(f->access);
   }
   else
   {
      logMsg("writeWfsToTcs - unable to get mutex for conversion frame\n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* take mutex semaphore to gain access to wfs arrays */

   if(semTake(wfsLock, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access wfsLock\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* fill ao data array */

      aoData[0] = (double)(*pTime);               /* time */
      aoData[1] = (double)(aoCtrlId->aoModeNb);   /* number of coefficients */

      aoData[2] = result.z2;
      aoData[3] = result.z3;
      aoData[4] = result.z4;
      aoData[5] = result.z5;
      aoData[6] = result.z6;
      aoData[7] = result.z7;
      aoData[8] = result.z8;
      aoData[9] = result.z9;
      aoData[10] = result.z10;
      aoData[11] = result.z11;
      aoData[12] = result.z12;
      aoData[13] = result.z13;
      aoData[14] = result.z14;
      aoData[15] = result.z15;
      aoData[16] = result.z16;
      aoData[17] = result.z17;
      aoData[18] = result.z18;
      aoData[19] = result.z19;
      aoData[20] = result.z20;

      /* copy across error terms */
      for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
      {
         aoData[21+i] = *(pAoErrorsVect +i);
      }

      /* release mutex */

      semGive(wfsLock);
   }

   return(OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * writeWfsToSynchro
 *
 * INVOCATION:
 * AO_CTRL_ID aoCtrlId
 * double     *pFgVect
 * double     *pFgErrorsVect
 * double     *pTime
 * long       STATUS;
 *
 * STATUS writeWfsToSynchro(AO_CTRL_ID aoCtrlId, double *pFgVect,
 *                          double *pFgErrorsVect, double *pTime)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId      - Pointer to the AO control context structure
 * > double *   pFgVect       - Vector containing the zernike modes
 * > double *   pFgErrorsVect - Vector containing the associated errors
 * > double *   pTime         - Pointer to the associated time stamp value
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Write tilt and focus data to the synchro bus and make available to 
 * the TCS via gensub records.
 *
 * DESCRIPTION:
 * The tip and tilt values are rotated and scaled as appropriate before 
 * being written to the synchro bus.
 * In addition the rotated tip and tilt values are also low pass filtered 
 * and written to an array ready for transmission to the TCS when required.
 *
 * EXTERNAL VARIABLES:
 * wfsLock       - Global mutex semaphore
 * ttfData       - ttf data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * Pointers to the ttf data are given.
 * The contents of theses vectors is not protected by mutex so there is a
 * requirement placed on the calling task that the contents of these vectors 
 * do not change for the duration of this routine.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version (srp)
 * 09-Nov-1998  Write fast tip/tilt to synchro bus (srp)
 * 05-Jan-1999  Add null zernike calculation
 * 23-Apr-1999  Simplified version for split backplane PWFS1 (cb)
 * 26-Nov-1999  Update interval as for P2 (cb)
 * 24-Apr-2000  Inputs now fit the new aoP1Lib (cb)
 *-
 */

STATUS writeWfsToSynchro 
   (
   AO_CTRL_ID aoCtrlId,
   double     *pFgVect,
   double     *pFgErrorsVect,
   double     *pTime
   )
{
   converted  result;
   frame      *f;
   double     *pz;


   /* access frame */

   f = ag2m2;

   pz = pFgVect;

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* first rotate the tip and tilt values to the m2 frame of reference */

      result.z2 = (f->cosTheta*(*pz) - f->sinTheta*(*(pz+1))) - f->null[5];
      result.z3 = (f->sinTheta*(*pz) + f->cosTheta*(*(pz+1))) - f->null[6];
      /*result.z4 = (*(pz+2)) - (pWfs->focusscale * f->null[7]);*/
      result.z4 = *(pz+2) ;

      semGive(f->access);
   }
   else
   {
      logMsg("writeWfsToSynchro - unable to get mutex for conversion frame\n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* scale data and write to the synchro bus, check that pointer has been 
      initialised with null check */

   if(ptrPwfs1 != NULL)
   {
     ptrPwfs1->z1 = (float)(result.z2);
     ptrPwfs1->z2 = (float)(result.z3);
     ptrPwfs1->z3 = (float)(result.z4);

     ptrPwfs1->err1   = (float)(*(pFgErrorsVect));
     ptrPwfs1->err2   = (float)(*(pFgErrorsVect + 1));
     ptrPwfs1->err3   = (float)(*(pFgErrorsVect + 2));
     ptrPwfs1->interval  += (float)(0.0001);

     ptrPwfs1->time = (double)(*pTime);

     /* raise interrupt on SCS */

     rmIntSend (INT3, SCS_NODE);
   }

   /* filter the tilt values and make available to the TCS gensubs */

   if(semTake(wfsLock, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access wfsLock\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      ttfData[0] = (*pTime);
      ttfData[1] = (double)(aoCtrlId->aoModeNb);
      ttfData[2] = dfilter(result.z2, (3 + 0));
      ttfData[3] = dfilter(result.z3, (3 + 1));
      ttfData[4] = dfilter(result.z4, (3 + 2));
      ttfData[5] = (double)(*(pFgErrorsVect));
      ttfData[6] = (double)(*(pFgErrorsVect+1));
      ttfData[7] = (double)(*(pFgErrorsVect+2));

      /* pop raw zernike values in for later display if desired */

      ttfData[8] = (double)result.z2;
      ttfData[9] = (double)result.z3;
      ttfData[10] = (double)result.z4;

      /* release mutex */

      semGive(wfsLock);
   }

   return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * ttfZero
 * 
 * Purpose:
 * Receive ttfZero information from the TCS combined with probe angle
 *
 * Invocation:
 * long ttfZero (struct genSubRecord * pgsub)
 *
 * Parameters in:
 * > struct genSubRecord *pgsub pointer to gensub record
 * > pgsub->a  string    guide probe angle
 *
 * Parameters out:
 * None
 * 
 * Return value:
 * < status   int      OK or ERROR
 *
 * Globals: 
 * External functions:
 * None
 * 
 * External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 21-Jan-1999: Read in probe angle on input A, add to tcs angle
 * 22-Jan-1999: Problem workaround - if cannot connect to probeAngle record
 *              then set probeAngle to 0.0 but do not logMsg
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add or 
 *              subtract of the Zeiss angle, port C provides an additional 
 *              rotation angle rotationAngle = tcsAngle + (polarityFudge * 
 *              (zeiss angle + rotationFudge))
 * 23-Apr-1999  Simplified version for split backplane PWFS1 (cb)
 *
 */

/* INDENT ON */

/* ===================================================================== */

long ttfZero 
   (
   struct genSubRecord * pgsub
   )
{
   int     index = 0;
   frame   *f;
   double  *ptr;
   double  tableAngle = 0.0;
   double  fudgeAngle = 0.0; 
   double  armAngle = 0.0; 
   double  compositeAngle = 0.0;

   ptr = (double *) pgsub->j;

   /* read conversion factors from input ports */

   if (sscanf(pgsub->a, "%lf", &tableAngle) != 1)
   {
      tableAngle = 0.0;
   }

   if (sscanf(pgsub->b, "%lf", &fudgeAngle) != 1)
   {
      fudgeAngle = 0.0;
   }

   if (sscanf(pgsub->c, "%lf", &armAngle) != 1)
   {
      armAngle = 0.0;
   }

   /* sanity check conversion factors */

   if (tableAngle < LOW_PROBE_ANGLE || tableAngle > HIGH_PROBE_ANGLE)
   {
      logMsg("ttfZero > %s probe angle out of range\n", (int)pgsub->name, 
             0, 0, 0, 0, 0);
      tableAngle = 0.0;
   }

   f = ag2m2;

   /* access frame */

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* read in the array */

      for (index = 0; index < TTF_ZERO_ARRAY_SIZE; index++)
      {
          f->null[index] = *(ptr++);
      }

      /* calculate composite correction angle */


      compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle - armAngle)*DEGS2RADS;   
                             /* null[3] corresponds to the cass rotator angle */

      f->theta       = compositeAngle;
      f->sinTheta    = sin(f->theta);
      f->cosTheta    = cos(f->theta);
      f->sin2Theta   = sin(2*f->theta);
      f->cos2Theta   = cos(2*f->theta);
      f->sin3Theta   = sin(3*f->theta);
      f->cos3Theta   = cos(3*f->theta);
      f->sin4Theta   = sin(4*f->theta);
      f->cos4Theta   = cos(4*f->theta);

      semGive(f->access);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for conversion frame\n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* write sample values to genSub ouputs */

   *(double *) pgsub->vala = f->null[0];         /* tSent */ 
   *(double *) pgsub->valb = f->null[1];         /* tAppl */
   *(double *) pgsub->valc = f->null[2];         /* trackId */
   *(double *) pgsub->vald = f->null[3];         /* tcsAngle (degrees) */
   *(double *) pgsub->vale = tableAngle;         /* tableAngle (degrees) */
   *(double *) pgsub->valf = compositeAngle/DEGS2RADS;   
                                                 /* composite angle (degrees) */
   *(double *) pgsub->valg = f->null[5];         /* z2 */
   *(double *) pgsub->valh = f->null[6];         /* z3 */
   *(double *) pgsub->vali = f->null[7];         /* z4 */

   return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * aoZero
 * 
 * Purpose:
 * Receive aoZero information from the TCS
 *
 * Invocation:
 * long aoZero (struct genSubRecord * pgsub)
 *
 * Parameters in:
 * > struct genSubRecord *pgsub pointer to gensub record
 *
 * Parameters out:
 * None
 * 
 * Return value:
 * < status   int      OK or ERROR
 *
 * Globals: 
 * External functions:
 * None
 * 
 * External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 21-Jan-1999: Read in probe angle on input A, add to tcs angle
 * 22-Jan-1999: Problem workaround - if cannot connect to probeAngle record
 *              then set probeAngle to 0.0 but do not logMsg
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add or 
 *              subtract of the Zeiss angle, port C provides an additional 
 *              rotation angle
 * rotationAngle = tcsAngle + (polarityFudge * (zeiss angle + rotationFudge))
 * 23-Apr-1999  Simplified version for split backplane PWFS1 (cb)
 * 21-June-1999 Modified to read tableAngle and the armAngle from a&g (cb)
 * 18-Nov-1999  Modified to add also cass rotator angle (cb)
 * 24-Nov-1999  Modified to add a fudge angle to the table angle (cb)
 * 26-Nov-1999  Change sign in the magic formula for the composite angle(cb)
 * 13-Dec-1999  Remove limit checks for the cass rotator angle (cb)
 * 12-jan-2000  Change sign in the magic formula +arm now (cb)
 *
 */

/* INDENT ON */

/* ===================================================================== */

long aoZero 
   (
   struct genSubRecord * pgsub
   )
{
   int     index = 0;
   frame   *f;
   double  *ptr;
   double  tableAngle = 0.0; 
   double  fudgeAngle = 0.0;
   double  armAngle = 0.0;
   double  compositeAngle = 0.0;
   double  applyAstig = 0.0;

   ptr = (double *) pgsub->j;

   /* read conversion factors from input ports */

   if (sscanf(pgsub->a, "%lf", &tableAngle) != 1)
   {
      tableAngle = 0.0;
   }

   if (sscanf(pgsub->b, "%lf", &fudgeAngle) != 1)
   {
      fudgeAngle = 0.0;
   }

   if (sscanf(pgsub->c, "%lf", &armAngle) != 1)
   {
      armAngle = 0.0;
   }

   if (sscanf(pgsub->d, "%lf", &applyAstig) != 1)
   {
      applyAstig = 0.0;
   }

   /* sanity check conversion factors */

   if (tableAngle < LOW_PROBE_ANGLE || tableAngle > HIGH_PROBE_ANGLE)
   {
      logMsg("aoZero > %s table angle out of range\n", (int)pgsub->name, 
             0, 0, 0, 0, 0);
      tableAngle = 0.0;
   }

   f = ag2tcs;

   /* access frame */

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* read in the array */

      for (index = 0; index < AO_ZERO_ARRAY_SIZE; index++)
      {
          f->null[index] = *(ptr++);
      }

      if ( applyAstig == 0.0 ) 
      {
         f->null[8] = 0.0 ;
         f->null[9] = 0.0 ;
      }
      else
      {
         f->null[8] = applyAstig * f->null[8];
         f->null[9] = applyAstig * f->null[9];
      }

      /* calculate composite correction angle */


      compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle + armAngle)*DEGS2RADS;

      f->theta       = compositeAngle;
      f->sinTheta    = sin(f->theta);
      f->cosTheta    = cos(f->theta);
      f->sin2Theta   = sin(2*f->theta);
      f->cos2Theta   = cos(2*f->theta);
      f->sin3Theta   = sin(3*f->theta);
      f->cos3Theta   = cos(3*f->theta);
      f->sin4Theta   = sin(4*f->theta);
      f->cos4Theta   = cos(4*f->theta);

      semGive(f->access);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for conversion frame\n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* write sample values to genSub ouputs */

   *(double *) pgsub->vala = f->null[0];         /* tSent */ 
   *(double *) pgsub->valb = f->null[1];         /* tAppl */
   *(double *) pgsub->valc = f->null[2];         /* trackId */
   *(double *) pgsub->vald = f->null[3];         /* tcsAngle (degrees) */
   *(double *) pgsub->vale = tableAngle;         /* tableAngle (degrees) */
   *(double *) pgsub->valf = compositeAngle/DEGS2RADS;   
                                                 /* composite angle (degrees) */
   *(double *) pgsub->valg = f->null[5];         /* z2 */
   *(double *) pgsub->valh = f->null[6];         /* z3 */
   *(double *) pgsub->vali = f->null[7];         /* z4 */

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag1
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag1 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 12-Jan-1999  Original version   Sean Prior
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showAoDiag1
   (
   struct genSubRecord * pgsub
   )
{
   int i = 0;
   int j = 0;
   int indexCb;
   int wfsStatus;
   double   *pCentroids;
   double   *pTotal;

   if (aoCbCtrlIdP1 == NULL)
   {
      /* context structure not yet initialised */
      return(OK);
   }

   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* grab data from the circular buffer */

      indexCb = aoCbCtrlIdP1->position;

      if (( indexCb == 0 ) && ( aoCbCtrlIdP1->counter == 0))
         return (OK);

      if ( (indexCb < 0) && (indexCb > (CB_CTRL_RECORD_NB - 1)) )
      {
         printf ( "showAoDiag1(): position in CB out of range\n" ) ;
         return (OK);
      }

      if ( indexCb != 0 )
         indexCb -= 1;
      else
         indexCb = CB_CTRL_RECORD_NB - 1;

      pCentroids = aoCbCtrlIdP1->cbCtrlRecord[indexCb].centroidsVect;
      pTotal = aoCbCtrlIdP1->cbCtrlRecord[indexCb].totalCountsVect;
      wfsStatus = aoCbCtrlIdP1->cbCtrlRecord[indexCb].wfsStatus;

      j = 0;
      for ( i = 0 ; i < aoCcdIdP1->subapNb ; i ++ )
      {
          if ( aoCcdIdP1->subapUsedVect[i] == TRUE )
          {
             *(localCentroidsVect + 2*i) = *(pCentroids + 2*j);
             *(localCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
             *(localTotalCountsVect + i) = *(pTotal + j);
             j ++ ;
          }
          else
          {
             *(localCentroidsVect + 2*i) = -99.99;
             *(localCentroidsVect + 2*i+1) = -99.99;
             *(localTotalCountsVect + i) = -99.99;
          }
      }

      *(localTotalCountsVect + i) = *(pTotal + j);

      /* data intact, write to genSub outputs */

      *(int *)pgsub->vala = indexCb;
      *(int *)pgsub->valb = wfsStatus;
      *(double *)pgsub->valc = *(localCentroidsVect+0); 
      *(double *)pgsub->vald = *(localCentroidsVect+1); 
      *(double *)pgsub->vale = *(localCentroidsVect+2); 
      *(double *)pgsub->valf = *(localCentroidsVect+3); 
      *(double *)pgsub->valg = *(localCentroidsVect+4); 
      *(double *)pgsub->valh = *(localCentroidsVect+5); 
      *(double *)pgsub->vali = *(localCentroidsVect+6); 
      *(double *)pgsub->valj = *(localCentroidsVect+7); 
      *(double *)pgsub->valk = *(localCentroidsVect+8); 
      *(double *)pgsub->vall = *(localCentroidsVect+9); 
      *(double *)pgsub->valm = *(localCentroidsVect+10); 
      *(double *)pgsub->valn = *(localCentroidsVect+11); 
      *(double *)pgsub->valo = *(localCentroidsVect+12); 
      *(double *)pgsub->valp = *(localCentroidsVect+13); 
      *(double *)pgsub->valq = *(localCentroidsVect+14); 
      *(double *)pgsub->valr = *(localCentroidsVect+15); 
      *(double *)pgsub->vals = *(localCentroidsVect+16); 
      *(double *)pgsub->valt = *(localCentroidsVect+17); 
      *(double *)pgsub->valu = *(localCentroidsVect+18); 

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag2 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb 
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showAoDiag2
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localCentroidsVect+19);
      *(double *)pgsub->valb = *(localCentroidsVect+20);
      *(double *)pgsub->valc = *(localCentroidsVect+21);
      *(double *)pgsub->vald = *(localCentroidsVect+22);
      *(double *)pgsub->vale = *(localCentroidsVect+23);
      *(double *)pgsub->valf = *(localCentroidsVect+24);
      *(double *)pgsub->valg = *(localCentroidsVect+25);
      *(double *)pgsub->valh = *(localCentroidsVect+26);
      *(double *)pgsub->vali = *(localCentroidsVect+27);
      *(double *)pgsub->valj = *(localCentroidsVect+28);
      *(double *)pgsub->valk = *(localCentroidsVect+29);
      *(double *)pgsub->vall = *(localCentroidsVect+30);
      *(double *)pgsub->valm = *(localCentroidsVect+31);
      *(double *)pgsub->valn = *(localCentroidsVect+32);
      *(double *)pgsub->valo = *(localCentroidsVect+33);
      *(double *)pgsub->valp = *(localCentroidsVect+34);
      *(double *)pgsub->valq = *(localCentroidsVect+35);
      *(double *)pgsub->valr = *(localCentroidsVect+36);
      *(double *)pgsub->vals = *(localCentroidsVect+37);
      *(double *)pgsub->valt = *(localCentroidsVect+38);
      *(double *)pgsub->valu = *(localCentroidsVect+39);

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag3
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag3 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version cb to display 6x6 centroids data
 *-
 */

STATUS showAoDiag3
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localCentroidsVect+40);
      *(double *)pgsub->valb = *(localCentroidsVect+41);
      *(double *)pgsub->valc = *(localCentroidsVect+42);
      *(double *)pgsub->vald = *(localCentroidsVect+43);
      *(double *)pgsub->vale = *(localCentroidsVect+44);
      *(double *)pgsub->valf = *(localCentroidsVect+45);
      *(double *)pgsub->valg = *(localCentroidsVect+46);
      *(double *)pgsub->valh = *(localCentroidsVect+47); 
      *(double *)pgsub->vali = *(localCentroidsVect+48); 
      *(double *)pgsub->valj = *(localCentroidsVect+49); 
      *(double *)pgsub->valk = *(localCentroidsVect+50); 
      *(double *)pgsub->vall = *(localCentroidsVect+51); 
      *(double *)pgsub->valm = *(localCentroidsVect+52); 
      *(double *)pgsub->valn = *(localCentroidsVect+53); 
      *(double *)pgsub->valo = *(localCentroidsVect+54); 
      *(double *)pgsub->valp = *(localCentroidsVect+55); 
      *(double *)pgsub->valq = *(localCentroidsVect+56); 
      *(double *)pgsub->valr = *(localCentroidsVect+57); 
      *(double *)pgsub->vals = *(localCentroidsVect+58); 
      *(double *)pgsub->valt = *(localCentroidsVect+59); 
      *(double *)pgsub->valu = *(localCentroidsVect+60); 

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag4
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag4 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  added by cb to display 6x6 centroids data
 *-
 */

STATUS showAoDiag4
   (
   struct genSubRecord * pgsub
   )
{
   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localCentroidsVect + 61); 
      *(double *)pgsub->valb = *(localCentroidsVect + 62); 
      *(double *)pgsub->valc = *(localCentroidsVect + 63);  
      *(double *)pgsub->vald = *(localCentroidsVect + 64);  
      *(double *)pgsub->vale = *(localCentroidsVect + 65);  
      *(double *)pgsub->valf = *(localCentroidsVect + 66);  
      *(double *)pgsub->valg = *(localCentroidsVect + 67);  
      *(double *)pgsub->valh = *(localCentroidsVect + 68);  
      *(double *)pgsub->vali = *(localCentroidsVect + 69); 
      *(double *)pgsub->valj = *(localCentroidsVect + 70); 
      *(double *)pgsub->valk = *(localCentroidsVect + 71); 

      *(double *)pgsub->vall = *(localTotalCountsVect + 0); 
      *(double *)pgsub->valm = *(localTotalCountsVect + 1); 
      *(double *)pgsub->valn = *(localTotalCountsVect + 2); 
      *(double *)pgsub->valo = *(localTotalCountsVect + 3); 
      *(double *)pgsub->valp = *(localTotalCountsVect + 4); 
      *(double *)pgsub->valq = *(localTotalCountsVect + 5); 
      *(double *)pgsub->valr = *(localTotalCountsVect + 6); 
      *(double *)pgsub->vals = *(localTotalCountsVect + 7); 
      *(double *)pgsub->valt = *(localTotalCountsVect + 8); 
      *(double *)pgsub->valu = *(localTotalCountsVect + 9); 

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag5
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag5 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  added by cb to display 6x6 centroids data
 *-
 */

STATUS showAoDiag5
   (
   struct genSubRecord * pgsub
   )
{
   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localTotalCountsVect + 10); 
      *(double *)pgsub->valb = *(localTotalCountsVect + 11); 
      *(double *)pgsub->valc = *(localTotalCountsVect + 12);  
      *(double *)pgsub->vald = *(localTotalCountsVect + 13);  
      *(double *)pgsub->vale = *(localTotalCountsVect + 14);  
      *(double *)pgsub->valf = *(localTotalCountsVect + 15);  
      *(double *)pgsub->valg = *(localTotalCountsVect + 16);  
      *(double *)pgsub->valh = *(localTotalCountsVect + 17);  
      *(double *)pgsub->vali = *(localTotalCountsVect + 18); 
      *(double *)pgsub->valj = *(localTotalCountsVect + 19); 
      *(double *)pgsub->valk = *(localTotalCountsVect + 20); 
      *(double *)pgsub->vall = *(localTotalCountsVect + 21); 
      *(double *)pgsub->valm = *(localTotalCountsVect + 22); 
      *(double *)pgsub->valn = *(localTotalCountsVect + 23); 
      *(double *)pgsub->valo = *(localTotalCountsVect + 24); 
      *(double *)pgsub->valp = *(localTotalCountsVect + 25); 
      *(double *)pgsub->valq = *(localTotalCountsVect + 26); 
      *(double *)pgsub->valr = *(localTotalCountsVect + 27); 
      *(double *)pgsub->vals = *(localTotalCountsVect + 28); 
      *(double *)pgsub->valt = *(localTotalCountsVect + 29); 
      *(double *)pgsub->valu = *(localTotalCountsVect + 30); 

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiag6
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiag6 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  added by cb to display 6x6 centroids data
 *-
 */

STATUS showAoDiag6
   (
   struct genSubRecord * pgsub
   )
{
   if(semTake(accessAoData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessAoData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localTotalCountsVect + 31); 
      *(double *)pgsub->valb = *(localTotalCountsVect + 32); 
      *(double *)pgsub->valc = *(localTotalCountsVect + 33);  
      *(double *)pgsub->vald = *(localTotalCountsVect + 34);  
      *(double *)pgsub->vale = *(localTotalCountsVect + 35);  
      *(double *)pgsub->valf = *(localTotalCountsVect + 36);  

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag1
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long    showFgDiag1(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from fg circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 12-Jan-1999  Original version   Sean Prior
 * 26-Apr-1999  Modified for split backplane version...
 *-
 */

STATUS showFgDiag1(struct genSubRecord * pgsub)
{
   int i=0;
   int j=0;
   int indexCb;
   int wfsStatus;
   double *pGuide;
   double *pTotal;
   double *pCentroids;

   if (aoCbFgCtrlIdP1 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      
      /* grab data from the circular buffer */

      indexCb = aoCbFgCtrlIdP1->position;

      if (( indexCb == 0 ) && ( aoCbFgCtrlIdP1->counter == 0))
         return (OK);

      if ( (indexCb < 0) && (indexCb > (CB_FG_CTRL_RECORD_NB - 1)) )
      {
         printf ( "showFgDiag1(): position in CB out of range\n" ) ;
         return (ERROR);
      }

      if ( indexCb != 0 )
         indexCb -= 1;
      else
         indexCb = CB_FG_CTRL_RECORD_NB - 1;

      wfsStatus = aoCbFgCtrlIdP1->cbFgCtrlRecord[indexCb].wfsStatus;
      pGuide = aoCbFgCtrlIdP1->cbFgCtrlRecord[indexCb].guidesVect;
      pTotal = aoCbFgCtrlIdP1->cbFgCtrlRecord[indexCb].totalCountsVect;
      pCentroids = aoCbFgCtrlIdP1->cbFgCtrlRecord[indexCb].centroidsVect;

      j = 0;
      for ( i = 0 ; i < aoCcdIdP1->subapNb ; i ++ )
      {
          if ( aoCcdIdP1->subapUsedVect[i] == TRUE )
          {
             *(localFgCentroidsVect + 2*i) = *(pCentroids + 2*j);
             *(localFgCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
             *(localFgTotalCountsVect + i) = *(pTotal + j);
             j ++ ;
          }
          else
          {
             *(localFgCentroidsVect + 2*i) = -99.99;
             *(localFgCentroidsVect + 2*i+1) = -99.99;
             *(localFgTotalCountsVect + i) = -99.99;
          }
      }

      *(localFgTotalCountsVect + i) = *(pTotal + j);

      /* data intact, write to genSub outputs */

      *(int *)pgsub->vala = indexCb;
      *(int *)pgsub->valb = wfsStatus;
      *(double *)pgsub->valc = *(pGuide);
      *(double *)pgsub->vald = *(pGuide + 1);
      *(double *)pgsub->vale = *(localFgCentroidsVect + 0);
      *(double *)pgsub->valf = *(localFgCentroidsVect + 1);
      *(double *)pgsub->valg = *(localFgCentroidsVect + 2);
      *(double *)pgsub->valh = *(localFgCentroidsVect + 3);
      *(double *)pgsub->vali = *(localFgCentroidsVect + 4);
      *(double *)pgsub->valj = *(localFgCentroidsVect + 5);
      *(double *)pgsub->valk = *(localFgCentroidsVect + 6);
      *(double *)pgsub->vall = *(localFgCentroidsVect + 7);
      *(double *)pgsub->valm = *(localFgCentroidsVect + 8);
      *(double *)pgsub->valn = *(localFgCentroidsVect + 9);
      *(double *)pgsub->valo = *(localFgCentroidsVect + 10);
      *(double *)pgsub->valp = *(localFgCentroidsVect + 11);
      *(double *)pgsub->valq = *(localFgCentroidsVect + 12);
      *(double *)pgsub->valr = *(localFgCentroidsVect + 13);
      *(double *)pgsub->vals = *(localFgCentroidsVect + 14);
      *(double *)pgsub->valt = *(localFgCentroidsVect + 15);
      *(double *)pgsub->valu = *(localFgCentroidsVect + 16);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiag2 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from FG circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showFgDiag2
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localFgCentroidsVect+17);
      *(double *)pgsub->valb = *(localFgCentroidsVect+18);
      *(double *)pgsub->valc = *(localFgCentroidsVect+19);
      *(double *)pgsub->vald = *(localFgCentroidsVect+20);
      *(double *)pgsub->vale = *(localFgCentroidsVect+21);
      *(double *)pgsub->valf = *(localFgCentroidsVect+22);
      *(double *)pgsub->valg = *(localFgCentroidsVect+23);
      *(double *)pgsub->valh = *(localFgCentroidsVect+24);
      *(double *)pgsub->vali = *(localFgCentroidsVect+25);
      *(double *)pgsub->valj = *(localFgCentroidsVect+26);
      *(double *)pgsub->valk = *(localFgCentroidsVect+27);
      *(double *)pgsub->vall = *(localFgCentroidsVect+28);
      *(double *)pgsub->valm = *(localFgCentroidsVect+29);
      *(double *)pgsub->valn = *(localFgCentroidsVect+30);
      *(double *)pgsub->valo = *(localFgCentroidsVect+31);
      *(double *)pgsub->valp = *(localFgCentroidsVect+32);
      *(double *)pgsub->valq = *(localFgCentroidsVect+33);
      *(double *)pgsub->valr = *(localFgCentroidsVect+34);
      *(double *)pgsub->vals = *(localFgCentroidsVect+35);
      *(double *)pgsub->valt = *(localFgCentroidsVect+36);
      *(double *)pgsub->valu = *(localFgCentroidsVect+37);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag3
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiag3 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from FG circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showFgDiag3
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localFgCentroidsVect+38);
      *(double *)pgsub->valb = *(localFgCentroidsVect+39);
      *(double *)pgsub->valc = *(localFgCentroidsVect+40);
      *(double *)pgsub->vald = *(localFgCentroidsVect+41);
      *(double *)pgsub->vale = *(localFgCentroidsVect+42);
      *(double *)pgsub->valf = *(localFgCentroidsVect+43);
      *(double *)pgsub->valg = *(localFgCentroidsVect+44);
      *(double *)pgsub->valh = *(localFgCentroidsVect+45);
      *(double *)pgsub->vali = *(localFgCentroidsVect+46);
      *(double *)pgsub->valj = *(localFgCentroidsVect+47);
      *(double *)pgsub->valk = *(localFgCentroidsVect+48);
      *(double *)pgsub->vall = *(localFgCentroidsVect+49);
      *(double *)pgsub->valm = *(localFgCentroidsVect+50);
      *(double *)pgsub->valn = *(localFgCentroidsVect+51);
      *(double *)pgsub->valo = *(localFgCentroidsVect+52);
      *(double *)pgsub->valp = *(localFgCentroidsVect+53);
      *(double *)pgsub->valq = *(localFgCentroidsVect+54);
      *(double *)pgsub->valr = *(localFgCentroidsVect+55);
      *(double *)pgsub->vals = *(localFgCentroidsVect+56);
      *(double *)pgsub->valt = *(localFgCentroidsVect+57);
      *(double *)pgsub->valu = *(localFgCentroidsVect+58);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag4
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiag4 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from FG circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showFgDiag4
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localFgCentroidsVect+59);
      *(double *)pgsub->valb = *(localFgCentroidsVect+60);
      *(double *)pgsub->valc = *(localFgCentroidsVect+61);
      *(double *)pgsub->vald = *(localFgCentroidsVect+62);
      *(double *)pgsub->vale = *(localFgCentroidsVect+63);
      *(double *)pgsub->valf = *(localFgCentroidsVect+64);
      *(double *)pgsub->valg = *(localFgCentroidsVect+65);
      *(double *)pgsub->valh = *(localFgCentroidsVect+66);
      *(double *)pgsub->vali = *(localFgCentroidsVect+67);
      *(double *)pgsub->valj = *(localFgCentroidsVect+68);
      *(double *)pgsub->valk = *(localFgCentroidsVect+69);
      *(double *)pgsub->vall = *(localFgCentroidsVect+70);
      *(double *)pgsub->valm = *(localFgCentroidsVect+71);

      *(double *)pgsub->valn = *(localFgTotalCountsVect+0);
      *(double *)pgsub->valo = *(localFgTotalCountsVect+1);
      *(double *)pgsub->valp = *(localFgTotalCountsVect+2);
      *(double *)pgsub->valq = *(localFgTotalCountsVect+3);
      *(double *)pgsub->valr = *(localFgTotalCountsVect+4);
      *(double *)pgsub->vals = *(localFgTotalCountsVect+5);
      *(double *)pgsub->valt = *(localFgTotalCountsVect+6);
      *(double *)pgsub->valu = *(localFgTotalCountsVect+7);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag5
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiag5 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from FG circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showFgDiag5
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localFgTotalCountsVect+8);
      *(double *)pgsub->valb = *(localFgTotalCountsVect+9);
      *(double *)pgsub->valc = *(localFgTotalCountsVect+10);
      *(double *)pgsub->vald = *(localFgTotalCountsVect+11);
      *(double *)pgsub->vale = *(localFgTotalCountsVect+12);
      *(double *)pgsub->valf = *(localFgTotalCountsVect+13);
      *(double *)pgsub->valg = *(localFgTotalCountsVect+14);
      *(double *)pgsub->valh = *(localFgTotalCountsVect+15);
      *(double *)pgsub->vali = *(localFgTotalCountsVect+16);
      *(double *)pgsub->valj = *(localFgTotalCountsVect+17);
      *(double *)pgsub->valk = *(localFgTotalCountsVect+18);
      *(double *)pgsub->vall = *(localFgTotalCountsVect+19);
      *(double *)pgsub->valm = *(localFgTotalCountsVect+20);
      *(double *)pgsub->valn = *(localFgTotalCountsVect+21);
      *(double *)pgsub->valo = *(localFgTotalCountsVect+22);
      *(double *)pgsub->valp = *(localFgTotalCountsVect+23);
      *(double *)pgsub->valq = *(localFgTotalCountsVect+24);
      *(double *)pgsub->valr = *(localFgTotalCountsVect+25);
      *(double *)pgsub->vals = *(localFgTotalCountsVect+26);
      *(double *)pgsub->valt = *(localFgTotalCountsVect+27);
      *(double *)pgsub->valu = *(localFgTotalCountsVect+28);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag6
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiag6 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from FG circular buffer to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Apr-1999  Original version   cb
 * 26-Apr-1999  Modified to display 6x6 centroids data
 *-
 */

STATUS showFgDiag6
   (
   struct genSubRecord * pgsub
   )
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localFgTotalCountsVect+29);
      *(double *)pgsub->valb = *(localFgTotalCountsVect+30);
      *(double *)pgsub->valc = *(localFgTotalCountsVect+31);
      *(double *)pgsub->vald = *(localFgTotalCountsVect+32);
      *(double *)pgsub->vale = *(localFgTotalCountsVect+33);
      *(double *)pgsub->valf = *(localFgTotalCountsVect+34);
      *(double *)pgsub->valg = *(localFgTotalCountsVect+35);
      *(double *)pgsub->valh = *(localFgTotalCountsVect+36);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showCbDiags
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showCbDiags(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from circular buffers to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 24-Apr-2000 Original version - cb
 *-
 */

STATUS showCbDiags(struct genSubRecord * pgsub)
{
   int           indexCbIm;
   int           indexCbCtrl;
   int           indexCbFgCtrl;
   int           counterCbIm;
   int           counterCbCtrl;
   int           counterCbFgCtrl;

   /* Check the circular buffer structures are initialised */

   if (aoCbCtrlIdP1 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   if (aoCbFgCtrlIdP1 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   if (aoCbImIdP1 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   /* Grab data from the circular buffers */

   indexCbCtrl = aoCbCtrlIdP1->position;
   counterCbCtrl = aoCbCtrlIdP1->counter;

   indexCbFgCtrl = aoCbFgCtrlIdP1->position;
   counterCbFgCtrl = aoCbFgCtrlIdP1->counter;

   indexCbIm = aoCbImIdP1->position;
   counterCbIm = aoCbImIdP1->counter;

   *(int *)pgsub->vala = indexCbIm;
   *(int *)pgsub->valb = counterCbIm;
   *(int *)pgsub->valc = indexCbCtrl;
   *(int *)pgsub->vald = counterCbCtrl;
   *(int *)pgsub->vale = indexCbFgCtrl;
   *(int *)pgsub->valf = counterCbFgCtrl;

   return (OK);
}


/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubFanDoubles
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long gensubFanDoubles(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Receive array of double on port A, copy individual elements to output
 * ports. Primarily intended for display to dm screens.
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 23-Jan-1999  Original version   Sean Prior
 *-
 */

long gensubFanDoubles 
   (
   struct genSubRecord * pgsub
   )
{
   int index = 0;
   double     localArray[19];
   double     *ptr = NULL;

   ptr = (double *) pgsub->a;

   /* read in the array from port A */

   for (index = 0; index < 19; index++)
   {
       localArray[index] = *(ptr++);
   }

   /* write values to gensub outputs for screen display */

   *(double *)pgsub->vala = localArray[0];        /* Z2 or E2 */
   *(double *)pgsub->valb = localArray[1];
   *(double *)pgsub->valc = localArray[2];
   *(double *)pgsub->vald = localArray[3];
   *(double *)pgsub->vale = localArray[4];
   *(double *)pgsub->valf = localArray[5];
   *(double *)pgsub->valg = localArray[6];
   *(double *)pgsub->valh = localArray[7];
   *(double *)pgsub->vali = localArray[8];
   *(double *)pgsub->valj = localArray[9];
   *(double *)pgsub->valk = localArray[10];
   *(double *)pgsub->vall = localArray[11];
   *(double *)pgsub->valm = localArray[12];
   *(double *)pgsub->valn = localArray[13];
   *(double *)pgsub->valo = localArray[14];
   *(double *)pgsub->valp = localArray[15];
   *(double *)pgsub->valq = localArray[16];
   *(double *)pgsub->valr = localArray[17];
   *(double *)pgsub->vals = localArray[18];   /* Z20 or E20 */

   return (OK);
}
