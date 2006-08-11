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
 * showFgDiag1P2     - Write diagnostic data from PWFS2 FG structure to gensub
 *                     outputs for display
 * showFgDiag2P2     - Write diagnostic data from PWFS2 FG structure to gensub
 *                     outputs for display
 * showAoDiagP2      - Write diagnostic data from PWFS2 aO structure to gensub
 *                     outputs for display
 * showCbDiag        - Write diagnostic data from cb structure to gensub
 *                     outputs for display 
 * showThreshDiagP2  - Write diagnostic data from ao control structure to 
 *                     gensub outputs for display
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
 * 08-Feb-2001: cb - writeWfsToSynchro(), store the zernikes values into CB
 *                   after rotation
 * 29-Mar-2001: cb - fix bug for rotation matrix (two bugs which compensate 
 *                   each others)
 * 22-Aug-2001: cb - Major modifications to have ao Correction with P2 also
 * 14-Sep-2001: cb - Add showThreshDiagP2()
 * 30-Nov-2001: cb - add writeToRm to writeWfsToSynchro
 * 14-Dec-2001: cb - add threshold in real time
 * 21-Jan-2002: cb - add wfsStatus to writeWfsToTcs()
 * 12-Mar-2002: cb - implement butterworth filter to focus
 * 04-Feb-2002: cb - Modify writeWfsToTcs to implement aO proportional law
 *
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

/*
#define RUNNING_AVERAGE
*/

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

#include "aoP2Lib.h"
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
wfs     *ptrPwfs2;
statusBlock *ptrCEM=NULL;
double  ttfData[AO_ARRAY_SIZE+2];
double  aoData[AO_ARRAY_SIZE+2+2]; /* add 2 data for astig0 and astig45 */
double  aoDataTcs[AO_ARRAY_SIZE+2];
float   data[AO_ARRAY_SIZE+2];
float   errors[AO_ARRAY_SIZE+2];
SEM_ID  wfsLock;

WFS_VECT localCentroidsVect;
WFS_VECT localTotalCountsVect;
WFS_VECT localFgCentroidsVect;
WFS_VECT localFgTotalCountsVect;
WFS_VECT localThresholdVect;
WFS_VECT localRealTimeFgThresholdVect;
WFS_VECT localRealTimeAoThresholdVect;
SEM_ID   accessAoData=NULL;
SEM_ID   accessFgData=NULL;

AO_CCD_ID aoCcdIdP2;
AO_CB_AO_CTRL_ID aoCbAoCtrlIdP2;
AO_CB_FG_CTRL_ID aoCbFgCtrlIdP2;
AO_CB_IM_ID aoCbImIdP2;
AO_CTRL_ID aoCtrlIdP2;
double angleWithM1=0.0;
double angleWithM2=0.0;

double sampleData[5][3];
double coeffData[5][3];

AST_ZP_MODEL_ID_STRUCT astigModel;
SEM_ID  accessAstigModel=NULL;

TREF_ZP_MODEL_ID_STRUCT trefoilModel;
SEM_ID  accessTrefoilModel=NULL;

COMA_ZP_MODEL_ID_STRUCT comaModel;
SEM_ID  accessComaModel=NULL;

