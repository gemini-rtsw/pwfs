/* id:$ */
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
 * showFgDiags       - Write diagnostic data from ao structure to gensub
 *                     outputs for display
 * showCbDiags       - Write diagnostic data from cb structure to gensub
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
 *              out, increment time count for each write to the synchro bus as 
 *              temporary measure.
 * 22-Jan-1999: Add function to read guide probe angle in ttfZero and aoZero.
 * 23-Jan-1999: Modify gensubToTcsAo to write arrays of zernikes and errors 
 *              to outputs a and b
 *              Add function gensubFanDoubles to read array of doubles and 
 *              write elements to the output ports
 * 24-Jan-1999: ttfZero and aoZero - read ports B and C for fudge factors in 
 *              polarity and rotation
 * 10-Feb-1999: cb - add check max/min TT and focus to writeWfsToSynchro()
 * 17-Feb-1999: cb - ttfZero change computation of theta for TCS and SCS
 * 29-Jun-1999: cb - ttfZero change computation of theta for TCS and SCS
 * 28-Mar-2000: cb - Major modifications, new aoP2Lib library
 *
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef PI
#define PI 3.14159265358979
#endif

#define MAX_WFS_SOURCES     5
#define TTF_ARRAY_SIZE      8
#define TTF_ZERO_ARRAY_SIZE 9
#define AO_ZERO_ARRAY_SIZE  24
#define AO_ARRAY_SIZE       40
#define MAX_FILTERS         (3 * MAX_WFS_SOURCES)
#define WFS_TIMEOUT         40
#define DEGS2RADS           ((double)(2.0*PI)/(double)360.0)
                                /* conversion factor for degrees to radians  */
#define DISCARD_THRESHOLD 60
#define LOW_PROBE_ANGLE     -360.0   
                                /* high limit on guide probe angle (degrees) */   
#define HIGH_PROBE_ANGLE    360.0   
                                /* low limit on guide probe angle (degrees)  */
#define MAX_TT_M2           12.5   
                                /* max tip/tilt for AO correction (arcsec)   */
#define MIN_TT_M2           -12.5   
                                /* min tip/tilt for AO correction (arcsec)   */
#define MAX_FOCUS_M2        0.84   
                                /* max focus for AO correction (microns)     */
#define MIN_FOCUS_M2        -0.84   
                                /* min focus for AO correction (microns)     */

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
#include <sdsuLib.h>

#include "aoP2Lib.h"
#include "synchroMap.h"

typedef struct
{
   double probeAngle; /* angle of guide probe supplied by Zeiss */
   double tcsAngle;   /* rotation angle supplied by TCS */
   double theta;
   double sinTheta;
   double cosTheta;
   double null[AO_ZERO_ARRAY_SIZE];
   SEM_ID access;
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

frame    *ag2m2;
frame    *ag2tcs;
wfs      *ptrPwfs2;
double   ttfData[AO_ARRAY_SIZE+2];
double   aoData[AO_ARRAY_SIZE+2];
float    data[AO_ARRAY_SIZE+2];
float    errors[AO_ARRAY_SIZE+2];
SEM_ID   wfsLock;
SDSU_ID  sdsuId;

AO_CCD_ID     aoCcdIdP2;
AO_CB_CTRL_ID aoCbCtrlIdP2;
AO_CB_IM_ID   aoCbImIdP2;

double sampleData[5][3];
double coeffData[5];

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
 *                            filters per wfs source hence:-
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
 * newDfilter
 *
 * INVOCATION:
 * double newSample
 * int Id
 *
 * double   newDfilter(double newSample, int Id)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > double newSample       - latest data sample
 * > int    iD              - identification of zernikes (0 = xtilt, 1 = ytilt,
 *                            2 = focus)
 *
 * FUNCTION VALUE:
 * double     returns current filtered value
 *
 * PURPOSE:
 * Filter the data in accordance with the IIR filter coefficients specified
 *
 * DESCRIPTION:
 * The function performs a low pass butterworth filter on the supplied
 * data. A history array is maintained for each zernikes identified by the 
 * index Id.
 * The cutoff frequency is set in detControl.c (detSigInitGain CAD) and 
 * coefficients of the filter are computed according the cutoof frequency and
 * exposure time.
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
 * 27-Oct-2000  Coeff are computing in detControl.c and the cutoffFreq set by
 *              the user
 * 28-Oct-1998  Original version - Sean Prior
 *-
 */

double newDfilter
   (
   double newSample,
   int Id
   )
{
   int i = 0;
   double sum = 0;

   /* put new sample into the array */

   sampleData[2][Id] = newSample;

   /* multiply samples by coefficients and accumulate */

   for(i=0; i < 5; i++)
      sum += sampleData[i][Id]*coeffData[i];

   /* ripple samples ready for next call */

   sampleData[4][Id] = sampleData[3][Id];
   sampleData[3][Id] = sampleData[2][Id];
   sampleData[1][Id] = sampleData[0][Id];
   sampleData[0][Id] = sum;

   /*printf ( "sum[%d] = %f\n" , Id, sum );*/
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
 * wfsLock - Global mutex semaphores
 * ag2m2   - pointer to coord conversion structures
 * ag2tcs  - pointer to coord conversion structures
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998: Original version (srp)
 * 05-Jan-1999: Put all initialisation and semaphore creation in this 
 *              section rather than creating as necessary during operation
 * 22-Jan-1999: Initialise time values on synchro bus to 0.0
 * 28-Mar-2000: Simplified version for P2 only (cb)
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

   if (wfsLock == NULL)
   {
      if ((wfsLock = 
           semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
           == NULL)
      {
         printf ("unable to create wfsLock sem\n");
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

   /*printf ( "adr of page0= %x\n" , &basePtr->page0) ;
   printf ( "adr of page1= %x\n" , &basePtr->page1) ;
   printf ( "adr of testResults= %x\n" , &basePtr->testResults) ;
   printf ( "adr of pad1= %x\n" , &basePtr->pad1) ;
   printf ( "adr of eventData= %x\n" , &basePtr->eventData) ;
   printf ( "adr of pad2= %x\n" , &basePtr->pad2) ;
   printf ( "adr of pwfs1= %x\n" , &basePtr->pwfs1) ;
   printf ( "adr of pad3= %x\n" , &basePtr->pad3) ;
   printf ( "adr of pwfs2= %x\n" , &basePtr->pwfs2) ;
   printf ( "adr of pad4= %x\n" , &basePtr->pad4) ;
   printf ( "adr of oiwfs= %x\n" , &basePtr->oiwfs) ;
   printf ( "adr of pad5= %x\n" , &basePtr->pad5) ;
   printf ( "adr of gaos= %x\n" , &basePtr->gaos) ;
   printf ( "adr of pad6= %x\n" , &basePtr->pad6) ;
   printf ( "adr of gyro= %x\n" , &basePtr->gyro) ;
   printf ( "adr of pad7= %x\n" , &basePtr->pad7) ;
   printf ( "adr of m2Eng= %x\n" , &basePtr->m2Eng) ;
   printf ( "adr of m2Eng.pad= %x\n" , basePtr->m2Eng.pad) ;*/

   /* if synchro card present, initialise structure pointers */

   /* assign pointers and write ID strings for synchro bus */

   if (ptrPwfs2 == NULL)
   {
      ptrPwfs2 = (wfs*)&basePtr->pwfs2;
      strncpy(ptrPwfs2->name, "pwfs2", 15);
      ptrPwfs2->time = 0.0;
      ptrPwfs2->interval = 0.0;
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
 * long status;
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
 * Copy data from the global ao data arrays to the VALJ port for reading by 
 * the TCS
 *
 * DESCRIPTION:
 * Mutex access to the global ao data is then attempted and data is copied 
 * out to the VALJ port. Status return will be bad if there is a timeout on 
 * mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock       - Global mutex semaphore
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
 * 28-Mar-2000: Simplified version for P2 only (cb)
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
 * out to the VALJ port. Status return will be bad if there is a timeout on
 * mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock      - Global mutex semaphore
 * aoData       - ao data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998   Original version - Sean Prior
 * 23-Jan-1999   Write arrays of zernikes and errors to vala and valb
 *               to be picked up and displayed by other gensubs (srp)
 * 28-Mar-2000: Simplified version for P2 only (cb)
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
 * double *pZernikesVect
 * double *pErrorsVect
 * double *pTime
 * long   STATUS;
 *
 * STATUS writeWfsToTcs(AO_CTRL_ID aoCtrlId, double *pZernikesVect,
 *                      double *pErrorsVect, double *pTime)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId    - Pointer to the AO control context structure
 * > double * pZernikesVect - Vector containing the zernike modes
 * > double * pErrorsVect   - Vector containing the associated errors
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
 * wfsLock - Global mutex semaphore
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * Pointers to the ao data are given.
 * The contents of these vectors is not protected by mutex so there is a 
 * requirement placed on the calling task that the contents of these vectors 
 * do not change for the duration of this routine which is actually the case.
 *
 * HISTORY (optional):
 * 28-Oct-1998   Original version (srp)
 * 11-Nov-1998   Add frame of reference conversion
 * 05-Jan-1999   Add null zernike calculation
 * 28-Mar-2000   Simplified version for P2 only (cb)
 */

STATUS writeWfsToTcs
   (
   AO_CTRL_ID aoCtrlId, 
   double     *pZernikesVect, 
   double     *pErrorsVect, 
   double     *pTime
   )
{
   int       i=0;
   frame     *f;
   converted result;
   double    *pz;

   /* check that array counts are within limits */

   if ( aoCtrlId->modeNb > 19 )
   {
       logMsg("zernike count np = %d out of limits\n", 
              aoCtrlId->modeNb, 0, 0, 0, 0, 0);
       return(ERROR);
   }

   /* access frame */

   pz = pZernikesVect;

   f = ag2tcs;

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* first rotate the tip and tilt values to the tcs frame of reference */

      result.z2 = (f->cosTheta*(*pz) - f->sinTheta*(*(pz+1))) - f->null[5];
      result.z3 = (f->sinTheta*(*pz) + f->cosTheta*(*(pz+1))) - f->null[6];
      result.z4 = *(pz+2) - f->null[7];
      result.z5 = (f->cosTheta*(*(pz+3)) - f->sinTheta*(*(pz+4))) - f->null[8];
      result.z6 = (f->sinTheta*(*(pz+3)) + f->cosTheta*(*(pz+4))) - f->null[9];
      result.z7 = (f->cosTheta*(*(pz+5)) - f->sinTheta*(*(pz+6))) - f->null[10];
      result.z8 = (f->sinTheta*(*(pz+5)) + f->cosTheta*(*(pz+6))) - f->null[11];
      result.z9 = *(pz+7) - f->null[12];
      result.z10 = (f->cosTheta*(*(pz+8)) - f->sinTheta*(*(pz+9))) 
                   - f->null[13];
      result.z11 = (f->sinTheta*(*(pz+8)) + f->cosTheta*(*(pz+9))) 
                   - f->null[14];
      result.z12 = (f->cosTheta*(*(pz+10)) - f->sinTheta*(*(pz+11))) 
                   - f->null[15];
      result.z13 = (f->sinTheta*(*(pz+10)) + f->cosTheta*(*(pz+11))) 
                   - f->null[16];
      result.z14 = (f->cosTheta*(*(pz+12)) - f->sinTheta*(*(pz+13))) 
                   - f->null[17];
      result.z15 = (f->sinTheta*(*(pz+12)) + f->cosTheta*(*(pz+13))) 
                   - f->null[18];
      result.z16 = *(pz+14) - f->null[19];
      result.z17 = (f->cosTheta*(*(pz+15)) - f->sinTheta*(*(pz+16))) 
                   - f->null[20];
      result.z18 = (f->sinTheta*(*(pz+15)) + f->cosTheta*(*(pz+16))) 
                   - f->null[21];
      result.z19 = (f->cosTheta*(*(pz+17)) - f->sinTheta*(*(pz+18))) 
                   - f->null[22];
      result.z20 = (f->sinTheta*(*(pz+17)) + f->cosTheta*(*(pz+18))) 
                   - f->null[23];

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

      aoData[0] = (double)(*pTime);             /* time */
      aoData[1] = (double)(aoCtrlId->modeNb);   /* number of coefficients */

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
      for ( i = 0 ; i < aoCtrlId->modeNb ; i ++ )
      {
         aoData[21+i] = *(pErrorsVect + i);
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
 * double     *pZernikesVect
 * double     *pErrorsVect
 * double     *pTime
 * long       STATUS;
 *
 * STATUS writeWfsToSynchro(AO_CTRL_ID aoCtrlId, double *pZernikesVect,
 *                          double *pErrorsVect, double *pTime)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId    - Pointer to the AO control context structure
 * > double * pZernikesVect - Vector containing the zernike modes
 * > double * pErrorsVect   - Vector containing the associated errors
 * > double * pTime         - Pointer to the associated time stamp value
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Write tilt and focus data to the synchro bus and make available to the 
 * TCS via gensub records.
 *
 * DESCRIPTION:
 * The tip, tilt and focus values are rotated if necessary and scaled as 
 * appropriate before being written to the synchro bus.
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
 * Pointers to the ao data are given.
 * The contents of these vectors is not protected by mutex so there is a 
 * requirement placed on the calling task that the contents of these vectors 
 * do not change for the duration of this routine which is actually the case.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version (srp)
 * 09-Nov-1998  Write fast tip/tilt to synchro bus (srp)
 * 05-Jan-1999  Add null zernike calculation
 * 28-Mar-2000  Simplified version for P2 only (cb)
 *-
 */

STATUS writeWfsToSynchro
   (
   AO_CTRL_ID aoCtrlId, 
   double     *pZernikesVect, 
   double     *pErrorsVect, 
   double     *pTime
   )
{
   converted  result;
   frame      *f;
   double     *pz;

   /* access frame */

   f = ag2m2;

   pz = pZernikesVect;

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* first rotate the tip and tilt values to the m2 frame of reference */

      result.z2 = (f->cosTheta*(*pz) - f->sinTheta*(*(pz+1))) - f->null[5];
      result.z3 = (f->sinTheta*(*pz) + f->cosTheta*(*(pz+1))) - f->null[6];
      result.z4 = *(pz+2);

      /*result.z4 = (*(pz+2)) - f->null[7];*/

/*
      result.z4 = newDfilter (*(pz+2),2);
*/

      semGive(f->access);
   }
   else
   {
      logMsg("writeWfsToSynchro - unable to get mutex for conversion frame\n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* Scale data and write to the synchro bus, check that pointer has been 
      initialised with null check */

   if (ptrPwfs2 != NULL)
   {
     ptrPwfs2->z1 = (float)(result.z2);
     ptrPwfs2->z2 = (float)(result.z3);
     ptrPwfs2->z3 = (float)(result.z4);

     ptrPwfs2->err1   = (float)(*pErrorsVect);
     ptrPwfs2->err2   = (float)(*(pErrorsVect + 1));
     ptrPwfs2->err3   = (float)(*(pErrorsVect + 2));
     ptrPwfs2->interval  += (float)(0.0001) ;

     ptrPwfs2->time = *pTime ;

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
      ttfData[1] = (double)(aoCtrlId->modeNb);
/*
      ttfData[2] = dfilter(result.z2, 6);
      ttfData[3] = dfilter(result.z3, 7);
      ttfData[4] = dfilter(result.z4, 8);
*/
      ttfData[2] = newDfilter(result.z2, 0);
      ttfData[3] = newDfilter(result.z3, 1);
      ttfData[4] = result.z4; /* focus is already filtered */

      ttfData[5] = (double)(*pErrorsVect);
      ttfData[6] = (double)(*(pErrorsVect + 1));
      ttfData[7] = (double)(*(pErrorsVect + 2));

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
 * Parameters:
 * > struct genSubRecord *pgsub pointer to gensub record
 * 
 * Return value:
 * < status   int      OK or ERROR
 *
 * Globals: 
 * None
 * 
 * External variables:
 * 
 * Requirements:
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 21-Jan-1999: Read in probe angle on input A, add to tcs angle
 * 22-Jan-1999: Problem workaround - if cannot connect to probeAngle record
 *              then set probeAngle to 0.0 but do not logMsg
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add 
 *              or subtract of the Zeiss angle, port C provides an additional 
 *              rotation angle
 *              rotationAngle = 
                tcsAngle + (polarityFudge * (zeiss angle + rotationFudge))
 * 29-Jun-1999: Now the computation of the composite angle is
 *              tcsAngle + tableAngle - armAngle
 * 19-Nov-1999: Add a fudge angle to the tableAngle
 * 26-Nov-1999: Change sign into the compiste angle formula (cb)
 * 13-Dec-1999: Remove limit checks for cass rot angle (cb)
 * 28-Mar-2000: Simplified version for P2 only (cb)
 * 11-Dec-2000: Composite angle now + PA in ttfZero (cb)
 *
 */

/* INDENT ON */

/* ===================================================================== */

long ttfZero 
   (
   struct genSubRecord * pgsub
   )
{
   int    index = 0;
   frame  *f;
   double *ptr;
   double tableAngle = 0.0;
   double fudgeAngle = 0.0; 
   double armAngle = 0.0;
   double compositeAngle = 0.0;

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
      logMsg("ttfZero > %s table angle out of range\n", 
             (int)pgsub->name, 0, 0, 0, 0, 0);
      tableAngle = 0.0;
   }

   f = ag2m2;

   /* access frame */

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* read in the array */

      for (index = 0; index < TTF_ARRAY_SIZE; index++)
      {
          f->null[index] = *(ptr++);
      }

      /* calculate composite correction angle */

      /*compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle - armAngle)*DEGS2RADS;*/

      compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle + armAngle)*DEGS2RADS; /*11dec2000*/

      f->theta      = compositeAngle;
      f->sinTheta   = sin(f->theta);
      f->cosTheta   = cos(f->theta);

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
 * Parameters:
 * > struct genSubRecord *pgsub pointer to gensub record
 *
 * Parameters out:
 * None
 * 
 * Return value:
 * < status   int      OK or ERROR
 *
 * Globals: 
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
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add 
 *              or subtract of the Zeiss angle, port C provides an additional 
 *              rotation angle 
 *              rotationAngle = 
                tcsAngle + (polarityFudge * (zeiss angle + rotationFudge))
 * 28-Mar-2000: simplified version for P2 only
 * 11-Dec-2000: Composite angle now + PA in aoZero (cb)
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

   ptr = (double *) pgsub->j;

   /* read all angles of guide probe */

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
      logMsg("aoZero > %s table angle out of range\n", 
             (int)pgsub->name, 0, 0, 0, 0, 0);
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

      /* calculate composite correction angle */

      /*compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle - armAngle)*DEGS2RADS;*/

      compositeAngle = 
      (tableAngle - f->null[3] + fudgeAngle + armAngle)*DEGS2RADS; /*11dec2000*/

      f->theta      = compositeAngle;
      f->sinTheta   = sin(f->theta);
      f->cosTheta   = cos(f->theta);

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
   *(double *) pgsub->vale = tableAngle;         /* probeAngle (degrees) */
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
 * showFgDiags
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showFgDiags(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from AO circular buffer to gensub outputs for display
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
 * 28-Mar-2000  Modified for taking into new aoP2Lib
 *-
 */

STATUS showFgDiags(struct genSubRecord * pgsub)
{
   int      i;
   int      j;
   int      indexCb;
   int      wfsStatus;
   double   *pCentroids;
   double   *pTotal;
   WFS_VECT localCentroidsVect;
   WFS_VECT localTotalCountsVect;

   if ( (aoCcdIdP2 == NULL) || (aoCbCtrlIdP2 == NULL) )
   {
       /* context structures not yet initialised */
       return(OK);
   }

   /* grab data from the circular buffer */

   indexCb = aoCbCtrlIdP2->position;

   if (( indexCb == 0 ) && ( aoCbCtrlIdP2->counter == 0))  
      return (OK);

   if ( (indexCb < 0) && (indexCb > (CB_CTRL_RECORD_NB - 1)) )
   {
      printf ( "showFgDiags(): position in CB out of range\n" ) ;
      return (OK);
   }

   if ( indexCb != 0 )
      indexCb -= 1;
   else
      indexCb = CB_CTRL_RECORD_NB - 1;

   pCentroids = aoCbCtrlIdP2->cbCtrlRecord[indexCb].centroidsVect;
   pTotal = aoCbCtrlIdP2->cbCtrlRecord[indexCb].totalCountsVect;
   wfsStatus = aoCbCtrlIdP2->cbCtrlRecord[indexCb].wfsStatus;

   j = 0;
   for ( i = 0 ; i < SUBAP_NB ; i ++ )
   {
      if ( aoCcdIdP2->subapUsedVect[i] == TRUE )
      {
         *(localCentroidsVect + 2*i) = *(pCentroids + 2*j);
         *(localCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
         *(localTotalCountsVect + i) = *(pTotal + j);
         j ++;
      }
      else
      {
         *(localCentroidsVect + 2*i) = -999.99;
         *(localCentroidsVect + 2*i+1) = -999.99;
         *(localTotalCountsVect + i) = -999.99;
      }
   }

   *(localCentroidsVect + 2*i) = *(pCentroids + 2*j);
   *(localCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
   *(localTotalCountsVect + i) = *(pTotal + j);

   /* write to genSub outputs */

   *(double *)pgsub->vala = *(localCentroidsVect);
   *(double *)pgsub->valb = *(localCentroidsVect + 1); 
   *(double *)pgsub->valc = *(localCentroidsVect + 2);
   *(double *)pgsub->vald = *(localCentroidsVect + 3);
   *(double *)pgsub->vale = *(localCentroidsVect + 4);
   *(double *)pgsub->valf = *(localCentroidsVect + 5);
   *(double *)pgsub->valg = *(localCentroidsVect + 6);
   *(double *)pgsub->valh = *(localCentroidsVect + 7);
   *(double *)pgsub->vali = *(localCentroidsVect + 8);
   *(double *)pgsub->valj = *(localCentroidsVect + 9);
   *(int *)pgsub->valk = indexCb;
   *(int *)pgsub->vall = wfsStatus;
   *(double *)pgsub->valm = *(localTotalCountsVect + 0);
   *(double *)pgsub->valn = *(localTotalCountsVect + 1);
   *(double *)pgsub->valo = *(localTotalCountsVect + 2);
   *(double *)pgsub->valp = *(localTotalCountsVect + 3);
   *(double *)pgsub->valq = *(localTotalCountsVect + 4);

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
 * 05-Apr-2000 Original version - cb
 *-
 */

STATUS showCbDiags(struct genSubRecord * pgsub)
{
   int           indexCbIm;
   int           indexCbCtrl;
   int           counterCbIm;
   int           counterCbCtrl;

   /* Check the circular buffer structures are initialised */

   if (aoCbCtrlIdP2 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   if (aoCbImIdP2 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   /* Grab data from the circular buffers */

   indexCbCtrl = aoCbCtrlIdP2->position;
   counterCbCtrl = aoCbCtrlIdP2->counter;
   indexCbIm = aoCbImIdP2->position;
   counterCbIm = aoCbImIdP2->counter;

   *(int *)pgsub->vala = indexCbIm;
   *(int *)pgsub->valb = counterCbIm;
   *(int *)pgsub->valc = indexCbCtrl;
   *(int *)pgsub->vald = counterCbCtrl;

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

long    gensubFanDoubles (struct genSubRecord * pgsub)
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