FOCUS_ZP_MODEL_ID_STRUCT focusModel;
SEM_ID  accessFocusModel=NULL;

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
 * 08-Dec-2000  Coeff are computing in detControl.c and the cutoffFreq set by
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
      sum += sampleData[i][Id]*coeffData[i][Id];

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

   /* create semaphore to prevent multiple access to astigModel data */

   if(accessAstigModel == NULL)
   {
      if ((accessAstigModel = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessAstigModel sem\n");
      }

      /* init structure astigModel */

      astigModel.a1 = 0.0;
      astigModel.a2 = 0.0;
      astigModel.a3 = 0.0;
      astigModel.p1 = 0.0;
      astigModel.p2 = 0.0;
      astigModel.p3 = 0.0;
      astigModel.c = 0.0;
      astigModel.b1 = 0.0;
      astigModel.b2 = 0.0;
      astigModel.b3 = 0.0;
      astigModel.pp1 = 0.0;
      astigModel.pp2 = 0.0;
      astigModel.pp3 = 0.0;
      astigModel.d = 0.0;
      astigModel.astig0 = 0.0;
      astigModel.astig45 = 0.0;
      astigModel.applyModel = 0.0;
      astigModel.gain0 = 1.0;
      astigModel.gain45 = 1.0;
      astigModel.offsetAstig0 = 0.0;
      astigModel.offsetAstig45 = 0.0;
   }

   /* create semaphore to prevent multiple access to trefoilModel data */

   if(accessTrefoilModel == NULL)
   {
      if ((accessTrefoilModel = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessTrefoilModel sem\n");
      }

      /* init structure trefoilModel */

      trefoilModel.a = 0.0;
      trefoilModel.p = 0.0;
      trefoilModel.c = 0.0;
      trefoilModel.b = 0.0;
      trefoilModel.pp = 0.0;
      trefoilModel.d = 0.0;
      trefoilModel.costref = 0.0;
      trefoilModel.sintref = 0.0;
      trefoilModel.applyModel = 0.0;
   }

   /* create semaphore to prevent multiple access to comaModel data */

   if(accessComaModel == NULL)
   {
      if ((accessComaModel = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessComaModel sem\n");
      }

      /* init structure comaModel */

      comaModel.a = 0.0;
      comaModel.p = 0.0;
      comaModel.c = 0.0;
      comaModel.b = 0.0;
      comaModel.pp = 0.0;
      comaModel.d = 0.0;
      comaModel.comaX = 0.0;
      comaModel.comaY = 0.0;
      comaModel.applyModel = 0.0;
   }

   /* create semaphore to prevent multiple access to focusModel data */

   if(accessFocusModel == NULL)
   {
      if ((accessFocusModel = 
          semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) 
          == NULL)
      {
             printf ("unable to create accessFocusModel sem\n");
      }

      /* init structure focusModel */

      focusModel.a1 = 0.0;
      focusModel.a2 = 0.0;
      focusModel.p1 = 0.0;
      focusModel.p2 = 0.0;
      focusModel.c = 0.0;
      focusModel.focus = 0.0;
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

   if(ptrPwfs2 == NULL)
   {
      ptrPwfs2 = (wfs*)&basePtr->pwfs2;
      strncpy(ptrPwfs2->name, "pwfs2", 15);
      ptrPwfs2->time = 0.0;
      ptrPwfs2->interval = 0.0;
   }

   if(ptrCEM == NULL)
   {
      ptrCEM = (statusBlock*)&basePtr->page1;
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
 * aoDataTcs     - ao Data actually sent to Tcs
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
   double ast0, astig0;
   double ast45, astig45;

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

      astig0 = aoData[40];
      astig45 = aoData[41];

      ast0 = aoData[42];
      ast45 = aoData[43];

      /* write whole array to valj for the TCS to pick up */

      /* but make sure there are nothing else than astigmatism sent to TCS */

      aoDataTcs[2]=0.0;
      aoDataTcs[3]=0.0;
      aoDataTcs[4]=0.0;
      aoDataTcs[7]=0.0;
      aoDataTcs[8]=0.0;
      aoDataTcs[9]=0.0;
      aoDataTcs[10]=0.0;
      aoDataTcs[11]=0.0;
      aoDataTcs[12]=0.0;
      aoDataTcs[13]=0.0;
      aoDataTcs[14]=0.0;
      aoDataTcs[15]=0.0;
      aoDataTcs[16]=0.0;
      aoDataTcs[17]=0.0;
      aoDataTcs[18]=0.0;
      aoDataTcs[19]=0.0;
      aoDataTcs[20]=0.0;

     memcpy (pgsub->valj, aoDataTcs, AO_ARRAY_SIZE * sizeof (double));

     /* write Zernike values to vala for display */

      memcpy (pgsub->vala, zernikes, 19 * sizeof (double));

      /* write error values to valb for display */

      memcpy (pgsub->valb, errors, 19 * sizeof (double));

      /* write astig0 to valc for display */
   
      *(double *)pgsub->valc = astig0 ;

      /* write astig45 to vald for display */

      *(double *)pgsub->vald = astig45 ;

      /* write ast0 to vale for display */
   
      *(double *)pgsub->vale = ast0 ;

      /* write ast45 to valf for display */

      *(double *)pgsub->valf = ast45 ;
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
 *                      double *pAoVectAfterRot, double *pAoErrorsVect, 
 *                      double *pTime, int *pWfsStatus)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId      - Pointer to the AO control context structure
 * > double * pAoVect         - Vector containing the zernike modes
 * > double * pAoVectAfterRot - Vector containing the zernike modes after 
 *                              rotation
 * > double * pAoErrorsVect   - Vector containing the associated errors
 * > double * pTime           - Pointer to the associated time stamp value
 * > int * pWfsStatus         - Pointer to the wfs status

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
 * 29-Mar-2001   Fix rotation matrix (cb)
 */

STATUS writeWfsToTcs
   (
   AO_CTRL_ID aoCtrlId,
   double     *pAoVect,
   double     *pAoVectAfterRot,
   double     *pAoErrorsVect,
   double     *pTime,
   int        *pWfsStatus
   )
{
   int       i=0;
   int       index;
   frame     *f;
   converted result;
   double    *pz;

   double    posThresh = (aoCtrlId->aoThreshold);
   double    negThresh = (aoCtrlId->aoThreshold) * -1.0;

   double    posMaxThresh = (aoCtrlId->aoMaxThreshold);
   double    negMaxThresh = (aoCtrlId->aoMaxThreshold) * -1.0;

   double    astig0 =0.0;
   double    astig45=0.0;

   double    ast0 =0.0;
   double    ast45=0.0;

   double    g0;
   double    g45;

   double    z2AfterRot;
   double    z3AfterRot;

   double    z5AfterRot;
   double    z6AfterRot;

   double    z7AfterRot;
   double    z8AfterRot;

   double    z10AfterRot;
   double    z11AfterRot;

   double    z12AfterRot;
   double    z13AfterRot;

   double    z14AfterRot;
   double    z15AfterRot;

   double    z17AfterRot;
   double    z18AfterRot;

   double    z19AfterRot;
   double    z20AfterRot;

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

   if ( (ptrCEM != NULL) && ((int)ptrCEM->statusWord.flags.chopOn) && (!(int)ptrCEM->chopTransition))
   {
     return(OK);
   }


   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      if ( *pWfsStatus != AO_SH_OFF )
      {
         /* first rotate the tip and tilt values to the tcs frame of reference*/
         /* tip and tilt: r * cos(t) and r * sin(t) */

         z2AfterRot = (f->cosTheta*(*pz) + f->sinTheta*(*(pz+1))); 
                     
         z3AfterRot = (f->cosTheta*(*(pz+1)) - f->sinTheta*(*pz));

         if ( z2AfterRot >= posThresh)
            result.z2 =
            (z2AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[0];
         else if ( z2AfterRot <= negThresh )
            result.z2 =
            (z2AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[0];
         else
            result.z2 = z2AfterRot*aoCtrlId->aoScaleFactorVect[0];

         if ( z3AfterRot >= posThresh )
            result.z3 =
            (z3AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[1];
         else if ( z3AfterRot <= negThresh )
            result.z3 =
            (z3AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[1];
         else
            result.z3 = z3AfterRot*aoCtrlId->aoScaleFactorVect[1];

         if (result.z2 >= posMaxThresh)
            result.z2 = posMaxThresh;
         else if (result.z2 <= negMaxThresh)
            result.z2 = negMaxThresh;

         if (result.z3 >= posMaxThresh)
            result.z3 = posMaxThresh;
         else if (result.z3 <= negMaxThresh)
            result.z3 = negMaxThresh;

/*
         result.z2 = (f->cosTheta*(*pz) + f->sinTheta*(*(pz+1))) 
                     * aoCtrlId->aoScaleFactorVect[0];
         result.z3 = (f->cosTheta*(*(pz+1)) - f->sinTheta*(*pz))
                     * aoCtrlId->aoScaleFactorVect[1];
*/

         /* focus : 2*r^2 -1 */

         result.z4 = (*(pz+2)) * aoCtrlId->aoScaleFactorVect[2];

         /* astig0 and astig45: r^2 * cos(2t) and r^2 * sin(2t) */

         ast0 = *(pz+3);
         ast45 = *(pz+4);

         astig0 = *(pz+3) - astigModel.offsetAstig0;
         astig45 = *(pz+4) - astigModel.offsetAstig45;

         g0 = astigModel.gain0;
         g45 = astigModel.gain45;

         z5AfterRot = (g0*f->cos2Theta*(astig0) + g0*f->sin2Theta*(astig45)) 
                      - ((f->null[8])*1000.0)
                      - (astigModel.astig0);
         z6AfterRot = (g45*f->cos2Theta*(astig45) - g45*f->sin2Theta*(astig0)) 
                      - ((f->null[9])*1000.0)
                      - (astigModel.astig45);

         if ( z5AfterRot >= posThresh )
            result.z5 =
            (z5AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[3];
         else if ( z5AfterRot <= negThresh )
            result.z5 =
            (z5AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[3];
         else
            result.z5 = z5AfterRot*aoCtrlId->aoScaleFactorVect[3];

         if ( z6AfterRot >= posThresh )
            result.z6 =
            (z6AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[4];
         else if ( z6AfterRot <= negThresh )
            result.z6 =
            (z6AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[4];
         else
            result.z6 = z6AfterRot*aoCtrlId->aoScaleFactorVect[4];

         if (result.z5 >= posMaxThresh)
            result.z5 = posMaxThresh;
         else if (result.z5 <= negMaxThresh)
            result.z5 = negMaxThresh;

         if (result.z6 >= posMaxThresh)
            result.z6 = posMaxThresh;
         else if (result.z6 <= negMaxThresh)
            result.z6 = negMaxThresh;

/*
         result.z5 = ( (g0*f->cos2Theta*(astig0) + g0*f->sin2Theta*(astig45)) 
                     - ((f->null[8])*1000.0)
                     - (astigModel.astig0) ) * (aoCtrlId->aoScaleFactorVect[3]);
         result.z6 = ( (g45*f->cos2Theta*(astig45) - g45*f->sin2Theta*(astig0)) 
                     - ((f->null[9])*1000.0)
                     - (astigModel.astig45) ) * 
                     (aoCtrlId->aoScaleFactorVect[4]);
*/

         /* comaX and comaY: (3*r^2 - 2) * r * cos(t) and 
         (3*r^2 - 2) * r * sin(t) */

         z7AfterRot = (f->cosTheta*(*(pz+5)) + f->sinTheta*(*(pz+6)))
                      - (comaModel.comaX);

         z8AfterRot = (f->cosTheta*(*(pz+6)) - f->sinTheta*(*(pz+5)))
                      - (comaModel.comaY);

         result.z7 = z7AfterRot*aoCtrlId->aoScaleFactorVect[5];
         result.z8 = z8AfterRot*aoCtrlId->aoScaleFactorVect[6];

/*
         if ( z7AfterRot >= posThresh )
            result.z7 =
            (z7AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[5];
         else if ( z7AfterRot <= negThresh )
            result.z7 =
            (z7AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[5];
         else
            result.z7 = z7AfterRot*aoCtrlId->aoScaleFactorVect[5];

         if ( z8AfterRot >= posThresh )
            result.z8 =
            (z8AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[6];
         else if ( z8AfterRot <= negThresh )
            result.z8 =
            (z8AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[6];
         else
            result.z8 = z8AfterRot*aoCtrlId->aoScaleFactorVect[6];

         if (result.z7 >= posMaxThresh)
            result.z7 = posMaxThresh;
         else if (result.z7 <= negMaxThresh)
            result.z7 = negMaxThresh;

         if (result.z8 >= posMaxThresh)
            result.z8 = posMaxThresh;
         else if (result.z8 <= negMaxThresh)
            result.z8 = negMaxThresh;
*/

/*
         result.z7 = ( (f->cosTheta*(*(pz+5)) + f->sinTheta*(*(pz+6)))
                     - (comaModel.comaX) ) * (aoCtrlId->aoScaleFactorVect[5]);

         result.z8 = ( (f->cosTheta*(*(pz+6)) - f->sinTheta*(*(pz+5)))
                     - (comaModel.comaY) ) * (aoCtrlId->aoScaleFactorVect[6]);
*/

         /* spherical: 6*r^4 - 6*r^2 + 1 */

         result.z9 = (*(pz+7)) * (aoCtrlId->aoScaleFactorVect[7]);

         /* trefoilX and trefoilY: r^3 * cos(3t) and r^3 * sin(3t) */

         z10AfterRot = (f->cos3Theta*(*(pz+8)) + f->sin3Theta*(*(pz+9)))
                       - (trefoilModel.costref);

         z11AfterRot = (f->cos3Theta*(*(pz+9)) - f->sin3Theta*(*(pz+8)))
                       - (trefoilModel.sintref);

         if ( z10AfterRot >= posThresh )
            result.z10 =
            (z10AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[8];
         else if ( z10AfterRot <= negThresh )
            result.z10 =
            (z10AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[8];
         else
            result.z10 = z10AfterRot*aoCtrlId->aoScaleFactorVect[8];

         if ( z11AfterRot >= posThresh )
            result.z11 =
            (z11AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[9];
         else if ( z11AfterRot <= negThresh )
            result.z11 =
            (z11AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[9];
         else
            result.z11 = z11AfterRot*aoCtrlId->aoScaleFactorVect[9];

         if (result.z10 >= posMaxThresh)
            result.z10 = posMaxThresh;
         else if (result.z10 <= negMaxThresh)
            result.z10 = negMaxThresh;

         if (result.z11 >= posMaxThresh)
            result.z11 = posMaxThresh;
         else if (result.z11 <= negMaxThresh)
            result.z11 = negMaxThresh;

/*
         result.z10 = ( (f->cos3Theta*(*(pz+8)) + f->sin3Theta*(*(pz+9)))
                      - (trefoilModel.costref) ) * 
                      (aoCtrlId->aoScaleFactorVect[8]);

         result.z11 = ( (f->cos3Theta*(*(pz+9)) - f->sin3Theta*(*(pz+8)))
                      - (trefoilModel.sintref) ) * 
                      (aoCtrlId->aoScaleFactorVect[9]);
*/

         /* (4*r^2-3) * r^2 * cos(2t) and (4*r^2-3) * r^2 * sin(2t) */

         z12AfterRot = (f->cos2Theta*(*(pz+10)) + f->sin2Theta*(*(pz+11))); 
         z13AfterRot = (f->cos2Theta*(*(pz+11)) - f->sin2Theta*(*(pz+10)));

         if ( z12AfterRot >= posThresh )
            result.z12 =
            (z12AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[10];
         else if ( z12AfterRot <= negThresh )
            result.z12 =
            (z12AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[10];
         else
            result.z12 = z12AfterRot*aoCtrlId->aoScaleFactorVect[10];

         if ( z13AfterRot >= posThresh )
            result.z13 =
            (z13AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[11];
         else if ( z13AfterRot <= negThresh )
            result.z13 =
            (z13AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[11];
         else
            result.z13 = z13AfterRot*aoCtrlId->aoScaleFactorVect[11];

         if (result.z12 >= posMaxThresh)
            result.z12 = posMaxThresh;
         else if (result.z12 <= negMaxThresh)
            result.z12 = negMaxThresh;

         if (result.z13 >= posMaxThresh)
            result.z13 = posMaxThresh;
         else if (result.z13 <= negMaxThresh)
            result.z13 = negMaxThresh;

/*
         result.z12 = (f->cos2Theta*(*(pz+10)) + f->sin2Theta*(*(pz+11))) 
                      * (aoCtrlId->aoScaleFactorVect[10]);
         result.z13 = (f->cos2Theta*(*(pz+11)) - f->sin2Theta*(*(pz+10)))
                      * (aoCtrlId->aoScaleFactorVect[11]);
*/

         /* (10*r^4 -12*r^3 + 3) * r * cos(t) and 
            (10*r^4 -12*r^3 + 3) * r * sin(t) */

         z14AfterRot = (f->cosTheta*(*(pz+12)) + f->sinTheta*(*(pz+13)));
         z15AfterRot = (f->cosTheta*(*(pz+13)) - f->sinTheta*(*(pz+12)));

         if ( z14AfterRot >= posThresh )
            result.z14 =
            (z14AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[12];
         else if ( z14AfterRot <= negThresh )
            result.z14 =
            (z14AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[12];
         else
            result.z14 = z14AfterRot*aoCtrlId->aoScaleFactorVect[12];

         if ( z15AfterRot >= posThresh )
            result.z15 =
            (z15AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[13];
         else if ( z15AfterRot <= negThresh )
            result.z15 =
            (z15AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[13];
         else
            result.z15 = z15AfterRot*aoCtrlId->aoScaleFactorVect[13];

         if (result.z14 >= posMaxThresh)
            result.z14 = posMaxThresh;
         else if (result.z14 <= negMaxThresh)
            result.z14 = negMaxThresh;

         if (result.z15 >= posMaxThresh)
            result.z15 = posMaxThresh;
         else if (result.z15 <= negMaxThresh)
            result.z15 = negMaxThresh;

/*
         result.z14 = (f->cosTheta*(*(pz+12)) + f->sinTheta*(*(pz+13)))
                      * (aoCtrlId->aoScaleFactorVect[12]);
         result.z15 = (f->cosTheta*(*(pz+13)) - f->sinTheta*(*(pz+12)))
                      * (aoCtrlId->aoScaleFactorVect[13]);
*/

         /* 20*r^6 - 30*r^4 + 12*r^2 - 1 */

         result.z16 = (*(pz+14)) * (aoCtrlId->aoScaleFactorVect[14]);

         /* r^4 * cos(4t) and r^4 * sin(4t) */

         z17AfterRot = (f->cos4Theta*(*(pz+15)) + f->sin4Theta*(*(pz+16)));
         z18AfterRot = (f->cos4Theta*(*(pz+16)) - f->sin4Theta*(*(pz+15)));

         if ( z17AfterRot >= posThresh )
            result.z17 =
            (z17AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[15];
         else if ( z17AfterRot <= negThresh )
            result.z17 =
            (z17AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[15];
         else
            result.z17 = z17AfterRot*aoCtrlId->aoScaleFactorVect[15];

         if ( z18AfterRot >= posThresh )
            result.z18 =
            (z18AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[16];
         else if ( z18AfterRot <= negThresh )
            result.z18 =
            (z18AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[16];
         else
            result.z18 = z18AfterRot*aoCtrlId->aoScaleFactorVect[16];

         if (result.z17 >= posMaxThresh)
            result.z17 = posMaxThresh;
         else if (result.z17 <= negMaxThresh)
            result.z17 = negMaxThresh;

         if (result.z18 >= posMaxThresh)
            result.z18 = posMaxThresh;
         else if (result.z18 <= negMaxThresh)
            result.z18 = negMaxThresh;

/*
         result.z17 = (f->cos4Theta*(*(pz+15)) + f->sin4Theta*(*(pz+16)))
                      * (aoCtrlId->aoScaleFactorVect[15]);
         result.z18 = (f->cos4Theta*(*(pz+16)) - f->sin4Theta*(*(pz+15)))
                      * (aoCtrlId->aoScaleFactorVect[16]);
*/

         /* (5*r^2 - 4) * r^3 * cos(3t) and (5*r^2 - 4) * r^3 * cos(3t) */
   
         z19AfterRot = (f->cos3Theta*(*(pz+17)) + f->sin3Theta*(*(pz+18)));
         z20AfterRot = (f->cos3Theta*(*(pz+18)) - f->sin3Theta*(*(pz+17)));

         if ( z19AfterRot >= posThresh )
            result.z19 =
            (z19AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[17];
         else if ( z19AfterRot <= negThresh )
            result.z19 =
            (z19AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[17];
         else
            result.z19 = z19AfterRot*aoCtrlId->aoScaleFactorVect[17];

         if ( z20AfterRot >= posThresh )
            result.z20 =
            (z20AfterRot*3.0 - posThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[18];
         else if ( z20AfterRot <= negThresh )
            result.z20 =
            (z20AfterRot*3.0 - negThresh*2.0)*
            aoCtrlId->aoScaleFactorVect[18];
         else
            result.z20 = z20AfterRot*aoCtrlId->aoScaleFactorVect[18];


         if (result.z19 >= posMaxThresh)
            result.z19 = posMaxThresh;
         else if (result.z19 <= negMaxThresh)
            result.z19 = negMaxThresh;

         if (result.z20 >= posMaxThresh)
            result.z20 = posMaxThresh;
         else if (result.z20 <= negMaxThresh)
            result.z20 = negMaxThresh;
/*
         result.z19 = (f->cos3Theta*(*(pz+17)) + f->sin3Theta*(*(pz+18)))
                      * (aoCtrlId->aoScaleFactorVect[17]);
         result.z20 = (f->cos3Theta*(*(pz+18)) - f->sin3Theta*(*(pz+17)))
                      * (aoCtrlId->aoScaleFactorVect[18]);
*/
      }
      else
      {
         result.z2 = 0.0;
         result.z3 = 0.0;
         result.z4 = 0.0;
         result.z5 = 0.0;
         result.z6 = 0.0;
         result.z7 = 0.0;
         result.z8 = 0.0;
         result.z9 = 0.0;
         result.z10 = 0.0;
         result.z11 = 0.0;
         result.z12 = 0.0;
         result.z13 = 0.0;
         result.z14 = 0.0;
         result.z15 = 0.0;
         result.z16 = 0.0;
         result.z17 = 0.0;
         result.z18 = 0.0;
         result.z19 = 0.0;
         result.z20 = 0.0;
      }

      /* Store the result into pAoVectAfterRot */

      *(pAoVectAfterRot) = result.z2;
      *(pAoVectAfterRot+1) = result.z3;
      *(pAoVectAfterRot+2) = result.z4;
      *(pAoVectAfterRot+3) = result.z5;
      *(pAoVectAfterRot+4) = result.z6;
      *(pAoVectAfterRot+5) = result.z7;
      *(pAoVectAfterRot+6) = result.z8;
      *(pAoVectAfterRot+7) = result.z9;
      *(pAoVectAfterRot+8) = result.z10;
      *(pAoVectAfterRot+9) = result.z11;
      *(pAoVectAfterRot+10) = result.z12;
      *(pAoVectAfterRot+11) = result.z13;
      *(pAoVectAfterRot+12) = result.z14;
      *(pAoVectAfterRot+13) = result.z15;
      *(pAoVectAfterRot+14) = result.z16;
      *(pAoVectAfterRot+15) = result.z17;
      *(pAoVectAfterRot+16) = result.z18;
      *(pAoVectAfterRot+17) = result.z19;
      *(pAoVectAfterRot+18) = result.z20;

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

      for(i = 0; i <= 20; i++)
      {
         aoDataTcs[i] = aoData[i];
      }


      /* copy across error terms */
      for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
      {
         aoData[21+i] = *(pAoErrorsVect +i);
         aoDataTcs[21+i] = *(pAoErrorsVect +i);
      }

      /* Copy intermediate values astig0 and astig45 */

      index = 2*(aoCtrlId->aoModeNb) + 2;
      aoData[index] = astig0;
      aoData[index+1] = astig45;
      aoData[index+2] = ast0;
      aoData[index+3] = ast45;
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
 *                          double *pFgVectAfterRot, double *pFgErrorsVect, 
 *                          double *pTime, int writeToRm)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > AO_CTRL_ID aoCtrlId        - Pointer to the AO control context structure
 * > double *   pFgVect         - Vector containing the zernike modes
 * > double *   pFgVectAfterRot - Vector containing the zernikes modes after
 * >                              rotation
 * > double *   pFgErrorsVect   - Vector containing the associated errors
 * > double *   pTime           - Pointer to the associated time stamp value
 * > int        writeToRm       - Flag to indicate if data are written to RM
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
 * 29-Mar-2001  Fix rotation matrix (cb)
 *-
 */

STATUS writeWfsToSynchro 
   (
   AO_CTRL_ID aoCtrlId,
   double     *pFgVect,
   double     *pFgVectAfterRot,
   double     *pFgErrorsVect,
   double     *pTime,
   int        writeToRm
   )
{
   converted  result;
   frame      *f;
   double     *pz;
   double     averageFocus;
   double     focus;


   /* access frame */

   f = ag2m2;

   pz = pFgVect;


   if ( (ptrCEM != NULL) && ((int)ptrCEM->statusWord.flags.chopOn) && (!(int)ptrCEM->chopTransition))
   {
     return(OK);
   }

   if(semTake(f->access, WFS_TIMEOUT) == OK)
   {
      /* first rotate the tip and tilt values to the m2 frame of reference */

      result.z2 = 
      ( (f->cosTheta*(*pz) + f->sinTheta*(*(pz+1))) 
        - f->null[5] ) * aoCtrlId->fgScaleFactorVect[0];

      result.z3 = 
      ( (f->cosTheta*(*(pz+1)) - f->sinTheta*(*pz)) 
        - f->null[6] ) * aoCtrlId->fgScaleFactorVect[1];

      focus = *(pz+2) - focusModel.focus;

#ifdef RUNNING_AVERAGE
      if ( aoCtrlId->focusCounter == 0 )
      {
         aoCtrlId->previousFocus = focus;
         aoCtrlId->focusCounter ++;
      }

      averageFocus = 
      (aoCtrlId->slidingFocusGain * focus) +
      (aoCtrlId->one_slidingFocusGain * aoCtrlId->previousFocus) ;
#else
      averageFocus = newDfilter (focus,2);
#endif 

      result.z4 = averageFocus * aoCtrlId->fgScaleFactorVect[2];

#ifdef RUNNING_AVERAGE
      aoCtrlId->previousFocus = averageFocus;
#endif

      /* store the vector after rotation into pFgVectAfterRot */

      *pFgVectAfterRot = result.z2;
      *(pFgVectAfterRot + 1) = result.z3;
      *(pFgVectAfterRot + 2) = result.z4;

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

   if ( (ptrPwfs2 != NULL) && (writeToRm == TRUE) )
   {
     ptrPwfs2->z1 = (float)(result.z2);
     ptrPwfs2->z2 = (float)(result.z3);
     ptrPwfs2->z3 = (float)(result.z4);

     ptrPwfs2->err1   = (float)(*(pFgErrorsVect));
     ptrPwfs2->err2   = (float)(*(pFgErrorsVect + 1));
     ptrPwfs2->err3   = (float)(*(pFgErrorsVect + 2));
     ptrPwfs2->interval  += (float)(0.0001);

     ptrPwfs2->time = (double)(*pTime);

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
/*
      ttfData[2] = dfilter(result.z2, (3 + 0));
      ttfData[3] = dfilter(result.z3, (3 + 1));
      ttfData[4] = dfilter(result.z4, (3 + 2));
*/
      ttfData[2] = newDfilter(result.z2, 0);
      ttfData[3] = newDfilter(result.z3, 1);
      ttfData[4] = result.z4;               /* focus is already filtered */

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
 * 11-Dec-2000  CompositeAngle = RT - CR + PA (cb)
 * 29-Mar-2001: Composite angle now * (-1) in ttfZero (cb)
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
      /* null[3] corresponds to the cass rotator angle */

      compositeAngle = angleWithM2 - 
      ((tableAngle - f->null[3] + fudgeAngle + armAngle)*DEGS2RADS); /*11dec00*/

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

   /* Compute focus zero point model */

   if(semTake(accessFocusModel, ZP_MODEL_SEM_TIMEOUT) == OK)
   {
     if (focusModel.applyModel == 0 )
     {
        focusModel.focus = 0.0;
     }
     else
     {
        focusModel.focus =
        focusModel.a1*cos(compositeAngle + focusModel.p1*DEGS2RADS) +
        focusModel.a2*cos(2*compositeAngle + focusModel.p2*DEGS2RADS) +
        focusModel.c;
     }

     semGive (accessFocusModel);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for focusModel \n",
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

   *(double *) pgsub->valj = focusModel.focus;

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
 * 29-Mar-2001: Composite angle now * (-1) in aoZero (cb)
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
   double  applyAstig = 1.0;

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
      applyAstig = 1.0;
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

      compositeAngle = angleWithM1 -
      ((tableAngle - f->null[3] + fudgeAngle + armAngle)*DEGS2RADS);

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

   /* compute astigmatism zero point model */

   if(semTake(accessAstigModel, ZP_MODEL_SEM_TIMEOUT) == OK)
   {
     if (astigModel.applyModel == 0 )
     {
        astigModel.astig0 = 0.0;
        astigModel.astig45 = 0.0;
     }
     else
     {
        astigModel.astig0 = 
        astigModel.a1*cos(compositeAngle + astigModel.p1*DEGS2RADS) +
        astigModel.a2*cos(2*compositeAngle + astigModel.p2*DEGS2RADS) +
        astigModel.a3*cos(4*compositeAngle + astigModel.p3*DEGS2RADS) +
        astigModel.c;

        astigModel.astig45 = 
        astigModel.b1*sin(compositeAngle + astigModel.pp1*DEGS2RADS) +
        astigModel.b2*sin(2*compositeAngle + astigModel.pp2*DEGS2RADS) +
        astigModel.b3*sin(4*compositeAngle + astigModel.pp3*DEGS2RADS) +
        astigModel.d;
     }

     semGive (accessAstigModel);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for astigModel \n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* compute trefoil zero point model */

   if(semTake(accessTrefoilModel, ZP_MODEL_SEM_TIMEOUT) == OK)
   {
     if (trefoilModel.applyModel == 0 )
     {
        trefoilModel.costref = 0.0;
        trefoilModel.sintref = 0.0;
     }
     else
     {
        trefoilModel.costref = 
        trefoilModel.a*cos(3*compositeAngle + trefoilModel.p*DEGS2RADS) +
        trefoilModel.c;

        trefoilModel.sintref = 
        trefoilModel.b*sin(3*compositeAngle + trefoilModel.pp*DEGS2RADS) +
        trefoilModel.d;
     }

     semGive (accessTrefoilModel);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for trefoilModel \n", 
             0, 0, 0, 0 ,0 ,0);
      return(ERROR);
   }

   /* compute coma zero point model */

   if(semTake(accessComaModel, ZP_MODEL_SEM_TIMEOUT) == OK)
   {
     if (comaModel.applyModel == 0 )
     {
        comaModel.comaX = 0.0;
        comaModel.comaY = 0.0;
     }
     else
     {
        comaModel.comaX = 
        comaModel.a*cos(compositeAngle + comaModel.p*DEGS2RADS) +
        comaModel.c;

        comaModel.comaY = 
        comaModel.b*sin(compositeAngle + comaModel.pp*DEGS2RADS) +
        comaModel.d;
     }

     semGive (accessComaModel);
   }
   else
   {
      logMsg("Modify frame - unable to get mutex for comaModel \n", 
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
   /* *(double *) pgsub->valg = f->null[5]; */        /* z2 */
   /* *(double *) pgsub->valh = f->null[6]; */        /* z3 */
   /* *(double *) pgsub->vali = f->null[7]; */        /* z4 */
   *(double *) pgsub->valg = astigModel.astig0;
   *(double *) pgsub->valh = astigModel.astig45;
   *(double *) pgsub->vali = trefoilModel.costref;
   *(double *) pgsub->valj = trefoilModel.sintref;
   *(double *) pgsub->valk = comaModel.comaX;
   *(double *) pgsub->vall = comaModel.comaY;

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiagP2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showAoDiagP2 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao circular buffer of pwfs2 to gensub outputs for 
 * display
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
 * 22-Aug-2001  Modified to display 2x2 centroids data
 *-
 */

STATUS showAoDiagP2
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
   double   *pThresh;
   double   time;

   if (aoCbAoCtrlIdP2 == NULL)
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

      indexCb = aoCbAoCtrlIdP2->position;

      if (( indexCb == 0 ) && ( aoCbAoCtrlIdP2->counter == 0))
         return (OK);

      if ( (indexCb < 0) && (indexCb > (CB_AO_CTRL_RECORD_NB - 1)) )
      {
         printf ( "showAoDiag1(): position in CB out of range\n" ) ;
         return (OK);
      }

      if ( indexCb != 0 )
         indexCb -= 1;
      else
         indexCb = CB_AO_CTRL_RECORD_NB - 1;

      pThresh = aoCbAoCtrlIdP2->cbAoCtrlRecord[indexCb].thresholdVect;
      pCentroids = aoCbAoCtrlIdP2->cbAoCtrlRecord[indexCb].centroidsVect;
      pTotal = aoCbAoCtrlIdP2->cbAoCtrlRecord[indexCb].totalCountsVect;
      wfsStatus = aoCbAoCtrlIdP2->cbAoCtrlRecord[indexCb].wfsStatus;
      time = aoCbAoCtrlIdP2->cbAoCtrlRecord[indexCb].time;

      j = 0;
      for ( i = 0 ; i < aoCcdIdP2->subapNb ; i ++ )
      {
          if ( aoCcdIdP2->subapUsedVect[i] == TRUE )
          {
             *(localCentroidsVect + 2*i) = *(pCentroids + 2*j);
             *(localCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
             *(localTotalCountsVect + i) = *(pTotal + j);
             *(localRealTimeAoThresholdVect + i) = *(pThresh + j);
             j ++ ;
          }
          else
          {
             *(localCentroidsVect + 2*i) = -99.99;
             *(localCentroidsVect + 2*i+1) = -99.99;
             *(localTotalCountsVect + i) = -99.99;
             *(localRealTimeAoThresholdVect + i) = -99.99;
          }
      }

      *(localTotalCountsVect + i) = *(pTotal + j);

      /* data intact, write to genSub outputs */

      *(int *)pgsub->vala = indexCb;
      *(int *)pgsub->valb = wfsStatus;
      *(double *)pgsub->valc = *(localCentroidsVect+0); /* subap 1 */
      *(double *)pgsub->vald = *(localCentroidsVect+1); 
      *(double *)pgsub->vale = *(localCentroidsVect+2); /* subap 2 */
      *(double *)pgsub->valf = *(localCentroidsVect+3); 
      *(double *)pgsub->valg = *(localCentroidsVect+4); /* subap 3 */
      *(double *)pgsub->valh = *(localCentroidsVect+5); 
      *(double *)pgsub->vali = *(localCentroidsVect+6); /* subap 4 */
      *(double *)pgsub->valj = *(localCentroidsVect+7); 
      *(double *)pgsub->valk = *(localTotalCountsVect + 0); /* total subap 1 */
      *(double *)pgsub->vall = *(localTotalCountsVect + 1); /* total subap 2 */
      *(double *)pgsub->valm = *(localTotalCountsVect + 2); /* total subap 3 */
      *(double *)pgsub->valn = *(localTotalCountsVect + 3); /* total subap 4 */
      *(double *)pgsub->valo = *(localTotalCountsVect + 4); /* whole total */
      *(double *)pgsub->valp = time;
      *(double *)pgsub->valq = *(localRealTimeAoThresholdVect + 0); /* subap 1*/
      *(double *)pgsub->valr = *(localRealTimeAoThresholdVect + 1); /* subap 2*/
      *(double *)pgsub->vals = *(localRealTimeAoThresholdVect + 2); /* subap 3*/
      *(double *)pgsub->valt = *(localRealTimeAoThresholdVect + 3); /* subap 4*/

      semGive (accessAoData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag1P2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long    showFgDiag1P2(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from fg circular buffer of pwfs2 to gensub outputs for 
 * display
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
 * 22-Aug-2001  Modified to display 2x2 centroids data
 *-
 */

STATUS showFgDiag1P2(struct genSubRecord * pgsub)
{
   int i=0;
   int j=0;
   int indexCb;
   int wfsStatus;
   double *pGuide;
   double *pTotal;
   double *pCentroids;
   double *pThresh;
   double time;

   if (aoCbFgCtrlIdP2 == NULL )
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

      indexCb = aoCbFgCtrlIdP2->position;

      if (( indexCb == 0 ) && ( aoCbFgCtrlIdP2->counter == 0))
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

      wfsStatus = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].wfsStatus;
      time = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].time;
      pGuide = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].guidesVect;
      pTotal = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].totalCountsVect;
      pCentroids = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].centroidsVect;
      pThresh = aoCbFgCtrlIdP2->cbFgCtrlRecord[indexCb].thresholdVect;

      j = 0;
      for ( i = 0 ; i < aoCcdIdP2->subapNb ; i ++ )
      {
          if ( aoCcdIdP2->subapUsedVect[i] == TRUE )
          {
             *(localFgCentroidsVect + 2*i) = *(pCentroids + 2*j);
             *(localFgCentroidsVect + 2*i+1) = *(pCentroids + 2*j+1);
             *(localFgTotalCountsVect + i) = *(pTotal + j);
             *(localRealTimeFgThresholdVect + i) = *(pThresh + j);
             j ++ ;
          }
          else
          {
             *(localFgCentroidsVect + 2*i) = -99.99;
             *(localFgCentroidsVect + 2*i+1) = -99.99;
             *(localFgTotalCountsVect + i) = -99.99;
             *(localRealTimeFgThresholdVect + i) = -99.99;
          }
      }

      *(localFgTotalCountsVect + i) = *(pTotal + j);

      /* data intact, write to genSub outputs */

      *(int *)pgsub->vala = indexCb;
      *(int *)pgsub->valb = wfsStatus;
      *(double *)pgsub->valc = *(pGuide);
      *(double *)pgsub->vald = *(pGuide + 1);
      *(double *)pgsub->vale = *(localFgCentroidsVect + 0); /* subap 1 */
      *(double *)pgsub->valf = *(localFgCentroidsVect + 1);
      *(double *)pgsub->valg = *(localFgCentroidsVect + 2); /* subap 2 */
      *(double *)pgsub->valh = *(localFgCentroidsVect + 3);
      *(double *)pgsub->vali = *(localFgCentroidsVect + 4); /* subap 3 */
      *(double *)pgsub->valj = *(localFgCentroidsVect + 5);
      *(double *)pgsub->valk = *(localFgCentroidsVect + 6); /* subap 4 */
      *(double *)pgsub->vall = *(localFgCentroidsVect + 7);
      *(double *)pgsub->valm = *(localFgTotalCountsVect+0); /* total subap 1 */
      *(double *)pgsub->valn = *(localFgTotalCountsVect+1); /* total subap 2 */
      *(double *)pgsub->valo = *(localFgTotalCountsVect+2); /* total subap 3 */
      *(double *)pgsub->valp = *(localFgTotalCountsVect+3); /* total subap 4 */
      *(double *)pgsub->valq = *(localFgTotalCountsVect+4); /* whole total */
      *(double *)pgsub->valr = time;


      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiag2P2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long    showFgDiag2P2(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from fg circular buffer of pwfs2 to gensub outputs for 
 * display
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
 * 14-Dec-2001  Add threshold in real time
 *-
 */

STATUS showFgDiag2P2(struct genSubRecord * pgsub)
{

   if(semTake(accessFgData, WFS_TIMEOUT) != OK)
   {
      logMsg("timeout on mutex access accessFgData\n", 0, 0, 0, 0, 0, 0);
      return(ERROR);
   }
   else
   {
      /* data intact, write to genSub outputs */

      *(double *)pgsub->vala = *(localRealTimeFgThresholdVect + 0);
      *(double *)pgsub->valb = *(localRealTimeFgThresholdVect + 1);
      *(double *)pgsub->valc = *(localRealTimeFgThresholdVect + 2);
      *(double *)pgsub->vald = *(localRealTimeFgThresholdVect + 3);

      semGive (accessFgData);
   }

   return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showCbDiag
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showCbDiag(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from circular buffers of to gensub outputs for 
 * display
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

STATUS showCbDiag(struct genSubRecord * pgsub)
{
   int           indexCbIm;
   int           indexCbAoCtrl;
   int           indexCbFgCtrl;
   int           counterCbIm;
   int           counterCbAoCtrl;
   int           counterCbFgCtrl;

   /* Check the circular buffer structures are initialised */

   if (aoCbAoCtrlIdP2 == NULL )
   {
       /* context structure not yet initialised */
       return(OK);
   }

   if (aoCbFgCtrlIdP2 == NULL )
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

   indexCbAoCtrl = aoCbAoCtrlIdP2->position;
   counterCbAoCtrl = aoCbAoCtrlIdP2->counter;

   indexCbFgCtrl = aoCbFgCtrlIdP2->position;
   counterCbFgCtrl = aoCbFgCtrlIdP2->counter;

   indexCbIm = aoCbImIdP2->position;
   counterCbIm = aoCbImIdP2->counter;

   *(int *)pgsub->vala = indexCbIm;
   *(int *)pgsub->valb = counterCbIm;
   *(int *)pgsub->valc = indexCbAoCtrl;
   *(int *)pgsub->vald = counterCbAoCtrl;
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

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showThreshDiagP2
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long   status;
 *
 * long showThreshDiagP2 (struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)   pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from aoCtrlId structure to gensub outputs for display
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
 * 14-Sep-2001  Original creation
 *-
 */

STATUS showThreshDiagP2
   (
   struct genSubRecord * pgsub
   )
{
   int i = 0;
   int j = 0;
   double   *pThreshold;

   if (aoCtrlIdP2 == NULL)
   {
      /* context structure not yet initialised */
      return(OK);
   }

   /* grab data from the ao control structure */

   pThreshold = aoCtrlIdP2->thresholdVect;

   j = 0;
   for ( i = 0 ; i < aoCcdIdP2->subapNb ; i ++ )
   {
       if ( aoCcdIdP2->subapUsedVect[i] == TRUE )
       {
          *(localThresholdVect + i) = *(pThreshold + j);
          j ++ ;
       }
       else
       {
          *(localThresholdVect + i) = -99.99;
       }
   }

   /* write to genSub outputs */

   *(double *)pgsub->vala = *(localThresholdVect+0);
   *(double *)pgsub->valb = *(localThresholdVect+1);
   *(double *)pgsub->valc = *(localThresholdVect+2);
   *(double *)pgsub->vald = *(localThresholdVect+3);

   return (OK);
}
