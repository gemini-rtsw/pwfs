static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: detControl.c,v 1.22 2002-02-23 02:27:22 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   detControl
 *
 *   FILENAME:
 *   detControl.c
 *
 *   PURPOSE:
 *   Detector controller application code for PWFS2
 *
 *   DESCRIPTION:
 *   This file contains the detector controller application code for PWFS2.
 *   The code runs in a VxWorks task.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   aoP2Lib.h
 *   gemTypes.h
 *   wfsLib.h
 *   epToVxLib.h
 *   sdsuLib.h
 *   errorLib.h
 *
 *   AUTHORS:
 *   Nick Dillon
 *   Steven Beard
 *
 *INDENT-OFF*
 *   22 Feb 2002: CB - detInit: init all geometry SIR records
 *   21 Feb 2002: CB - detObserveStart: init prevThresh when ggFrame=0
 *                     and AO_MODE_CLOSED_LOOP
 *   12 Feb 2002: CB - averageImageNb was not init in the aoCbAoCtrlId. 
 *                     Now it is done.
 *   07 Feb 2002: CB - Reset signal processing when detInit and detReset and
 *                     reject observe command if signal processing not
 *                     initialized
 *   18 Jan 2002: CB - add detPowerOn
 *   02 Jan 2002: CB - detDhsInit started from detControl now
 *   14 Dec 2001: CB - Compute threshold per sub-apertures in real time
 *   30 Nov 2001: CB - Add flag writeToRm to aoGlobalGuide() and 
 *                     aoGuideAndFocus()
 *   21 Aug 2001: CB - Major modifications to have aO correction with P2 also
 *                     In fact detControl.c is now a copy of pwfs1
 *   15 Jun 2001: CB - reject observe command if exposure time < 0.01 and no
 *                     binning
 *   02 Apr 2001: CB - Add adc0, adc1, adc2, adc3 sir records
 *   05 Mar 2001: CB - Fix bug dhsQlRate when only 1 frame
 *   20 Feb 2001: CB - add detDhsConnected flag and dhsCon sir record
 *   07 Feb 2001: CB - ADC offset now for bin and no bin
 *   06 Feb 2001: CB - also move all the DATREC_CONTEXT into detControl.h
 *   12 jan 2001: CB - read the detector init file according to the site
 *   10 jan 2001: CB - read the ao init file according to the site
 *   11 dec 2000: CB - add detSigReset
 *   31 oct 2000: CB - remove error when stop observation not in progress
 *   27 Oct 2000: CB - add possibility to change butterworth filter
 *                     parameters when probe arm guiding
 *   25 Oct 2000: CB - add fast guide and focus when computing the threshold 
 *                     (with spots method only)
 *                     add update scale factor when computing average flux
 *                     threshold
 *                     Replace aoRmsNoiseDarkCompute aoRmsNoiseImageCompute
 *                     add aoSaveCbIm and aoSaveCbCtrl sir records
 *   05 Sep 2000: CB - modify detHeadTempGet to compute the average 
 *                     temperature over 1 sample
 *   07 Jun 2000: CB - add detSigModeSeqDark + detSigModeFgCoadd
 *                     rename fast guide by global guide...
 *                     add directory for save CB
 *   14 Apr 2000: CB - add detDhsDisplay
 *   12 Apr 2000: CB - add detType, detId, dataLabel, intTime, nexpRQ,
 *                     nexp, nframes, bunit, exposedRQ, exposed, utstart, 
 *                     utend, elapsed sir records
 *   11 Apr 2000: CB - replace detSigMode by several detSigModexxx cad
 *   04 Apr 2000: CB - 2 cb + save cb + coadd file name
 *   03 Apr 2000: CB - Add detDhsReconnect command 
 *                     Add temp at -20C per default, add detHeadTempGet()
 *                     Add readout mode per default to NONE
 *   30 Mar 2000: CB - Add detFrameSize command + 3 SIR records for state of 
 *                     signal processing
 *   28 Mar 2000: CB - DHS tasks started by dhs and not WFS are running with 
 *                     priority 1. For now do not use the WFS DHS task
 *   23 Mar 2000: CB - Major modification: Create a DHS task
 *   16 Mar 2000: CB - Major modification: Include aoPwfs2Lib and remove ospLib
 *   13 Mar 2000: CB - Replace dc:testResults per testResults
 *   22 Nov 1999: CB - Now telescope name is read from TCS ie TELESCOP and
 *                     OBSERVAT
 *   02 Nov 1999: CB - debug detGeometry, allow binning x2,y2
 *                     modify ospUpdate, ospNewTrackingAndFocus
 *   01 Nov 1999: CB - Create detCreateFileName -> combine path and file
 *                     name and remove .fits at the end
 *                     New observe command + new DHS I/F + new signal
 *                     processing commands
 *   13 Oct 1999: CB - Download the DSP until it works
 *   09 Apr 1999: CB - simplified version for PWFS2 only
 *INDENT-ON*
 *-
 */

/***************************************************************** Includes ***/

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <pipeDrv.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <semLib.h>
#include <timers.h>
#include <float.h>
#include <math.h>
#include <selectLib.h>
#include <sirRecord.h>
#include "menuCarstates.h"

#include "dhs.h"                    /* Include Data Handling System constants */

#include "timeLib.h"
#include "slalib.h"
#include "astLib.h"

#include "fitsio.h"

#include "gemTypes.h"
#include "timeoutLib.h"
#include "epToVxLib.h"
#include "wfsLib.h"
#include "wfsWcs.h"
#include "errorLib.h"
#include "sdsuLib.h"
#include "aoP2Lib.h"
#include "synchroMap.h"
#include "wfsControl.h"
#include "wfsDb.h"
#include "cicsLib.h"

#include "detControl.h"

/****************************************************************** Defines ***/

/*#define DEBUG*/               /* Define this macro to enable debug messages.*/

#define DEBUG_DOWNLOAD          /* Define this macro to enable debug messages */
                                /* when downloading DSP code                  */

#define DEBUG_DHS               /* Define this macro to enable debug messages */
                                /* for DHS only.                              */

#define DHS_WAIT_TIMEOUT   3600 /* Timeout waiting for DHS semaphore 60s      */

#define OBS_WAIT_TIMEOUT   1200 /* Timeout waiting for obs sync semaphore 20s */

#ifndef PI
#define PI 3.14159265358979
#endif

/******************************************** Macro for checking DHS status ***/

#define CHECK_DHS(dhsErrno) detDhsCheckErrno ((dhsErrno),__LINE__, __FILE__)

/********************************************************* Global variables ***/

char    pDetDhsClientName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                              /* Name of DHS client = Instrument name.        */
                              /* Assumed the same for all WFSs on CPU.        */

char    pDetDhsHostName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                              /* Name of host running DHS data server.        */
                              /* Assumed the same for all WFSs.               */

char    pDetDhsServerName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                              /* Name of DHS data server.                     */
                              /* Assumed the same for all WFSs.               */

int     detDhsNumConnect;     /* Maximum number of DHS connections            */

BOOL    detDhsInitialised = FALSE; 
                              /* Flag to determine whether the DHS            */
                              /* library has been initialised.                */

BOOL    detDhsConnected = NOT_CONNECTED;
                                   /* Flag to determine whether the WFS is    */
                                   /* connected to the DHS.                   */

int     detDhsTaskId = 0;     /* Task Id of the dhs task                      */

DHS_CONNECT detDhsConnection = NULL; 
                              /* DHS connection ID for this controller.       */

SEM_ID  detDhsStartSem = NULL;/* Semaphore to start DHS when starting a       */
                              /* new observation                              */

SDSU_ID detSdsuIdP2 = NULL;   /* SDSU context structure for PWFS2.            */

OBS_ID  detObsIdP2 = NULL;    /* Observation context structure for PWFS2.     */

uint32  detControlStop = 0x0; /* This bit mask provides a way of aborting     */
                              /* the detector control task(s) cleanly.        */
                              /* Each task will keep running until it         */
                              /* sees its own bit in this mask set.           */

int     readTempReadyFlag=FALSE;
                              /* Flag used by detHeadTempGet() to check       */
                              /* if we are ready to read temperature from     */
                              /* SDSU controller                              */

/***************************************************** External global data ***/

extern int sdsuFrameLost ;         /* Defined in sdsuLib.c                    */

extern wfs *ptrPwfs2;              /* Pointer to the reflective memory page   */
                                   /* defined in writeZernikes.c              */

extern AO_CCD_ID aoCcdIdP2;        /* Pointer to the ccd geometry structure   */
                                   /* defined in writeZernikes.c              */

extern AO_CB_AO_CTRL_ID aoCbAoCtrlIdP2; 
                                   /* Pointer to the aO control circular      */
                                   /* buffer defined in writeZernikes.c       */

extern AO_CB_FG_CTRL_ID aoCbFgCtrlIdP2; /* Pointer to the FG control circular */
                                   /* buffer defined in writeZernikes.c       */

extern AO_CB_IM_ID aoCbImIdP2;     /* Pointer to the image circular buffer    */
                                   /* defined in writeZernikes.c              */

extern AO_CTRL_ID aoCtrlIdP2 ;     /* Pointer to the aO control context       */
                                   /* structure defined in writeZernikes.c    */

extern double sampleData[5][3];    /* Samples for butterworth filter          */
                                   /* defined in writeZernikes.c              */

extern double coeffData[5];        /* Coefficients for butterworth filter     */
                                   /* defined in writeZernikes.c              */

extern AST_ZP_MODEL_ID_STRUCT astigModel;
                                   /* Astig model defined in writeZernikes.c  */

extern SEM_ID accessAstigModel;    /* Semaphore Astig model defined in        */
                                   /* writeZernikes.c                         */

extern TREF_ZP_MODEL_ID_STRUCT trefoilModel;
                                   /* Trefoil model defined in writeZernikes.c*/

extern SEM_ID accessTrefoilModel;  /* Semaphore Trefoil model defined in      */
                                   /* writeZernikes.c                         */

extern COMA_ZP_MODEL_ID_STRUCT comaModel;
                                   /* Coma model defined in writeZernikes.c   */

extern SEM_ID accessComaModel;     /* Semaphore Coma model defined in         */
                                   /* writeZernikes.c                         */

extern FOCUS_ZP_MODEL_ID_STRUCT focusModel;
                                   /* Focus model defined in writeZernikes.c  */

extern SEM_ID accessFocusModel;    /* Semaphore Focus model defined in        */
                                   /* writeZernikes.c                         */

extern double angleWithM1;         /* Angle with M1 (equivalent to the one    */
                                   /* contained in aoCtrlIdP2) defined in     */
                                   /* writeZernikes.c                         */

extern double angleWithM2;         /* Angle with M2 (equivalent to the one    */
                                   /* contained in aoCtrlIdP2) defined in     */
                                   /* writeZernikes.c                         */

/******************************************************* External functions ***/

extern void ImpMaster ();

/******************************** Private functions - one for each command. ***/

STATUS detReadDefaultDspCcdGeometry (SDSU_ID sdsuId, AO_CCD_ID aoCcdId);

STATUS detSetDefaultDspCcdGeometry (SDSU_ID sdsuId, AO_CCD_ID aoCcdId);

LOCAL uint32   detChop  (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                         SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detExposure (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detObstype (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSetWcs (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detObserveStart (CAD_CMD_CONTEXT cadCmdContext,
                                int commandNumber, SDSU_ID sdsuId, 
                                OBS_ID obsId);

LOCAL uint32   detStop (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detAbort (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                         SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detInit (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID * pSdsuId, OBS_ID obsId, uint32 * pVmeAddress, 
                        int * pMaxFrames);

LOCAL uint32   detReset (const char * pWfsName, const char * pRecordPrefix, 
                         CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                         SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detTest (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSave (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detGeometry (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId, 
                            long * pOffsetFullVect, long * pOffsetBinVect);

LOCAL uint32   detPrimitive (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                             SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detPowerOn (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detMode   (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detOffset (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detTemp   (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detFrameSize (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                             SDSU_ID sdsuId, OBS_ID obsId, 
                             long *pOffsetFullVect, long *pOffsetBinVect);

LOCAL uint32   detDhsReconnect (CAD_CMD_CONTEXT cadCmdContext,
                                int commandNumber, SDSU_ID sdsuId, 
                                OBS_ID obsId);

LOCAL uint32   detDhsDisplay (CAD_CMD_CONTEXT cadCmdContext,
                              int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigInit (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigInitAoGain (CAD_CMD_CONTEXT cadCmdContext, 
                                 int commandNumber, SDSU_ID sdsuId, 
                                 OBS_ID obsId);

LOCAL uint32   detSigInitBw (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                             SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigInitFgGain (CAD_CMD_CONTEXT cadCmdContext, 
                                 int commandNumber, SDSU_ID sdsuId, 
                                 OBS_ID obsId);

LOCAL uint32   detSigModeNone (const char * pRecordPrefix,
                               CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                               SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigReset (const char * pRecordPrefix,
                            CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigModeDark (const char * pRecordPrefix,
                               CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                               SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigModeGg (const char * pRecordPrefix,
                             CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                             SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigModeGgAo (const char * pRecordPrefix,
                               CAD_CMD_CONTEXT cadCmdContext,
                               int commandNumber, SDSU_ID sdsuId,
                               OBS_ID obsId);

LOCAL uint32   detSigModeAo (const char * pRecordPrefix,
                             CAD_CMD_CONTEXT cadCmdContext,
                             int commandNumber, SDSU_ID sdsuId,
                             OBS_ID obsId);

LOCAL uint32   detSigModeCoadd (const char * pRecordPrefix,
                                CAD_CMD_CONTEXT cadCmdContext,
                                int commandNumber, SDSU_ID sdsuId,
                                OBS_ID obsId);

LOCAL uint32   detSigModeThresh (const char * pRecordPrefix,
                                 CAD_CMD_CONTEXT cadCmdContext,
                                 int commandNumber, SDSU_ID sdsuId,
                                 OBS_ID obsId);

LOCAL uint32   detSigModeGgCoadd (const char * pRecordPrefix,
                                  CAD_CMD_CONTEXT cadCmdContext,
                                  int commandNumber, SDSU_ID sdsuId,
                                  OBS_ID obsId);

LOCAL uint32   detSigModeFgCoadd (const char * pRecordPrefix,
                                  CAD_CMD_CONTEXT cadCmdContext,
                                  int commandNumber, SDSU_ID sdsuId,
                                  OBS_ID obsId);

LOCAL uint32   detSigModeSeq (const char * pRecordPrefix,
                              CAD_CMD_CONTEXT cadCmdContext,
                              int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigModeTotal (const char * pRecordPrefix,
                                CAD_CMD_CONTEXT cadCmdContext,
                                int commandNumber, SDSU_ID sdsuId,
                                OBS_ID obsId);

LOCAL uint32   detSigModeFgFocus (const char * pRecordPrefix,
                                  CAD_CMD_CONTEXT cadCmdContext,
                                  int commandNumber, SDSU_ID sdsuId,
                                  OBS_ID obsId);

LOCAL uint32   detSigModeFgFocusAo (const char * pRecordPrefix,
                                    CAD_CMD_CONTEXT cadCmdContext,
                                    int commandNumber, SDSU_ID sdsuId,
                                    OBS_ID obsId);

LOCAL uint32   detSigSaveCb (CAD_CMD_CONTEXT cadCmdContext,
                             int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigMeasAoIm (const char * pRecordPrefix,
                               CAD_CMD_CONTEXT cadCmdContext, int commandNumber, 
                               SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32   detSigCompAoMat (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                                SDSU_ID sdsuId, OBS_ID obsId); 

LOCAL uint32   detSigModeSeqDark (const char * pRecordPrefix,
                                  CAD_CMD_CONTEXT cadCmdContext, 
                                  int commandNumber, SDSU_ID sdsuId, 
                                  OBS_ID obsId);
 
LOCAL uint32   detInitObserveRecord (const char * pRecordPrefix, long * pNExp,
                                     double * pExpTime, long * pOutOption);

LOCAL uint32 detSigInitModAst (CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                               SDSU_ID sdsuId, OBS_ID obsId);

LOCAL uint32 detSigInitModTref (CAD_CMD_CONTEXT cadCmdContext, 
                                int commandNumber, SDSU_ID sdsuId, 
                                OBS_ID obsId);

LOCAL uint32 detSigInitModComa (CAD_CMD_CONTEXT cadCmdContext, 
                                int commandNumber, SDSU_ID sdsuId, 
                                OBS_ID obsId);

LOCAL uint32 detSigInitModFoc (CAD_CMD_CONTEXT cadCmdContext, 
                               int commandNumber, SDSU_ID sdsuId, 
                               OBS_ID obsId);

/******************************************* Plus some additional functions ***/

OBS_ID detObsContextCreate( void );

STATUS detDownloadDefault (const char * pWfsName, const char * pRecordPrefix, 
                           SDSU_ID sdsuId);

STATUS detDhsConnect ();

STATUS detDhsInit ();

STATUS detDhsTaskOpen ();

void   detDhsTask ();

void   detDhsCheckErrno (const DHS_STATUS dhsErrno, const int line,
                         const char * filename);

STATUS detDhsCheckCmdStatus (const DHS_TAG dhsTag);

void   detObserveEnd (SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );

STATUS detFrameUnscramble (const int xPixels, const int yPixels, 
                           const int outputs, SDSU_FRAME * inFrame, 
                           float * outBuffer );

STATUS detFrameScramble (const int xPixels, const int yPixels,
                         const int outputs, float * inBuffer,
                         uint16 * outBuffer );

void   detObserveTimeout (timer_t timeId, int obsIdInt);

STATUS detSimulateData (const int xPixels, const int yPixels, const int option,
                        SDSU_FRAME *pFrameBuffer);

STATUS detCreateFileName ( char * pFilePath, char * pOutFileName,
                           char * pFullOutFileName);

STATUS detReadFitsHeaderInt (char * fileName, int nKey, char ** keyName,
                             int * keyVal);

STATUS detReadFitsImageUint16 (uint16 * pImageBuffer, char * fileName,
                               int buffSize);

STATUS detWriteFits (char * filename, OBS_ID obsId, int xPixels, int yPixels,
                     float * pFrameBuffer);

uint32 detSimulateImage (int xPixels, int yPixels, float * pImage);

uint32 detComputeCoeffButterworth (double expTime, double cutoffFreq,
                                   double * pCoeffData);

uint32 detContInit (char * pInitFileName, uint32 * pTempCode,
                    uint32 * pTempCoeff, long *pOffsetFullVect, 
                    long * pOffsetBinVect, char * pCcdSn);

uint32 detGetSirContext (const char * pRecordPrefix, OBS_ID obsId);

uint32 detWriteDefSirContext (OBS_ID obsId);


/* -------------------------------------------------------------------------- */

STATUS   detControl
   (
   const char *   pWfsName,         /* Name of wavefront sensor "p2"          */
   const char *   pRecordPrefix     /* Record name prefix                     */
   )
{
   /* Variables associated with VxWorks environment. */

   int            taskOptions;      /* VxWorks task options.                  */
   STATUS         (* pipeCreate) ();/* Pointer to appropriate pipeCreate func.*/

   /* Variables associated with CAD/CAR/genSub command protocol. */

   CAD_CMD_CONTEXT cadCmdContext;   /* CAD command context structure.         */
   int            commandNumber;    /* Command number.                        */
   int            updateNumber;     /* Data update ID number.                 */
   uint32         errorNumber;      /* Error number reported by task.         */

   /* Variables associated with the use of the select() facility. */

   struct fd_set  updateFds;        /* File descr. structure for select().    */
   int            widthSelect;      /* Number of file descrs. to monitor.     */

   /* Variables associated with genSub records. */

   GSUB_DATA_CONTEXT   dataUpdateContext; /* Data update context structure.   */

   /* Variables associated with the SDSU controller. */

   uint32         vmeAddress = 0;   /* VME address of SDSU controller. (Set to*/
                                    /* 0 if the controller is not installed   */
                                    /* and is to be simulated).               */
   BOOL           simulate;         /* TRUE if controller is to be simulated. */
   BOOL           initFailed = FALSE; /* Set TRUE if a significant but non    */
                                    /* fatal error occurs during init.        */
                                    /* (Fatal errors will cause the task to   */
                                    /* abort completely).                     */
   BOOL           initWarning = FALSE; /* Set TRUE if a warning occurs during */
                                    /* initialisation.                        */
   long           initState;        /* Initialisation state.                  */

   SDSU_ID        sdsuId = NULL;    /* SDSU context structure.                */

   OBS_ID         obsId = NULL;     /* Observation context structure.         */

   uint32         detControlStopMask;
                                    /* Mask for detecting which detControlStop*/
                                    /* bit refers to this detector controller.*/
   uint32         tryDownload ;     /* Counter to stop attempt for downloading*/
                                    /* DSP code                               */
   uint32         tempCode;         /* Target temperature code                */
   uint32         tempCoeff;        /* Coefficient for temperature control    */
   long           i;                /* index                                  */
   long           offsetFullVect[4];/* ADC offset vector - no binning.        */
   long           offsetBinVect[4]; /* ADC offset vector - binning.           */
   long           offsetVect[4];    /* ADC offset vector                      */


   char           detContInitFileName [ STRING_SIZE ] ;
                                    /* Full Name of the detector controller   */
                                    /* init file                              */

   /* Variables associated with active optics */

   AO_CCD_ID    aoCcdId = NULL;     /* AO CCD geometry context structure      */
   AO_CTRL_ID   aoCtrlId = NULL;    /* AO control context structure           */
   AO_CB_IM_ID  aoCbImId = NULL;    /* AO image circular buffer context       */
                                    /* structure                              */
   AO_CB_AO_CTRL_ID aoCbAoCtrlId = NULL; 
                                    /* aO control circular buffer context     */
                                    /* structure                              */
   AO_CB_FG_CTRL_ID aoCbFgCtrlId = NULL; 
                                    /* FG control circular buffer context     */
                                    /* structure                              */

   char         defFileName [ STRING_SIZE ] ;
                                    /* Default file name according to the site*/
   char         aoInitFileName [ STRING_SIZE ] ;
                                    /* Name of the ao control structure init  */
                                    /* file                                   */

   /* Variables used to define the buffer to be used for storing data.   */

   int          maxFrames;          /* Maximum number of frames in data buffer*/

   /* Timer variables. */

   timer_t      timeId;             /* Alarm timer ID.                        */

   /* Zero point model temporary variables */

   AST_ZP_MODEL_ID_STRUCT   tempAstModel;
   TREF_ZP_MODEL_ID_STRUCT  tempTrefModel;
   COMA_ZP_MODEL_ID_STRUCT  tempComaModel;
   FOCUS_ZP_MODEL_ID_STRUCT tempFocModel;
   char         modInitFileName [ STRING_SIZE ] ;
                                    /* Name of the model init file            */

   /* Other general variables. */

   char         pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Status string.                         */

   long         nExp;               /* Number of exposure                     */
   long         outOption;          /* Output option                          */
   double       expTime;            /* Exposure time                          */
   double       cutoffFreq;         /* Cutoff frequency                       */
   double       rateSampFreq;       /* Cutoff frequency                       */


   /* Create and init an error context structure for this task */

   if (errorInit () == ERROR)
   {
      printErr ("detControl: Failed to init error context structure.\n");
      return (ERROR);
   }

   /* Check the task executes with floating point co-processor support. */

   if (taskOptionsGet (taskIdSelf (), & taskOptions) == ERROR)
   {
      ERROR_SET (0, "Could not get VxWorks task options", ERROR_LOG_NOW);
      return (ERROR);
   }

   if ((taskOptions & VX_FP_TASK) == 0)
   {
      ERROR_SET (0, "Task must be run with VX_FP_TASK option", ERROR_LOG_NOW);
      return (ERROR);
   }

   pipeCreate = pipeDevCreate; 

   /*
    * Use the wavefront sensor name provided as a function argument to
    * obtain the VME address of the corresponding SDSU controller, reporting
    * an error if the wavefront sensor name is not "p2".
    *
    * Also init the default data frame size for the appropriate wavefront
    * sensor.
    */

   if (strcmp (pWfsName, "p2") == 0)
   {
      vmeAddress = DET_CONTROL_PWFS2_SDSU_ADRS_VME;
      detControlStopMask = DET_CONTROL_PWFS2_MASK;
      maxFrames = DET_CONTROL_PWFS2_MAX_FRAMES;
   }
   else
   {
      ERROR_SET1 (S_detControl_BAD_WFS_NAME, "Unrecognised WFS name, %s, given",
                  ERROR_LOG_NOW, pWfsName);
      return (ERROR);
   }

   /*
    * Initialise the alarm timer.
    */

   if ( timeoutAlarmInit( &timeId ) == ERROR )
   {
      ERROR_SET (0, "Failed to init alarm timer", ERROR_LOG_NOW);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("detControl:%s: Alarm timer initialised. Timer ID = %d\n", 
           pWfsName, (int) timeId);
#endif

   /*
    * Get the CAD command context structure (using the appropriate pipe driver)
    * which is used subsequently as a handle for the CAD/CAR command-input and
    * response-generation routines.
    */

   if ((cadCmdContext = epToVxCmdInit (NULL, pipeCreate)) == NULL)
   {
      ERROR_LOG ("Error getting CAD command context");
      return (ERROR);
   }

   /* The number of bits that need to be monitored in the "select" function
    * (used later) needs to be set to the maximum file descriptor value in
    * use. Set the initial value to the value of the file descriptor used to
    * communicate CAD commands plus 1.
    */

   widthSelect = cadCmdContext->cadPipeFd + 1;

   /*
    * Get the genSub data update context structure (using the appropriate pipe 
    * driver), which is used subsequently as a handle for the genSub data 
    * update routines. Each time a new largest file descriptor is found, 
    * update the "widthSelect" variable to be used by "select()" later.
    */

   if ((dataUpdateContext = epToVxUpdateInit (pWfsName, NULL, pipeCreate)) == 
        NULL)
   {
      ERROR_LOG ("Error getting genSub data update context");
      return (ERROR);
   }

   if ((dataUpdateContext->gensubPipeFd + 1) > widthSelect)
   {
      widthSelect = dataUpdateContext->gensubPipeFd + 1;
   }

   /*
    * Create an observation context structure.
    */

   obsId = detObsContextCreate();
   if ( obsId == NULL )
   {
      ERROR_LOG ("Failed to init observation context on startup");
      return (ERROR);
   }

   /* Initialise the type and SN of the CCD */

   strcpy ( obsId->detType , DET_TYPE ) ;
   strcpy ( obsId->detId , DET_CCD_SN ) ;

   /* Initialise the "observing" flag and number of frames. */

   obsId->observing = FALSE;
   obsId->totalFrames = 1;
   obsId->saveCbIm = FALSE;
   obsId->saveCbAoCtrl = FALSE;
   obsId->saveCbFgCtrl = FALSE;
   obsId->sigMode = AO_MODE_NONE;
   obsId->dhsQlRate = 100;
   obsId->writeToRm = 1;

   /*
    * Get the context structures for the SIR records. 
    */

   if ( detGetSirContext (pRecordPrefix , obsId) == ERROR )
   {
      ERROR_LOG ("Error getting sir records context structures");
      return (ERROR);
   }

   /* 
    * As soon as we have the SIR record context, set the "initialising" flag. 
    */

   initState = menuCarstatesBUSY;
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to BUSY");
   }

   if (epToVxPipeWrite (NULL, "INITIALIZING", obsId->pStateContext) == ERROR)
   {
      ERROR_LOG ("Failed to set state to INITIALIZING");
   }

   /*
    * Init the SIR records with default values. 
    */

   if ( detWriteDefSirContext (obsId) == ERROR )
   {
      ERROR_LOG ("Error initializing the sir records");
   }

   /*
    * Create the AO CCD context structure geometry
    */

   aoCcdId = aoCcdContextCreate();
   if ( aoCcdId == NULL )
   {
      ERROR_LOG ("Failed to create AO CCD geometry structure on startup");
      return (ERROR);
   }

   obsId->aoCcdId = aoCcdId;
   aoCcdIdP2 = aoCcdId;

   /*
    * Create the AO control context structure and the circular buffer
    */

   aoCtrlId = aoCtrlContextCreate();
  
   if ( aoCtrlId == NULL )
   {
      ERROR_LOG ("Failed to create AO control context structure on startup");
      return (ERROR);
   };

   obsId->aoCtrlId = aoCtrlId;
   aoCtrlIdP2 = aoCtrlId;

   aoCbImId = aoCbImContextCreate();

   if ( aoCbImId == NULL )
   {
      ERROR_LOG ("Failed to create AO image circular buffer on startup");
      return (ERROR);
   };

   obsId->aoCbImId = aoCbImId;
   aoCbImIdP2 = aoCbImId;

   aoCbAoCtrlId = aoCbAoCtrlContextCreate();

   if ( aoCbAoCtrlId == NULL )
   {
      ERROR_LOG ("Failed to create AO control circular buffer on startup");
      return (ERROR);
   };

   obsId->aoCbAoCtrlId = aoCbAoCtrlId;
   aoCbAoCtrlIdP2 = aoCbAoCtrlId;

   aoCbFgCtrlId = aoCbFgCtrlContextCreate();

   if ( aoCbFgCtrlId == NULL )
   {
      ERROR_LOG ("Failed to create FG control circular buffer on startup");
      return (ERROR);
   };

   obsId->aoCbFgCtrlId = aoCbFgCtrlId;
   aoCbFgCtrlIdP2 = aoCbFgCtrlId;

   /*
    * If the WFS has control over the SDSU hardware, attempt to init 
    * sdsuLib using the VME address obtained above (which involves establishing 
    * communications with the SDSU hardware), remembering to call sdsuReset() 
    * immediately after sdsuContextCreate() to ensure a "Set Reply Address" 
    * command is issued.
    *
    * If the VME address is zero this indicates the SDSU controller for this 
    * WFS is not installed and should be simulated.
    *
    * A message describing the status of this initialisation is written to the
    * DET_CONTROL_INIT_STATUS_SIR_NAME record.
    *
    * Note: The task does not abort if the SDSU context structure could not be 
    * created because another attempt can be made by issuing the detInit 
    * command.
    */

   if (vmeAddress == 0)
   {
      simulate = TRUE;
   }
   else
   {
      simulate = FALSE;
   }

   sdsuId = sdsuContextCreate (vmeAddress, simulate);
   if ( (sdsuId == NULL) ||
        (sdsuReset (sdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) == ERROR)
      )
   {
      /*
       * The SDSU controller could not be initialised. Issue an error message
       * and also write a message to the DET_CONTROL_INIT_STATUS_SIR_NAME 
       * record.
       */

      ERROR_LOG ("Failed to init SDSU controller");
      initFailed = TRUE;

      /*
       * Note. When epToVxPipeWrite has a NULL record name argument, as it does 
       * below, the record name is extracted from the "pDetInitStatusContext" 
       * structure.
       */

      if (epToVxPipeWrite (NULL, "WARNING: SDSU Not Initialised", 
          obsId->pDetInitStatusContext) == ERROR)
      {
         ERROR_LOG (
         "Also failed to write warning message to SDSU status pipe.");
      }
   }
   else
   {
      if ( simulate )
      {
         sprintf (pStatusString, "SDSU SIMULATED: ID = 0x%-8x", (int) sdsuId);
      }
      else
      {
         sprintf (pStatusString, "SDSU Initialised OK: ID = 0x%-8x", 
                  (int) sdsuId);
      }

      MESSAGE_LOG2 (MSG_LOG, "%s: %s", pWfsName, pStatusString);

      if (epToVxPipeWrite (NULL, pStatusString, obsId->pDetInitStatusContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to write initialisation message to SDSU status pipe.");
      }

      /*
       * Update the global variables used to remember the SDSU and observing 
       * contexts, as an aid to engineering.
       */

      sdsuId->fastCamera = TRUE;
      detSdsuIdP2 = sdsuId;
   }

   detObsIdP2 = obsId;
   obsId->sdsuId = sdsuId;

   /*
    * Download the default OMF code to the SDSU controller automatically on 
    * startup. The health is set to WARNING if this fails
    */

#ifdef DEBUG_DOWNLOAD
   sdsuPrintCmdBuf (sdsuId, TRUE) ;
   sdsuPrintRepBuf (sdsuId) ;
#endif

   tryDownload = 0 ;
   while ( (detDownloadDefault (pWfsName, pRecordPrefix, sdsuId) == ERROR)
           &&(tryDownload < 10) )
   {
         /* RESET REP BUFFER, VME and CONTROLLER */
#ifdef DEBUG_DOWNLOAD
         sdsuPrintCmdBuf (sdsuId, TRUE) ;
         sdsuPrintRepBuf (sdsuId) ;
#endif
         if ( sdsu_initRepBuf (sdsuId) == ERROR )
            ERROR_LOG ("Failed to reset to zero the reply buffer ");
#ifdef DEBUG_DOWNLOAD
         sdsuPrintCmdBuf (sdsuId, TRUE) ;
         sdsuPrintRepBuf (sdsuId) ;
#endif
         if ( sdsuReset (sdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) 
              == ERROR )
            ERROR_LOG ("Failed to reset SDSU interface and controller");
#ifdef DEBUG_DOWNLOAD
         sdsuPrintCmdBuf (sdsuId, TRUE) ;
         sdsuPrintRepBuf (sdsuId) ;
#endif

         tryDownload ++ ;
   }

   if ( tryDownload == 10 )
   {
      ERROR_LOG ("Failed to download default DSP code on startup");
      initFailed = TRUE;
   }

   /*
    * Initialize aoCcdId with the default detector geometry.
    */

   if (detReadDefaultDspCcdGeometry (sdsuId, aoCcdId) == ERROR)
   {
      ERROR_LOG ( "Error while init default detector geometry on startup");
      initFailed = TRUE;
   }

   /* 
    * Set the new default CCD geometry 40,40
    */

   if (detSetDefaultDspCcdGeometry (sdsuId, aoCcdId) == ERROR)
   {
      ERROR_LOG ( "Error while setting default detector geometry on startup");
      initFailed = TRUE;
   }

   /*
    * Create data buffer to frames of data, using the aoCcdId->xMax and 
    * aoCcdId->yMax determined above.
    */

   if (sdsuBufferCreate (sdsuId, (aoCcdId->xMax * aoCcdId->yMax), maxFrames) 
       == ERROR)
   {
      ERROR_LOG ("Failed to create data buffer on startup");
      initFailed = TRUE;
   }

   /*
    * Initialise the readout process with our frame callback.
    * There is no packet callback in this version of the code.
    */
   
   if (sdsuSimpleReadoutOpen (sdsuId, NULL, detObserveEnd, 0, TRUE) == ERROR)
   {
      ERROR_LOG ("Failed to start readout task on startup");
      initFailed = TRUE;
   }

   strcpy (obsId->pWfsName, "PWFS2");

   /* 
    * Read the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->aoCcdId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /*
    * Set the temperature to the default
    */

   if ( sdsuId == NULL )
   {
      ERROR_LOG ("Failed to set CCD default temperature");
      initFailed = TRUE;
   }
   else
   {
      MESSAGE_LOG2 (MSG_LOG,
                    "Defining temperature control parameters: %#lx %#lx",
                    tempCode, tempCoeff);

      if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
            == ERROR) ||
           (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
            == ERROR) )
      {
         ERROR_LOG ("Error setting temperasture control parameters");
         initFailed = TRUE;
      }
      readTempReadyFlag = TRUE ;
   }

   /*
    * Set the default offsets for the PWFS2 CCD sectors
    */

   if ( sdsuId == NULL )
   {
      ERROR_LOG ("Failed to set CCD default offset");
      initFailed = TRUE;
   }
   else
   {
      MESSAGE_LOG4 (MSG_LOG, 
              "Defining new ADC offset levels: %#lx %#lx %#lx %#lx",
              offsetVect[0], offsetVect[1], offsetVect[2], offsetVect[3]);

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                         (uint32) offsetVect[0] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         initFailed = TRUE;
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                         (uint32) offsetVect[1] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         initFailed = TRUE;
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2",
                         (uint32) offsetVect[2] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 2 parameter");
         initFailed = TRUE;
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3",
                         (uint32) offsetVect[3] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 3 parameter");
         initFailed = TRUE;
      }

      if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
      {
         ERROR_LOG (
         "Failed to activate TIMING DSP parameters with LDP command");
         initFailed = TRUE;
      }
   }

   /*
    * Update the adc sir records 
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[2]) ,
                        obsId->pAdc2Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc2 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[3]) ,
                        obsId->pAdc3Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc3 sad record");
      return (ERROR);
   }

   /*
    * Now init all the geometry SIR records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->outputsNb) ,
                        obsId->pOutputsContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init outputs sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xSize) ,
                        obsId->pDetXsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init x size sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->ySize) ,
                        obsId->pDetYsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ysize sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xStart) ,
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->yStart) ,
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xSubapNb) ,
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->ySubapNb) ,
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xRaster) ,
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->yRaster) ,
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xSpace) ,
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->ySpace) ,
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->xBin) ,
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (aoCcdId->yBin) ,
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   /*
    * Init the AO control context structure
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_CP_INIT_FILE);
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( aoInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( aoInitFileName , "/" ) ;
      strcat ( aoInitFileName , defFileName ) ;

      if ( aoCtrlContextInit ( aoInitFileName, aoCcdId, aoCtrlId ) == ERROR )
      {
         ERROR_LOG ("Failed to init ao control context structure on startup");
         initFailed = TRUE;
      }

      if ( aoCtrlId->initFlag == TRUE )
      {
         angleWithM1 = aoCtrlId->angleWithM1;
         angleWithM2 = aoCtrlId->angleWithM2;

         if (epToVxPipeWrite (NULL, "Initialized", obsId->pAoCtrlInitContext) 
             == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
         }
      }

      if ( aoCtrlId->darkInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, aoCtrlId->darkFileName, 
                              obsId->pAoDarkInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
         }
      }

      if ( aoCtrlId->flatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, aoCtrlId->flatFileName, 
                              obsId->pAoFlatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
         }
      }

      if ( aoCtrlId->aoIntMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, aoCtrlId->aoIntMatFileName, 
                              obsId->pAoIntMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
         }
      }

      if ( aoCtrlId->aoContMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, aoCtrlId->aoContMatFileName, 
                              obsId->pAoContMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
         }
      }

      if ( aoCtrlId->fgContMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, aoCtrlId->fgContMatFileName, 
                              obsId->pFgContMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
         }
      }

      if (epToVxPipeWrite (NULL, (char *)(int)& (aoCtrlId->rms),
                           obsId->pAoRmsContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_RMS_SIR_NAME record");
      }

      if (epToVxPipeWrite (NULL, (char *)(int)& (aoCtrlId->threshold),
                           obsId->pAoThreshContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
      }

      if (epToVxPipeWrite (NULL, (char *)(int)& (aoCtrlId->totalThreshold),
                           obsId->pAoTotalContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
      }

#ifdef DEBUG
      aoCtrlContextShow (aoCcdId, aoCtrlId, FALSE);
#endif
   }
   else
   {
      MESSAGE_LOG (MSG_LOG, "PWFS2 - AO control context not initialised");
   }

   /*
    * Mode is no processing, init the fields of the observe CAD record
    */

   nExp = -1 ;          /* mode continuous */
   if (detDhsInitialised)
      outOption = 1 ;      /* DHS */
   else
      outOption = 0 ;      /* NO DHS */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   obsId->exposureTime = expTime;
   rateSampFreq = 6.0 / 100.0 ;                  /* 6% of sampling frequency */
   obsId->rateSamplingFrequency = rateSampFreq;
   cutoffFreq = rateSampFreq / expTime ;
   obsId->cutoffFrequency = cutoffFreq;

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /*
    * Init the ao zero point models 
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_CP_INIT_FILE);
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( modInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( modInitFileName , "/" ) ;
      strcat ( modInitFileName , defFileName ) ;

      if ( aoModInit ( modInitFileName, &tempAstModel, &tempTrefModel,
                       &tempComaModel, &tempFocModel ) == ERROR )
      {
         ERROR_LOG ( "Failed to init zero point models on startup" );
      }

      /* Now init the real models */

      if (semTake (accessAstigModel, ZP_MODEL_SEM_TIMEOUT) != OK)
      {
         ERROR_LOG ( "Timeout on mutex acess to astigModel" );
      }
      else
      {
         astigModel.a1 = tempAstModel.a1;
         astigModel.a2 = tempAstModel.a2;
         astigModel.a3 = tempAstModel.a3;
         astigModel.p1 = tempAstModel.p1;
         astigModel.p2 = tempAstModel.p2;
         astigModel.p3 = tempAstModel.p3;
         astigModel.c = tempAstModel.c;
         astigModel.b1 = tempAstModel.b1;
         astigModel.b2 = tempAstModel.b2;
         astigModel.b3 = tempAstModel.b3;
         astigModel.pp1 = tempAstModel.pp1;
         astigModel.pp2 = tempAstModel.pp2;
         astigModel.pp3 = tempAstModel.pp3;
         astigModel.d = tempAstModel.d;
         astigModel.applyModel = tempAstModel.applyModel;
         astigModel.gain0 = tempAstModel.gain0;
         astigModel.gain45 = tempAstModel.gain45;
         astigModel.offsetAstig0 = tempAstModel.offsetAstig0;
         astigModel.offsetAstig45 = tempAstModel.offsetAstig45;

         semGive (accessAstigModel); 
      }

      if (semTake (accessTrefoilModel, ZP_MODEL_SEM_TIMEOUT) != OK)
      {
         ERROR_LOG ( "Timeout on mutex acess to trefoilModel" );
      }
      else
      {
         trefoilModel.a = tempTrefModel.a;
         trefoilModel.p = tempTrefModel.p;
         trefoilModel.c = tempTrefModel.c;
         trefoilModel.b = tempTrefModel.b;
         trefoilModel.pp = tempTrefModel.pp;
         trefoilModel.d = tempTrefModel.d;
         trefoilModel.applyModel = tempTrefModel.applyModel;

         semGive (accessTrefoilModel);
      }

      if (semTake (accessComaModel, ZP_MODEL_SEM_TIMEOUT) != OK)
      {
         ERROR_LOG ( "Timeout on mutex acess to comaModel" );
      }
      else
      {
         comaModel.a = tempComaModel.a;
         comaModel.p = tempComaModel.p;
         comaModel.c = tempComaModel.c;
         comaModel.b = tempComaModel.b;
         comaModel.pp = tempComaModel.pp;
         comaModel.d = tempComaModel.d;
         comaModel.applyModel = tempComaModel.applyModel;

         semGive (accessComaModel);
      }

      if (semTake (accessFocusModel, ZP_MODEL_SEM_TIMEOUT) != OK)
      {
         ERROR_LOG ( "Timeout on mutex acess to focusModel" );
      }
      else
      {
         focusModel.a1 = tempFocModel.a1;
         focusModel.a2 = tempFocModel.a2;
         focusModel.p1 = tempFocModel.p1;
         focusModel.p2 = tempFocModel.p2;
         focusModel.c = tempFocModel.c;
         focusModel.applyModel = tempFocModel.applyModel;

         semGive (accessFocusModel);
      }
   }
   else
   {
      MESSAGE_LOG ( MSG_LOG, "aO zero point model not initialized");
   }

   /*
    * If the DHS parameters have been initialised successfully, attempt to
    * init the DHS.
    */

   if ( (strcmp (pDetDhsClientName,"NONE") != 0) &&
        (strcmp (pDetDhsHostName,"NONE") != 0) &&
        (strcmp (pDetDhsServerName,"NONE") != 0) )
   {
      if (detDhsInit() == ERROR)
      {
         ERROR_LOG ("Failed to init to DHS");
         initWarning = TRUE;
      }
   }

   /*
    * If the DHS has initialised successfully, attempt to connect to it.
    */

   if (detDhsInitialised)
   {
      if ( detDhsConnect () == ERROR )
      {
         ERROR_LOG ("Failed to connect to DHS");
         initWarning = TRUE;
      }      

      if ( detDhsConnected == CONNECTED )
      {
         if (epToVxPipeWrite (NULL, "CONNECTED", obsId->pDhsConContext)
             == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_DHS_CON_SIR_NAME record");
            errorNumber = ERROR;
         }
      };

      if ( detDhsConnected == NOT_CONNECTED )
      {
         if (epToVxPipeWrite (NULL, " NOT CONNECTED", obsId->pDhsConContext)
             == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_DHS_CON_SIR_NAME record");
            errorNumber = ERROR;
         }
      };
   }
   else
   {
      MESSAGE_LOG (MSG_LOG, "WARNING: DHS not initialised");
      detDhsConnected = NOT_INIT;
      if (epToVxPipeWrite (NULL, "NOT INIT", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_DHS_CON_SIR_NAME record");
         errorNumber = ERROR;
      }
   }

   /*
    * Initialise the health of this detector control task to "GOOD" if 
    * successful or "BAD" if a significant problem occurred during the 
    * initialisation.
    */

   if ( initFailed )
   {
      if ( epToVxSetHealth (pRecordPrefix, "BAD") == ERROR )
      {
         ERROR_LOG ("Failed to init detector controller health to BAD");
      }
   }
   else if ( initWarning )
   {
      if ( epToVxSetHealth (pRecordPrefix, "WARNING") == ERROR )
      {
         ERROR_LOG (
         "Failed to init detector controller health to WARNING");
      }
   }
   else
   {
      if ( epToVxSetHealth (pRecordPrefix, "GOOD") == ERROR )
      {
         ERROR_LOG ("Failed to init detector controller health to GOOD");
      }
   }

   /* Finally, reset the "initialising" flag and set the system state to
    * RUNNING
    */

   initState = menuCarstatesIDLE;
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to IDLE");
   }

   if (epToVxPipeWrite (NULL, "RUNNING", obsId->pStateContext) == ERROR)
   {
      ERROR_LOG ("Failed to set state to RUNNING");
   }

   /*
    * The task has been successfully initialised, so it can now go into a loop 
    * waiting for commands. The detector controller task can be terminated by 
    * setting the "detControlStop" variable from the console. The task will 
    * stop when it discovers its bit set. 
    */

   MESSAGE_LOG3 (MSG_MINDEBUG,
   "Entering loop waiting for commands... pWfsName=%s, pRecordPrefix=%s pCmdPacket=%#x",
   pWfsName, pRecordPrefix, (int) cadCmdContext->pCmdPacket);

   while ( (detControlStop & detControlStopMask) == 0 )
   {
      /*
       * Zero all the bits in the file descriptor read structure and then
       * set each bit corresponding to the file descriptors of the data
       * update pipes of all the wavefront sensors being monitored by this
       * task. Also set the bit corresponding to the pipe used to receive CAD 
       * commands.
       */

      FD_ZERO (& updateFds);

      if ( dataUpdateContext != NULL )
         FD_SET (dataUpdateContext->gensubPipeFd, & updateFds);

      FD_SET (cadCmdContext->cadPipeFd, & updateFds);

      /*
       * Wait for an input from any of the file descriptors set above.
       * There is no timeout.
       */

      if (select (widthSelect, & updateFds, NULL, NULL, NULL) == ERROR)
      {
         ERROR_SET (0, "File descriptor selection function, select(), failed", 
                    ERROR_LOG_NOW );
         return (ERROR);
      }

      /* Check whether an input has come from the pipe communicating CAD
       * commands.
       */

      if (FD_ISSET (cadCmdContext->cadPipeFd, & updateFds))
      {

         /*
          * Initialise the error number and then read the command number from 
          * the pipe communicating CAD commands. The epToVxCmdRead() call will 
          * block until a command becomes available. The detControl task is 
          * aborted if it fails to read a command.
          */

         errorNumber = 0;
         if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
         {
            ERROR_LOG ("Error reading CAD command - detControl task aborted");
            epToVxSetHealth( pWfsName, "BAD" );
            return (ERROR);
         }

         /* Log a message each time a command is received. */

         MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command %d received.", 
                       commandNumber);

         /* Process the command.
          * In simulation mode simply report the command,
          * otherwise switch according to the command number received.
          */

         if (EPTOVX_IS_SIMULATION (cadCmdContext, EPTOVX_SIM_MODE_FULL) ||
             EPTOVX_IS_SIMULATION (cadCmdContext, EPTOVX_SIM_MODE_FAST) )
         {
            /*
             * In simulation mode nothing needs to be done except to log
             * a message. The function epToVxCmdFinish() will simulate the
             * response from the command.
             */

            MESSAGE_LOG1 (MSG_LOG, 
                  "Command %d received in simulation mode... no action taken",
                  commandNumber);
         }

         else if (commandNumber == DET_CONTROL_CMD_CHOP)
         {

            /* Specify chop states. */

            errorNumber = 
            detChop (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_FRAME_SIZE)
         {
            /* Specify frame size. */

            errorNumber =
            detFrameSize (cadCmdContext, commandNumber, sdsuId, obsId, 
                          offsetFullVect, offsetBinVect);
         }

         else if (commandNumber == DET_CONTROL_CMD_DHS_RECONNECT)
         {
            /* Set dhs connection. */

            errorNumber = detDhsReconnect (cadCmdContext, commandNumber,
                                           sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_DHS_DISPLAY)
         {
            /* Set dhs display parameters. */

            errorNumber = detDhsDisplay (cadCmdContext, commandNumber,
                                         sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_EXPOSURE)
         {

            /* Define exposure parameters. */

            errorNumber = 
            detExposure (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OBS_TYPE)
         {

            /* Define observation type. */

            errorNumber = 
            detObstype(cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SET_WCS)
         {

            /* Define World Coordinate System parameters. */

            errorNumber = 
            detSetWcs(cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OBSERVE)
         {

            /*
             * Make observation.
             *
             * Before starting the observation, load up the observation ID 
             * structure.
             */

            obsId->sdsuId = sdsuId;
            obsId->timeId = timeId;

            errorNumber = 
            detObserveStart (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_PAUSE)
         {

             /* Pause observation (not supported by SDSU controller). */

            ERROR_SET (S_detControl_BAD_COMMAND, 
                       "Pause observation command not supported",
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_COMMAND;
         }

         else if (commandNumber == DET_CONTROL_CMD_CONTINUE)
         {

            /* Continue observation (not supported by SDSU controller). */

            ERROR_SET (S_detControl_BAD_COMMAND, 
                       "Continue observation command not supported",
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_COMMAND;
         }

         else if (commandNumber == DET_CONTROL_CMD_STOP)
         {

            /* Stop observation and keep the data. */

            errorNumber = 
            detStop (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_ABORT)
         {

            /* Abort observation and throw away the data. */

            errorNumber = 
            detAbort(cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_INITIALISE)
         {

            /* Initialise SDSU context and redownload DSP code. */

            errorNumber = 
            detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     &sdsuId, obsId, &vmeAddress, &maxFrames);
         }

         else if (commandNumber == DET_CONTROL_CMD_RESET)
         {

            /* Reset SDSU controller and redownload DSP code. */

            errorNumber = 
            detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                      sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_TEST)
         {

            /* Test SDSU controller. */

            errorNumber = 
            detTest (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SAVE)
         {

            /* Save SDSU control parameters. */

            errorNumber = 
            detSave (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_GEOMETRY)
         {

            /* Set readout geometry. */

            errorNumber = 
            detGeometry (cadCmdContext, commandNumber, sdsuId, obsId, 
                         offsetFullVect, offsetBinVect);
         }

         else if (commandNumber == DET_CONTROL_CMD_PRIMITIVE)
         {

            /* Execute SDSU primitive command. */

            errorNumber = 
            detPrimitive (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_POWER_ON)
         {

            /* Execute SDSU POWER ON primitive command. */

            errorNumber =
            detPowerOn (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_MODE)
         {

            /* Set SDSU readout mode. */

            errorNumber = 
            detMode (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OFFSET)
         {

            /* Set SDSU ADC offsets. */

            errorNumber = 
            detOffset (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_TEMP)
         {

            /* Define SDSU temperature control parameters. */

            errorNumber = 
            detTemp (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT)
         {

            /* Initialise AO control structure. */

            errorNumber =
            detSigInit (cadCmdContext, commandNumber, sdsuId, obsId);
         }
         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_AO_GAIN)
         {

            /* Update open/closed loop aO scale factors */

            errorNumber =
            detSigInitAoGain (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_FG_GAIN)
         {

            /* Update open/closed loop FG scale factors */

            errorNumber =
            detSigInitFgGain (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_BW)
         {

            /* Update butterworth filter coefficients */

            errorNumber =
            detSigInitBw (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_RESET)
         {

            /* Reset the signal processing */

            errorNumber =
            detSigReset (pRecordPrefix,
                         cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_NONE)
         {

            /* Set to no processing the processig mode */

            errorNumber =
            detSigModeNone (pRecordPrefix,
                            cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_DARK)
         {

            /* Set to dark subtraction the signal processing */

            errorNumber =
            detSigModeDark (pRecordPrefix, cadCmdContext, commandNumber, 
                            sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_GG)
         {

            /* Set to global guide the signal processing */

            errorNumber =
            detSigModeGg (pRecordPrefix, cadCmdContext, commandNumber, 
                          sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_GG_AO)
         {

            /* Set to global guide and aO correction the signal processing */

            errorNumber =
            detSigModeGgAo (pRecordPrefix, cadCmdContext, commandNumber, 
                            sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_AO)
         {

            /* Set to aO correction the signal processing */

            errorNumber =
            detSigModeAo (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
                          obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_COADD)
         {

            /* Set to coadd the signal processing */

            errorNumber =
            detSigModeCoadd (pRecordPrefix, cadCmdContext, commandNumber, 
                             sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_THRESH)
         {

            /* Set to threshold computation the signal processing */

            errorNumber =
            detSigModeThresh (pRecordPrefix, cadCmdContext, commandNumber, 
                              sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_GG_COADD)
         {

            /* Set to global guide and coadd the signal processing */

            errorNumber =
            detSigModeGgCoadd (pRecordPrefix, cadCmdContext, commandNumber, 
                               sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_SEQ)
         {

            /* Set to sequence closed loop the signal processing */

            errorNumber =
            detSigModeSeq (pRecordPrefix,
                           cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_TOTAL)
         {

            /* Set to average flux computation the signal processing */

            errorNumber =
            detSigModeTotal (pRecordPrefix, cadCmdContext, commandNumber, 
                             sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_FG_FOCUS)
         {

            /* Set to FG and focus the signal processing */

            errorNumber =
            detSigModeFgFocus (pRecordPrefix, cadCmdContext, commandNumber, 
                               sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_FG_FOCUS_AO)
         {

            /* Set to FG and focus and aO the signal processing */

            errorNumber =
            detSigModeFgFocusAo (pRecordPrefix,
                                 cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_FG_FOCUS_COADD)
         {

            /* Set to FG and focus and coadd the signal processing */

            errorNumber =
            detSigModeFgCoadd (pRecordPrefix,
                               cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_SAVE_CB)
         {

            /* Init parameters for saving circular buffers */

            errorNumber =
            detSigSaveCb (cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MEAS_AO_IM)
         {

            /* Set parameters to measure a column of the interaction matrix */

            errorNumber = 
            detSigMeasAoIm (pRecordPrefix,
                            cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_COMP_AO_MAT)
         {

            /* Compute and save the interaction and control matrixes */
            errorNumber =
            detSigCompAoMat (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_MODE_SEQ_DARK)
         {

            /* Sequence dark mode */
            errorNumber =
            detSigModeSeqDark (pRecordPrefix,
                               cadCmdContext, commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_AST_MODEL)
         {

            /* Init zero point model for astig off axis */
            errorNumber =
            detSigInitModAst (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_TREF_MODEL)
         {

            /* Init zero point model for trefoil off axis */
            errorNumber =
            detSigInitModTref (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_COMA_MODEL)
         {

            /* Init zero point model for coma off axis */
            errorNumber =
            detSigInitModComa (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else if (commandNumber == DET_CONTROL_CMD_SIG_INIT_FOCUS_MODEL)
         {

            /* Init zero point model for focus off axis */
            errorNumber =
            detSigInitModFoc (cadCmdContext, commandNumber, sdsuId, obsId); 
         }

         else
         {
            ERROR_SET1 (S_detControl_BAD_COMMAND, 
                        "Command %d not currently implemented",
                        ERROR_LOG_NOW, commandNumber);
            errorNumber = S_detControl_BAD_COMMAND;
         }

         /* Finish the command after a 0.5 second delay. */

         taskDelay ( (int) (sysClkRateGet () * 0.5) );
         if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
         {
            ERROR_LOG ("Error finishing command");
         }
      }

      /*
       * Check whether an input has come from the pipe communicating
       * data updates from the genSub records.
       */

      if ( (dataUpdateContext != NULL) &&
           (FD_ISSET (dataUpdateContext->gensubPipeFd, & updateFds)) )
      {

         /*
          * A message has arrived on the genSub data update pipe.
          * Read the message from the pipe and check it has been read 
          * successfully.
          */

         if ((updateNumber = epToVxUpdateRead (dataUpdateContext)) < 0)
         {
            ERROR_LOG ("Error reading genSub update");
         }

         /* Log a message each time a data update is received. */

         MESSAGE_LOG3 (MSG_FULLDEBUG, 
            "%s - data update %d received. Packet=%#x",
            pWfsName, updateNumber, (int) dataUpdateContext->pUpdatePacket);
      }
   }

   /*
    * If the task is stopped, free the resources allocated to it.
    */

   if (sdsuId != NULL) sdsuContextDelete (sdsuId);

   MESSAGE_LOG1 (MSG_WARNING, "Detector Control task for WFS %s stopped.", 
                 pWfsName);
   epToVxSetHealth( pWfsName, "BAD" );

   epToVxCmdFree (cadCmdContext);
   errorFlush();
   errorFree();

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObsContextCreate
 *
 *   INVOCATION:
 *   detObsContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (OBS_ID)   Pointer to observation ID, or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create an observation ID structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises an observation ID structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

OBS_ID detObsContextCreate (void)
{
   OBS_ID   obsId;

   /* Allocate memory for the observation ID structure, initialising its
    * contents to zero.
    */

#ifdef DEBUG
  printf (
  "detObsContextCreate: Allocating %d bytes of memory for OBS_ID structure.\n",
  sizeof (OBS_ID_STRUCT));
#endif /* DEBUG */

   if ((obsId = (OBS_ID) calloc ((size_t) 1, sizeof (OBS_ID_STRUCT))) == NULL)
   {
      ERROR_SET (0,"Memory allocation for observation context failed",
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   /* Create a binary semaphore for synchronising observation threads. */

   obsId->syncSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
   if ( obsId->syncSem == NULL )
   {
      ERROR_SET (0, "Failed to create observation synchronisation semaphore",
                 ERROR_LOG_SAVE);
      cfree ((char *) obsId);
      return (NULL);
   }

   return (obsId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDownloadDefault
 *
 *   INVOCATION:
 *   detDownloadDefault (pWfsName, pRecordPrefix, sdsuId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *) Name of wavefront sensor p2
 *   (>) pRecordPrefix (const char *) Record Name prefix
 *   (>) sdsuId        (SDSU_ID)      Current SDSU context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Download default OMF files
 *
 *   DESCRIPTION:
 *   This function downloads DSP code from the default OMF files. Executed on 
 *   startup.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDownloadDefault
   (
   const char *   pWfsName,         /* Name of wavefront sensor.              */
   const char *   pRecordPrefix,    /* Record Name prefix                     */
   SDSU_ID        sdsuId            /* SDSU context structure.                */
   )
{
   char           pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                             /* Combined path name and file name.             */
   /* 
    * Variables associated with "Download OMF file" command.
    * (omfPath, vmeFile, timFile and utlFile use general filename parameters)
    */

   BOOL           limitAdrsRange;      
                             /* Flag for limiting address range in DSP memory */

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Download default OMF code to the VME DSP, unless the default file name is
    * "NONE" or blank. If the code could not be downloaded, the controller 
    * health is set "BAD", since it cannot do anything until this code is 
    * downloaded.
    */

   if ( (strcmp (DET_CONTROL_OMF_VME_FILE, "") != 0) &&
       (strcmp (DET_CONTROL_OMF_VME_FILE, "NONE") != 0)
      )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", 
               DET_CONTROL_OMF_FILE_PATH, DET_CONTROL_OMF_VME_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download default OMF file to VME DSP");
         epToVxSetHealth( pRecordPrefix, "BAD" );
         return (ERROR);
      }
   }

   /*
    * Download default OMF code to the TIMING DSP, unless the default file 
    * name is "NONE" or blank. If the code could not be downloaded, the 
    * controller health is set "BAD", since it cannot do anything until this 
    * code is downloaded. 
    */


   if ( (strcmp (DET_CONTROL_GBD_OMF_TIM_FILE, "") != 0) &&
        (strcmp (DET_CONTROL_GBD_OMF_TIM_FILE, "NONE") != 0)
      )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH,
               DET_CONTROL_GBD_OMF_TIM_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download default OMF file to TIMING DSP");
         epToVxSetHealth( pRecordPrefix, "BAD" );
         return (ERROR);
      }
   }

   /*
    * Download default OMF code to the UTILITY DSP, unless the default file 
    * name is "NONE" or blank. If the code could not be downloaded, the 
    * controller health is set "BAD", since it cannot do anything until this 
    * code is downloaded.
    */

   if ( (strcmp (DET_CONTROL_OMF_UTL_FILE, "") != 0) &&
       (strcmp (DET_CONTROL_OMF_UTL_FILE, "NONE") != 0)
      )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH, 
               DET_CONTROL_OMF_UTL_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", 
               pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
               limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download default OMF file to UTILITY DSP");
         epToVxSetHealth( pRecordPrefix, "BAD" );
         return (ERROR);
      }
   }

   if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 160) == ERROR)
   {
      ERROR_LOG ("Failed to increase the PWFS packet size");
   }

   /*
    * After successfully downloading new OMF code, the controller must be 
    * reinitialised by sending an "INI" command to the utility DSP and a 
    * "LDP" command to the timing DSP.
    */

   if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to init UTILITY DSP with INI command");
      epToVxSetHealth( pRecordPrefix, "BAD" );
      return (ERROR);
   }
   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to init TIMING DSP with LDP command");
      epToVxSetHealth( pRecordPrefix, "BAD" );
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReadDefaultDspCcdGeometry
 *
 *   INVOCATION:
 *   detReadDefaultDspCcdGeometry (sdsuId, aoCcdId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId   (SDSU_ID)      Current SDSU context structure
 *   (<) aoCcdId  (AO_CCD_ID)    AO CCD geometry context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the default configuration for the AO CCD geometry from the DSP code.
 *
 *   DESCRIPTION:
 *   This function read all the CCD geometry parameters from the DSP code. 
 *   Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   aoP2Lib.h
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detReadDefaultDspCcdGeometry
   (
   SDSU_ID       sdsuId,          /* SDSU context structure.                  */
   AO_CCD_ID     aoCcdId          /* AO CCD geometry context structure.       */
   )
{
   uint32        xSdsuRas;        /* SDSU parameter (T_XRAS).                 */
   uint32        ySdsuRas;        /* SDSU parameter (T_YRAS).                 */
   uint32        xSdsuSubap;      /* SDSU parameter (T_XSUBAP).               */
   uint32        ySdsuSubap;      /* SDSU parameter (T_YSUBAP).               */
   uint32        sdsuOutputs;     /* SDSU parameter (T_OUTPUTS).              */
   uint32        xSdsuChip;       /* SDSU parameter (T_XSIZE).                */
   uint32        ySdsuChip;       /* SDSU parameter (T_YSIZE).                */
   uint32        pSizeSdsu;       /* SDSU parameter (V_PSIZE).                */
   uint32        uscanSdsu;       /* SDSU parameter (T_USCAN)                 */
   uint32        xSdsuStart;      /* SDSU parameter (T_XSTART).               */
   uint32        ySdsuStart;      /* SDSU parameter (T_YSTART).               */
   uint32        xSdsuBin;        /* SDSU parameter (T_XBIN).                 */
   uint32        ySdsuBin;        /* SDSU parameter (T_YBIN).                 */
   uint32        xSdsuSpace;      /* SDSU parameter (T_XSPACE).               */
   uint32        ySdsuSpace;      /* SDSU parameter (T_YSPACE).               */
   uint32        xSdsuTail;       /* SDSU parameter (T_XTAIL).                */

   int           xPixelsOutput;   /* Number of X super pixels per output.     */
   int           yPixelsOutput;   /* Number of Y super pixels per output.     */
   int           dspxPixels;      /* Number of X pixels expected by DSP code. */
   int           dspyPixels;      /* Number of Y pixels expected by DSP code. */
   int           dspxMax;         /* Maximum X pixels expected by DSP code.   */
   int           dspyMax;         /* Maximum Y pixels expected by DSP code.   */
   int           i;               /* index                                    */

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * In simulation mode no DSP code will have been downloaded, and nothing
    * will be downloaded
    */

   if ( !sdsuId->simulate )
   {
      /*
       * Initialise xMax and yMax of the aoCcdId structure 
       */

      aoCcdId->xMax = CCD_XSIZE;
      aoCcdId->yMax = CCD_YSIZE;

      /*
       * Initialise xPixels and yPixels of the aoCcdId structure 
       */

      aoCcdId->xPixels = aoCcdId->xMax;
      aoCcdId->yPixels = aoCcdId->yMax;

      /*
       * Obtain the xSdsuChip, ySdsuChip, xSdsuRas, ySdsuRas, xSdsuSubap, 
       * ySdsuSubap and number of outputs parameters from the SDSU controller 
       * and use these to calculate the default size expected by the DSP code.
       */

      if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSIZE", &xSdsuChip) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSIZE", &ySdsuChip) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XRAS", &xSdsuRas) == 
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YRAS", &ySdsuRas) == 
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", &xSdsuSubap) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", &ySdsuSubap) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", &sdsuOutputs) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_VME, "V_PSIZE", &pSizeSdsu) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_USCAN", &uscanSdsu) == 
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSTART", &xSdsuStart) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSTART", &ySdsuStart) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XBIN", &xSdsuBin) == 
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YBIN", &ySdsuBin) == 
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", &xSdsuSpace) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", &ySdsuSpace) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XTAIL", &xSdsuTail) ==
            ERROR)
         )
      {
         ERROR_SET (0, "Failed to read SDSU parameters", ERROR_LOG_SAVE);
         return (ERROR);
      }

      aoCcdId->xSize = xSdsuChip;
      aoCcdId->ySize = ySdsuChip;
      aoCcdId->outputsNb = sdsuOutputs;
      aoCcdId->packetSize = pSizeSdsu;
      aoCcdId->uscanNb = (int)uscanSdsu;
      aoCcdId->xSubapNb = xSdsuSubap;
      aoCcdId->ySubapNb = ySdsuSubap;
      aoCcdId->xRaster = xSdsuRas;
      aoCcdId->yRaster = ySdsuRas;
      aoCcdId->xStart = xSdsuStart;
      aoCcdId->yStart = ySdsuStart;
      aoCcdId->xBin = xSdsuBin;
      aoCcdId->yBin = ySdsuBin;
      aoCcdId->xSpace = xSdsuSpace;
      aoCcdId->ySpace = ySdsuSpace;
      aoCcdId->xTail = xSdsuTail;
      
      /*
       * xPixelsOutput=(xSdsuRas*xSdsuSubap) and 
       * yPixelsOutput=(ySdsuRas*ySdsuSubap) represent the number of pixels 
       * per output. The arrangement depends on the number of outputs.
       * In our case, there are 4 outputs the sectors generated from each 
       * output are arranged like this
       *
       *   0----->------+-----<------0
       *   |  sector 4  |  sector 3  |
       *   +------------+------------+
       *   |  sector 1  |  sector 2  |
       *   0----->------+-----<------0
       *
       * "0" shows the origin of each sector and ">" the direction of readout.
       */

      xPixelsOutput = (int) (xSdsuRas * xSdsuSubap);
      yPixelsOutput = (int) (ySdsuRas * ySdsuSubap);

      dspxPixels = xPixelsOutput * 2;
      dspyPixels = yPixelsOutput * 2;
      dspxMax = (int) xSdsuChip * 2;
      dspyMax = (int) ySdsuChip * 2;

      /*
       * Compare the default detector size downloaded in the DSP code with
       * xMax and yMax and increase if necessary. Replace the current xPixels
       * and yPixels with that found in the DSP code.
       */

      if ( dspxMax > aoCcdId->xMax )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Maximum number of X pixels increased from %d to %d\n",
            aoCcdId->xMax, dspxMax);
         aoCcdId->xMax = dspxMax;
      }

      if ( dspyMax > aoCcdId->yMax )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Maximum number of Y pixels increased from %d to %d\n",
            aoCcdId->yMax, dspyMax);
         aoCcdId->yMax = dspyMax;
      }

      if ( dspxPixels != aoCcdId->xPixels )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Default number of X pixels changed from %d to %d\n",
            aoCcdId->xPixels, dspxPixels);
         aoCcdId->xPixels = dspxPixels;
      }

      if ( dspyPixels != aoCcdId->yPixels )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Default number of Y pixels changed from %d to %d\n",
            aoCcdId->yPixels, dspyPixels);
         aoCcdId->yPixels = dspyPixels;
      }

      aoCcdId->pixelsNb = (aoCcdId->xPixels) * (aoCcdId->yPixels) ;
      
      /*
       * Update the expected number of packets per frame using the number of
       * pixels read from the controller.
       */

      if ( pSizeSdsu > 0 )
      {
         aoCcdId->packetNb = 
         (int)ceil ( (double) (sdsuOutputs * xPixelsOutput * yPixelsOutput) / 
         (double) pSizeSdsu );
      }
      else
      {
         aoCcdId->packetNb = 1;
      }

      sdsuId->packetsPerFrame = aoCcdId->packetNb;

      /*
       * Set the binningFlag. Only binning is implemented.
       */

      if ( ( aoCcdId->xBin != 1 ) || ( aoCcdId->yBin != 1) )
         aoCcdId->binningFlag = TRUE;
      else
         aoCcdId->binningFlag = FALSE;

      /*
       * By default all subapertures are used 
       */

      aoCcdId->subapNb = aoCcdId->xSubapNb * aoCcdId->ySubapNb * 
                         aoCcdId->outputsNb;
      aoCcdId->subapNotUsedNb = 0;
      aoCcdId->subapUsedNb = aoCcdId->subapNb - aoCcdId->subapNotUsedNb;
      aoCcdId->centroidsNb = 2 * aoCcdId->subapUsedNb;

      for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
          aoCcdId->subapUsedVect[i] = TRUE;
   }
   else
   {
      /* Simulation mode */
   
      aoCcdId->outputsNb = 4;
      aoCcdId->xSize = CCD_XSIZE / 2;
      aoCcdId->ySize = CCD_YSIZE / 2;
      aoCcdId->xMax = CCD_XSIZE;
      aoCcdId->yMax = CCD_YSIZE;
      aoCcdId->xStart = 0;
      aoCcdId->yStart = 0;
      aoCcdId->xBin = 1;
      aoCcdId->yBin = 1;
      aoCcdId->xRaster = CCD_XSIZE / 2;
      aoCcdId->yRaster = CCD_YSIZE / 2;
      aoCcdId->xSpace = 0;
      aoCcdId->ySpace = 0;
      aoCcdId->xSubapNb = 1;
      aoCcdId->ySubapNb = 1;
      aoCcdId->subapNb = aoCcdId->xSubapNb * aoCcdId->ySubapNb * 
                         aoCcdId->outputsNb;
      aoCcdId->subapNotUsedNb = 0;
      aoCcdId->subapUsedNb = aoCcdId->subapNb - aoCcdId->subapNotUsedNb;
      aoCcdId->centroidsNb = 2 * aoCcdId->subapUsedNb;
      for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
          aoCcdId->subapUsedVect[i] = TRUE;
      aoCcdId->xPixels = aoCcdId->xMax;
      aoCcdId->yPixels = aoCcdId->yMax;
      aoCcdId->pixelsNb = aoCcdId->xPixels * aoCcdId->yPixels;
      aoCcdId->uscanNb = 4;
      aoCcdId->xTail = 0;
      aoCcdId->packetSize = CCD_XSIZE * CCD_YSIZE ;
      aoCcdId->packetNb = 1;
      aoCcdId->binningFlag = FALSE;

      sdsuId->packetsPerFrame = 1;
   }

   aoCcdContextShow ( aoCcdId ) ;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSetDefaultDspCcdGeometry
 *
 *   INVOCATION:
 *   detSetDefaultDspCcdGeometry (sdsuId, aoCcdId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId   (SDSU_ID)      Current SDSU context structure
 *   (<) aoCcdId  (AO_CCD_ID)    AO CCD geometry context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set the default configuration for the AO CCD geometry from the DSP code.
 *   This default configuration is :
 *   T_XRAS = 40 , T_YRAS = 40
 *   T_XSUBAP = 1 , T_YSUBAP = 1
 *   T_XSTART = 0 , T_YSTART = 0
 *   T_XSPACE = 0 , T_YSPACE = 0
 *   T_XBIN = 1 , T_YBIN = 1
 *
 *   DESCRIPTION:
 *   This function sets all the CCD geometry parameters to the DSP code. 
 *   Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   aoP2Lib.h
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detSetDefaultDspCcdGeometry
   (
   SDSU_ID       sdsuId,          /* SDSU context structure.                  */
   AO_CCD_ID     aoCcdId          /* AO CCD geometry context structure.       */
   )
{
   uint32        errorNumber;     /* Error number reported by task.           */
   uint32        xRasterReq;      /* SDSU parameter (T_XRAS).                 */
   uint32        yRasterReq;      /* SDSU parameter (T_YRAS).                 */
   uint32        xSubapNbReq;     /* SDSU parameter (T_XSUBAP).               */
   uint32        ySubapNbReq;     /* SDSU parameter (T_YSUBAP).               */
   uint32        xStartReq;       /* SDSU parameter (T_XSTART).               */
   uint32        yStartReq;       /* SDSU parameter (T_YSTART).               */
   uint32        xTailReq;        /* SDSU parameter (T_XTAIL).                */
   int           xPixelsReq;      /* Number of X pixels                       */
   int           yPixelsReq;      /* Number of Y pixels                       */
   int           pixelsNbReq;     /* Total number of pixels.                  */
   int           nPackets;        /* Number of packets expected per frame.    */
   int           i;

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Initialise the new configuration
    */

   xStartReq = 0; 
   yStartReq = 0; 
   xSubapNbReq = 1;
   ySubapNbReq = 1;
   xRasterReq = 40; 
   yRasterReq = 40; 

   xPixelsReq = xSubapNbReq * xRasterReq * 2;
   yPixelsReq = ySubapNbReq * yRasterReq * 2;
   pixelsNbReq = xPixelsReq * yPixelsReq;

   /*
    * Calculate the number of trailing X pixels. This is required by the DSP
    * code as a check. If the value is negative then the subapertures span 
    * the boundary between outputs (not physically possible), and xTail 
    * should be set zero.
    *
    * xTail is the number of pixels that need to be discarded at the end of
    * each row, and is calculated by starting with the total number of pixels
    * to read (xSize) and subtracting off the pixels that are read out and/or
    * discarded during a readout.
    */

   xTailReq = aoCcdId->xSize - 
   (((xRasterReq * aoCcdId->xBin) + aoCcdId->xSpace) * xSubapNbReq) + 
   aoCcdId->xSpace - xStartReq - aoCcdId->uscanNb;

   if ( xTailReq < 0 )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
      "Xtail is %ld. Should not be less than zero", 
      ERROR_LOG_NOW, xTailReq);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   if ( (!sdsuId->simulate) && (aoCcdId->packetSize > 0) )
   {
      aoCcdId->packetSize = 
      xRasterReq * xSubapNbReq * (aoCcdId->outputsNb/2) * 2;

      nPackets = (int) ceil ( (double) (pixelsNbReq) /
                 (double) aoCcdId->packetSize );
   }
   else
   {
      nPackets = 1;
   }

   /*
    * Everything is ok, init aoCcdId.
    * Determine whether the given parameters will put the detector controller
    * into full frame mode. This happens when the there is one subaperture per
    * output and the subapertures fill the detector surface without any gaps.
    */

   aoCcdId->xStart = xStartReq;
   aoCcdId->yStart = yStartReq;
   aoCcdId->xRaster = xRasterReq;
   aoCcdId->yRaster = yRasterReq;
   aoCcdId->xSubapNb = xSubapNbReq;
   aoCcdId->ySubapNb = ySubapNbReq;
   aoCcdId->subapNb = xSubapNbReq * ySubapNbReq * aoCcdId->outputsNb;
   aoCcdId->subapNotUsedNb = 0;
   aoCcdId->subapUsedNb = aoCcdId->subapNb - aoCcdId->subapNotUsedNb;
   aoCcdId->centroidsNb = 2 * aoCcdId->subapUsedNb;

   for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
       aoCcdId->subapUsedVect[i] = TRUE;

   aoCcdId->xPixels = xPixelsReq;
   aoCcdId->yPixels = yPixelsReq;
   aoCcdId->pixelsNb = pixelsNbReq;
   aoCcdId->xTail = xTailReq;
   aoCcdId->packetNb = nPackets;

   aoCcdContextShow (aoCcdId);

   MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
      (aoCcdId->binningFlag ? "binned":"full"));
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%d, YSIZE=%d, XPIXELS=%d, YPIXELS=%d",
      aoCcdId->xSize, aoCcdId->ySize, aoCcdId->xPixels, aoCcdId->yPixels);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%d, YSUBAP=%d, XBIN=%d, YBIN=%d",
      aoCcdId->xSubapNb, aoCcdId->ySubapNb, aoCcdId->xBin, aoCcdId->yBin);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%d, YRAS=%d, XSPACE=%d, YSPACE=%d",
      aoCcdId->xRaster, aoCcdId->yRaster, aoCcdId->xSpace, aoCcdId->ySpace);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%d, YSTART=%d, XTAIL=%d, NPIXELS=%d",
      aoCcdId->xStart, aoCcdId->yStart, aoCcdId->xTail, aoCcdId->pixelsNb);

   MESSAGE_LOG2 (MSG_FULLDEBUG,
      "Each frame will consist of %d packets of %d pixels each",
      nPackets, aoCcdId->packetSize);

   /*
    * Update the geometry parameters in the SDSU timing DSP. These are all
    * "on-the-fly" parameters and need to be downloaded with sdsuParamWRP()
    * and activated by sending a "LDP" command.
    */

   if ( !sdsuId->simulate ) 
   {
      if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 
                          aoCcdId->packetSize) == ERROR)
      {
         ERROR_LOG ("Failed to increase the PWFS packet size");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", 
                          (uint32) xSubapNbReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", 
                          (uint32) ySubapNbReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", 
                          (uint32) xStartReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", 
                          (uint32) yStartReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   
                          (uint32) xRasterReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   
                          (uint32) yRasterReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  
                          (uint32) xTailReq) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", 
                          (uint32) pixelsNbReq) == ERROR)
        )
      {
         ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
      {
         ERROR_LOG ("Failed to load TIMING DSP parameters with LDP command");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      /*
       * Update the number of packets per frame in the SDSU context structure.
       */

      sdsuId->packetsPerFrame = nPackets;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detChop
 *
 *   INVOCATION:
 *   detChop (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detChop command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU chop parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detChop
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */

   long            chopMask;        /* Chop state mask.                       */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & chopMask);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG (MSG_LOG, "Specify chop states - NOT IMPLEMENTED YET");

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detExposure
 *
 *   INVOCATION:
 *   detExposure (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detExposure command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU exposure parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detExposure
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            nframe;        /* Number of frames.                        */
   long            nframePerDataset; /* Number of frames per dataset          */
   double          exposure;      /* Exposure time in seconds.                */

   uint32          sdsuNframe;    /* Value for SDSU parameter NFRAME.         */
   uint32          sdsuTexp;      /* Value for SDSU parameter T_EXP.          */

   int             nexp;          /* Number of exposure/dataset               */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & nframe);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & exposure);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Check the number of frames is sensible */

   if ( (nframe <= 0) && (nframe != -1) )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Invalid number of frames, %ld",
         ERROR_LOG_NOW, nframe);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Check the exposure time is sensible. The SDSU controller measures 
    * exposures in units of 81.92 microseconds and stores the exposure in a 
    * 32 bit integer, so the upper limit in seconds is 2**32 * 0.000008192 = 
    * 351,843 seconds
    */

   if ( (exposure < 0.0) || (exposure > 351843.0) )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid exposure time, %f seconds.",
         ERROR_LOG_NOW, exposure);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)&exposure, obsId->pIntTimeContext ) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set integration time SIR record");
   }

   obsId->exposureTime = exposure;
   sdsuId->readMethod = 1;

   if ( nframe == -1 )
   {
      MESSAGE_LOG1 (MSG_LOG, 
         "Setting up for an infinite series of exposures of %f seconds each",
         exposure);

      nframePerDataset = nframe ;


      /* BUG WORK AROUND: THE SDSU CONTROLLER RETURNS FRAME COUNT=1 WHEN ASKED 
       * FOR AN INFINITE
       * NUMBER OF FRAMES, WHICH DETCONTROL THEN ASSUMES MEANS THE LAST FRAME 
       * HAS BEEN RECEIVED.
       * UNTIL THE SDSU CODE IS FIXED, SET A FLAG TO INDICATE THE FRAME COUNT 
       * IS INFINITE.
       */

      nframe = 0;      /* DSP code assumes 0 means infinite number of frames. */
      obsId->continuous = TRUE;

   }
   else if ( nframe == 1 )
   {
      MESSAGE_LOG1 (MSG_LOG, "Setting up for one exposure of %f seconds", 
                    exposure);

      nframePerDataset = nframe ;

      obsId->continuous = FALSE;
   }
   else
   {
      MESSAGE_LOG2 (MSG_LOG, 
      "Setting up for %ld exposures of %f seconds each", nframe, exposure);

      nframePerDataset = 1 ;

      obsId->continuous = FALSE;
   }

   /* Set the number of frames by writing to the T_NFRAME parameter in the 
    * timing DSP Also define the total number of frames in the observation 
    * context structure. */

   sdsuNframe = (uint32) nframe;
   obsId->totalFrames = nframe;

#ifdef DEBUG
   printf ("detExposure: Setting T_NFRAME parameter to %lu\n", sdsuNframe);
#endif /* DEBUG */

   if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_NFRAME", sdsuNframe ) == 
        ERROR )
   {
      ERROR_LOG ("Error setting number of frames parameter");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Set the exposure time by writing to the T_EXP_TIM parameter in the 
    * timing DSP */

   sdsuTexp = (uint32) (exposure / SDSU_EXPOSURE_UNIT);

#ifdef DEBUG
   printf ("detExposure: Setting T_EXP_TIM parameter to %lu\n", sdsuTexp);
#endif /* DEBUG */

   if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", sdsuTexp ) 
        == ERROR )
   {
      ERROR_LOG ("Error setting exposure time parameter");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Update the requested total exposure time in the observation context 
    * structure. */

   obsId->exposedRQ = 1 * exposure;
   sdsuId->exposureTicks = (int) (exposure * sysClkRateGet());
   /*printf ( "exposureTicks =%d\n" , sdsuId->exposureTicks ) ;*/

#ifdef DEBUG
   printf ("detExposure: Setting exposureTicks to %d\n", sdsuId->exposureTicks);
#endif /* DEBUG */

   /*
    * Set up the requested and actual number of exposure/dataset.
    * Always 1 for the moment
    */
   nexp = 1 ;
   if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpRQContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Number exp/dataset SIR record");
   }
   if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Number exp/dataset SIR record");
   }

   /*
    * Set up the the total integration time requested
    */

   if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposedRQ), 
                        obsId->pExposedRQContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init total integration time requested SIR record");
   }

   /*
    * Set up the the number of frames per dataset
    */

   if (epToVxPipeWrite( NULL, (char *)(int)&nframePerDataset, 
                        obsId->pNFramesContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set number of frames SIR record");
   }

   /*
    * Init the butterworth filter for probe arm guiding
    */

   obsId->cutoffFrequency = obsId->rateSamplingFrequency / obsId->exposureTime;

   if ( detComputeCoeffButterworth ( obsId->exposureTime,
                                     obsId->cutoffFrequency,
                                     coeffData ) == ERROR )
   {
      ERROR_LOG ( "Failed to init coefficients of butterworth filter");
   }

   /*
    * Update the dhsQlRate
    */

   if ( obsId->continuous == TRUE )
   {
      if ( obsId->exposureTime <= 1.0 )
         obsId->dhsQlRate = (int)(1.0 / obsId->exposureTime);
      else
         obsId->dhsQlRate = 1;
   }
   else
   {
      if ( obsId->totalFrames == 1 )
         obsId->dhsQlRate = 1;
      else
      {
         if ( obsId->exposureTime <= 1.0 )
         {
            if ( obsId->totalFrames > (int)(1.0 / obsId->exposureTime) )
               obsId->dhsQlRate = (int)(1.0 / obsId->exposureTime);
            else
               obsId->dhsQlRate = obsId->totalFrames;
         }
         else
            obsId->dhsQlRate = 1;
      }
   }

#ifdef DEBUG
   printf ( "dhsQlRate = %d\n", obsId->dhsQlRate );
#endif

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObstype
 *
 *   INVOCATION:
 *   detObstype (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detObstype command
 *
 *   DESCRIPTION:
 *   This function defines the observation type.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detObstype
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;        /* Error number reported by task.         */

   char         obsType[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Observation type string.               */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, obsType);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   strncpy (obsId->pObsType, obsType, EPICS_MAX_BYTES_STRING_ATTRIB);

   MESSAGE_LOG1 (MSG_LOG, "Observation type defined as %s", obsType);

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSetWcs
 *
 *   INVOCATION:
 *   detSetWCs (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSetWcs command
 *
 *   DESCRIPTION:
 *   This function defines the World Coordinate System calibration parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSetWcs
   (
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID         sdsuId,           /* SDSU context structure.               */
   OBS_ID          obsId             /* Observation context structure.        */
   )
{
   uint32          errorNumber;      /* Error number reported by task.        */

   char      pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                     /* Path name for file.                   */
   char      pWcsFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                     /* Name of file containing WCS calib.    */
   char      pFullWcsFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                     /* Combined path name and file name.     */

   FILE *      pWcsFile;
   char        pLine[100];           /* Line read from file.                  */
   int         nread;                /* Number of items read from file.       */
   int         nextChar;             /* Character sensed from next line.      */
   int         p;                    /* Number of points.                     */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pWcsFileName);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "SDSU context not initialised", ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Observation context not initialised", ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together, and append the string ".wcs" 
    * to the file name
    * if it is not already present. Ignore the file path if not provided.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullWcsFileName, pWcsFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      sprintf (pFullWcsFileName, "%s/%s", pFilePath, pWcsFileName );
   }

   if (strstr (pFullWcsFileName, ".wcs") == NULL)
      strncat (pFullWcsFileName, ".wcs", EPICS_MAX_BYTES_STRING_ATTRIB);

   MESSAGE_LOG1 (MSG_LOG, "Defining WCS parameters from %s", pFullWcsFileName);

   /*
    * Open the WCS calibration file and read its contents into the pixij and 
    * fpxy arrays.
    */

   if ((pWcsFile = fopen (pFullWcsFileName, "r")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open WCS calibration file, %s", 
                  ERROR_LOG_NOW, pFullWcsFileName);
      errorNumber = S_detControl_BAD_FILE;
      return (errorNumber);
   }

   /*
    * Skip over any comment lines at the beginning of the file denoted by a 
    * ";" in column 1.
    */

   while ( (nextChar = fgetc (pWcsFile)) == ';' )      
   {
      ungetc (nextChar, pWcsFile);        /* Undo the effect of fgetc().      */
      fgets (pLine, 100, pWcsFile);       /* Skip the whole line.             */
   }
   ungetc (nextChar, pWcsFile);           /* Undo the final fgetc().          */

   /*
    * Read through the rest of the file, assuming each line contains
    * pixel coordinate i, pixel coordinate j, x coordinate, y coordinate.
    */

   p = 0;
   while ( (p < DET_CONTROL_MAX_WCSPOINTS) &&
           (nread = fscanf (pWcsFile, "%lf %lf %lf %lf",
                            &(obsId->pixij[p][0]), &(obsId->pixij[p][1]),
                            &(obsId->fpxy[p][0]),  &(obsId->fpxy[p][1])
                           )
            != EOF)
         )
   {

      /* Skip blank or unreadable lines. */

      if ( nread > 0 )
      {
#ifdef DEBUG
         printf ("detSetWcs: Point %d: %f %f %f %f\n", p, obsId->pixij[p][0], 
                 obsId->pixij[p][1], obsId->fpxy[p][0], obsId->fpxy[p][1]);
#endif
         p++;
      }
   }

   obsId->nWcsPoints = p;
   MESSAGE_LOG1 (MSG_MINDEBUG, "%d points read from WCS calibration file.", p);
   if ( (p == DET_CONTROL_MAX_WCSPOINTS) && (nread != EOF) )
   {
      MESSAGE_LOG (MSG_WARNING, 
      "WARNING: Maximum number of points reached before end of file");
   }

   /*
    * Close the file and tidy up.
    */
   
   if (fclose (pWcsFile) == ERROR)
   {
      ERROR_SET (0, "Failed to close WCS file", ERROR_LOG_NOW);
      errorNumber = S_detControl_BAD_FILE;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

void detDhsErrorCallback         /* DHS error callback function.              */
   (
   DHS_CONNECT     connect,      /* DHS connection ID for connection causing  */
                                 /* error.                                    */
   DHS_STATUS      errorNum,     /* DHS error number.                         */
   DHS_ERR_LEVEL   errorLev,     /* DHS error level.                          */
   char *          msg,          /* DHS error message string.                 */
   DHS_TAG         tag,          /* DHS command tag of the error.             */
   void *          userData      /* Pointer to user data (if any).            */
   )
{
   printErr ("DHS error callback: connection=%d errNum=%d level=%d \"%s\"\n",
             (int) connect, (int) errorNum, (int) errorLev, msg);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsParamInit
 *
 *   INVOCATION:
 *   detDhsParamInit (pClientName, numConnect, pHostName, pSeverName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pClientName  (const char *)  Unique name for DHS client.
 *   (>) numConnect   (const int)     Maximum number of DHS connections.
 *   (>) pHostName    (const char *)  Name of DHS data server host.
 *   (>) pServerName  (const char *)  Name of DHS data server.
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise the parameters needed to init the DHS library and define DHS
 *   server information
 *
 *   DESCRIPTION:
 *   This function initialises the parameters needed to init the DHS library
 *   and sets up the DHS server information used by the detector controller.
 *
 *   EXTERNAL VARIABLES:
 *   (<) pDetDhsClientName (char *) Current name of DHS client= Instrument name.
 *   (<) pDetDhsHostName   (char *) Current name of DHS server host.
 *   (<) pDetDhsServerName (char *) Current name of DHS server.
 *   (<) pDetDhsNumConnect (char *) Current number of DHS connections.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *-
 */

STATUS detDhsParamInit
   (
   const char *   pClientName,      /* Unique name of DHS client.             */
   const int      numConnect,       /* Maximum number of DHS connections.     */
   const char *   pHostName,        /* Name of data server host.              */
   const char *   pServerName       /* Name of server.                        */
   )
{

   /* Store the given client name, host name and server name in global
    * variables.
    */

   strncpy (pDetDhsClientName, pClientName, EPICS_MAX_BYTES_STRING_ATTRIB);
   strncpy (pDetDhsHostName, pHostName, EPICS_MAX_BYTES_STRING_ATTRIB);
   strncpy (pDetDhsServerName, pServerName, EPICS_MAX_BYTES_STRING_ATTRIB);
   detDhsNumConnect = numConnect;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsInit
 *
 *   INVOCATION:
 *   detDhsInit ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise the DHS library and define DHS server information
 *
 *   DESCRIPTION:
 *   This function initialises the DHS library and sets up the DHS server 
 *   information used by the detector controller.
 *
 *   EXTERNAL VARIABLES:
 *   (<) detDhsInitialised (BOOL)   DHS initialised flag.
 *   (<) detDhsStartSem    (SEM_ID) DHS semaphore
 *   (<) pDetDhsClientName (char *) Current name of DHS client= Instrument name.
 *   (<) pDetDhsHostName   (char *) Current name of DHS server host.
 *   (<) pDetDhsServerName (char *) Current name of DHS server.
 *   (<) pDetDhsNumConnect (char *) Current number of DHS connections.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *-
 */

STATUS detDhsInit
   (
   )
{
   DHS_STATUS     dhsErrno;         /* DHS error number.                      */
   DHS_THREAD     dhsThreadId;      /* DHS thread ID.                         */

   /* Initialise the DHS error number. */

   dhsErrno = DHS_S_SUCCESS;        /* <--- DHS error number initialised here.*/

   /*
    * Check the DHS library has not already been initialised.
    */

   if (detDhsInitialised)
   {
      ERROR_SET (S_detControl_DHS_ERROR, "DHS already initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Initialise the DHS, specifying a unique name and maximum number of 
    * connections.
    */

   MESSAGE_LOG2 (MSG_MINDEBUG, 
                 "dhsInit pClientName=%s numConnect=%d\n", 
                 pDetDhsClientName, detDhsNumConnect);

   dhsInit (pDetDhsClientName, detDhsNumConnect, &dhsErrno);
   CHECK_DHS (dhsErrno);

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
                  "Failed to init DHS (dhsErrno=%d)",
                  ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /* Set up callbacks. */

   MESSAGE_LOG2 ( MSG_MINDEBUG,
                  "dhsCallbackSet DHS_CBT_ERROR=%d detDhsErrorCallback=%p\n",
                  DHS_CBT_ERROR, detDhsErrorCallback);

   dhsCallbackSet (DHS_CBT_ERROR, detDhsErrorCallback, &dhsErrno);
   CHECK_DHS (dhsErrno);

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
         "Failed to set up DHS error callback (dhsErrno=%d)",
         ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /*
    * Start the DHS event loop.
    */

   MESSAGE_LOG1 (MSG_MINDEBUG, 
                 "detDhsInit: dhsEventLoop DHS_ELT_THREADED=%d ... ", 
                 DHS_ELT_THREADED);

   dhsEventLoop (DHS_ELT_THREADED, &dhsThreadId, &dhsErrno);
   CHECK_DHS (dhsErrno);

   MESSAGE_LOG2 (MSG_MINDEBUG, "dhsThreadId=%d dhsErrno=%d\n", 
                 dhsThreadId, dhsErrno);

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
         "Failed to start DHS event loop (dhsErrno=%d)",
         ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /* Create the start DHS semaphores */

   detDhsStartSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
   if ( detDhsStartSem == NULL )
   {
      ERROR_SET (0, "Failed to create start DHS semaphore", ERROR_LOG_NOW);
      return (ERROR);
   }

   /* Finally, set the detDhsInitialised flag and return the semaphore. */

   detDhsInitialised = TRUE;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsTaskOpen
 *
 *   INVOCATION:
 *   detDhsTaskOpen ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Start a dhs task for PWFS2
 *
 *   DESCRIPTION:
 *   This routine will start a DHS task. This routine uses global variables. 
 *   Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *   (!) detDhsTaskId (int) Task Id of the dhs task
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDhsTaskOpen
   (
   )
{
   char *       dhsName;
   const char * myName;

   /* Complain if the dhs task already exists */

   if ( detDhsTaskId != 0 )
   {
      ERROR_SET (0, "Dhs task already exists", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Create the name of the task: name of the current task and append an
    * extra 5 bytes ":dhs" 
    */

   myName = taskName(0);

   if ((dhsName = (char *) malloc(strlen(myName) + 5)) == NULL)
   {
      ERROR_SET (0, "Memory allocation for dhs task name failed",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   strcpy ( dhsName, myName ) ;
   strcat ( dhsName, ":dhs" ) ;

   /* Taskspawn the dhs task */

   detDhsTaskId = taskSpawn (dhsName, DET_DHS_TASK_PRIORITY, VX_FP_TASK, 
                             DET_DHS_TASK_STACK_SIZE, (FUNCPTR) detDhsTask, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

   if ( detDhsTaskId == ERROR )
   {
      ERROR_SET1 (0, "Failed to spawn dhs task %s\n", 
                  ERROR_LOG_SAVE, dhsName );
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "taskSpawn ( %s, %d, %x, %x, detDhsTask, ...) ok\n" , 
            dhsName, DET_DHS_TASK_PRIORITY, VX_FP_TASK, 
            DET_DHS_TASK_STACK_SIZE);
#endif

   /* Free the temporary name buffer. */

   free (dhsName);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsTaskClose
 *
 *   INVOCATION:
 *   detDhsTaskClose ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Shut down the dhs task for PWFS2
 *
 *   DESCRIPTION:
 *   This routine will stop the DHS task. This routine uses global variables. 
 *   Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *   (!) detDhsTask (int) Task Id of the dhs task
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDhsTaskClose
   (
   )
{

   /* if the dhs task already exists do nothing */

   if ( detDhsTaskId == 0 )
   {
      return (OK);
   }

   /* Delete the task */

   if ( taskDelete (detDhsTaskId) == ERROR )
   {
      ERROR_SET (0, "Failed to delete the dhs task \n", ERROR_LOG_SAVE );
      return (ERROR);
   }

   detDhsTaskId = 0;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsConnect
 *
 *   INVOCATION:
 *   detDhsConnect ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise connection to DHS 
 *
 *   DESCRIPTION:
 *   This function initialises the connection to the DHS.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   detDhsInitialised   (BOOL)        DHS initialised flag
 *   (>)   pDetDhsHostName     (char *)      DHS server host name
 *   (>)   pDetDhsServerName   (char *)      DHS server name
 *   (!)   detDhsConnection    (DHS_CONNECT) DHS connection Id
 *
 *   PRIOR REQUIREMENTS:
 *   The DHS library should already have been initialised by calling detDhsInit.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDhsConnect
   (
   )
{
   DHS_STATUS     dhsErrno;           /* DHS error number.                    */


   /* Initialise the DHS error number. */

   dhsErrno = DHS_S_SUCCESS;

   /*
    * Check the DHS library has been initialised.
    */

   if (!detDhsInitialised)
   {
      ERROR_SET (S_detControl_DHS_ERROR, "DHS not initialised", ERROR_LOG_NOW);
      detDhsConnected = NOT_INIT;
      return (ERROR);
   }

   /*
    * Connect to the DHS server. There is no user data to be supplied
    * (hence NULL).
    */

   MESSAGE_LOG2 (MSG_LOG, "Connecting to DHS server %s on host %s",
      pDetDhsServerName, pDetDhsHostName);

   detDhsConnection = dhsConnect (pDetDhsHostName, pDetDhsServerName, NULL,
                                  &dhsErrno);
   CHECK_DHS (dhsErrno);

#ifdef DEBUG
   printf ("dhsConnect: dhsConnection=%ld dhsErrno=%d\n", detDhsConnection,
           dhsErrno);
#endif /* DEBUG */

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET3 (S_detControl_DHS_ERROR,
         "Failed to connect to DHS server %s on %s (dhsErrno=%d)",
         ERROR_LOG_SAVE, pDetDhsServerName, pDetDhsHostName, dhsErrno);
      return (ERROR);
   }

   /* Finally, return. */

   detDhsConnected = CONNECTED;
   MESSAGE_LOG (MSG_LOG, "Connected to DHS");

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsCheckErrno
 *
 *   INVOCATION:
 *   detDhsCheckErrno (dhsErrno, line, filename)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) dhsErrno   (const DHS_STATUS)  DHS error number (unchanged)
 *   (>) line       (const int)         Line number to report
 *   (>) filename   (const char *)      File name to report
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Check DHS error number and report any error messages
 *
 *   DESCRIPTION:
 *   This function checks the DHS error number provided. If the status suggests
 *   an error has occurred, the dhsMessage() functions are used to extract
 *   information from the DHS message stack.
 *   This function should be called after every DHS function to ensure all the
 *   relevant DHS errors are reported.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   This function appears to cause problems with the DHS.
 *   For the time being its contents are commented out and replaced by a
 *   trivial report. SMB - 17 Jan 1999.
 *-
 */

void detDhsCheckErrno
   (
   const DHS_STATUS  dhsErrno,        /* DHS error number.                    */
   const int         line,            /* Line number.                         */
   const char *      filename         /* File name.                           */
   )
{

   /*
    * If the DHS error number is ok, this function will return without doing
    * anything.
    */

   if ( dhsErrno != DHS_S_SUCCESS )
   {
      errorSet ( line, filename, 0, "DHS error detected", ERROR_LOG_NOW );
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsCheckCmdStatus
 *
 *   INVOCATION:
 *   detDhsCheckCmdStatus (dhsTag)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   dhsTag   (const DHS_TAG)      DHS command tag (unchanged)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Check and reports DHS command status
 *
 *   DESCRIPTION:
 *   This function checks the DHS command status and reports a message if the
 *   status is not DHS_CS_DONE.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   The DHS allocates a buffer to store the command status message. It would
 *   be more sensible if the buffer was allocated here and provided to the DHS,
 *   as there would then be more control over the buffer. At the moment the
 *   buffer has to be explicitly freed because the DHS does not do this.
 *   SMB - 17 Jan 1998.
 *-
 */

STATUS detDhsCheckCmdStatus
   (
   const DHS_TAG   dhsTag            /* DHS command tag.                      */
   )
{
   DHS_CMD_STATUS  sendStatus;       /* DHS command status.                   */
   DHS_STATUS      dhsErrno;         /* DHS error number.                     */
   char            *msg = NULL;      /* Command status message.               */


   /* Initialise the DHS error number */

   dhsErrno = DHS_S_SUCCESS;

   /*
    * Query the command status associated with the tag.
    * Note that the DHS allocates a buffer to hold the command status message
    * and returns a pointer to this buffer in "msg".
    */

#ifdef DEBUG
   printf ("detDhsCheckCmdStatus: dhsStatus\n");
#endif /* DEBUG */

   /*sendStatus = dhsStatus (dhsTag, &msg, &dhsErrno);
   CHECK_DHS (dhsErrno);*/

   sendStatus = DHS_CS_DONE ;

   /*
    * Check that the query worked and report an error if it didn't.
    * If the query returned DHS_CS_DONE nothing more needs to be done.
    * Any other command status is reported as an error.
    */

   if ( dhsErrno != DHS_S_SUCCESS )
   {

      ERROR_SET2 (0,
         "Failed to query DHS command status for tag %ld, (dhsErrno=%d)",
         ERROR_LOG_SAVE, dhsTag, dhsErrno);

      /* Free the message buffer if allocated. */
      if ( msg != NULL ) free (msg);
      return (ERROR);
   }
   else if ( sendStatus != DHS_CS_DONE )
   {
      switch (sendStatus)
      {
         case (DHS_CS_IDLE):

            ERROR_SET1 (0,
               "Command still waiting to execute, %s", ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_BUSY):

            ERROR_SET1 (0,
               "Command is still executing, %s", ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_ERROR):

            ERROR_SET1 (0, "Command completed with error, %s",
                       ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_ABORTED):

            ERROR_SET1 (0, "Command was aborted, %s.", ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_PENDING):

            ERROR_SET1 (0, "Command is still pending, %s", ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_LOST):

            ERROR_SET1 (0, "Connection was lost before command completed, %s",
                       ERROR_LOG_SAVE, msg);
            break;

         default:
            ERROR_SET2 (0,
               "Unknown command status, %d, %s", ERROR_LOG_SAVE, sendStatus,
               msg);
            break;
      }

      /* Free the message buffer if allocated. */
      if ( msg != NULL ) free (msg);

      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsTask
 *
 *   INVOCATION:
 *   detDhsTask ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Dhs task
 *
 *   DESCRIPTION:
 *   
 *   Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detDhsTask
   (
   )
{
   /* Circular buffer variable */

   int         indexCb;
   int         imageSize;
   float *     pImage;
   float *     pi;
   float *     pc;
   float *     pMax;

   int         i;
   
   double      elapsed;

   /* DHS variables (see dhstests.c) */

   DHS_STATUS  dhsErrno;          /* DHS error number.                        */
   DHS_STATUS  dummyDhsErrno;     /* DHS error number used for freeing        */
                                  /* resources.                               */
   DHS_TAG     putTag;            /* DHS data transfer tag.                   */

   /* Create and init an error context structure for this task */

   if (errorInit () == ERROR)
   {
      printErr ("detDhsTask: Failed to init error context structure.\n");
      return;
   }

   /*
    * Initialise the DHS error number.
    */

   dhsErrno = DHS_S_SUCCESS;         /* <---- DHS error number is reset here. */

   /* Flush the start semaphore */

   if ( semTake ( detDhsStartSem, NO_WAIT ) == ERROR )
   {
      ERROR_SET ( 0, "Failed to flush the DHS start semaphore",
                  ERROR_LOG_SAVE );
   }
   else
   {
      MESSAGE_LOG (MSG_FULLDEBUG, "Flush the DHS start semaphore") ;
   }

   /* Now enter a infinite loop waiting for an observation */

   MESSAGE_LOG ( MSG_LOG , "detDhsTask(): Enter the infinite loop" ) ;

   while ( TRUE )
   {
       /* Wait for the start DHS semaphore  */

       if ( detObsIdP2->stopped != TRUE )
       {
          if ( semTake ( detDhsStartSem, WAIT_FOREVER ) == ERROR )
          {
             ERROR_SET ( 0, "Failed to take the DHS start semaphore",
                         ERROR_LOG_SAVE );
             return;
          }

          MESSAGE_LOG ( MSG_FULLDEBUG, "Take the start DHS semaphore" ); 

          /* Init the address of the image to display */

          indexCb = detObsIdP2->aoCbImId->position;

          while ( ( indexCb == 0 ) && ( detObsIdP2->aoCbImId->counter == 0) )
                taskDelay(1);

          if ( indexCb != 0 )
             indexCb -= 1;    
          else
             indexCb = CB_IM_RECORD_NB - 1;

          MESSAGE_LOG1 ( MSG_FULLDEBUG, "Position read in the image CB: %d",
                         indexCb ) ;

          pImage = detObsIdP2->aoCbImId->cbImRecord[indexCb].imageVect;

          /* Copy the image into pCurFrame */

          imageSize = detObsIdP2->aoCcdId->xPixels * 
                      detObsIdP2->aoCcdId->yPixels;
          pMax = (float *)((int)pImage + imageSize*sizeof(float));
          pc = detObsIdP2->pCurFrame ;

          for ( pi = pImage ; pi < pMax ; pi ++ )
              *(pc ++) = *pi;

          for ( i = 0 ; i < 10 ; i ++ )
              printf ( "dhs pixel %d= %f\n" , i , 
                       *(detObsIdP2->pCurFrame + i) );
       
          /*
           * Compute the elapsed time
           */

          elapsed = detObsIdP2->rawtEnd - detObsIdP2->rawtStart ;

          /* Write some keyworks */

          if ( detObsIdP2->totalFrames == 1 )
          {
             /*
              * Convert the time stamps from Gemini raw time into Universal Time
              * and construct these into character strings.
              */

             if (timeThenC( detObsIdP2->rawtEnd, UT1, 2, 
                            detObsIdP2->timeArrayEnd ) != OK)
             {
                ERROR_SET (0,
                "Failed to convert time stamp at observation end to date/time" ,
                ERROR_LOG_NOW);
             }
             else
             {

                sprintf (detObsIdP2->utEndString, 
                         "%04d-%02d-%02d:%02d:%02d:%02d",
                         detObsIdP2->timeArrayEnd[0], 
                         detObsIdP2->timeArrayEnd[1], 
                         detObsIdP2->timeArrayEnd[2], 
                         detObsIdP2->timeArrayEnd[3], 
                         detObsIdP2->timeArrayEnd[4], 
                         detObsIdP2->timeArrayEnd[5]);

                dhsBdAttribAdd (detObsIdP2->dhsDataFrame, "utend", 
                                DHS_DT_STRING, 0, NULL, 
                                detObsIdP2->utEndString, &dhsErrno);
                CHECK_DHS (dhsErrno);

                if (epToVxPipeWrite( NULL, (char *)detObsIdP2->utEndString, 
                                     detObsIdP2->pUTendContext) == ERROR)
                {
                   ERROR_LOG (
                         "Failed to set UT at end of observation SIR record");
                }

                if (epToVxPipeWrite( NULL, (char *)(int)&elapsed, 
                                     detObsIdP2->pElapsedContext ) == ERROR)
                {
                   ERROR_LOG ("Failed to set elapsed time SIR record");
                }
             }
          }

#ifdef DEBUG
          dhsBdDsPrint (detObsIdP2->dhsDataset, &dhsErrno);
          CHECK_DHS (dhsErrno);
#endif /* DEBUG */

          /* Send the data to the dhs */

          MESSAGE_LOG3 (MSG_FULLDEBUG, 
                        "dhsBdPut, dhsConnection=%d, pDataLabel=%s, dataset=%d",
                        (int) detDhsConnection, detObsIdP2->pDataLabel, 
                        (int) detObsIdP2->dhsDataset);

          if ( detObsIdP2->dhsOutOptions == 2 ) /* QL only */
          {
             if ( detObsIdP2->totalFrames == 1 )
                putTag = dhsBdPut (detDhsConnection, detObsIdP2->pDataLabel,
                                   DHS_BD_PT_DS_QL, DHS_TRUE,
                                   detObsIdP2->dhsDataset, NULL, &dhsErrno);
             else
                putTag = dhsBdPut (detDhsConnection, detObsIdP2->pDataLabel,
                                   DHS_BD_PT_DS_QL, DHS_FALSE,
                                   detObsIdP2->dhsDataset, NULL, &dhsErrno);
          }
          else
          {
             if ( detObsIdP2->totalFrames == 1 )
             {
                putTag =
                dhsBdPut (detDhsConnection, detObsIdP2->pDataLabel,
                          DHS_BD_PT_DS, DHS_TRUE, detObsIdP2->dhsDataset, NULL, 
                          &dhsErrno);
             }
             else
             {
                putTag =
                dhsBdPut (detDhsConnection, detObsIdP2->pDataLabel,
                          DHS_BD_PT_DS, DHS_FALSE, detObsIdP2->dhsDataset, NULL,
                          &dhsErrno);
             }
          }

          CHECK_DHS (dhsErrno);

          if (dhsErrno != DHS_S_SUCCESS)
          {
             ERROR_SET1 (S_detControl_DHS_ERROR,
                         "Failed to initiate data transfer (dhsErrno=%d)",
                         ERROR_LOG_NOW, dhsErrno);
             dummyDhsErrno = DHS_S_SUCCESS;
             dhsTagFree (putTag, &dummyDhsErrno);
             CHECK_DHS (dummyDhsErrno);
             dummyDhsErrno = DHS_S_SUCCESS;
             dhsBdDsFree (detObsIdP2->dhsDataset, &dummyDhsErrno);
             CHECK_DHS (dummyDhsErrno);
          }
          else
          {
             /* Wait for completion */

             MESSAGE_LOG1 (MSG_FULLDEBUG, "dhsWait putTag=%d ...", 
                           (int) putTag);

             dhsWait (1, &putTag, &dhsErrno);
             CHECK_DHS (dhsErrno);
             if (dhsErrno != DHS_S_SUCCESS)
             {
                ERROR_SET1 (S_detControl_DHS_ERROR,
                            "Error during wait for data transfer (dhsErrno=%d)",
                            ERROR_LOG_NOW, dhsErrno);

                dummyDhsErrno = DHS_S_SUCCESS;
                dhsTagFree (putTag, &dummyDhsErrno);
                CHECK_DHS (dummyDhsErrno);
                dummyDhsErrno = DHS_S_SUCCESS;
                dhsBdDsFree (detObsIdP2->dhsDataset, &dummyDhsErrno);
                CHECK_DHS (dummyDhsErrno);
             }
             else
             {

                MESSAGE_LOG1 (MSG_FULLDEBUG,
                              "detDhsCheckCmdStatus putTag=%d ...",
                              (int) putTag);

                if ( detDhsCheckCmdStatus (putTag) == ERROR )
                {
                   ERROR_SET (S_detControl_DHS_ERROR, "Data transfer failed",
                              ERROR_LOG_NOW);

                   dummyDhsErrno = DHS_S_SUCCESS;
                   dhsTagFree (putTag, &dummyDhsErrno);
                   CHECK_DHS (dummyDhsErrno);
                   dummyDhsErrno = DHS_S_SUCCESS;
                   dhsBdDsFree (detObsIdP2->dhsDataset, &dummyDhsErrno);
                   CHECK_DHS (dummyDhsErrno);
                }
                else
                {

                   /*
                    * If the last frame has been received free the DHS dataset.
                    */

                   MESSAGE_LOG1 (MSG_FULLDEBUG, "dhsTagFree putTag=%d",
                                (int) putTag);

                   dhsErrno = DHS_S_SUCCESS;
                   dhsTagFree (putTag, &dhsErrno);

                   if ( (detObsIdP2->totalFrames == 1) || 
                        (detObsIdP2->stopped) )
                   {
                      dhsBdDsFree (detObsIdP2->dhsDataset, &dhsErrno);
                      CHECK_DHS (dhsErrno);
                      semFlush (detDhsStartSem) ;
                      MESSAGE_LOG (MSG_FULLDEBUG, 
                           "Last frame displayed and flush detDhsStartSem" );
                   }
                }
             }
          }
       }
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveStart
 *
 *   INVOCATION:
 *   detObserveStart (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detObserve command and start observation
 *
 *   DESCRIPTION:
 *   This function starts an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   DHS and WCS code needs tidying up.
 *
 *-
 */

uint32 detObserveStart
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context data structure.      */
   )
{

   uint32       errorNumber;  /* Error number reported by task.               */
   
   int          i, j, k;

   /* Variables describing the observation. */

   int          defOutputs;   /* Default number of outputs.                   */

   /* Variables used to specify data label and file names. */

   long         outOptions;   /* Output options (0=none, 1=DHS, 2=file).      */
   long         dhsOutOptions;/* DHS output options (0=PERM, 1=TEMP, 2=QL)    */

   char *       pLabelFromDhs;/* Data label provided by DHS server.           */

   char         pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* DHS data label.                              */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pOutFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Output file name (only if DHS not being used)*/
   char         pSimFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Simulated data file name (only if detector   */
                              /* controller is being simulated).              */
   char         pFullOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and output file name.     */
   char         pFullSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and simulated data file   */
                              /* name.                                        */

   /* Other DHS variables (see dhstests.c) */

   DHS_STATUS     dhsErrno;
   char *         axisLabel[2]={"Xaxis","Yaxis"};
   uint32         dims[1];
   uint32         axisSize[2];
   uint32         origin[2];
   char           *qlStreams[1];
   char           *contrib[1];
   char           telName [40];

   /* Variables associated with the provision of WCS information. */

   int            wcsStatus=0;     /* WCS status.                             */
   double         pixis;           /* x to i scale factor.                    */
   double         pixjs;           /* y to j scale factor.                    */
   double         perp;            /* Non-perpendicularity of i and j axes in */
                                   /* radians                                 */
   double         orient;          /* Orientation of (i,j) axes with respect  */
                                   /* to (x,y) in radians.                    */
   struct WCS_CTX ctx;             /* World Coordinate System context.        */
   struct WCS     wcs;             /* Basic TCS World Coordinate System.      */
   struct WCS     wcsij;           /* Transformed WCS for IJ.                 */
   double         trackRA;         /* TCS track Right Ascension.              */
   double         trackDec;        /* TCS track Declination.                  */

   FRAMETYPE      trackFrame;      /* TCS track frame                         */
   struct EPOCH   trackEquinox;    /* TCS track equinox.                      */
   struct EPOCH   trackEpoch;      /* TCS track epoch.                        */
   double         trackWavelength; /* Track wavelength in microns.            */
   double         timeTAI;         /* International Atomic Time.              */
   double         rawTimeWcs;      /* Gemini raw time at which WCS info is    */
                                   /* valid.                                  */
   int            chopState;       /* Chop state to which WCS information     */
                                   /* refers.                                 */
   int            p;               /* Point counter.                          */

   char           raString[16];    /* String which contains the RA value      */
   char           decString[16];   /* String which contains the Dec value     */
   float          crpix1Float;     /* Float value of crpix1                   */
   float          crpix2Float;     /* Float value of crpix2                   */
   float          cd1_1Float;      /* Float value of cd1_1                    */
   float          cd1_2Float;      /* Float value of cd1_2                    */
   float          cd2_1Float;      /* Float value of cd2_1                    */
   float          cd2_2Float;      /* Float value of cd2_2                    */

   /* SDSU parameters. */

   long           nframe;          /* Number of frames.                       */
   long           nframePerDataset;/* Number of frames per dataset            */
   int            nexp;            /* Number of exposure/dataset              */

   double         exposure;        /* Exposure time in seconds.               */

   uint32         sdsuNframe;      /* Value for SDSU parameter NFRAME.        */

   uint32         expTim;          /* Exp. time in SDSU units from T_EXPTIM.  */
   double         readoutTimeout;  /* Readout timeout in seconds.             */
   double         waitTimeSecs;    /* Wait time in seconds.                   */

   /* 
    * Variables associated with "observe" command.
    * (Label, datapath and filename use general filename parameters)
    */

   long           observingState;  /* Observation status (busy or idle).      */
   long           measuringState;  /* Continuous mode 1 else 0                */

   /*
    * Initialise the error number and DHS error number.
    */

   errorNumber = 0;
   dhsErrno = DHS_S_SUCCESS;       /* <---- DHS error number is reset here.   */

   /*
    * Check there are valid SDSU, observation and AO context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCcdId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "AO CCD geometry context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   
   if ( obsId->aoCbImId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "AO image circular buffer context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   
   if ( obsId->aoCbAoCtrlId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "AO control circular buffer context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCbFgCtrlId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL,
                 "AO FG control circular buffer context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   
   /* Determine whether a START or STOP directive has been received. */

   if ( EPTOVX_IS_STOP_DIRECTIVE(cadCmdContext) )
   {
      /* Stop directive received - treat as a STOP command and stop the 
       * observation. 
       */

#ifdef DEBUG
      printf ( "ptrPwfs2->interval=%f\n" , ptrPwfs2->interval ) ;
#endif
      errorNumber = detStop (cadCmdContext, commandNumber, sdsuId, obsId);
   }
   else
   {
      /*
       * START directive obtained. This directive cannot be used when an
       * observation is already in progress.
       */

      if ( obsId->observing )
      {
         ERROR_SET (S_detControl_BUSY, 
                    "Observation already in progress", ERROR_LOG_NOW);
         errorNumber = S_detControl_BUSY;
         return (errorNumber);
      }

      /* Reset to zero some ao control data */

      obsId->coaddCounter = 0;
      obsId->dhsCounter = 0;
      obsId->saveAoCbCounter = 0;
      obsId->saveFgCbCounter = 0;
      obsId->averageRms = 0.0;
      obsId->averageMean = 0.0;
      obsId->averageFlux = 0.0;
      obsId->updateFgScale = FALSE;
      obsId->updateAoScale = FALSE;
#ifdef DEBUG
      printf ( "detControl : updateFgScale = %d\n" , obsId->updateFgScale );
      printf ( "detControl : updateAoScale = %d\n" , obsId->updateAoScale );
#endif

      ptrPwfs2->interval = 0.0 ;
#ifdef DEBUG
      printf ( "ptrPwfs2->interval=%f\n" , ptrPwfs2->interval ) ;
#endif

      for ( i = 0 ; i < 5 ; i ++ )   /* reset the butterworth filter */
          for ( j = 0 ; j < 3 ; j ++ )
              sampleData[i][j] = 0.0;    

      if ( obsId->aoCtrlId != NULL )
      {
         obsId->aoCtrlId->coaddCounter = 0;
         obsId->aoCtrlId->focusCounter = 0;
         obsId->aoCtrlId->previousFocus = 0.0;
      }

      /* Init the position of the circular buffers */

      (void) aoCbImZero ( obsId->aoCbImId );
      obsId->aoCbImId->position = 0 ;
      obsId->aoCbImId->counter = 0 ;
      obsId->aoCbImId->processingMode = obsId->sigMode ;

      (void) aoCbAoCtrlZero ( obsId->aoCbAoCtrlId );
      obsId->aoCbAoCtrlId->position = 0 ;
      obsId->aoCbAoCtrlId->counter = 0 ;
      obsId->aoCbAoCtrlId->processingMode = obsId->sigMode ;

      (void) aoCbFgCtrlZero ( obsId->aoCbFgCtrlId );
      obsId->aoCbFgCtrlId->position = 0 ;
      obsId->aoCbFgCtrlId->counter = 0 ;
      obsId->aoCbFgCtrlId->processingMode = obsId->sigMode ;

      /* Obtain the attributes */

      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                             (char *) &nframe);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                             (char *) &exposure);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                             (char *) &outOptions);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pDataLabel);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4,
                             (char *) &dhsOutOptions);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, pFilePath);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, pOutFileName);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, pSimFileName);

      /* Check the number of frames is sensible */

      if ( (nframe <= 0) && (nframe != -1) )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
                     "Invalid number of frames, %ld",
                     ERROR_LOG_NOW, nframe);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /*
       * Check the exposure time is sensible. The SDSU controller measures
       * exposures in units of 81.92 microseconds and stores the exposure in a
       * 32 bit integer, so the upper limit in seconds is 2**32 * 0.000008192 =
       * 351,843 seconds
       */

      if ( (exposure < 0.0) || (exposure > 351843.0) )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
                     "Invalid exposure time, %f seconds.",
                     ERROR_LOG_NOW, exposure);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      if ( (exposure < 0.01 ) && (obsId->aoCcdId->binningFlag == FALSE ) )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
                     "Invalid exposure time %f seconds if no binning",
                     ERROR_LOG_NOW, exposure);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      obsId->exposureTime = exposure;
      obsId->aoCbImId->exposureTime = exposure;
      obsId->aoCbAoCtrlId->exposureTime = exposure;
      obsId->aoCbFgCtrlId->exposureTime = exposure;

      if (epToVxPipeWrite( NULL, (char *)(int)&exposure, 
                           obsId->pIntTimeContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set integration time SIR record");
      }

      /*
       * Reject the command, if signal processing is not initialized and
       * sigMode is not set to one of the following mode: AO_MODE_NONE
       */

      if ( ( obsId->aoCtrlId->initFlag == FALSE ) &&
           (obsId->sigMode != AO_MODE_NONE) )
      {
         ERROR_SET (S_detControl_BAD_ATTRIBUTE,
         "Signal processing not init, use the init command for the signal processing first",
         ERROR_LOG_NOW);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /* 
       * Init some parameters according to the signal processing mode
       */

      if ( (obsId->sigMode == AO_MODE_AO) ||
           (obsId->sigMode == AO_MODE_GG_AO ) ||
           (obsId->sigMode == AO_MODE_FG_FOCUS_AO ) ||
           (obsId->sigMode == AO_MODE_CLOSED_LOOP ) )
      {
         obsId->nCoaddFrames = (int)ceil(obsId->aoTime/exposure);
         MESSAGE_LOG1 ( MSG_LOG,
                        "For aO: nCoaddFrames=%d",
                        (int)(obsId->nCoaddFrames));
         obsId->aoCbAoCtrlId->averageImageNb = obsId->nCoaddFrames;
      }

      if ( obsId->sigMode == AO_MODE_TOTAL )
      {
         obsId->aoCtrlId->totalThreshold = 0.0;
         if (epToVxPipeWrite (NULL, 
                              (char *)(int)& (obsId->aoCtrlId->totalThreshold), 
                              obsId->pAoTotalContext) == ERROR)
         {
            ERROR_LOG ( "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
         }
      }

      if ( obsId->sigMode == AO_MODE_CLOSED_LOOP ) 
      {
         if ( obsId->ggTime == 0.0 )
            obsId->ggFrame = 0;
         else
            obsId->ggFrame = (int)ceil(obsId->ggTime/exposure);

#ifdef DEBUG
         printf ( "MODE CLOSED LOOP: FG during %d frames\n" , 
                  (int)obsId->ggFrame);
#endif
      }

      if ( ( obsId->sigMode == AO_MODE_FG_FOCUS ) ||
           ( obsId->sigMode == AO_MODE_FG_FOCUS_AO ) ||
           ( obsId->sigMode == AO_MODE_FG_FOCUS_COADD ) ||
           ( obsId->sigMode == AO_MODE_CLOSED_LOOP ) )
      {
         if ( obsId->threshRealTimeFlag == TRUE )
         {
            /* Init first threshold vector */

            if ( obsId->aoCcdId->binningFlag == FALSE )
               obsId->aoCtrlId->threshold = 
               obsId->aoCtrlId->thresholdDarkFull;
            else
               obsId->aoCtrlId->threshold = 
               obsId->aoCtrlId->thresholdDarkBin;

            for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                obsId->aoCtrlId->thresholdVect[k] = 
                obsId->aoCtrlId->threshold;

            if (epToVxPipeWrite (NULL, 
                (char *)(int)& (obsId->aoCtrlId->threshold), 
                obsId->pAoThreshContext) == ERROR)
            {
               ERROR_LOG (
               "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
            }
           
            if ( obsId->sigMode != AO_MODE_CLOSED_LOOP )
            {
               for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                   obsId->aoCbFgCtrlId->cbFgCtrlRecord[0].thresholdVect[k] =
                   obsId->aoCtrlId->threshold;
            }
            else
            {
               if ( obsId->ggFrame != 0 )
               {
                  for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                      obsId->aoCbFgCtrlId->cbFgCtrlRecord[obsId->ggFrame-1].thresholdVect[k] =
                      obsId->aoCtrlId->threshold;
               }
               else
               {
                  for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                      obsId->aoCbFgCtrlId->cbFgCtrlRecord[0].thresholdVect[k] =
                      obsId->aoCtrlId->threshold;
               }
            }

            /* Compute the total */

            obsId->aoCtrlId->totalThreshold =
            aoTotalThresholdCompute ( obsId->aoCcdId, obsId->aoCtrlId);

            obsId->methodFluxComp = AO_TOTAL_FORMULA;
            obsId->aoCtrlId->totalMethod = AO_TOTAL_FORMULA;

            if (epToVxPipeWrite (NULL,
                    (char *)(int)& (obsId->aoCtrlId->totalThreshold),
                    obsId->pAoTotalContext) == ERROR)
            {
               ERROR_LOG (
                     "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
            }
         }
      }
           
      if ( obsId->sigMode == AO_MODE_CLOSED_LOOP )
      {
         if ( obsId->threshRealTimeFlag == FALSE )
         {
            if ( obsId->averageFluxFlag == TRUE )
            {
               obsId->aoCtrlId->totalThreshold = 0.0;
               if (epToVxPipeWrite (NULL, 
                             (char *)(int)& (obsId->aoCtrlId->totalThreshold), 
                             obsId->pAoTotalContext) == ERROR)
               {
                  ERROR_LOG (
                  "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
               }
            }

            if ( obsId->threshFlag == FALSE )
               obsId->nAverageDataThreshComp = 0;
            else
            {
               if ( obsId->aoCcdId->binningFlag == FALSE )
                  obsId->aoCtrlId->threshold = 
                  obsId->aoCtrlId->thresholdDarkFull;
               else
                  obsId->aoCtrlId->threshold = 
                  obsId->aoCtrlId->thresholdDarkBin;

               for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                   obsId->aoCtrlId->thresholdVect[k] = 
                   obsId->aoCtrlId->threshold;

               if (epToVxPipeWrite (NULL, 
                   (char *)(int)& (obsId->aoCtrlId->threshold), 
                   obsId->pAoThreshContext) == ERROR)
               {
                  ERROR_LOG (
                  "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
               }
            }
         }


         if ( obsId->saveCbFgCtrlClosedLoopTime == 0.0 )
            obsId->saveCbFgCtrlClosedLoop = FALSE;
         else
            obsId->saveCbFgCtrlClosedLoopFrame = 
            (int)ceil((obsId->saveCbFgCtrlClosedLoopTime*60.0)/exposure);

#ifdef DEBUG
         printf ( "MODE CLOSED LOOP: Save FG CB every %d frames\n" , 
                  (int)obsId->saveCbFgCtrlClosedLoopFrame);
#endif

         if ( obsId->saveCbAoCtrlClosedLoopTime == 0.0 )
            obsId->saveCbAoCtrlClosedLoop = FALSE;
         else
            obsId->saveCbAoCtrlClosedLoopFrame =
            (int)ceil((obsId->saveCbAoCtrlClosedLoopTime*60.0)/exposure);

#ifdef DEBUG
         printf ( "MODE CLOSED LOOP: Save aO CB every %d frames\n" ,
                  (int)obsId->saveCbAoCtrlClosedLoopFrame);
#endif

      }

      /* Check if the dhs is connected in case of outOptions = 1 (dhs */

      if ( (outOptions == 1) && (detDhsConnected == NOT_CONNECTED) )
      {
         ERROR_SET (S_detControl_BAD_ATTRIBUTE,
                    "Output option: DHS, but DHS not connected",
                    ERROR_LOG_NOW);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /* Check if the number of frames fits with the dhs output */
      /* Permanent storage should be used with nframe = 1 */

      if ( (outOptions == 1) && (dhsOutOptions == 0) && (nframe != 1) )
      {
         ERROR_SET (S_detControl_BAD_ATTRIBUTE,
               "For permanent DHS storage, the number of frame should be 1",
               ERROR_LOG_NOW);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /*
       * Determine how to read the CCD
       * if outOptions = 0 -> CCD in continous mode (readMethod = 1)
       * else -> serie of 1 frame (readMethod = 0)
       */

      if ( outOptions == 0 )
      {
         sdsuId->readMethod = 1;
      }
      else
      {
         sdsuId->readMethod = 0;
      }

#ifdef DEBUG
      printf ( "outOptions = %d, readMethod = %d\n" , (int)outOptions,
               (int)sdsuId->readMethod );
#endif

      /* Combine file and path name for output file name */

      detCreateFileName ( pFilePath ,
                          pOutFileName ,
                          pFullOutFileName ) ;

      /*
       * Combine the file path and file names, ignoring the path if not 
       * specified and preserving any file name set to "NONE".
       */

      if ( strcmp (pFilePath, "") == 0 )
      {
        strncpy (pFullSimFileName, pSimFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {

        if ( strcmp(pSimFileName, "NONE") == 0 )
        {
           strncpy (pFullSimFileName, pSimFileName, 
                    EPICS_MAX_BYTES_STRING_ATTRIB);
        }
        else
        {
           sprintf (pFullSimFileName, "%s/%s", pFilePath, pSimFileName );
        }
      }

      /*
       * Append the string ".fits" if it is not already present in any file 
       * name, and the name in question is not "NONE".
       */

      if ((strcmp(pFullSimFileName, "NONE") != 0) && 
          (strstr (pFullSimFileName, ".fits") == NULL))
         strncat (pFullSimFileName, ".fits", EPICS_MAX_BYTES_STRING_ATTRIB);

      /*
       * Init ccdSec, dataSec, origSec. 
       */

      obsId->xPixelsDhs = obsId->aoCcdId->xPixels;
      obsId->yPixelsDhs = obsId->aoCcdId->yPixels;

      sprintf ( obsId->dataSec , "[1:%d,1:%d]" ,
                obsId->aoCcdId->xPixels , obsId->aoCcdId->yPixels ) ;
      sprintf ( obsId->ccdSec , "[1:%d,1:%d]" ,
                obsId->aoCcdId->xPixels , obsId->aoCcdId->yPixels ) ;
      sprintf ( obsId->origSec , "[1:%d,1:%d]" ,
                obsId->aoCcdId->xPixels , obsId->aoCcdId->yPixels ) ;

      /*
       * If a request has been made to send data to the DHS, check that the 
       * DHS is available, otherwise reject the command.
       */

      if ( outOptions == 1 )
      {
         if ( ( !detDhsInitialised ) ||
              ( detDhsConnection == NULL) ||
              /* ( dhsIsConnected (detDhsConnection, &dhsErrno) 
                   != DHS_TRUE ) */ /* DOESN'T WORK */
              ( FALSE )             /* BUG WORK AROUND */)
         {
            ERROR_SET (S_detControl_BAD_ATTRIBUTE, "DHS is not available", 
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_ATTRIBUTE;
            return (errorNumber);
         }
      }

      /*
       * Set the observation in progress and observation stopped flags, 
       * init the frame counter and set the observeC CAR record to BUSY, 
       * via the "observing" record.
       */

      obsId->observing = TRUE;
      obsId->stopped = FALSE;
      obsId->nframes = 0;
      obsId->outNFrames = 0;
      observingState = menuCarstatesBUSY;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing state to BUSY.");
      }

      /* Ensure whoever is using the system knows when it is in simulation 
       * mode. 
       */

      if ( sdsuId->simulate )
      {
         MESSAGE_LOG (MSG_WARNING, "Observation started in SIMULATION MODE");
      }
      else
      {
         MESSAGE_LOG (MSG_MINDEBUG, "Observation started");
      }

      if ( outOptions == 1 )
      {
         if ( (strcmp (pDataLabel,"") != 0) && 
              (strcmp (pDataLabel,"NONE") != 0) )
         {
            MESSAGE_LOG1 (MSG_LOG, 
            "Will send data to DHS with data label provided (%s)",
            pDataLabel);
         }
         else
         {
            pLabelFromDhs = dhsBdName (detDhsConnection, &dhsErrno);
            CHECK_DHS (dhsErrno);

            if ( dhsErrno != DHS_S_SUCCESS )
            {
               MESSAGE_LOG1 (MSG_WARNING,
               "WARNING: Failed to get data label from DHS (dhsErrno=%d) - using NOLABEL",
               dhsErrno);
               strcpy (pDataLabel, "NOLABEL");
            }
            else
            {
               sprintf (pDataLabel, "%s.0.0", pLabelFromDhs);

               MESSAGE_LOG1 (MSG_LOG, 
                  "Successfully obtained data label from DHS (%s)",
                  pDataLabel);
            }
         }
      }
      else if ( outOptions == 2 )
      {
         MESSAGE_LOG1 (MSG_LOG, "Will save data to directly to file \"%s\"",
            pFullOutFileName);
      }

      /* Load up the observation ID structure with the new information. */

      obsId->outOptions = (int) outOptions;
      obsId->dhsOutOptions = (int) dhsOutOptions;
      strncpy( obsId->pDataLabel, pDataLabel, EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy( obsId->pOutFileName, pFullOutFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB*2 );
      strncpy( obsId->pSimFileName, pFullSimFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB*2 );

      if (epToVxPipeWrite( NULL, obsId->pDataLabel, obsId->pDataLabelContext ) 
          == ERROR)
      {
         ERROR_LOG ("Failed to init Data label SIR record");
      }

      /*
       * Before starting the observation, query some parameters from the SDSU 
       * controller.
       * Default values for the parameters are assumed in simulation mode or 
       * if the parameters could not be obtained.
       */

      defOutputs = 4;            /* Default number of outputs.   */
      readoutTimeout = 2.0;      /* Readout timeout in seconds.   */

      if ( sdsuId->simulate )
      {
         obsId->outputs = defOutputs;
         obsId->exposed = obsId->exposedRQ;
      }
      else
      {
         if ( nframe == -1 )
         {
            MESSAGE_LOG1 (MSG_LOG,
            "Setting up for an infinite series of exposures of %f seconds each",
            exposure);

            nframePerDataset = nframe ;

            /* BUG WORK AROUND: THE SDSU CONTROLLER RETURNS FRAME COUNT=1
             * WHEN ASKED FOR AN INFINITE
             * NUMBER OF FRAMES, WHICH DETCONTROL THEN ASSUMES MEANS THE
             * LAST FRAME HAS BEEN RECEIVED.
             * UNTIL THE SDSU CODE IS FIXED, SET A FLAG TO INDICATE THE FRAME
             * COUNT IS INFINITE.
             */

            if ( sdsuId->readMethod == 1 )
            {
               nframe = 0;
               obsId->continuous = TRUE;
               obsId->totalFrames = nframe;
               sdsuNframe = (uint32) nframe;
            }
            else
            {
               nframe = 1;
               obsId->continuous = TRUE;
               obsId->totalFrames = 0;
               sdsuNframe = (uint32) nframe;
            }
         }
         else if ( nframe == 1 )
         {
            MESSAGE_LOG1 (MSG_LOG,
                    "Setting up for one exposure of %f seconds", exposure);

            nframePerDataset = nframe ;

            /* BUG WORK AROUND */
            obsId->continuous = FALSE;

            obsId->totalFrames = nframe;

            sdsuNframe = (uint32) nframe;
         }
         else
         {
            MESSAGE_LOG2 (MSG_LOG,
            "Setting up for %ld exposures of %f seconds each",
            nframe, exposure);

            nframePerDataset = 1 ;

            /* BUG WORK AROUND */
            if ( sdsuId->readMethod == 1)
            {
               obsId->continuous = FALSE;
               obsId->totalFrames = nframe;

               sdsuNframe = (uint32) 0;  /* Modif 01 nov 1999 - cb */
            }
            else
            {
               obsId->continuous = FALSE;
               obsId->totalFrames = nframe;

               sdsuNframe = (uint32) 1;  
            }
         }

         /* Set the number of frames by writing to the T_NFRAME parameter in the
          * timing DSP Also define the total number of frames in the observation
          * context structure. */

#ifdef DEBUG
         printf ("detExposure: Setting T_NFRAME parameter to %lu\n",
                 sdsuNframe);
#endif /* DEBUG */

         if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_NFRAME", sdsuNframe )
              == ERROR )
         {
            ERROR_LOG ("Error setting number of frames parameter");
            errorNumber = S_detControl_SDSU_ERROR;
         }

         /* Set the exposure time by writing to the T_EXP_TIM parameter in
          * the timing DSP
          */

         expTim = (uint32) (exposure / SDSU_EXPOSURE_UNIT);

#ifdef DEBUG
         printf ("detExposure: Setting T_EXP_TIM parameter to %lu\n", expTim);
#endif /* DEBUG */

         if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", expTim )
              == ERROR )
         {
            ERROR_LOG ("Error setting exposure time parameter");
            errorNumber = S_detControl_SDSU_ERROR;
         }

         /* Update the requested total exposure time in the observation context
          * structure.
          */

         obsId->exposedRQ = 1 * exposure;
         sdsuId->exposureTicks = (int) (exposure * sysClkRateGet());

         /*
          * Set up the the total integration time requested
          */

         if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposedRQ), 
                              obsId->pExposedRQContext ) == ERROR)
         {
            ERROR_LOG (
               "Failed to init total integration time requested SIR record");
         }
         /*
          * Set up the requested and actual number of exposure/dataset.
          * Always 1 for the moment
          */

         nexp = 1 ;
         if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpRQContext ) 
             == ERROR)
         {
            ERROR_LOG ("Failed to init Number exp/dataset SIR record");
         }
         if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpContext ) 
             == ERROR)
         {
            ERROR_LOG ("Failed to init Number exp/dataset SIR record");
         }

         /*
          * Set up the the number of frames per dataset
          */

         if (epToVxPipeWrite( NULL, (char *)(int)&nframePerDataset, 
             obsId->pNFramesContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set number of frames SIR record");
         }

         /*
          * Init the butterworth filter for probe arm guiding
          */

         obsId->cutoffFrequency = 
         obsId->rateSamplingFrequency / obsId->exposureTime;

         if ( detComputeCoeffButterworth ( obsId->exposureTime, 
                                           obsId->cutoffFrequency, 
                                           coeffData ) == ERROR )
         {
            ERROR_LOG ( "Failed to init coefficients of butterworth filter");
         }

         /* 
          * Update the dhsQlRate
          */

         if ( obsId->continuous == TRUE )
         {
            if ( obsId->exposureTime <= 1.0 )
               obsId->dhsQlRate = (int)(1.0 / obsId->exposureTime);
            else
               obsId->dhsQlRate = 1;
         }
         else
         {
            if ( obsId->totalFrames == 1 )
               obsId->dhsQlRate = 1;
            else
            {
               if ( obsId->exposureTime <= 1.0 )
               {
                  if ( obsId->totalFrames > (int)(1.0 / obsId->exposureTime) )
                     obsId->dhsQlRate = (int)(1.0 / obsId->exposureTime);
                  else
                     obsId->dhsQlRate = obsId->totalFrames;
               }
               else
                  obsId->dhsQlRate = 1;
            }
         }

#ifdef DEBUG
         printf ( "dhsQlRate = %d\n", obsId->dhsQlRate );
#endif

         /*
          * BUG WORK AROUND: Before attempting to query parameters from the 
          * timing board, send an ABT command to the VME board. This should 
          * ensure the board is not in a state where it thinks it is still 
          * waiting for data from a previous observation. The parameter reads 
          * from the timing board will fail in this circumstance.
          */

         if (sdsuPrimitive (sdsuId, "ABT", SDSU_IDENT_VME, NULL, NULL) == ERROR)
         {
            ERROR_LOG ("ABT command failed prior to starting observation");
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", 
                            &(obsId->outputs)) == ERROR)
         {
           ERROR_LOG ("Failed to query number of outputs from SDSU controller");
           MESSAGE_LOG1 (MSG_WARNING, 
                         "Assuming number of outputs is %d", defOutputs);
           obsId->outputs = defOutputs;
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", &expTim) 
             == ERROR)
         {
            ERROR_LOG ("Failed to query exposure time from SDSU controller");
            MESSAGE_LOG (MSG_WARNING, 
            "Assuming exposure time is 1 second per frame");
            if (obsId->totalFrames > 0)
            {
               obsId->exposed = (double) obsId->totalFrames;
            }
            else
            {
               obsId->exposed = 1.0;
            }
         }
         else if (expTim == 0)
         {
            MESSAGE_LOG (MSG_WARNING,
            "Zero exp time obtained from SDSU controller. Assuming minimum");
            if (obsId->totalFrames > 0)
            {
               obsId->exposed = 
               (double) obsId->totalFrames * (double) SDSU_EXPOSURE_UNIT;
            }
            else
            {
               obsId->exposed = (double) SDSU_EXPOSURE_UNIT;
            }
         }
         else
         {
            if (obsId->totalFrames > 0)
            {
               obsId->exposed = (double) obsId->totalFrames *
                                (double) (expTim * SDSU_EXPOSURE_UNIT);
            }
            else
            {
               obsId->exposed = (double) (expTim * SDSU_EXPOSURE_UNIT);
            }
            sdsuId->exposureTicks = 
            (int) (expTim * SDSU_EXPOSURE_UNIT * sysClkRateGet());
#ifdef DEBUG
            printf ( "detControl: T_EXP_TIM=%d, exposureTicks=%d\n", 
                     (int)(expTim) , sdsuId->exposureTicks) ;
#endif
         }
      }

      /* Set the measuring state */
      
      if ( obsId->totalFrames != 1 )
         measuringState = 1;           /* case continuous or sevaral frames */
      else
         measuringState = 0;           /* case 1 frame only */

      if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                           obsId->pDetMeasuringContext) == ERROR)
      {
         ERROR_LOG ("Failed to set measuring state to 1 or 0.");
      }

      /*
       * Set up the the total integration time
       */

      if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposed), 
                           obsId->pExposedContext ) == ERROR)
      {
         ERROR_LOG ("Failed to init total integration time SIR record");
      }

      /* Get a timestamp to record the time at which the observation started. */

      if ( timeNow (&(obsId->rawtStart)) != OK )
      {
#ifdef DEBUG
         ERROR_SET (0, "Failed to get time stamp at observation start", 
                    ERROR_LOG_NOW);
#endif
         obsId->rawtStart = (double)AO_TIME_NOW_ERROR ;
      }

#ifdef DEBUG
      printf ("detObserveStart: Time at observation start: %f seconds.\n", 
              obsId->rawtStart);
#endif


      /*
       * Start the readout process. The observation should now start in a 
       * parallel thread.
       */

      sdsuId->frameTimeout = (int) (obsId->exposureTime + 30) * sysClkRateGet();

      MESSAGE_LOG2 (MSG_MINDEBUG, 
                    "Starting exposure of %f seconds in %d frames...",
                    obsId->exposed, obsId->totalFrames);
      if ( obsId->totalFrames > 1 )
      {
         if (sdsuSimpleReadoutStart (sdsuId, 0, (void *) obsId) == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = menuCarstatesERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;
      
            measuringState = 0;
            if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                                 obsId->pDetMeasuringContext) == ERROR)
            {
               ERROR_LOG ("Failed to set measuring state to 0.");
            }

            return (errorNumber);
         }
      }
      else
      {
         if (sdsuSimpleReadoutStart (sdsuId, obsId->totalFrames, (void *) obsId) 
             == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = menuCarstatesERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;

            measuringState = 0;
            if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                                 obsId->pDetMeasuringContext) == ERROR)
            {
               ERROR_LOG ("Failed to set measuring state to 0.");
            }

            return (errorNumber);
         }
      }

      /*
       * To maximise the efficiency, the following code runs in parallel with 
       * the observation. If the observation happens to finish before this code
       * completes (which is unlikely) it will wait for the binary semaphore 
       * which is given at the end of this function.
       */

      /*
       * Start an alarm timer which will trigger if the frame sync callback 
       * never runs. Set the delay time to the readout timeout plus the largest 
       * frame exposure time obtained earlier.
       *
       * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
       */

      if ( sdsuId->simulate )
      {
         if ( obsId->exposed >= obsId->exposedRQ )
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposed;
            }
         }
         else
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposedRQ/(double) obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposedRQ;
            }
         }

         if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, 
              detObserveTimeout, (int) obsId) == ERROR )
         {
            ERROR_SET (0, "Failed to set alarm timer", ERROR_LOG_NOW);
            obsId->observing = FALSE;
            observingState = menuCarstatesERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext)
               == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            measuringState = 0;
            if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                                 obsId->pDetMeasuringContext)
               == ERROR)
            {
               ERROR_LOG ("Also failed to set measuring state to 0.");
            }
            errorNumber = S_detControl_INTERNAL;
            return (errorNumber);
         }
      }

      /*
       * Convert the start time into International Atomic Time (TAI).
       * This time will be used to generate the MJD-OBS field in the FITS 
       * header. There is currently no internationally agreed standard 
       * defining the timescale for MJD-OBS. TAI is used here because it is 
       * a sensible choice and, in fact, was once specified in a draft 
       * standard in July 1996 that was subsequently withdrawn.
       * Whatever timescale is specified here, it is important that it be
       * continuous across a leap second. Suitable alternatives are
       * Terrestrial Time (TT) and Universal Time 1 (UT1). UTC is NOT suitable.
       * NOTE: At time of writing Pat Wallace is checking this with the FITS 
       * committee.
       */

      if (timeThenD (obsId->rawtStart, TAI, &timeTAI) != OK)
      {
         ERROR_SET (0,  "Failed to convert time stamp to TAI", ERROR_LOG_NOW);
      }

      /*
       * Get the current tracking frame, as read from the TCS.
       * (Default values will be supplied if the TCS is not available).
       */

      wfsGetTrackFrame (&trackFrame, &(trackEquinox.type),
         &(trackEquinox.year), &trackWavelength,
         &trackRA, &trackDec, &(trackEpoch.type), &(trackEpoch.year));

      obsId->equinox = trackEquinox.year;
      obsId->epoch   = trackEpoch.year;
      obsId->RA      = trackRA;
      obsId->Dec     = trackDec;


      /*
       * If sufficient WCS calibration points are available, define the WCS
       * information for this observation.
       */

      if ( obsId->nWcsPoints >= 3 )
      {
         /*
          * Define the (i,j) to (X,Y) transformation.
          * N.B. For efficiency, this need only be done once, each time the 
          * detector binning is changed. CHANGE THIS EVENTUALLY.
          *
          * First check if any binning or windowing of the pixels on the 
          * detector has been defined.
          */

         if ( ( obsId->aoCcdId != NULL ) &&
              ( ( obsId->aoCcdId->xStart != 0 ) ||
                ( obsId->aoCcdId->yStart != 0 ) ||
                ( obsId->aoCcdId->xBin != 1 ) ||
                ( obsId->aoCcdId->yBin != 1 )
              )
            )
         {
            /*
             * There has been some binning and windowing. The original 
             * calibration is assumed to have been made on a full frame of 
             * data without binning, so the calibration points need to be 
             * transformed.
             */

            for (p=0; p<obsId->nWcsPoints; p++)
            {
               obsId->detij[p][0] =
               ((obsId->pixij[p][0] - 0.5 - 
                 (double) obsId->aoCcdId->xStart) /
                (double) obsId->aoCcdId->xBin) + 0.5;

               obsId->detij[p][1] =
               ((obsId->pixij[p][1] - 0.5 - 
                (double) obsId->aoCcdId->yStart) /
                (double) obsId->aoCcdId->yBin) + 0.5;
            }

            /*
             * Calculate the best fit to the focal plane X,Y coordinates 
             * against binned detector coordinates.
             */

            wcsStatus = astFitij ( obsId->nWcsPoints, obsId->fpxy, 
               obsId->detij, obsId->cij,
               &pixis, &pixjs, &perp, &orient );
         }
         else
         {
            /*
             * No windowing or binning have been used.
             * Calculate the best fit to the focal plane X,Y coordinates 
             * against the original full frame, unbinned pixel coordinates.
             */

            wcsStatus = astFitij ( obsId->nWcsPoints, obsId->fpxy, 
               obsId->pixij, obsId->cij,
               &pixis, &pixjs, &perp, &orient );
         }

         if ( wcsStatus != 0 )
         {
            ERROR_SET1 (S_detControl_WCS_ERROR,
               "Failed to define (i,j) to (X,Y) transformation. Status=%d",
               ERROR_LOG_NOW, wcsStatus);
         }

#ifdef DEBUG
         printf ("Best fit scale is %f X units per i pixel and "
                 "%f Y units per j pixel\n", pixis, pixjs);
         printf ("i/j non-perpendicularity is %f radians.\n", perp);
         printf ("i/j is rotated by %f radians with respect to x/y axis.\n",
                 orient);
         printf ("Cij matrix contains %f %f %f %f %f %f\n", obsId->cij[0], 
                 obsId->cij[1], obsId->cij[2], obsId->cij[3], obsId->cij[4], 
                 obsId->cij[5]);
#endif

         /*
          * Obtain the current TCS context from the locally stored copy.
          * This assumes that a TCS context has been obtained elsewhere and 
          * stored using astSetCtx(), as described in section 3 of document 
          * tcs_ptw_008.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astGetctx (&ctx);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR, 
                  "Failed to get current WCS context. Status=%d",
                  ERROR_LOG_NOW, wcsStatus);
            }
         }

         /*
          * Set the chop state to which the WCS coordinate information refers.
          * NOTE: THE ACTUAL CHOP STATE NEEDS TO BE OBTAINED FROM THE PARAMETER
          * GIVEN TO THE "SET CHOP STATE" COMMAND.
          */

         chopState = 0;         /* 0 means chop state A. */

         /*
          * Extract the current focal plane to sky WCS transformation from the 
          * TCS context.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astCtx2tr (ctx, trackFrame, trackEquinox, 
               trackWavelength, chopState, &wcs, &rawTimeWcs);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR,
               "Failed to get focal plane to sky WCS transformation. Status=%d",
               ERROR_LOG_NOW, wcsStatus);
            }
         }

#ifdef DEBUG
         printf (
         "WCS information extracted from TCS context is valid at time %f\n",
         rawTimeWcs);
#endif

         /*
          * Combine the (i,j) to (x,y) model, cij, and (x,y) to (RA,Dec) model, 
          * wcs, into a single (i,j) to (RA,Dec) model, wcsij.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astXtndtr ( obsId->cij, wcs, &wcsij );
            if (wcsStatus != 0)
            {
             ERROR_SET1 (S_detControl_WCS_ERROR,
             "Failed to combine i-j to x-y and x-y to RA-Dec models. Status=%d",
             ERROR_LOG_NOW, wcsStatus);
            }
         }

          /*
           * Calculate the World Cooordinate System header values, expressed in 
           * terms of standard FITS header items.
           */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astFITSv (wcsij, trackFrame, trackEquinox, timeTAI,
               obsId->ctype1, &(obsId->crpix1), &(obsId->crval1),
               obsId->ctype2, &(obsId->crpix2), &(obsId->crval2),
               &(obsId->cd1_1), &(obsId->cd1_2), &(obsId->cd2_1), 
               &(obsId->cd2_2), obsId->radecsys, &(obsId->equinox), 
               &(obsId->mjdobs));
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR,
                  "Failed to calculate FITS standard WCS header.. Status=%d",
                  ERROR_LOG_NOW, wcsStatus);
            }
         }
         obsId->wcsStatus = wcsStatus;

#ifdef DEBUG
         printf ("World Coordinate System Header\n");
         printf ("------------------------------\n");
         printf ("wcsStatus= %d\n", obsId->wcsStatus);
         printf ("ctype1   = %s\n", obsId->ctype1);
         printf ("crpix1   = %f pixels\n", obsId->crpix1);
         printf ("crval1   = %f degrees = %f hours\n", obsId->crval1,
                 (obsId->crval1 / (double) 15.0));
         printf ("ctype2   = %s\n", obsId->ctype2);
         printf ("crpix2   = %f pixels\n", obsId->crpix2);
         printf ("crval2   = %f degrees\n", obsId->crval2);
         printf ("cd1_1    = %f\n", obsId->cd1_1);
         printf ("cd1_2    = %f\n", obsId->cd1_2);
         printf ("cd2_1    = %f\n", obsId->cd2_1);
         printf ("cd2_2    = %f\n", obsId->cd2_2);
         printf ("RA       = %f hours\n", obsId->RA);
         printf ("Dec      = %f degrees\n", obsId->Dec);
         printf ("radecsys = %s\n", obsId->radecsys);
         printf ("equinox  = %f\n", obsId->equinox);
         printf ("epoch    = %f\n", obsId->epoch);
         printf ("mjd-obs  = %f\n", obsId->mjdobs);
#endif
      }
      else
      {
         /*
          * There is insufficient information to provide WCS information.
          * The only valid item which can be added to the header is the MJD of 
          * the observation.
          */

         MESSAGE_LOG (MSG_WARNING,
        "No World Coordinate System calibration - there will be no WCS header");

         obsId->mjdobs = timeTAI;
         obsId->wcsStatus = -1;
      }

      /*
       * Convert the time stamps from Gemini raw time into Universal Time
       * and construct these into character strings.
       */

      if (timeThenC( obsId->rawtStart, UT1, 2, obsId->timeArrayStart ) != OK)
      {
         ERROR_SET (0,
            "Failed to convert time stamp at observation start to date/time",
            ERROR_LOG_NOW);
      }
      sprintf (obsId->utStartString, "%04d-%02d-%02d:%02d:%02d:%02d",
               obsId->timeArrayStart[0], obsId->timeArrayStart[1], 
               obsId->timeArrayStart[2], obsId->timeArrayStart[3], 
               obsId->timeArrayStart[4], obsId->timeArrayStart[5]);

      if (epToVxPipeWrite( NULL, (char *)obsId->utStartString, 
                           obsId->pUTstartContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set UT at start of observation SIR record");
      }

#ifdef DEBUG
      printf ( "obsId->utStartString = %s\n" , obsId->utStartString ) ;
#endif


      /*
       * If the DHS is being used then create a dataset to hold the 
       * unscrambled data. Otherwise allocate a buffer directly.
       */

      if ( obsId->outOptions == 1 )
      {

         MESSAGE_LOG (MSG_MINDEBUG, "Creating DHS dataset...");

         /* Set up for the quick look */

         contrib[0] = pDetDhsClientName;  
                                      /* a global variable, set in detDhsInit */

         qlStreams[0] = "pwfs2Science"; 
                                /* THIS IS A FUDGE. DEFINE IN setDhs command. */

         wfsGetTelName ( telName ) ;
         printf ( "telName=%s\n" , telName ) ;

         /* NOTE: Lifetime should be definable
          * PERMANENT for permanent data (e.g. calibrations)
          * TRANSIENT for display only (e.g. acquisition camera in continuous 
          * mode) (see ICD 3).
          */

         /*if ( obsId->totalFrames == 1 )*/             /* only one exposure */
         if ( dhsOutOptions == 0 )
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_PERMANENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_PERMANENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         else           /* either continuous mode with totalFrames = 0 or > 1 */
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_TRANSIENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_TRANSIENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_CONTRIB, obsId->pDataLabel, 
            1, contrib, &dhsErrno);
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_QLSTREAM, obsId->pDataLabel, 
            1, qlStreams, &dhsErrno);
         CHECK_DHS (dhsErrno);

         /* Create the DHS dataset and add the default attributes. */

         obsId->dhsDataset = dhsBdDsNew (&dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataset, "instrument", DHS_DT_STRING, 
                            0, NULL, pDetDhsClientName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "telescope", DHS_DT_STRING, 
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "observatory", DHS_DT_STRING,
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create dataset (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }

         /* Create a frame to contain the data and add the frame header info. */

         dims[0] = 2;
         axisSize[0] = obsId->xPixelsDhs;  
         axisSize[1] = obsId->yPixelsDhs;
         origin[0] = 1;  
         origin[1] = 1;

         dhsErrno = DHS_S_SUCCESS;    
         obsId->dhsDataFrame = 
         dhsBdFrameNew (obsId->dhsDataset, "dataArray", 0, DHS_DT_FLOAT,
                        2, axisSize, (const void **) &(obsId->pCurFrame), 
                        &dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataFrame, "dataType", DHS_DT_STRING, 
                            0, NULL, "Intensity", &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "bunit", DHS_DT_STRING, 
                            0, NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "units", DHS_DT_STRING, 
                            0, NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origin", DHS_DT_INT32, 
                            1, dims, origin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisSize", DHS_DT_INT32, 
                            1, dims, axisSize, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisLabel", DHS_DT_STRING, 
                            1, dims, axisLabel, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "obstype", DHS_DT_STRING, 0,
                            NULL, obsId->pObsType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "exptime", DHS_DT_DOUBLE, 0,
                            NULL, obsId->exposureTime, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "darktime", DHS_DT_DOUBLE, 0,
                            NULL, obsId->exposureTime, &dhsErrno);
            CHECK_DHS (dhsErrno);


            /* World Coordinate System attributes */

            if ( wcsStatus == 0 )
            {
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype1", DHS_DT_STRING,
                               0, NULL, obsId->ctype1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix1Float = (float)(obsId->crpix1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX1", DHS_DT_FLOAT, 0,
                               NULL, crpix1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL1", DHS_DT_DOUBLE, 0,
                               NULL, obsId->crval1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype2", DHS_DT_STRING, 0,
                               NULL, obsId->ctype2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix2Float = (float)(obsId->crpix2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX2", DHS_DT_FLOAT, 0,
                               NULL, crpix2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL2", DHS_DT_DOUBLE, 0,
                               NULL, obsId->crval2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_1Float = (float)(obsId->cd1_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_1", DHS_DT_FLOAT, 0,
                               NULL, cd1_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_2Float = (float)(obsId->cd1_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_2", DHS_DT_FLOAT, 0,
                               NULL, cd1_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_1Float = (float)(obsId->cd2_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_1", DHS_DT_FLOAT, 0,
                               NULL, cd2_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_2Float = (float)(obsId->cd2_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_2", DHS_DT_FLOAT, 0,
                               NULL, cd2_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
            }

            sprintf ( raString , "%f" , obsId->RA ) ;
            sprintf ( decString , "%f" , obsId->Dec ) ;

            dhsBdAttribAdd (obsId->dhsDataFrame, "RA", DHS_DT_STRING, 0, NULL,
                            raString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "DEC", DHS_DT_STRING, 0, NULL,
                            decString, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "equinox", DHS_DT_DOUBLE, 0,
                            NULL, obsId->equinox, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "epoch", DHS_DT_DOUBLE, 0,
                            NULL, obsId->epoch, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "mjd-obs", DHS_DT_DOUBLE, 
                            0, NULL, &(obsId->mjdobs), &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "xbin", DHS_DT_INT32,
                            0, NULL, obsId->aoCcdId->xBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ybin", DHS_DT_INT32,
                            0, NULL, obsId->aoCcdId->yBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "datasec", DHS_DT_STRING,
                            0, NULL, obsId->dataSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ccdsec", DHS_DT_STRING,
                            0, NULL, obsId->ccdSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origsec", DHS_DT_STRING,
                            0, NULL, obsId->origSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "utstart", DHS_DT_STRING,
                            0, NULL, obsId->utStartString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "dettype", DHS_DT_STRING,
                            0, NULL, obsId->detType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "detid", DHS_DT_STRING,
                            0, NULL, obsId->detId, &dhsErrno);

            CHECK_DHS (dhsErrno);
         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create new data frame (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }
      }
      else
      {
         /* The DHS is not being used. */

      }

      /*
       * Give the binary semaphore, which will allow the observation thread to
       * process the data.
       */

      semGive (obsId->syncSem);

#ifdef DEBUG
      printf ("detObserveStart: START directive finished.\n");
#endif /* DEBUG */

   }

   return (errorNumber);
}
/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detStop
 *
 *   INVOCATION:
 *   detStop (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Current observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detStop command
 *
 *   DESCRIPTION:
 *   This function stops an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

uint32 detStop
   (
   CAD_CMD_CONTEXT   cadCmdContext,  /* CAD command context structure.        */
   int               commandNumber,  /* Command number.                       */
   SDSU_ID           sdsuId,         /* SDSU context structure.               */
   OBS_ID            obsId           /* Observation context structure.        */
   )
{
   uint32         errorNumber;       /* Error number reported by task.        */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Observation context not initialised", ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * This function should only be called when an observation is in progress.
    */

   if ( !obsId->observing )
   {
      /*ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;*/

      MESSAGE_LOG (MSG_LOG, "Observation not in progress");
      errorNumber = 0;

      return (errorNumber);
   }

   MESSAGE_LOG (MSG_LOG, "Stopping observation.");

   /*
    * Cancel any observation timer.
    *
    * THIS IS NOW ONLY DONE IN SIMULATION MODE.
    */

   if ( sdsuId->simulate )
   {
      if ( obsId->timeId != NULL )
      {
         if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
         {
            ERROR_LOG ("Failed to cancel observation timer");
         }
      }
   }

   /*
    * Stop the observation prematurely by setting the obsId->stopped flag.
    * The next time a frame of data appears it will be treated as the last one.
    */

   obsId->stopped = TRUE;
   /* add 27 sept 99 for slow stop pb */
   printf ( "detStop(): obsId->stopped=TRUE\n" );

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detCreateFileName
 *
 *   INVOCATION:
 *   detCreateFileName (pFilePath, pOutFileName, pFullOutFileName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFilePath        (char *)  Pointer to the file path name
 *   (>) pOutFileName     (char *)  Pointer to the output file name
 *   (<) pFullOutFileName (char *)  Pointer to the combined path and file name
 *
 *   FUNCTION VALUE:
 *   (uint32)   always OK
 *
 *   PURPOSE:
 *   Combine path and file name
 *
 *   DESCRIPTION:
 *   Combine path and file name and cancel the .fits at the end if this
 *   one exists
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detCreateFileName
   (
   char *   pFilePath,         /* Pointer to the file path name               */
   char *   pOutFileName,      /* Pointer to the output file name             */
   char *   pFullOutFileName   /* Pointer to the combined path and file name  */
                               /* Size of path and file name is               */
                               /* EPICS_MAX_BYTES_STRING_ATTRIB + 1           */
                               /* Size of full file name is                   */
                               /* 2*(EPICS_MAX_BYTES_STRING_ATTRIB + 1)       */
   )
{
   char     firstPartOutFileName [ EPICS_MAX_BYTES_STRING_ATTRIB + 1 ] ;
   char     lastCharOutFileName [ EPICS_MAX_BYTES_STRING_ATTRIB + 1 ] ;
   int      sizeOutFileName ;
   int      sizeFits ;
   int      i, j ;

   sizeFits = strlen ( ".fits" ) ;

   /* Check if pOutFileName contains a string */

   if ( strcmp ( pOutFileName, "" ) == 0 )
   {
      /* Default file name hrwfs.fits */

      if ( strcmp ( pFilePath, "" ) == 0 )
         strcpy ( pFullOutFileName, "pwfs2" ) ;
      else
         sprintf ( pFullOutFileName, "%s/pwfs2" , pFilePath ) ;

      return ( OK ) ;
   }

   /* Check if pOutFileName contains the string .fits */

   if ( strstr ( pOutFileName, ".fits" ) != NULL )
   {
      sizeOutFileName = strlen ( pOutFileName ) ;

      if ( sizeOutFileName < sizeFits )
         strncpy ( firstPartOutFileName , pOutFileName ,
                   EPICS_MAX_BYTES_STRING_ATTRIB ) ;
      else
      {
         /* Check if the last 5 char are .fits */
         i = 0 ;
         for ( j = sizeOutFileName - sizeFits ; j < sizeOutFileName ; j ++ )
         {
             lastCharOutFileName[i] = pOutFileName[j];
             i ++ ;
         }
         lastCharOutFileName [i] = '\0' ;

         if ( strcmp ( lastCharOutFileName , ".fits" ) == 0 )
         {
            /* Read the first part of pOutFileName witout .fits */
            for ( j = 0 ; j < sizeOutFileName - sizeFits ; j ++ )
            {
                firstPartOutFileName[j] = pOutFileName[j];
            }
            firstPartOutFileName [j] = '\0' ;
         }
         else
         {
            strncpy ( firstPartOutFileName , pOutFileName ,
                      EPICS_MAX_BYTES_STRING_ATTRIB ) ;
         }

      }
   }
   else
   {
      strncpy ( firstPartOutFileName , pOutFileName ,
                EPICS_MAX_BYTES_STRING_ATTRIB ) ;
   }

   /* Now combine firstPartOutFileName and pFilePath */

   if ( strcmp ( pFilePath , "" ) == 0 )
      strncpy ( pFullOutFileName, firstPartOutFileName,
                EPICS_MAX_BYTES_STRING_ATTRIB ) ;
   else
      sprintf ( pFullOutFileName, "%s/%s" , pFilePath , firstPartOutFileName ) ;

   return ( OK ) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detAbort
 *
 *   INVOCATION:
 *   detAbort (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Current observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detAbort command
 *
 *   DESCRIPTION:
 *   This function aborts an observation. It will send an abort to the SDSU
 *   controller even if an observation appears not to be taking place.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detAbort(
   CAD_CMD_CONTEXT   cadCmdContext, /* CAD command context structure.         */
   int               commandNumber, /* Command number.                        */
   SDSU_ID           sdsuId,        /* SDSU context structure.                */
   OBS_ID            obsId          /* Observation context structure.         */
)
{
   uint32         errorNumber;      /* Error number reported by task.         */

   int            observingState;   /* Observation status (busy or idle).     */
   int            measuringState;   /* Measuring status (1 or 0).             */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * This function will normally be called when an observation is in progress.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_LOG, "Aborting observation");
   }
   else
   {
      MESSAGE_LOG (MSG_WARNING,
      "WARNING: Observation not in progress but attempting to abort anyway");
   }

   /*
    * Cancel any observation timer.
    * NOTE: THIS IS NOW ONLY DONE IN SIMULATION MODE.
    */

   if ( sdsuId->simulate )
   {
      if ( obsId->timeId != NULL )
      {
         if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
         {
            ERROR_LOG ("Failed to cancel observation timer");
         }
      }
   }

   /*
    * Abort the readout process and throw away the data.
    */

   if (sdsuReadoutAbort (sdsuId) == ERROR)
   {
      ERROR_LOG ("Failed to abort readouts");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /*
    * Reset the "observation in progress" flag and set the observeC CAR record
    * to IDLE, via the "observing" record.
    */

   if ( obsId->observing )
   {

      obsId->observing = FALSE;
      observingState = menuCarstatesIDLE;
      if (epToVxPipeWrite (NULL, (char *) &observingState,
          obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to IDLE.");
      }
      measuringState = 0;
      if (epToVxPipeWrite (NULL, (char *) &measuringState,
          obsId->pDetMeasuringContext) == ERROR)
      {
         ERROR_LOG ("Failed to set Measuring flag to 0.");
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInit
 *
 *   INVOCATION:
 *   detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, pSdsuId, 
 *            obsId, pVmeAddress, pmaxFrames)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor p2
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (!) pSdsuId       (SDSU_ID *)       Pointer to current SDSU context 
 *                                       structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *   (!) pVmeAddress   (uint32 *)        Pointer to VME address of SDSU 
 *                                       controller
 *   (!) pMaxFrames    (int *)           Pointer to max frames in data buffer
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detInit command
 *
 *   DESCRIPTION:
 *   This function initialises the SDSU controller and redownloads the DSP code.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   detSdsuIdP2   (SDSU_ID)         SDSU context structure for PWFS2
 *
 *   PRIOR REQUIREMENTS:
 *   The VME address supplied must have been previously verified to be 
 *   correct (see below).
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Memory problems can arise if an attempt is made to init the 
 *   controller at an invalid address. The address should be verified to be 
 *   correct before attempting to init the controller.
 *   Note that a zero address is used to flag simulation mode, and is 
 *   therefore acceptable.
 *-
 */

uint32 detInit
   (
   const char *    pWfsName,         /* Name of wavefront sensor.             */
   const char *    pRecordPrefix,    /* Record Name Prefix.                   */
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID *       pSdsuId,          /* Pointer to SDSU context structure.    */
   OBS_ID          obsId,            /* Observation context structure.        */
   uint32 *        pVmeAddress,      /* Pointer to VME address of SDSU        */
                                     /* controller.                           */
   int *           pMaxFrames        /* Pointer to max frames in data buffer  */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */
   long         initState;        /* Initialisation state.                    */
   long         simulate;         /* TRUE if SDSU interface is simulated.     */

   char         pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Status string.                               */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for file.                          */
   char         pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* File name.                                   */
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and file name.            */
   BOOL         limitAdrsRange;  /* Flag for limiting address range in DSP    */
                                 /* memory                                    */
   int          nPixels;         /* Total number of digitised pixels.         */
   int          newMaxFrames;    /* New maximum number of frames.             */
   long         i;               /* index                                     */
   long         offsetVect[4];   /* ADC offset vector                         */
   long         offsetFullVect[4];
                                 /* ADC offset vector - no binning            */
   long         offsetBinVect[4];/* ADC offset vector - binning               */
   uint32       tempCode;        /* Target temperature code                   */
   uint32       tempCoeff;       /* Coefficient for temperature control       */
   char         defFileName [ STRING_SIZE ] ;
                                 /* Default file name according to the site   */
   char         detContInitFileName [ STRING_SIZE ] ;
                                 /* Full Name of the detector controller      */
                                 /* init file                                 */
   
   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * If the controller does not currently have control of the hardware try and
    * get it. A failure to gain access to the hardware is not regarded as an 
    * error, since if the hardware is being shared it is normal for one of the 
    * controllers not to have access (and all the controllers may be being 
    * initialised at the same time). If a failure occurs just issue a warning.
    */

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( (obsId != NULL) && (obsId->observing) )
   {
      ERROR_SET (S_detControl_BUSY,
      "Observation in progress - abort observation and try again", 
      ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set to FALSE the temperature Flag */

   readTempReadyFlag = FALSE;

   /* Set the initialisation state to BUSY. */

   initState = menuCarstatesBUSY;
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to BUSY");
   }

   /* Set the system state to "INITIALIZING" */

   if (epToVxPipeWrite (NULL, "INITIALIZING", obsId->pStateContext) == ERROR)
   {
      ERROR_LOG ("Failed to set state to INITIALIZING");
   }

   /*
    * Obtain the VME address of the SDSU controller.
    * An address of zero signifies simulation mode.
    */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *) pVmeAddress);

#ifdef DEBUG
   printf ("detInit: VME address = %ld = %#lx\n", *pVmeAddress, *pVmeAddress);
#endif /* DEBUG */

   if ( *pVmeAddress == 0 )
   {
      simulate = TRUE;
   }
   else
   {
      simulate = FALSE;
   }

   /* 
    * Reset the signal processing 
    */

   obsId->aoCtrlId->initFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoCtrlInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->darkInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->flatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->aoIntMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->aoContMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized",
                        obsId->pAoContMatInitContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->fgContMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized",
                        obsId->pFgContMatInitContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
   }

   /*
    * If an SDSU context structure already exists, delete it.
    */

   if (*pSdsuId != NULL)
   {
      if (sdsuContextDelete (*pSdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to delete old SDSU context structure");
      }
   }

   /*
    * Now attempt to create a new context structure, remembering to call 
    * sdsuReset() immediately after sdsuContextCreate().
    */

   *pSdsuId = sdsuContextCreate (*pVmeAddress, simulate);
   if ( (*pSdsuId  == NULL) ||
        (sdsuReset (*pSdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) == ERROR)
      )
   {
      if ( simulate )
      {
         ERROR_SET (0, 
         "Error initialising SDSU controller in simulation mode",
         ERROR_LOG_NOW);
      }
      else
      {
         ERROR_SET1 (0, 
         "Error initialising SDSU controller at VME address %#lx",
         ERROR_LOG_NOW, *pVmeAddress);
      }
      errorNumber = S_detControl_SDSU_ERROR;

      /* Set the initialisation state to ERROR. */

      initState = menuCarstatesERROR;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to ERROR");
      }

      /* Set the system state to "RUNNING" even if init fails */

      if (epToVxPipeWrite (NULL, "RUNNING", obsId->pStateContext) == ERROR)
      {
         ERROR_LOG ("Failed to set state to RUNNING");
      }

      /*
       * If an error occurs while initialising the SDSU controller its health
       * must be set "BAD" because it can no longer function.
       */

      epToVxSetHealth (pRecordPrefix, "BAD");
      return (errorNumber);
   }
   else
   {

      /*
       * A new SDSU context structure has been obtained successfully.
       */

      MESSAGE_LOG1 (MSG_LOG, 
      "SDSU ID structure at %#x initialised successfully", (int) *pSdsuId);

      /* Write a new string to the SDSU initialisation status SIR record. */

      if ( simulate )
      {
         sprintf (pStatusString, "SDSU SIMULATED: ID = %-#8x", (int) *pSdsuId);
      }
      else
      {
         sprintf (pStatusString, "SDSU Initialised OK: ID = %-#8x", 
                  (int) *pSdsuId);
      }

      if (epToVxPipeWrite (NULL, pStatusString, obsId->pDetInitStatusContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write init message to SDSU status pipe.");
      }

      /* Update the global variables used to remember the SDSU contexts, as an
       * aid to engineering.
       */

      (*pSdsuId)->fastCamera = TRUE;
      detSdsuIdP2 = *pSdsuId;
   }

   /* Now obtain the path of the directory containing the DSP code. */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);

   /* Determine whether any code should be downloaded to the VME DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {
      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
                    pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_VME,
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to VME DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /* Set the initialisation state to ERROR. */

         initState = menuCarstatesERROR;
         if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext)
             == ERROR)
         {
            ERROR_LOG ("Failed to set initialisation state to ERROR");
         }

         /*
          * If an OMF file could not be downloaded the SDSU controller is in a 
          * state where it can only obey a subset of the commands and cannot 
          * make observations, so set the health to WARNING.
          */

         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Timing DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;
      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /*
          * If an OMF file could not be downloaded the SDSU controller is in 
          * a state where it can only obey a subset of the commands and cannot 
          * make observations, so set the health to WARNING.
          */

         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Utility DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /*
          * If an OMF file could not be downloaded the SDSU controller is in 
          * a state where it can only obey a subset of the commands and cannot 
          * make observations, so set the health to WARNING.
          */

         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   if ( errorNumber == 0 )
   {
      /* The default PWFS packet size should be larger when used in full 
       * frame mode. 
       */

      if (sdsuParamWrite (*pSdsuId, SDSU_IDENT_VME, "V_PSIZE", 160) == ERROR)
      {
         ERROR_LOG ("Failed to increase the PWFS packet size");
      }
   }

   /*
    * After successfully downloading new OMF code, the controller must be 
    * reinitialised by sending an "INI" command to the utility DSP and a "LDP" 
    * command to the timing DSP. If this fails the controller may not be usable,
    * so the health must be set to WARNING.
    */

   if ( errorNumber == 0 )
   {
      if (sdsuPrimitive (*pSdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
      {
         ERROR_LOG ("Failed to init UTILITY DSP with INI command");
         errorNumber = S_detControl_SDSU_ERROR;
         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
      if (sdsuPrimitive (*pSdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
      {
         ERROR_LOG ("Failed to init TIMING DSP with LDP command");
         errorNumber = S_detControl_SDSU_ERROR;
         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /*
    * Find out if a different maximum number of frames is needed.
    * (A value of zero or less means "no change").
    */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, 
                          (char *) &newMaxFrames);

   if ( newMaxFrames > 0 ) *pMaxFrames = newMaxFrames;

   /*
    * Initialize aoCcdId with the default detector geometry.
    */

   if (detReadDefaultDspCcdGeometry (*pSdsuId, obsId->aoCcdId) == ERROR)
   {
      ERROR_LOG ( "Error while init default detector geometry ");
   }

   /*
    * Set aoCcdId with the default detector geometry.
    */

   if (detSetDefaultDspCcdGeometry (*pSdsuId, obsId->aoCcdId) == ERROR)
   {
      ERROR_LOG ( "Error while setting default detector geometry ");
   }

   /*
    * Allocate a buffer capable of holding several frames of data, using the 
    * aoCcdId->xMax and aoCcdId->yMax determined above. If this fails, the 
    * controller will not be able to store data, so the health must be set 
    * WARNING.
    */

#ifdef DEBUG
   printf (
   "detInit: Creating new data buffer to hold %d frames of (%d x %d) pixels.\n",
   *pMaxFrames, obsId->aoCcdId->xMax, obsId->aoCcdId->yMax);
#endif /* DEBUG */

   nPixels = (obsId->aoCcdId->xMax) * (obsId->aoCcdId->yMax);
   if (sdsuBufferCreate (*pSdsuId, nPixels, *pMaxFrames) == ERROR)
   {
      ERROR_LOG ("Failed to create frame data buffers on initialisation");
      errorNumber = S_detControl_SDSU_ERROR;
      epToVxSetHealth( pRecordPrefix, "WARNING" );
   }
   
   /*
    * Next we init the readout process with our frame callback.
    * If this fails the SDSU controller will be unable to readout data, so the 
    * health must be set to WARNING.
    *
    * There is no packet callback in this version of the code.
    * INTERRUPTS DISABLED. SIMPLE VERSION. HRWFS RUNS AT LOWER PRIORITY.
    */
   
#ifdef DEBUG
   printf ("detInit: Starting the readout task and frame sync callback.\n");
#endif /* DEBUG */

   if (sdsuSimpleReadoutOpen (*pSdsuId, NULL, detObserveEnd, 0, TRUE) == ERROR)
   {
      ERROR_LOG ("Failed to start readout task on initialisation");
      errorNumber = S_detControl_SDSU_ERROR;
      epToVxSetHealth( pRecordPrefix, "WARNING" );
   }

   /*
    * Read the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->aoCcdId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
      for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
          offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /*
    * Set the default offsets for the PWFS2 CCD sectors
    */

   MESSAGE_LOG4 (MSG_LOG,
           "Defining new ADC offset levels: %#lx %#lx %#lx %#lx",
           offsetVect[0], offsetVect[1], offsetVect[2], offsetVect[3]);

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                      (uint32) offsetVect[0] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 0 parameter");
   }

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                      (uint32) offsetVect[1] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 1 parameter");
   }

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS2",
                      (uint32) offsetVect[2] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 2 parameter");
   }

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS3",
                      (uint32) offsetVect[3] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 3 parameter");
   }

   if (sdsuPrimitive (*pSdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ( "Failed to activate TIMING DSP parameters with LDP command");
   }

   /*
    * Update the adc sir records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[2]) ,
                        obsId->pAdc2Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc2 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[3]) ,
                        obsId->pAdc3Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc3 sad record");
      return (ERROR);
   }

   /*
    * Now init all geometry SIR records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->outputsNb) ,
                        obsId->pOutputsContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init outputs sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSize) ,
                        obsId->pDetXsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init x size sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySize) ,
                        obsId->pDetYsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ysize sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xStart) ,
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yStart) ,
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSubapNb) ,
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySubapNb) ,
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xRaster) ,
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yRaster) ,
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSpace) ,
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySpace) ,
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xBin) ,
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yBin) ,
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   /*
    * Set the default temperature 
    */

   MESSAGE_LOG2 (MSG_LOG,
                 "Defining temperature control parameters: %#lx %#lx",
                 tempCode, tempCoeff);

   if ( (sdsuParamWrite (*pSdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
         == ERROR) ||
        (sdsuParamWrite (*pSdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
         == ERROR) )
   {
      ERROR_LOG ("Error setting temperasture control parameters");
   }

   /*
    * If the error number is good after initialisation the health of
    * the controller can be restored to "GOOD".
    *
    * Also set the initialisation state to IDLE or ERROR, depending on the 
    * error number.
    */

   if ( errorNumber == 0 )
   {
      epToVxSetHealth( pRecordPrefix, "GOOD" );

      initState = menuCarstatesIDLE;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to IDLE");
      }

      /* Set to TRUE the temperature Flag */

      readTempReadyFlag = TRUE;
   }
   else
   {
      initState = menuCarstatesERROR;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to ERROR");
      }
   }

   /* Set the system state to "RUNNING" even if init fails */

   if (epToVxPipeWrite (NULL, "RUNNING", obsId->pStateContext) == ERROR)
   {
      ERROR_LOG ("Failed to set state to RUNNING");
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReset
 *
 *   INVOCATION:
 *   detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *             obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor p2
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *                                       (0=simulate)
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detReset command
 *
 *   DESCRIPTION:
 *   Reset the SDSU controller
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detReset
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record Name Prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32       errorNumber;       /* Error number reported by task.          */
   long         resetVme;          /* Flag set to reset SDSU VME interface.   */
   long         resetCtrl;         /* Flag set to reset SDSU controller.      */

   uint32       resetMask;         /* Mask specifying what to reset.          */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* Path name for file.                     */
   char         pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* File name.                              */
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                   /* Combined path name and file name.       */
   BOOL         limitAdrsRange;    /* Flag for limiting address range         */
   long         i;                 /* index                                   */
   long         offsetFullVect[4]; /* ADC offset vector - no binning.         */
   long         offsetBinVect[4];  /* ADC offset vector - binning.            */
   long         offsetVect[4];     /* ADC offset vector                       */
   uint32       tempCode;          /* Target temperature code                 */
   uint32       tempCoeff;         /* Coefficient for temperature control     */
   char         defFileName [ STRING_SIZE ] ;
                                 /* Default file name according to the site   */
   char         detContInitFileName [ STRING_SIZE ] ;
                                 /* Full Name of the detector controller      */
                                 /* init file                                 */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & resetVme);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                          (char *) & resetCtrl);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Observation context not initialised", ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again", 
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set to FLASE the temperature Flag */

   readTempReadyFlag = FALSE ;

   /*
    * Reset the signal processing
    */

   obsId->aoCtrlId->initFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoCtrlInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->darkInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->flatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->aoIntMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->aoContMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized",
                        obsId->pAoContMatInitContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
   }
   obsId->aoCtrlId->fgContMatInitFlag = FALSE;
   if (epToVxPipeWrite (NULL, "Not initialized",
                        obsId->pFgContMatInitContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
   }

   /*
    * Reset the SDSU hardware.
    */

   MESSAGE_LOG2 (MSG_LOG, 
   "About to %s SDSU VME interface and %s SDSU controller",
   (resetVme ? "reset" : "NOT reset"), (resetCtrl ? "reset" : "NOT reset"));

   /* Set the appropriate bits in the mask specifying what to reset. */

   resetMask = 0;
   if ( resetVme )  resetMask |= SDSU_RESET_VME;
   if ( resetCtrl ) resetMask |= SDSU_RESET_CONTROLLER;

   if (sdsuReset (sdsuId, resetMask) == ERROR)
   {
      ERROR_LOG ("Error resetting SDSU hardware");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Now obtain the path of the directory containing the DSP code. */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pFilePath);

   /* Determine whether any code should be downloaded to the VME DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

   if ( (errorNumber == 0) && (resetVme) &&
        (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
      )
   {

      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
         pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to VME DSP");
         errorNumber = S_detControl_SDSU_ERROR;
      }
   }

   /* Determine whether any code should be downloaded to the Timing DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pOmfFileName);

   if ( (errorNumber == 0) && (resetCtrl) &&
        (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
      )
   {

      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;
      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;
      }
   }

   /* Determine whether any code should be downloaded to the Utility DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, pOmfFileName);

   if ( (errorNumber == 0) && (resetCtrl) &&
        (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
      )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
         errorNumber = S_detControl_SDSU_ERROR;
      }
   }

   if ( (resetCtrl) && (errorNumber == 0) )
   {

      /* The PWFS packet size should be larger when used in full frame mode. */

      if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 160) == ERROR)
      {
         ERROR_LOG ("Failed to increase the PWFS packet size");
      }
   }

   /*
    * After successfully downloading new OMF code, the controller must be 
    * reinitialised by sending an "INI" command to the utility DSP and a "LDP" 
    * command to the timing DSP.
    */

   if ( (resetCtrl) && (errorNumber == 0) )
   {
      if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
      {
         ERROR_LOG ("Failed to init UTILITY DSP with INI command");
         errorNumber = S_detControl_SDSU_ERROR;
      }
      if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
      {
         ERROR_LOG ("Failed to init TIMING DSP with LDP command");
         errorNumber = S_detControl_SDSU_ERROR;
      }
   }

   /*
    * Set the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->aoCcdId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
      for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
          offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /*
    * Set the default offsets for the PWFS2 CCD sectors
    */

   MESSAGE_LOG4 (MSG_LOG,
                 "Defining new ADC offset levels: %#lx %#lx %#lx %#lx",
                 offsetVect[0], offsetVect[1], offsetVect[2], offsetVect[3]);

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                      (uint32) offsetVect[0] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 0 parameter");
   }

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                      (uint32) offsetVect[1] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 1 parameter");
   }

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2",
                      (uint32) offsetVect[2] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 2 parameter");
   }

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3",
                      (uint32) offsetVect[3] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 3 parameter");
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ( "Failed to activate TIMING DSP parameters with LDP command");
   }

   /*
    * Update the adc sir records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[2]) ,
                        obsId->pAdc2Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc2 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[3]) ,
                        obsId->pAdc3Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc3 sad record");
      return (ERROR);
   }

   /*
    * Set the default temperature 
    */

   MESSAGE_LOG2 (MSG_LOG,
                 "Defining temperature control parameters: %#lx %#lx",
                 tempCode, tempCoeff);

   if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
         == ERROR) ||
        (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
         == ERROR) )
   {
      ERROR_LOG ("Error setting temperature control parameters");
   }

   /*
    * If no errors have occurred during the reset set the controller health to 
    * GOOD.
    * If the reset failed the controller may be in an unusable state, so set the
    * health to BAD.
    */

   if ( errorNumber == 0 )
   {
      epToVxSetHealth( pRecordPrefix, "GOOD" );

      /* Set to TRUE the temperature Flag */

      readTempReadyFlag = TRUE ;
   }
   else
   {
      epToVxSetHealth( pRecordPrefix, "BAD" );
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTest
 *
 *   INVOCATION:
 *   detTest (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detTest command
 *
 *   DESCRIPTION:
 *   Test the SDSU controller
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detTest
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            testLevel;     /* Test level.                              */
   long            verbose;       /* Flag for verbose mode.                   */
   long            testState;     /* Testing state                            */

   uint32          testMask;      /* Mask for types of tests.                 */
   uint32          dspMask;       /* Mask for DSP to be tested.               */

   char            pTestResults[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                 /* String to contain test results.           */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & testLevel);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) & verbose);

   /*
    * The controller can only self-test if it has access to the SDSU hardware.
    * Note having access to the hardware should not be regarded as a test
    * failure, since all controllers will be tested routinely on startup,
    * and when the hardware is shared there will always be one controller
    * without access. Instead issue a warning.
    */
   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      if (epToVxPipeWrite (NULL, "Bad SDSU context", obsId->pTestResultsContext)
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      if (epToVxPipeWrite (NULL, "Bad observation context",
          obsId->pTestResultsContext) == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set the testing state to BUSY. */

   testState = menuCarstatesBUSY;
   if (epToVxPipeWrite (NULL, (char *) &testState, obsId->pDetTestContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set testing state to BUSY");
   }

   MESSAGE_LOG1 (MSG_LOG, "Testing SDSU controller: level=%ld", testLevel);

   /*
    * Load up the required masks and test the controller.
    * For now, all DSPs are tested in verbose mode.
    */

   testMask = 0;

   if ( testLevel > 0 )
      testMask |= SDSU_TEST_LINK;    /* Test integrity of data link to DSP.   */

   if ( testLevel > 1 )
      testMask |= SDSU_TEST_RDM;     /* Test read access to DSP memory.       */

   if ( testLevel > 2 )
      testMask |= SDSU_TEST_WRM;     /* Test write access to DSP memory.      */

   /* Other tests can be added when they are supported by sdsuTest. */

   dspMask = 0x7;                  /* Test all three DSPs (bits 0,1,2).   */

   if ( sdsuTest (sdsuId, (BOOL) verbose, &testMask, dspMask) == ERROR )
   {
      ERROR_LOG ("SDSU controller test failed");
      errorNumber = S_detControl_SDSU_ERROR;

      strncpy (pTestResults, "Tests failed: ", EPICS_MAX_BYTES_STRING_ATTRIB);

      if (testMask & SDSU_TEST_LINK)
      {
         strncat (pTestResults, "TDL ", EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      if (testMask & SDSU_TEST_RDM)
      {
         strncat (pTestResults, "RDM ", EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      if (testMask & SDSU_TEST_WRM)
      {
         strncat (pTestResults, "WRM ", EPICS_MAX_BYTES_STRING_ATTRIB);
      }

      if (epToVxPipeWrite (NULL, pTestResults, obsId->pTestResultsContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }

      testState = menuCarstatesERROR;
      if (epToVxPipeWrite (NULL, (char *) &testState, obsId->pDetTestContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set testing state to ERROR");
      }
   }
   else
   {
      MESSAGE_LOG (MSG_LOG, "Test completed successfully");

      if (epToVxPipeWrite (NULL, "Tested OK", obsId->pTestResultsContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }

      testState = menuCarstatesIDLE;
      if (epToVxPipeWrite (NULL, (char *) &testState, obsId->pDetTestContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set testing state to IDLE");
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSave
 *
 *   INVOCATION:
 *   detSave (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSave command
 *
 *   DESCRIPTION:
 *   This function uploads SDSU parameters to a file.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSave
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            destId;        /* Destination DSP ID.                      */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for file.                          */
   char         pParamFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Name of file to contain SDSU parameter values*/
   char         pFullParamFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and file name.            */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pParamFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & destId);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together, and append the string ".par"
    * to the file name if it is not already present. Ignore the path if not
    * specified.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullParamFileName, pParamFileName,
               EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      sprintf (pFullParamFileName, "%s/%s", pFilePath, pParamFileName );
   }

   if (strstr (pFullParamFileName, ".par") == NULL)
      strncat (pFullParamFileName, ".par", EPICS_MAX_BYTES_STRING_ATTRIB);

   /*
    * Upload SDSU parameters from the specified DSP to the specified file.
    * (If the DSP is specified as "-1" all the known SDSU parameters will be
    * uploaded to the file).
    */

   MESSAGE_LOG1 (MSG_LOG, "Uploading SDSU parameters to %s",
                 pFullParamFileName);

   if ( sdsuParamUpload( sdsuId, pFullParamFileName, (uint32) destId) == ERROR )
   {
      ERROR_LOG ("Failed to upload SDSU parameters");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detGeometry
 *
 *   INVOCATION:
 *   detGeometry (cadCmdContext, commandNumber, sdsuId, obsId, 
 *                pOffsetFullVect, pOffsetBinVect)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (!) obsId           (OBS_ID)          Observation context structure
 *   (>) pOffsetFullVect (long *)          ADC offset vector - no binning
 *   (>) pOffsetBinVect  (long *)          ADC offset vector - binning
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detGeometry command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU detector geometry parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detGeometry
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId,           /* Observation context structure.         */
   long *          pOffsetFullVect, /* ADC offset vector - no binning         */
   long *          pOffsetBinVect   /* ADC offset vector - binning            */

   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   /*
    * Variables associated with "Set detector readout geometry and binning
    * mode" command.
    */

   long         xReqSubap;  /* Number of subapertures per sector in X         */
                            /* direction (WFS only).                          */
   long         yReqSubap;  /* Number of subapertures per sector in Y         */
                            /* direction (WFS only).                          */
   long         xReqBin;    /* X binning factor (pixels per superpixel)       */
   long         yReqBin;    /* Y binning factor (pixels per superpixel)       */
   long         xReqRas;    /* Size of each subaperture in X direction in     */
                            /* super-pixels (wfs ONLY)                        */
   long         yReqRas;    /* Size of each subaperture in Y direction in     */
                            /* super-pixels (WFS only)                        */
   long         xReqSpace;  /* Spacing between subapertures in X direction in */
                            /* pixels (WFS only)                              */
   long         yReqSpace;  /* Spacing between subapertures in Y direction in */
                            /* pixels (WFS only)                              */
   long         xReqStart;  /* X offset from bottom left corner of array in   */
                            /* pixels.                                        */
   long         yReqStart;  /* Y offset from bottom left corner of array in   */
                            /* pixels.                                        */

   long         xReqPixels; /* Number of X pixels in digitised image (AC only)*/
   long         yReqPixels; /* Number of Y pixels in digitised image (AC only)*/
   long         xReqTail;   /* Number of trailing X pixels to be discarded on */
                            /* each row.                                      */
   long         reqPixelsNb;/* Total number of digitised pixels.              */
   long         defPixelsNb;/* Total number of digitised pixels.              */
   int          nPackets;   /* Number of packets expected per frame.          */
   int          updateOffset;
                            /* Flag to indicate if we have to update the ADC  */
                            /* offsets or not                                 */
   long         i;          /* index                                          */
   long         offsetVect[4];
                            /* ADC offset vector                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &xReqSubap);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) &yReqSubap);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) &xReqRas);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) &yReqRas);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *) &xReqBin);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *) &yReqBin);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *) &xReqStart);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, (char *) &yReqStart);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, (char *) &xReqSpace);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, (char *) &yReqSpace);

   /*
    * Initialise updateOffset
    */

   updateOffset = FALSE ;


   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
         "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   /*
    * The number of subapertures per sector must be positive and non-zero.
    */

   if ( (xReqSubap < 1) || (yReqSubap < 1) )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
         "Invalid number of subapertures: %ld x %ld",
         ERROR_LOG_NOW, xReqSubap, yReqSubap);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * The binning factors must be 1 or 2 
    */

   if ( (xReqBin < 1) || (yReqBin < 1) || (xReqBin > 2) || (yReqBin > 2) )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
         "Invalid binning factors: %ld, %ld",
         ERROR_LOG_NOW, xReqBin, yReqBin);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Calculate the number of digitized pixels per frame, and update the
    * current number of X and Y pixels. Also calculate the number of packets
    * into which these pixels will fit. (The ceil function is used because the
    * number of packets is always rounded up to the nearest integer).
    * xReqRas contains already the xReqBin division
    */

   xReqPixels = xReqSubap * xReqRas * 2; 
   yReqPixels = yReqSubap * yReqRas * 2;

   reqPixelsNb = xReqPixels * yReqPixels;

   defPixelsNb = (CCD_XSIZE)*(CCD_YSIZE);

   if ( reqPixelsNb > defPixelsNb )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
         "Invalid total number of pixels to read : %ld, max: %ld",
         ERROR_LOG_NOW, reqPixelsNb, defPixelsNb);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Calculate the number of trailing X pixels. This is required by the DSP
    * code as a check. If the value is negative then the subapertures span the
    * boundary between outputs (not physically possible), and xTail should
    * be set zero.
    *
    * xTail is the number of pixels that need to be discarded at the end of
    * each row, and is calculated by starting with the total number of pixels
    * to read (xSize) and subtracting off the pixels that are read out and/or
    * discarded during a readout.
    */

   xReqTail = obsId->aoCcdId->xSize - 
              (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
              + xReqSpace - xReqStart - obsId->aoCcdId->uscanNb;
   if ( xReqTail < 0 )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
      "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, xReqTail);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   if ( (!sdsuId->simulate) && (obsId->aoCcdId->packetSize > 0) )
   {
      obsId->aoCcdId->packetSize = 
      xReqRas * xReqSubap * (obsId->aoCcdId->outputsNb/2) * 2;

      nPackets = (int) ceil ( (double) (reqPixelsNb) / 
                 (double) obsId->aoCcdId->packetSize );
   }
   else
   {
      nPackets = 1;
   }

   /* Init the binning flag */

   if ( (xReqBin == 2) || ( yReqBin == 2) )
   {
      if ( obsId->aoCcdId->binningFlag == TRUE )
      {
         obsId->aoCcdId->binningFlag = TRUE ; /* no change */
      }
      else
      {
         obsId->aoCcdId->binningFlag = TRUE ;
         obsId->aoCtrlId->initFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoCtrlInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
         }
         obsId->aoCtrlId->darkInitFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoDarkInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
         }
         obsId->aoCtrlId->flatInitFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoFlatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
         }

         /* Init the ADC offset vector */

         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = pOffsetBinVect[i];

         updateOffset = TRUE;
      }
   }
   else
   {
      if ( obsId->aoCcdId->binningFlag == TRUE )
      {
         obsId->aoCcdId->binningFlag = FALSE ;
         obsId->aoCtrlId->initFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoCtrlInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
         }
         obsId->aoCtrlId->darkInitFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoDarkInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
         }
         obsId->aoCtrlId->flatInitFlag = FALSE;
         if (epToVxPipeWrite (NULL, "Not initialized", 
                              obsId->pAoFlatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
         }

         /* Init the ADC offset vector */

         for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = pOffsetFullVect[i];

         updateOffset = TRUE;
      }
      else
      {
         obsId->aoCcdId->binningFlag = FALSE ; /* no change */
      }
   }

   /*
    * Everything is ok, init aoCcdId.
    * Determine whether the given parameters will put the detector controller
    * into full frame mode. This happens when the there is one subaperture per
    * output and the subapertures fill the detector surface without any gaps.
    */

   obsId->aoCcdId->xStart = xReqStart;
   obsId->aoCcdId->yStart = yReqStart;
   obsId->aoCcdId->xBin = xReqBin;
   obsId->aoCcdId->yBin = yReqBin;
   obsId->aoCcdId->xRaster = xReqRas;
   obsId->aoCcdId->yRaster = yReqRas;
   obsId->aoCcdId->xSpace = xReqSpace;
   obsId->aoCcdId->ySpace = yReqSpace;
   obsId->aoCcdId->xSubapNb = xReqSubap;
   obsId->aoCcdId->ySubapNb = yReqSubap;
   obsId->aoCcdId->xPixels = xReqPixels;
   obsId->aoCcdId->yPixels = yReqPixels;
   obsId->aoCcdId->pixelsNb = reqPixelsNb;
   obsId->aoCcdId->xTail = xReqTail;
   obsId->aoCcdId->packetNb = nPackets;

   aoCcdContextShow (obsId->aoCcdId);

   MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
      (obsId->aoCcdId->binningFlag ? "binned":"full"));
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%d, YSIZE=%d, XPIXELS=%d, YPIXELS=%d",
      obsId->aoCcdId->xSize, obsId->aoCcdId->ySize, obsId->aoCcdId->xPixels, 
      obsId->aoCcdId->yPixels);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%d, YSUBAP=%d, XBIN=%d, YBIN=%d",
      obsId->aoCcdId->xSubapNb, obsId->aoCcdId->ySubapNb, obsId->aoCcdId->xBin,
      obsId->aoCcdId->yBin);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%d, YRAS=%d, XSPACE=%d, YSPACE=%d",
      obsId->aoCcdId->xRaster, obsId->aoCcdId->yRaster, obsId->aoCcdId->xSpace,
      obsId->aoCcdId->ySpace);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%d, YSTART=%d, XTAIL=%d, NPIXELS=%d",
      obsId->aoCcdId->xStart, obsId->aoCcdId->yStart, obsId->aoCcdId->xTail, 
      obsId->aoCcdId->pixelsNb);

   MESSAGE_LOG2 (MSG_FULLDEBUG,
      "Each frame will consist of %d packets of %d pixels each",
      nPackets, obsId->aoCcdId->packetSize);

   /*
    * Update the geometry parameters in the SDSU timing DSP. These are all
    * "on-the-fly" parameters and need to be downloaded with sdsuParamWRP()
    * and activated by sending a "LDP" command.
    */
   if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 
                       obsId->aoCcdId->packetSize) == ERROR)
   {
      ERROR_LOG ("Failed to increase the PWFS packet size");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", (uint32) xReqSubap)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", (uint32) yReqSubap)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", (uint32) xReqStart)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", (uint32) yReqStart)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   (uint32) xReqRas)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   (uint32) yReqRas)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", (uint32) xReqSpace)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", (uint32) yReqSpace)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XBIN",   (uint32) xReqBin)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YBIN",   (uint32) yReqBin)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  (uint32) xReqTail)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", (uint32) reqPixelsNb)
        == ERROR)
     )
   {
      ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   if ( updateOffset == TRUE )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                        (uint32) offsetVect[0] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                         (uint32) offsetVect[1] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2",
                         (uint32) offsetVect[2] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 2 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3",
                         (uint32) offsetVect[3] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 3 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }
   
      /*
       * Update the adc sir records 
       */

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                           obsId->pAdc0Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc0 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                           obsId->pAdc1Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc1 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[2]) ,
                           obsId->pAdc2Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc2 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[3]) ,
                           obsId->pAdc3Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc3 sad record");
         return (ERROR);
      }
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   /*
    * Update the number of packets per frame in the SDSU context structure.
    */

   sdsuId->packetsPerFrame = nPackets;

   /*
    * Init the SAD records 
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xStart) ,
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yStart) ,
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSubapNb) ,
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySubapNb) ,
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xRaster) ,
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yRaster) ,
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSpace) ,
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySpace) ,
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xBin) ,
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yBin) ,
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detPrimitive
 *
 *   INVOCATION:
 *   detPrimitive (cadCmdContext, commandNumber, sdsuId, obsId) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext        (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber        (int)             Command number
 *   (>) sdsuId               (SDSU_ID)         Current SDSU context structure
 *   (>) obsId                (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detPrimitive command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU exposure parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detPrimitive
   (
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */

   long            destId;          /* Destination DSP ID.                    */

   long            pCmdArg [6] = {0, 0, 0, 0, 0, 0};
                                    /* Primitive command arguments.           */
   long            pRepArg [3] = {0, 0, 0};
                                    /* Primitive command reply arguments.     */

   char            pStringAttrib [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Contents of general string attribute.  */

   uint32          i;               /* Loop counter.                          */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Some primitive commands can be accepted while an observation is in
    * progress. However, warn the user if there is an observation in progress.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
      "NOTE: Issuing primitive command while observation in progress.");
   }

   /*
    * Get the command name, destination DSP and up to 6 command arguments
    * from the attributes supplied with the CAD command.
    */
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pStringAttrib);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & destId);

   for (i = 0; i < 6; i++)
   {
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, i + 2,
                             (char *) & pCmdArg [i]);
   }

   MESSAGE_LOG4 (MSG_LOG,
   "About to execute SDSU primitive command \"%s\" to %ld with %#lx ... %#lx",
   pStringAttrib, destId, pCmdArg[0], pCmdArg[5]);

   /*
    * Issue the primitive command to the SDSU controller.
    */

   if (sdsuPrimitive (sdsuId, pStringAttrib, (uint32) destId,
       (uint32 *) pCmdArg, (uint32 *) pRepArg) == ERROR)
   {
      ERROR_LOG ("Failed to execute SDSU primitive command");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Write the reply to the SDSU primitive reply SIR record. */

   sprintf (pStringAttrib, "0x%08lx 0x%08lx 0x%08lx", pRepArg [0],
            pRepArg [1], pRepArg [2]);
   if (epToVxPipeWrite (NULL, pStringAttrib, obsId->pDetPrimReplyContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to write message to SDSU primitive reply pipe.");
      if ( errorNumber == 0 ) errorNumber = (uint32) errnoGet();
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detPowerOn
 *
 *   INVOCATION:
 *   detPowerOn (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext        (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber        (int)             Command number
 *   (>) sdsuId               (SDSU_ID)         Current SDSU context structure
 *   (>) obsId                (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detPowerOn command
 *
 *   DESCRIPTION:
 *   This function executes POWER ON command for the Bob Leach controller
 *
 *   EXTERNAL VARIABLES:
 *   NONE
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detPowerOn
   (
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Issue the primitive commands to the SDSU controller.
    */

   if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to init UTILITY DSP with INI command");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to init TIMING DSP with LDP command");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detMode
 *
 *   INVOCATION:
 *   detMode (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detMode command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU readout mode.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detMode
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            mode;          /* Readout mode parameter.                  */
   long            samples;       /* Number of samples per frame.             */
   long            tInt;          /* CDI integration time.                    */
   long            gainSp;        /* Combined amplifier gain and integrator   */
                                  /* speed.                                   */

   BOOL        ccdFailed = FALSE; /* Set TRUE if CCD parameter setup fails.   */
   BOOL        irFailed = FALSE;  /* Set TRUE if CCD parameter setup fails.   */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & mode);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & tInt);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & gainSp);
   /* IR only */
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & samples);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */
   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
         "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   MESSAGE_LOG4 (MSG_LOG, "Defining new readout mode: %#lx %#lx %#lx %#lx",
      mode, tInt, gainSp, samples);

   /*
    * Set the readout mode by writing the appropriate SDSU parameters. All are
    * on-the-fly parameters, except GAIN_SP, and need to be downloaded with
    * sdsuParamWRP() and activated by sending a "LDP" command. GAIN_SP cannot
    * be changed if an observation is in progress, and must be updated
    * separately with sdsuParamWrite().
    *
    * If any parameter is defined as -1 it is not changed.
    *
    * Only the T_MODE parameter is universal. The T_INT_TIM and T_GAIN_SP
    * parameters are valid for CCD detectors only, and T_SAMPLES is valid for
    * IR detectors only. An error is only reported if an attempt to write both
    * the CCD and IR parameters fails.
    */

   if ( mode != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_MODE", (uint32) mode ) ==
           ERROR )
      {
         ERROR_LOG ("Error setting readout mode parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }
   }

   if ( tInt != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_INT_TIM", (uint32) tInt ) ==
           ERROR )
      {
         ccdFailed = TRUE;
      }
   }

   if ( samples != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_SAMPLES", (uint32) samples )
           == ERROR )
      {
         irFailed = TRUE;
      }
   }

   if ( (!obsId->observing) && (gainSp != -1) )
   {
      if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_GAIN_SP",
                           (uint32) gainSp ) == ERROR )
      {
         ccdFailed = TRUE;
      }
   }
   else if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
      "T_GAIN_SP parameter not changed while observing");
   }

   if ( ccdFailed && irFailed )
   {
      ERROR_LOG ("Error setting readout configuration parameters");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detOffset
 *
 *   INVOCATION:
 *   detOffset (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detMode command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU readout mode.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detOffset
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.         */

   long            offset0;       /* ADC offset for output 0.               */
   long            offset1;       /* ADC offset for output 1.               */
   long            offset2;       /* ADC offset for output 2.               */
   long            offset3;       /* ADC offset for output 3.               */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & offset0);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & offset1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & offset2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & offset3);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
      "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   MESSAGE_LOG4 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx %#lx %#lx",
      offset0, offset1, offset2, offset3);

   /*
    * Set the offsets by writing the appropriate SDSU parameters.
    * If any parameter is defined as -1 it is not changed.
    */

   if ( offset0 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0", (uint32) offset0 )
           == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset0) ,
			   obsId->pAdc0Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc0 sad record");
	 return (ERROR);
      }
   }

   if ( offset1 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1", (uint32) offset1 )
            == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset1) ,
			   obsId->pAdc1Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc1 sad record");
	 return (ERROR);
      }
   }

   if ( offset2 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2", (uint32) offset2 )
           == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 2 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset2) ,
			   obsId->pAdc2Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc2 sad record");
	 return (ERROR);
      }
   }

   if ( offset3 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3", (uint32) offset3 )
           == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 3 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset3) ,
			   obsId->pAdc3Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc2 sad record");
	 return (ERROR);
      }
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTemp
 *
 *   INVOCATION:
 *   detTemp (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detTemp command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU temperature parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Assumes SDSU_TEMP_UNIT is not zero.
 *-
 */

uint32 detTemp
   (
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID         sdsuId,           /* SDSU context structure.               */
   OBS_ID          obsId             /* Observation context structure.        */
   )
{
   uint32          errorNumber;      /* Error number reported by task.        */

   /*
    * Variables associated with the "set detector temperature parameters"
    * command.
    */

   double          tempTarget;       /* Target temperature in Celsius.        */
   uint32          tempCode;         /* Target temperature code.              */
   long            tempCoeff;        /* Coefficient for temperature control.  */

   /*
    * Initialise the error number and obtain the attributes provided with the
    *  command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & tempTarget);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) & tempCoeff);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   if ( tempTarget <= 0.0 )
   {
      tempCode = (uint32) ((SDSU_TEMP_BASE - tempTarget) / SDSU_TEMP_UNIT);
      tempCode &= 0xfff; /* Truncate to 0xfff (which is the maximum allowed) */
   }
   else
   {
      /* Switch off cooling altogether for temperatures above 0C. */
      tempCode = 0;
   }

   MESSAGE_LOG2 (MSG_LOG, "Defining temperature control parameters: %#lx %#lx",
      tempCode, (uint32) tempCoeff);

   /*
    * Write the temperature control parameters to the SDSU controller.
    */

   if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
        == ERROR) ||
        (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32) tempCoeff )
        == ERROR)
      )
   {
      ERROR_LOG ("Error setting temperasture control parameters");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameUnscramble
 *
 *   INVOCATION:
 *   detFrameUnscramble (xPixels, yPixels, outputs, inFrame, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)    Number of columns
 *   (>) yPixels   (const int)    Number of rows
 *   (>) outputs   (const int)    Number of detector outputs (2 or 4)
 *   (>) inFrame   (SDSU_FRAME *) Pointer to input frame
 *   (<) outBuffer (float *)      Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Unscramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a raw frame of data containing pixels in the order 
 *   they are read from the detector and unscrambles them to generate an output 
 *   frame with pixels in the correct order.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided 
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics, 
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inFrame is a data frame which contains the scrambled SDSU pixels.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels floating point values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameUnscramble
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   SDSU_FRAME *   inFrame,         /* Pointer to input frame                  */
   float *        outBuffer        /* Pointer to output frame buffer.         */
   )
{

   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */

   int            i, j;            /* Counters.                               */

   int            nPixels;         /* Total number of pixels.                 */

   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */

   volatile uint16 *  inDataPtr;   /* Pointer to start of input data.         */
   float *            outDataPtr;  /* Pointer to start of output data.        */

   float *        ps1;             /* Pointer to beginning of sector 1.       */
   float *        ps2;             /* Pointer to beginning of sector 2.       */
   float *        ps3;             /* Pointer to beginning of sector 3.       */
   float *        ps4;             /* Pointer to beginning of sector 4.       */

#ifdef DEBUG
   float          min, max;         /* Minimum and maximum.                    */
#endif /* DEBUG */


#ifdef DEBUG
   if ( (inFrame == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
                "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }
#endif /* DEBUG */

#ifdef DEBUG
   printf (
   "detFrameUnscramble: Unscrambling %d x %d pixels from frame at %p to %p\n",
   xPixels, yPixels, inFrame, outBuffer);

   min = FLT_MAX;
   max = -FLT_MAX;
#endif /* DEBUG */

   /*
    * Set pointers to the start of the data.
    */

   inDataPtr = & inFrame->pixel[0];
   outDataPtr = outBuffer;

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * for four outputs the sectors are arranged like this
    *
    *   0----->------+-----<------0
    *   |  sector 4  |  sector 3  |
    *   +------------+------------+
    *   |  sector 1  |  sector 2  |
    *   0----->------+-----<------0
    *
    * "0" shows the origin of each sector and ">" the direction of readout.
    */

   /* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

   nPixels = xPixels * yPixels;
   xPixelsSector = xPixels / 2;
   yPixelsSector = yPixels / 2;

#ifdef DEBUG
   printf ("Four sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

   /* Initialise the starting position for each sector */

   ps1 = outDataPtr;
   ps2 = &outDataPtr[xPixels - 1];
   ps3 = &outDataPtr[nPixels - 1];
   ps4 = &outDataPtr[nPixels - xPixels];
   ptr = inDataPtr;

   /* Treat one line at a time, moving sector pointers */

   for (i = 0; i < yPixelsSector; i++)
   {
      for (j = 0; j < xPixelsSector; j++)
      {
#ifdef DEBUG
         if ( (float) *ptr < min ) min = (float) *ptr;
         if ( (float) *ptr > max ) max = (float) *ptr;
         if ( (float) *(ptr+1) < min ) min = (float) *(ptr+1);
         if ( (float) *(ptr+1) > max ) max = (float) *(ptr+1);
         if ( (float) *(ptr+2) < min ) min = (float) *(ptr+2);
         if ( (float) *(ptr+2) > max ) max = (float) *(ptr+2);
         if ( (float) *(ptr+3) < min ) min = (float) *(ptr+3);
         if ( (float) *(ptr+3) > max ) max = (float) *(ptr+3);
#endif
         /*
          * change the order here if sectors 1, 2, 3, 4 is
          * different from the order of arrival
          */

         *ps1++ = (float) *ptr++;
         *ps2-- = (float) *ptr++;
         *ps3-- = (float) *ptr++;
         *ps4++ = (float) *ptr++;
      }
      ps1 += xPixelsSector;
      ps2 += xPixelsSector * 3;
      ps3 -= xPixelsSector;
      ps4 -= xPixelsSector * 3;
   }
#ifdef DEBUG
   printf ("Values range from %g to %g\n", min, max);
#endif /* DEBUG */

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameScramble
 *
 *   INVOCATION:
 *   detFrameScramble (xPixels, yPixels, outputs, inBuffer, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)  Number of columns
 *   (>) yPixels   (const int)  Number of rows
 *   (>) outputs   (const int)  Number of detector outputs (2 or 4)
 *   (>) inFrame   (float *)   Pointer to input frame buffer
 *   (<) outBuffer (uint16 *)   Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Scramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a simulated frame of data and scrambles the pixels 
 *   into the order they are read from the detector.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided 
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics, 
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inBuffer must point to a buffer containing 
 *   xPixels*yPixels floating point values.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels unsigned short integer values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameScramble
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   float *        inBuffer,        /* Pointer to input frame buffer           */
   uint16 *       outBuffer        /* Pointer to output frame buffer.         */
   )
{

   float *        ptr;             /* Pointer into input frame buffer.        */
   int            i, j;            /* Counters.                               */
   int            nPixels;         /* Total number of pixels.                 */
   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */


   uint16 *       ps1;             /* Pointer to beginning of sector 1.       */
   uint16 *       ps2;             /* Pointer to beginning of sector 2.       */
   uint16 *       ps3;             /* Pointer to beginning of sector 3.       */
   uint16 *       ps4;             /* Pointer to beginning of sector 4.       */


   if ( (inBuffer == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
      "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
      "detFrameUnscramble: Scrambling %d x %d pixels from frame at %p to %p\n",
      xPixels, yPixels, inBuffer, outBuffer);
#endif /* DEBUG */

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
    *
    *   +------------+------------+
    *   |  sector 1  |  sector 2  |
    *   0----->------+-----<------0
    *
    * and if there are four outputs the sectors are arranged like this
    *
    *   0----->------+-----<------0
    *   |  sector 4  |  sector 3  |
    *   +------------+------------+
    *   |  sector 1  |  sector 2  |
    *   0----->------+-----<------0
    *
    * "0" shows the origin of each sector and ">" the direction of readout.
    */

   switch (outputs)
   {
      case (2):

         /* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

         nPixels = xPixels * yPixels;
         xPixelsSector = xPixels / 2;
         yPixelsSector = yPixels;

#ifdef DEBUG
         printf ("Two sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

         /* Initialise the starting position for each sector */

         ps1 = outBuffer;
         ps2 = &outBuffer[xPixels - 1];
         ptr = inBuffer;

         /* Treat one line at a time, moving sector pointers */

         for (i = 0; i < yPixelsSector; i++)
         {
            for (j = 0; j < xPixelsSector; j++)
            {
               /*
                * Change the order here if sectors 1, 2 is
                * different from the order of arrival
                */

               *ps1++ = (uint16) *ptr++;
               *ps2-- = (uint16) *ptr++;
            }
            ps1 += xPixelsSector;
            ps2 += xPixelsSector * 3;
         }
         break;

      case (4):

         /* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

         nPixels = xPixels * yPixels;
         xPixelsSector = xPixels / 2;
         yPixelsSector = yPixels / 2;

#ifdef DEBUG
         printf ("Four sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

         /* Initialise the starting position for each sector */

         ps1 = outBuffer;
         ps2 = &outBuffer[xPixels - 1];
         ps3 = &outBuffer[nPixels - 1];
         ps4 = &outBuffer[nPixels - xPixels];
         ptr = inBuffer;

         /* Treat one line at a time, moving sector pointers */

         for (i = 0; i < yPixelsSector; i++)
         {
            for (j = 0; j < xPixelsSector; j++)
            {
               /*
                * change the order here if sectors 1, 2, 3, 4 is
                * different from the order of arrival
                */

               *ps1++ = (uint16) *ptr++;
               *ps2-- = (uint16) *ptr++;
               *ps3-- = (uint16) *ptr++;
               *ps4++ = (uint16) *ptr++;
            }
            ps1 += xPixelsSector;
            ps2 += xPixelsSector * 3;
            ps3 -= xPixelsSector;
            ps4 -= xPixelsSector * 3;
         }
         break;

      default:
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
                     "Bad number of outputs given, %d", ERROR_LOG_SAVE,
                     outputs);
         return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveEnd
 *
 *   INVOCATION:
 *   detObserveEnd (sdsuId, obsIdIn, pRawFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId    (SDSU_ID)      Controller ID
 *   (>) obsIdIn   (void *)       Pointer to observation definition, cast to 
 *                                void *
 *   (>) pRawFrame (SDSU_FRAME *) Pointer to image frame
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Complete observation
 *
 *   DESCRIPTION:
 *   This function ends an observation.
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *
 *-
 */

void detObserveEnd
   (
   SDSU_ID        sdsuId,         /* SDSU ID                                  */
   void *         obsIdIn,        /* Pointer to observation ID cast to void * */
   SDSU_FRAME *   pRawFrame       /* Incoming Image frame                     */
   )
{
   /* Variables describing the observation. */

   OBS_ID         obsId;          /* Pointer to observation ID structure.     */

   /* DHS variables */

   DHS_STATUS     dhsErrno;       /* DHS error number.                        */
   DHS_STATUS     dummyDhsErrno;  /* DHS error number used for freeing        */
                                  /* resources.                               */
   DHS_TAG        putTag;         /* DHS data transfer tag.                   */

   /* Signal processing variable. */
   
   int            nCoadds=1;        /* Number of frames per coadd.            */
   int            i;
   int            j;
   int            k;
   int            indexIm;
   int            indexFgCtrl;
   int            indexCtrl;
   int            imageSize;
   int *          pWfsStatus;
   float *        pImage;
   float *        pi;
   float *        pc;
   float *        pMax;
   double *       pTotal;
   double *       pFlux;
   double *       pGuides;
   double *       pErrorGuides;
   double *       pAoCentroids;
   double *       pCentroids;
   double *       pErrorCentroids;
   double *       pPrevThresh;
   double *       pThresh;
   double *       pFg;
   double *       pFgAfterRot;
   double *       pErrorsFg;
   double *       pTime;
   double         elapsed;
   double         rms;
   double         mean;

   /* File names. */

   char         pFileNameString[ (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 + 4];
                                   /* String containing file name.            */

   /* SDSU parameters. */

   uint32       frameCount;        /* SDSU frame counter.                     */
   BOOL         bufferReserved;    /* TRUE if the SDSU frame buffer been      */
                                   /* reserved.                               */
   BOOL         obsAlreadyAborted; /* TRUE if observation already  aborted.   */

   double       readoutTimeout;    /* Readout timeout in seconds.             */
   double       waitTimeSecs;      /* Wait time in seconds.                   */

   /* 
    * Variables associated with "observe" command.
    * (Label, datapath and filename use general filename parameters)
    */

   long         observingState;    /* Observation status (busy or idle).      */
   long         measuringState;    /* Measuring status (0 or 1).              */

#ifdef DEBUG
   printf ("detObserveEnd: %p %p %p\n", sdsuId, obsIdIn, pRawFrame);
#endif

   bufferReserved = FALSE;
   obsAlreadyAborted = FALSE;

#ifdef DEBUG
   /* Check the pointers provided as arguments. */

   if ( obsIdIn == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "NULL observation ID", ERROR_LOG_NOW);
      return;
   }

   if ( pRawFrame == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "NULL raw frame pointer", 
                 ERROR_LOG_NOW);
      return;
   }
#endif

   /* Convert the observation ID pointer provided as an argument. */

   obsId = (OBS_ID) obsIdIn;

   /*
    * Initialise the DHS error number.
    */

   dhsErrno = DHS_S_SUCCESS;         /* <---- DHS error number is reset here. */

#ifdef DEBUG
   /* This function should only be called when an observation is in progress */

   if ( !obsId->observing )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", 
                 ERROR_LOG_NOW);
      return;
   }
#endif

   /*
    * Cancel any observation timer. Failing to cancel this is not a serious 
    * error. THIS IS NOW ONLY DONE IN SIMULATION MODE.
    */

   if ( sdsuId->simulate )
   {
      if ( obsId->timeId != NULL )
      {
         if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
         {
            ERROR_SET (0, "Failed to cancel observation timer", ERROR_LOG_NOW);
         }
      }
   }

   /*
    * Get a timestamp to record the time at which the observation finished.
    * Failing to cancel this is not a serious error.
    */

   if ( timeNow (&(obsId->rawtEnd)) != OK )
   {
#ifdef DEBUG
      ERROR_SET (0, "Failed to get time stamp at observation end", 
                 ERROR_LOG_NOW);
#endif
      obsId->rawtEnd = (double)AO_TIME_NOW_ERROR ;
   }

#ifdef DEBUG
   printf ("detObserveEnd: Time at observation end: %f seconds.\n", 
           obsId->rawtEnd);
#endif

   /*
    * Get the frame countdown counter attached to the data and increment 
    * the frame counter.
    */

   frameCount = pRawFrame->header.frameCount;
   obsId->nframes++;

   /* BUG WORK AROUND: THE SDSU CONTROLLER REPORTS FRAME COUNT=1 WHEN AN 
    * INFINITE NUMBER OF FRAMES ARE BEING RETURNED. IF THE CONTROLLER IS 
    * RUNNING IN CONTINUOUS MODE, POKE THE FRAME COUNT WITH ZERO. 
    * (REMOVE WHEN SDSU DSP CODE IS FIXED).
    */

   if ( obsId->continuous ) frameCount = 0;
   if ( obsId->totalFrames > 1 ) frameCount = 0;  /* MODIF 01 nov 99 */

   /*
    * Report the frame counter and the number of frames remaining.
    */

   if ( obsId->stopped )
   {
      if ( frameCount > 1 )
      {
         MESSAGE_LOG1 (MSG_MINDEBUG,
            "... exp. complete and obs. stopped. Frame count=%d",
            obsId->nframes);
         MESSAGE_LOG1 (MSG_WARNING,
            "WARNING: Remaining %ld frames will be aborted", (frameCount-1));
      }
      else if ( frameCount == 1 )
      {
         MESSAGE_LOG1 (MSG_MINDEBUG,
         "... exp. complete and obs. stopped. Frame count=%d (last frame)",
         obsId->nframes);
      }
      else
      {
         MESSAGE_LOG1 (MSG_MINDEBUG,
         "... exp. complete and continuous obs. stopped. Frame count=%d",
         obsId->nframes);
      }
   }
   else
   {
      if ( frameCount > 1 )
      {
         MESSAGE_LOG2 (MSG_MINDEBUG, 
         "... exposure complete. Frame count=%d (%ld remaining)",
         obsId->nframes, (frameCount-1));
      }
      else if ( frameCount == 1 )
      {
         MESSAGE_LOG1 (MSG_MINDEBUG, 
            "... exposure complete. Frame count=%d (last frame)",
            obsId->nframes);
      }
      else
      {
         MESSAGE_LOG1 (MSG_MINDEBUG, 
            "... exposure complete. Frame count=%d (continuous)",
            obsId->nframes);
      }
   }

   /*
    * If this is the first frame, wait for the binary semaphore indicating 
    * that the code executed at the start of the the observation has 
    * completed. (This will only matter for very short observations).
    *
    * If an error occurs jump to the ERROR_EXIT at the end of this function.
    * I do not like "goto" statements but they seem to be necessary in this case
    * where the function is void and I cannot use "return (ERROR)" and have 
    * the caller set the "observing" flag. The alternative to the "goto" would 
    * be to fill the rest of the function with "if (!error)" tests, which 
    * would be even more incomprehensible.
    * SMB - 11 December 1998.
    */

   if ( obsId->nframes <= 1 )
   {
#ifdef DEBUG
      printf ("detObserveEnd: Waiting for observation sync semaphore...");
#endif
      if ( semTake ( obsId->syncSem, OBS_WAIT_TIMEOUT ) == ERROR )
      {
         ERROR_SET (0, 
         "Failed to take observation synchronisation semaphore", ERROR_LOG_NOW);
         goto ERROR_EXIT;
      }
#ifdef DEBUG
      printf (" ... got observation sync semaphore...\n");
#endif
   }

   /*
    * Check the status of the frame just received and only process the data 
    * if the frame has been received successfully.
    */

 
   if ( sdsuId->fatal )
   {
      ERROR_SET1 (0, "Fatal error at frame %lu - observation abandoned",
         ERROR_LOG_NOW, frameCount);
      goto ERROR_EXIT;
   }
   else if ( pRawFrame->header.status != 0 )
   {
      if ((pRawFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
                       "Timeout in frame %lu - frame ignored", frameCount);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_OVERRUN) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
                       "Data overrun in frame %lu - frame ignored", frameCount);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
                       "Sync error in frame %lu - ignored", frameCount);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
                       "Checksum error in frame %lu - ignored", frameCount);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_NOK) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
                       "Overwritten error in frame %lu - ignored", frameCount);
      }
   }
   else
   {
      /* Update the dhs counter */

      obsId->dhsCounter ++ ;

      /* Init the circular buffer */
 
      indexIm = obsId->aoCbImId->position;
      pImage = obsId->aoCbImId->cbImRecord[indexIm].imageVect;

      indexFgCtrl = obsId->aoCbFgCtrlId->position;
      pTotal = obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].totalCountsVect;
      pGuides = obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].guidesVect;
      pErrorGuides = 
      obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].errorGuidesVect;
      pCentroids = 
      obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].centroidsVect;
      pErrorCentroids = 
      obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].errorCentroidsVect;
      pFg = obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].fgVect;
      pFgAfterRot = 
      obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].fgVectAfterRot;
      pErrorsFg = obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].fgErrorsVect;
      pWfsStatus = 
      &(obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].wfsStatus);
      pTime = &(obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].time);

      pFlux = pTotal + obsId->aoCcdId->subapUsedNb;

      if ( obsId->threshRealTimeFlag == FALSE )
      {
         pPrevThresh = obsId->aoCtrlId->thresholdVect;
         pThresh = obsId->aoCtrlId->thresholdVect;
      }
      else
      {
         pThresh = 
         obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl].thresholdVect;

         if ( ( indexFgCtrl == 0 ) && ( obsId->aoCbFgCtrlId->counter == 0 ) )
            pPrevThresh = pThresh;
         else
         {
            if ( indexFgCtrl != 0 )
               pPrevThresh =
               obsId->aoCbFgCtrlId->cbFgCtrlRecord[indexFgCtrl-1].thresholdVect;
            else
               pPrevThresh =
               obsId->aoCbFgCtrlId->cbFgCtrlRecord[CB_FG_CTRL_RECORD_NB-1].thresholdVect;
         }
      }

      obsId->aoCbImId->cbImRecord[indexIm].imageStatus = 
      (int)(pRawFrame->header.status) ;

      /*printf ( "index image CB =%d\n", indexIm) ;
      printf ( "index control CB =%d\n", indexCtrl) ;*/

      /*
       * Unscramble the data. The algorithm used depends on the number of 
       * detector outputs, obtained earlier.
       */

      if ( detFrameUnscramble( obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                               (int) obsId->aoCcdId->outputsNb,
                               pRawFrame,  pImage)
           == ERROR )
      {
         ERROR_LOG ("Failed to unscramble data");
         if ( obsId->outOptions == 1 )
         {
            dummyDhsErrno = DHS_S_SUCCESS;         
            dhsBdDsFree ( obsId->dhsDataset, &dummyDhsErrno );
         }
         else
         {
         }
         goto ERROR_EXIT;
      }

      obsId->outNFrames ++ ;

#ifdef DEBUG
      printf ( "outNFrames = %d\n" , obsId->outNFrames ) ;
#endif

      /*
       * If a signal processing context has been initialised, process the data.
       */

      if ( (obsId->aoCtrlId == NULL) || 
           (obsId->aoCtrlId->initFlag == FALSE) )
      {
         MESSAGE_LOG (MSG_MINDEBUG,
         "PWFS2: Cannot process data - no AO control structure defined");
      }   
      else
      {
         /*
          * Switch according to the signal processing mode, as defined with 
          * the detSigMode command, or as defaulted in the detSigInit 
          * command.
          */

         switch (obsId->sigMode)
         {
            case (AO_MODE_DARK):

               /*
               * Subtract Dark mode.
               */
#ifdef DEBUG
               printf ("aoDarkSubtract: %p %p %d %d\n", 
                       pImage, obsId->aoCtrlId->darkVect, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels);
#endif
               if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                    obsId->aoCcdId->xPixels, 
                                    obsId->aoCcdId->yPixels) == ERROR )
               {
                  ERROR_LOG ("Failed to subtract DARK from current frame");
               }

               break;

            case (AO_MODE_GG):

               /*
                * Global Guide mode.
                */
#ifdef DEBUG
               printf (
                 "aoGlobalGuide (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
                 pImage, obsId->aoCcdId, obsId->aoCtrlId, pTotal, 
                 pGuides, pFg, pFgAfterRot, pErrorsFg, pTime, pWfsStatus,
                 (int)obsId->writeToRm);
#endif
               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale ;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale ;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( aoGlobalGuide (pImage, obsId->aoCcdId, obsId->aoCtrlId, 
                                   pTotal, pGuides, pFg, pFgAfterRot, pErrorsFg,
                                   pTime, pWfsStatus, (int)obsId->writeToRm) 
                    == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide correction");
               };

               break;

            case (AO_MODE_GG_COADD):

               /*
                * Global Guide and Coadd mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf (
                 "aoGlobalGuide (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
                 pImage, obsId->aoCcdId, obsId->aoCtrlId, pTotal, 
                 pGuides, pFg, pFgAfterRot, pErrorsFg, pTime, pWfsStatus, 
                 (int)obsId->writeToRm);
#endif
               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( aoGlobalGuide (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                   pTotal, pGuides, pFg, pFgAfterRot, 
                                   pErrorsFg, pTime, pWfsStatus, 
                                   (int)obsId->writeToRm) == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide correction");
               }
#ifdef DEBUG
               printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                       obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif
               if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId, nCoadds) == ERROR )
               {
                  ERROR_LOG ("Failed to average images");
               }

               /*
                * Increment the coadd counter and when it reaches nCoadds 
                * save the coadded data to disk. Coadded data are only 
                * saved once per observation.
                */

               obsId->coaddCounter++;
               if ( obsId->coaddCounter == nCoadds )
               {
                  /*
                   * Make up a file name by adding the string ".coadd.fits"
                   * to the given file name. Use a default file name if one 
                   * has not been given.
                   */

                  if ( strcmp(obsId->pCoaddFileName, "") == 0 )
                  {
                     strcpy ( pFileNameString, "coadd.fits" );
                  }
                  else
                  {
                     sprintf( pFileNameString, "%s.fits", 
                              obsId->pCoaddFileName );
                  }

                  MESSAGE_LOG1 (MSG_MINDEBUG, 
                  "Saving coadded data to %s", pFileNameString);

                  if ( detWriteFits (pFileNameString, obsId, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                       obsId->aoCtrlId->sumVect) == ERROR )
                  {
                     ERROR_LOG ("Failed to save coadded data to disk");
                   }
               }
            break;

            case (AO_MODE_COADD):

               /*
                * Coadd only mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                       obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif
               if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                    obsId->aoCcdId->xPixels, 
                                    obsId->aoCcdId->yPixels) == ERROR )
               {
                  ERROR_LOG ("Failed to subtract DARK from current frame");
               }

               if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId, nCoadds) == ERROR )
               {
                  ERROR_LOG ("Failed to average images");
               }

               
               /*
                * Increment the coadd counter and when it reaches nCoadds 
                * save the coadded data to disk. Coadded data are only 
                * saved once per observation.
                */

               obsId->coaddCounter++;
#ifdef DEBUG
               printf ( "obsId->coaddCounter=%d\n" , obsId->coaddCounter);
#endif
               if ( obsId->coaddCounter == nCoadds )
               {
                  /*
                   * Make up a file name by adding the string ".coadd.fits" to 
                   * the given file name. Use a default file name if one 
                   * has not been given.
                   */


                  if ( strcmp(obsId->pCoaddFileName, "") == 0 )
                  {
                     strcpy ( pFileNameString, "coadd.fits" );
                  }
                  else
                  {
                     sprintf( pFileNameString, "%s.fits", 
                              obsId->pCoaddFileName );
                  }

                  MESSAGE_LOG1 (MSG_MINDEBUG, 
                  "Saving coadded data to %s", pFileNameString);

                  if ( detWriteFits (pFileNameString, obsId, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                       obsId->aoCtrlId->sumVect) == ERROR )
                  {
                     ERROR_LOG ("Failed to save coadded data to disk");
                  }
               }
            break;

            case (AO_MODE_THRESH):

               /*
                * Threshold computation mode.
                */

               if ( obsId->methodThreshComp == AO_THRESH_SPOTS ) 
               {
                  /* 
                   * With spots method -> average images and look for the 
                   * brightest pixels 
                   */

                  nCoadds = (int) obsId->nAverageDataThreshComp;
#ifdef DEBUG
                  printf (
                  "aoGuideAndFocus (%p, %p, %p, %p , %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
                  pImage, obsId->aoCcdId, obsId->aoCtrlId, pPrevThresh, pTotal,
                  pCentroids, pErrorCentroids, pFg, pFgAfterRot, pErrorsFg, 
                  pTime, pWfsStatus, (int)obsId->writeToRm);
#endif
                  if ( obsId->updateFgScale == TRUE )
                  {
                     obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale ;
                     obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale ;
                     obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                     obsId->aoCtrlId->slidingFocusGain =
                     obsId->slidingFocusGain;
                     obsId->aoCtrlId->one_slidingFocusGain =
                     1.0 - obsId->slidingFocusGain ;

                     obsId->updateFgScale = FALSE ;
                  };

                  if ( aoGuideAndFocus (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                        pPrevThresh, pTotal, pCentroids, 
                                        pErrorCentroids, pFg, pFgAfterRot, 
                                        pErrorsFg, pTime, pWfsStatus, 
                                        (int)obsId->writeToRm) 
                       == ERROR )
                  {
                     ERROR_LOG (
                           "Failed to run fast guide and focus correction");
                  };

                  /*if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                       obsId->aoCcdId->xPixels, 
                                       obsId->aoCcdId->yPixels) == ERROR )
                  {
                     ERROR_LOG ("Failed to subtract DARK from current frame");
                  }*/

#ifdef DEBUG
                  printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                          obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif

                  if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                            obsId->aoCtrlId, nCoadds) == ERROR )
                  {
                     ERROR_LOG ("Failed to average images");
                  }

                  /*
                   * Increment the coadd counter and when it reaches nCoadds 
                   * save the coadded data to disk. Coadded data are only 
                   * saved once per observation.
                   */

                  obsId->coaddCounter++;
#ifdef DEBUG
                  printf ( "obsId->coaddCounter=%d\n" , obsId->coaddCounter);
#endif
                  if ( obsId->coaddCounter == nCoadds )
                  {
                     if ( aoThresholdPerSubapCompute (obsId->aoCtrlId->sumVect,
                                              obsId->aoCcdId, obsId->aoCtrlId, 
                                              obsId->rateBrightPixThreshComp,
                                              obsId->aoCtrlId->thresholdVect) 
                                              == ERROR )
                     {
                       ERROR_LOG ("Failed to subtract DARK from current frame");
                     }

                     if (epToVxPipeWrite (NULL, 
                           (char *)(int)& (obsId->aoCtrlId->threshold), 
                           obsId->pAoThreshContext) == ERROR)
                     {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
                     }
                  }
               }
               else if ( obsId->methodThreshComp == AO_THRESH_NOSPOTS )
               {
                  /*
                   * Without spots -> rms of a frame * multCoeff 
                   */

                  nCoadds = (int) obsId->nAverageDataThreshComp;

                  if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                       obsId->aoCcdId->xPixels, 
                                       obsId->aoCcdId->yPixels) == ERROR )
                  {
                     ERROR_LOG ("Failed to subtract DARK from current frame");
                  }

                  if ( aoRmsNoiseImageCompute (pImage,
                                               obsId->aoCcdId, &rms, &mean) 
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to compute mean, rms of current frame");
                  }

                  obsId->averageRms += rms;
                  obsId->averageMean += mean;
                  obsId->coaddCounter ++;
                  
                  if ( obsId->coaddCounter == nCoadds )
                  {
                     obsId->averageRms /= nCoadds;
                     obsId->averageMean /= nCoadds;

                     obsId->aoCtrlId->rms = obsId->averageRms;

                     obsId->aoCtrlId->threshold = obsId->averageMean +
                     obsId->multCoeffRmsThreshComp * obsId->averageRms ;

                     if ( obsId->aoCcdId->binningFlag == FALSE )
                     {
                        obsId->aoCtrlId->rmsDarkFull =
                        obsId->aoCtrlId->rms ;
                        obsId->aoCtrlId->thresholdDarkFull =
                        obsId->aoCtrlId->threshold ;
                     }
                     else
                     {
                        obsId->aoCtrlId->rmsDarkBin =
                        obsId->aoCtrlId->rms ;
                        obsId->aoCtrlId->thresholdDarkBin =
                        obsId->aoCtrlId->threshold ;
                     }

                     for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                         obsId->aoCtrlId->thresholdVect[k] =
                         obsId->aoCtrlId->threshold;

                     if (epToVxPipeWrite (NULL, 
                           (char *)(int)& (obsId->aoCtrlId->rms), 
                           obsId->pAoRmsContext) == ERROR)
                     {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_RMS_SIR_NAME record");
                     }

                     if (epToVxPipeWrite (NULL, 
                           (char *)(int)& (obsId->aoCtrlId->threshold), 
                           obsId->pAoThreshContext) == ERROR)
                     {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
                     }

                  }
               }
               else
               {
                  /*
                   * A value has been selected - nothing to do
                   */
                  if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                       obsId->aoCcdId->xPixels, 
                                       obsId->aoCcdId->yPixels) == ERROR )
                  {
                     ERROR_LOG ("Failed to subtract DARK from current frame");
                  }
               }    
            break;

            case (AO_MODE_AO):

               /*
                * aO correction mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf ("aoDarkSubtract: %p %p %d %d\n", 
                       pImage, obsId->aoCtrlId->darkVect, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels);
#endif
               if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                    obsId->aoCcdId->xPixels, 
                                    obsId->aoCcdId->yPixels) == ERROR )
               {
                  ERROR_LOG ("Failed to subtract DARK from current frame");
               }

#ifdef DEBUG
               printf ("aoModeCompute (%p, %p, %p, %d, %p, %p)\n",
                       pImage, obsId->aoCcdId, obsId->aoCtrlId, nCoadds, 
                       pThresh, obsId->aoCbAoCtrlId);
#endif
               if ( obsId->updateAoScale == TRUE )
               {
                  if ( aoScaleUpdate (obsId->aoScaleVect, obsId->aoCtrlId) 
                       == ERROR )
                  {
                     ERROR_SET (0, "Failed to update aO scale factors",
                                ERROR_LOG_NOW);
                  }
                  obsId->updateAoScale = FALSE ;
               } ;

               if ( aoModeCompute (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                   nCoadds, pThresh, obsId->aoCbAoCtrlId) 
                    == ERROR )
               {
                  ERROR_LOG ("Failed to aO correction");
               }

            break;

            case (AO_MODE_GG_AO):

               /*
                * Global guide and aO correction mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf ("aoGlobalGuide (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
                       pImage, obsId->aoCcdId, obsId->aoCtrlId, pTotal, 
                       pGuides, pFg, pFgAfterRot, pErrorsFg, pTime, pWfsStatus,
                       (int)obsId->writeToRm);
               printf ("aoModeCompute (%p, %p, %p, %d, %p, %p)\n",
                       pImage, obsId->aoCcdId, obsId->aoCtrlId, nCoadds, 
                       pThresh, obsId->aoCbAoCtrlId);
#endif

               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( obsId->updateAoScale == TRUE )
               {
                  if ( aoScaleUpdate (obsId->aoScaleVect, obsId->aoCtrlId) 
                       == ERROR )
                  {
                     ERROR_SET (0, "Failed to update aO scale factors",
                                ERROR_LOG_NOW);
                  }
                  obsId->updateAoScale = FALSE ;
               } ;

               if ( aoGlobalGuide (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                   pTotal, pGuides, pFg, pFgAfterRot, 
                                   pErrorsFg, pTime, pWfsStatus,
                                   (int)obsId->writeToRm) == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide correction");
               }
               
               if ( aoModeCompute (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                   nCoadds, pThresh, obsId->aoCbAoCtrlId) 
                    == ERROR )
               {
                  ERROR_LOG ("Failed to aO correction");
               }

            break;

            case (AO_MODE_TOTAL):

               if ( obsId->methodFluxComp == AO_TOTAL_SPOTS ) 
               {
                  if ( obsId->coaddCounter < obsId->nFramesAverageFlux)
                  {
                     if ( obsId->updateFgScale == TRUE )
                     {
                        obsId->aoCtrlId->fgScaleFactorVect[0] = 
                        obsId->tipScale ;
                        obsId->aoCtrlId->fgScaleFactorVect[1] = 
                        obsId->tiltScale ;
                        obsId->aoCtrlId->fgScaleFactorVect[2] = 
                        obsId->focusScale ;
                        obsId->aoCtrlId->slidingFocusGain =
                        obsId->slidingFocusGain;
                        obsId->aoCtrlId->one_slidingFocusGain =
                        1.0 - obsId->slidingFocusGain ;

                        obsId->updateFgScale = FALSE ;
                     };

                     /*if ( aoGlobalGuide (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId, pTotal, pGuides, 
                                         pFg, pFgAfterRot, pErrorsFg, pTime, 
                                         pWfsStatus, (int) obsId->writeToRm) 
                         == ERROR )
                     {
                        ERROR_LOG ("Failed to run FG correction");
                     }*/

                     if ( aoGuideAndFocus (pImage, obsId->aoCcdId, 
                                           obsId->aoCtrlId, pPrevThresh,
                                           pTotal, pCentroids, pErrorCentroids,
                                           pFg, pFgAfterRot, pErrorsFg, pTime, 
                                           pWfsStatus, (int)obsId->writeToRm) 
                          == ERROR )
                     {
                        ERROR_LOG (
                              "Failed to run fast guide and focus correction");
                     };

                     obsId->averageFlux += *pFlux ;
                     obsId->coaddCounter ++;

                     if ( obsId->coaddCounter == obsId->nFramesAverageFlux)
                     {
                        obsId->averageFlux /= 
                        (double)obsId->nFramesAverageFlux;
                        obsId->aoCtrlId->averageTotal = obsId->averageFlux;
                        obsId->aoCtrlId->totalThreshold = 
                        obsId->averageFlux * obsId->multCoeffAverageFlux;
                        if (epToVxPipeWrite (NULL, 
                              (char *)(int)& (obsId->aoCtrlId->totalThreshold), 
                              obsId->pAoTotalContext) == ERROR)
                        {
                         ERROR_LOG (
                         "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
                        }
                     }
                  }
               }

            break;

            case (AO_MODE_FG_FOCUS):

               /*
                * Fast Guide and Focus mode.
                */
#ifdef DEBUG
               printf (
               "aoGuideAndFocus (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
               pImage, obsId->aoCcdId, obsId->aoCtrlId, pPrevThresh, pTotal, 
               pCentroids, pErrorCentroids, pFg, pFgAfterRot, pErrorsFg, pTime, 
               pWfsStatus, (int)obsId->writeToRm);
#endif
               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale ;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale ;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( aoGuideAndFocus (pImage, obsId->aoCcdId, obsId->aoCtrlId, 
                                     pPrevThresh, pTotal, pCentroids, 
                                     pErrorCentroids, pFg, pFgAfterRot, 
                                     pErrorsFg, pTime, pWfsStatus, 
                                     (int)obsId->writeToRm) == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide and focus correction");
               };

               if ( obsId->threshRealTimeFlag == TRUE )
               {
                  if ( aoThresholdPerSubapCompute (pImage, obsId->aoCcdId,
                                           obsId->aoCtrlId,
                                           obsId->rateBrightPixThreshComp,
                                           pThresh) == ERROR )
                  {
                     ERROR_LOG ("Failed to compute threshold per sub-aperture");
                  }
               }

            break;

            case (AO_MODE_FG_FOCUS_COADD):

               /*
                * Fast Guide Focus and Coadd mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf (
               "aoGuideAndFocus (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
               pImage, obsId->aoCcdId, obsId->aoCtrlId, pPrevThresh, pTotal,
               pCentroids, pErrorCentroids, pFg, pFgAfterRot, pErrorsFg, pTime,
               pWfsStatus, (int)obsId->writeToRm);

#endif
               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( aoGuideAndFocus (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                     pPrevThresh, pTotal, pCentroids, 
                                     pErrorCentroids, pFg, pFgAfterRot, 
                                     pErrorsFg, pTime, pWfsStatus, 
                                     (int)obsId->writeToRm) == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide and focus correction");
               }

               if ( obsId->threshRealTimeFlag == TRUE )
               {
                  if ( aoThresholdPerSubapCompute (pImage, obsId->aoCcdId, 
                                           obsId->aoCtrlId,
                                           obsId->rateBrightPixThreshComp,
                                           pThresh) == ERROR )
                  {
                     ERROR_LOG ("Failed to compute threshold per sub-aperture");
                  }
               }
#ifdef DEBUG
               printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                       obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif
               if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId, nCoadds) == ERROR )
               {
                  ERROR_LOG ("Failed to average images");
               }

               /*
                * Increment the coadd counter and when it reaches nCoadds 
                * save the coadded data to disk. Coadded data are only 
                * saved once per observation.
                */

               obsId->coaddCounter++;
               if ( obsId->coaddCounter == nCoadds )
               {
                  /*
                   * Make up a file name by adding the string ".coadd.fits"
                   * to the given file name. Use a default file name if one 
                   * has not been given.
                   */

                  if ( strcmp(obsId->pCoaddFileName, "") == 0 )
                  {
                     strcpy ( pFileNameString, "coadd.fits" );
                  }
                  else
                  {
                     sprintf( pFileNameString, "%s.fits", 
                              obsId->pCoaddFileName );
                  }

                  MESSAGE_LOG1 (MSG_MINDEBUG, 
                  "Saving coadded data to %s", pFileNameString);

                  if ( detWriteFits (pFileNameString, obsId, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                       obsId->aoCtrlId->sumVect) == ERROR )
                  {
                     ERROR_LOG ("Failed to save coadded data to disk");
                  }

                  MESSAGE_LOG (MSG_MINDEBUG,
                  "Analyze centroids and ao modes of the coadded data");

                  if ( aoModeAnalyze (obsId->aoCtrlId->sumVect, obsId->aoCcdId, 
                                      obsId->aoCtrlId, pThresh, 
                                      obsId->aoCbAoCtrlId) 
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to analyze coadded data");
                  }
               }
            break;

            case (AO_MODE_MEAS_IM):

               /*
                * Interaction matrix measurement mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf (
               "aoGuideAndFocus (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
               pImage, obsId->aoCcdId, obsId->aoCtrlId, pPrevThresh, pTotal,
               pCentroids, pErrorCentroids, pFg, pFgAfterRot, pErrorsFg, pTime,
               pWfsStatus, (int)obsId->writeToRm);
#endif
               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( aoGuideAndFocus (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                     pPrevThresh, pTotal, pCentroids, 
                                     pErrorCentroids, pFg, pFgAfterRot, 
                                     pErrorsFg, pTime, pWfsStatus, 
                                     (int)obsId->writeToRm) == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide and focus correction");
               }
#ifdef DEBUG
               printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                       obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif
               if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId, nCoadds) == ERROR )
               {
                  ERROR_LOG ("Failed to average images");
               }

               /*
                * Increment the coadd counter and when it reaches nCoadds 
                * save the coadded data to disk. Coadded data are only 
                * saved once per observation.
                */

               obsId->coaddCounter++;
               if ( obsId->coaddCounter == nCoadds )
               {
                  /*
                   * Make up a file name by adding the string ".coadd.fits"
                   * to the given file name. Use a default file name if one 
                   * has not been given.
                   */

                  if ( strcmp(obsId->pCoaddFileName, "") == 0 )
                  {
                     strcpy ( pFileNameString, "coadd.fits" );
                  }
                  else
                  {
                     sprintf( pFileNameString, "%s.fits", 
                              obsId->pCoaddFileName );
                  }

                  MESSAGE_LOG1 (MSG_MINDEBUG, 
                  "Saving coadded data to %s", pFileNameString);

                  if ( detWriteFits (pFileNameString, obsId, 
                       obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                       obsId->aoCtrlId->sumVect) == ERROR )
                  {
                     ERROR_LOG ("Failed to save coadded data to disk");
                  }

                  MESSAGE_LOG (MSG_MINDEBUG,
                  "Analyze centroids and ao modes of the coadded data");

                  if ( aoModeAnalyze (obsId->aoCtrlId->sumVect, obsId->aoCcdId, 
                                      obsId->aoCtrlId, pPrevThresh, 
                                      obsId->aoCbAoCtrlId) 
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to analyze coadded data");
                  }

                  if ( obsId->saveCentroids == TRUE )
                  {
                     indexCtrl = obsId->aoCbAoCtrlId->position;
                     if ( indexCtrl == 1)
                     {
                        pAoCentroids = 
                        obsId->aoCbAoCtrlId->cbAoCtrlRecord[0].centroidsVect;
                        
                        i = obsId->nMode;
                        if ( obsId->amplitude > 0.0 )
                        {
                         for ( j = 0 ; j < obsId->aoCcdId->centroidsNb ; j ++)
                         obsId->aoCtrlId->aoIntMatStruct[i].posCentroidsVect[j]=
                         *(pAoCentroids + j);
                        }
                        else
                        {
                         for ( j = 0 ; j < obsId->aoCcdId->centroidsNb ; j ++)
                         obsId->aoCtrlId->aoIntMatStruct[i].negCentroidsVect[j]=
                         *(pAoCentroids + j);
                        }

                        aoCentroidsWrite (obsId->pCentFileName, pAoCentroids, 
                                          obsId->aoCcdId->centroidsNb, 
                                          obsId->pCentComment);
                     }
                     else
                     {
                        ERROR_SET1 ( 0, 
                        "Save col int mat: bad position of CB CTRL: %d",  
                        ERROR_LOG_NOW, indexCtrl);
                     }
                  }
               }
            break;

            case (AO_MODE_FG_FOCUS_AO):

               /*
                * Fast guide and focus and aO correction mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
               printf (
               "aoGuideAndFocus (%p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %d)\n",
               pImage, obsId->aoCcdId, obsId->aoCtrlId, pPrevThresh, pTotal, 
               pCentroids, pErrorCentroids, pFg, pFgAfterRot, pErrorsFg, pTime, 
               pWfsStatus, (int)obsId->writeToRm);
               printf ("aoModeCompute (%p, %p, %p, %d, %p, %p)\n",
                       pImage, obsId->aoCcdId, obsId->aoCtrlId, nCoadds, 
                       pThresh, obsId->aoCbAoCtrlId);
#endif

               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( obsId->updateAoScale == TRUE )
               {
                  if ( aoScaleUpdate (obsId->aoScaleVect, obsId->aoCtrlId) 
                       == ERROR )
                  {
                     ERROR_SET (0, "Failed to update aO scale factors",
                                ERROR_LOG_NOW);
                  }
                  obsId->updateAoScale = FALSE ;
               } ;

               if ( aoGuideAndFocus (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                     pPrevThresh, pTotal, pCentroids, 
                                     pErrorCentroids, pFg, pFgAfterRot, 
                                     pErrorsFg, pTime, pWfsStatus,
                                     (int)obsId->writeToRm) 
                    == ERROR )
               {
                  ERROR_LOG ("Failed to run fast guide and focus correction");
               }
               
               if ( obsId->threshRealTimeFlag == TRUE )
               {
                  if ( aoThresholdPerSubapCompute (pImage, obsId->aoCcdId, 
                                           obsId->aoCtrlId,
                                           obsId->rateBrightPixThreshComp,
                                           pThresh) == ERROR )
                  {
                     ERROR_LOG ("Failed to compute threshold per sub-aperture");
                  }
               }

               if ( aoModeCompute (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                   nCoadds, pThresh, obsId->aoCbAoCtrlId) 
                    == ERROR )
               {
                  ERROR_LOG ("Failed to aO correction");
               }

            break;

            case (AO_MODE_SEQ_DARK):

               /*
                * Sequence dark mode.
                */

               nCoadds = (int) obsId->nCoaddFrames;

               if ( obsId->coaddCounter < nCoadds )
               {
#ifdef DEBUG
                  printf ("aoImageFloatAverage: %p %p %p %d\n", pImage,
                          obsId->aoCcdId, obsId->aoCtrlId, nCoadds);
#endif
                  if ( aoImageFloatAverage (pImage, obsId->aoCcdId, 
                                            obsId->aoCtrlId, nCoadds) == ERROR )
                  {
                     ERROR_LOG ("Failed to average images");
                  }

                  /*
                   * Increment the coadd counter and when it reaches nCoadds 
                   * save the coadded data to disk. Coadded data are only 
                   * saved once per observation.
                   */

                  obsId->coaddCounter++;
#ifdef DEBUG
                  printf ( "obsId->coaddCounter=%d\n" , obsId->coaddCounter);
#endif
                  if ( obsId->coaddCounter == nCoadds )
                  {
                     /*
                      * Make up a file name by adding the string ".fits" to 
                      * the given file name. Use a default file name if one 
                      * has not been given.
                      */

                      if ( strcmp(obsId->pCoaddFileName, "") == 0 )
                      {
                         strcpy ( pFileNameString, "dark.fits" );
                      }
                      else
                      {
                         sprintf( pFileNameString, "%s.fits", 
                                  obsId->pCoaddFileName );
                      }

                      MESSAGE_LOG1 (MSG_MINDEBUG, 
                      "Saving dark data to %s", pFileNameString);

                      if ( detWriteFits (pFileNameString, obsId, 
                           obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
                           obsId->aoCtrlId->sumVect) == ERROR )
                      {
                         ERROR_LOG ("Failed to save coadded data to disk");
                      }

                      if ( aoDarkUpdate ( pFileNameString, obsId->aoCcdId,
                                          obsId->aoCtrlId ) == ERROR )
                      {
                         ERROR_LOG ("Failed to load new dark" );
                      }

                      if (epToVxPipeWrite (NULL, pFileNameString, 
                                           obsId->pAoDarkInitContext) == ERROR)
                      {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME rec");
                      }
                   }
               }
               
 
               if ( obsId->coaddCounter >= nCoadds )
               {
                  if ( aoDarkSubtract (pImage, obsId->aoCtrlId->darkVect,
                                       obsId->aoCcdId->xPixels,
                                       obsId->aoCcdId->yPixels) == ERROR )
                  {
                     ERROR_LOG ("Failed to subtract DARK from current frame");
                  }

                  if ( aoRmsNoiseImageCompute (pImage,
                                               obsId->aoCcdId, &rms, &mean) 
                     == ERROR )
                  {
                     ERROR_LOG ("Failed to compute mean, rms of current frame");
                  }

                  obsId->averageRms += rms;
                  obsId->averageMean += mean;
                  obsId->coaddCounter ++;

                  if ( obsId->coaddCounter == 
                       nCoadds + obsId->nAverageDataThreshComp)
                  {
                     printf ( "image %d\n", obsId->coaddCounter);
                     obsId->averageRms /= obsId->nAverageDataThreshComp;
                     obsId->averageMean /= obsId->nAverageDataThreshComp;

                     obsId->aoCtrlId->rms = obsId->averageRms;

                     obsId->aoCtrlId->threshold = obsId->averageMean +
                     obsId->multCoeffRmsThreshComp * obsId->averageRms ;

                     if ( obsId->aoCcdId->binningFlag == FALSE )
                     {
                        obsId->aoCtrlId->rmsDarkFull = obsId->aoCtrlId->rms ;
                        obsId->aoCtrlId->thresholdDarkFull =
                        obsId->aoCtrlId->threshold ;
                     }
                     else
                     {
                        obsId->aoCtrlId->rmsDarkBin = obsId->aoCtrlId->rms ;
                        obsId->aoCtrlId->thresholdDarkBin =
                        obsId->aoCtrlId->threshold ;
                     }

                     for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
                         obsId->aoCtrlId->thresholdVect[k] =
                         obsId->aoCtrlId->threshold;

                     if (epToVxPipeWrite (NULL,
                           (char *)(int)& (obsId->aoCtrlId->rms),
                           obsId->pAoRmsContext) == ERROR)
                     {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_RMS_SIR_NAME record");
                     }

                     if (epToVxPipeWrite (NULL,
                           (char *)(int)& (obsId->aoCtrlId->threshold),
                           obsId->pAoThreshContext) == ERROR)
                     {
                        ERROR_LOG (
                        "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
                     }
                  }
               }

            break;

            case (AO_MODE_CLOSED_LOOP):
                
               nCoadds = (int) obsId->nCoaddFrames;

               if ( obsId->updateFgScale == TRUE )
               {
                  obsId->aoCtrlId->fgScaleFactorVect[0] = obsId->tipScale;
                  obsId->aoCtrlId->fgScaleFactorVect[1] = obsId->tiltScale;
                  obsId->aoCtrlId->fgScaleFactorVect[2] = obsId->focusScale ;
                  obsId->aoCtrlId->slidingFocusGain =
                  obsId->slidingFocusGain;
                  obsId->aoCtrlId->one_slidingFocusGain =
                  1.0 - obsId->slidingFocusGain ;

                  obsId->updateFgScale = FALSE ;
               };

               if ( obsId->updateAoScale == TRUE )
               {
                  if ( aoScaleUpdate (obsId->aoScaleVect, obsId->aoCtrlId) 
                       == ERROR )
                  {
                     ERROR_SET (0, "Failed to update aO scale factors",
                                ERROR_LOG_NOW);
                  }
                  obsId->updateAoScale = FALSE ;
               } ;

               if ( (obsId->ggFrame != 0) && 
                    (obsId->coaddCounter < obsId->ggFrame) )
               {
                  /*printf ( "coaddCounter =%d fast guide only\n", 
                           obsId->coaddCounter );*/
    
                  if ( aoGlobalGuide (pImage, obsId->aoCcdId, obsId->aoCtrlId,
                                      pTotal, pGuides, pFg, pFgAfterRot, 
                                      pErrorsFg, pTime, pWfsStatus, 
                                      (int)obsId->writeToRm) == ERROR )
                  {
                     ERROR_LOG ("Failed to run fast guide correction");
                  }
                  obsId->coaddCounter ++;
               }
               else if ( (obsId->threshRealTimeFlag == FALSE) && 
                         (obsId->threshFlag == TRUE) &&
                         (obsId->coaddCounter < obsId->nAverageDataThreshComp +
                                                obsId->ggFrame) )
               {
                  /* printf ( "coaddCounter =%d compute thresh \n",
                           obsId->coaddCounter );*/

                  if ( aoGuideAndFocus (pImage, obsId->aoCcdId, 
                                        obsId->aoCtrlId, pPrevThresh, pTotal, 
                                        pCentroids, pErrorCentroids, 
                                        pFg, pFgAfterRot, pErrorsFg, pTime, 
                                        pWfsStatus, (int) obsId->writeToRm) 
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to run FG correction");
                  }

                  if ( aoImageFloatAverage (pImage, obsId->aoCcdId,
                                            obsId->aoCtrlId,
                                            obsId->nAverageDataThreshComp)
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to average images");
                  }
                  obsId->coaddCounter ++;

                  if ( obsId->coaddCounter ==
                       (obsId->nAverageDataThreshComp + obsId->ggFrame) )
                  {
                     if ( aoThresholdPerSubapCompute (obsId->aoCtrlId->sumVect,
                                              obsId->aoCcdId,
                                              obsId->aoCtrlId,
                                              obsId->rateBrightPixThreshComp,
                                              obsId->aoCtrlId->thresholdVect)
                          == ERROR )
                     {
                        ERROR_LOG ("Failed to compute threshold") ;
                     }
                     if (epToVxPipeWrite (NULL,
                                  (char *)(int)& (obsId->aoCtrlId->threshold),
                                  obsId->pAoThreshContext) == ERROR)
                     {
                        ERROR_LOG (
                               "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME");
                     }
                  }
               }
               else if ( (obsId->threshRealTimeFlag == FALSE ) &&
                         (obsId->averageFluxFlag == TRUE) &&
                         (obsId->coaddCounter < obsId->nFramesAverageFlux +
                          obsId->nAverageDataThreshComp + obsId->ggFrame) )
               {
                  /*printf ( "coaddCounter =%d compute total \n", 
                           obsId->coaddCounter );*/
                  if ( aoGuideAndFocus (pImage, obsId->aoCcdId, 
                                        obsId->aoCtrlId, pPrevThresh, pTotal, 
                                        pCentroids, pErrorCentroids, pFg, 
                                        pFgAfterRot, pErrorsFg, pTime, 
                                        pWfsStatus, (int)obsId->writeToRm) 
                       == ERROR )
                  {
                     ERROR_LOG ("Failed to run FG correction");
                  }
                  obsId->averageFlux += *pFlux ;
                  obsId->coaddCounter ++;

                  if ( obsId->coaddCounter == (obsId->nFramesAverageFlux+
                       obsId->nAverageDataThreshComp + obsId->ggFrame) )
                  {
                     obsId->averageFlux /= 
                     (double)obsId->nFramesAverageFlux;
                     obsId->aoCtrlId->averageTotal = obsId->averageFlux;
                     obsId->aoCtrlId->totalThreshold = 
                     obsId->averageFlux * obsId->multCoeffAverageFlux;
                     if (epToVxPipeWrite (NULL, 
                              (char *)(int)& (obsId->aoCtrlId->totalThreshold), 
                              obsId->pAoTotalContext) == ERROR)
                     {
                        ERROR_LOG ( "Failed to init AO_TOTAL_SIR_NAME record");
                     }
                     /*printf ( "coaddCounter =%d total =%f \n", 
                     obsId->coaddCounter,obsId->averageFlux );*/
                  }
               }
               else
               {
                  /*printf ( "coaddCounter =%d ao guide \n", 
                        obsId->coaddCounter );*/

                  if ( aoGuideAndFocus (pImage, obsId->aoCcdId, 
                                        obsId->aoCtrlId, pPrevThresh, pTotal, 
                                        pCentroids, pErrorCentroids, pFg, 
                                        pFgAfterRot, pErrorsFg, pTime, 
                                        pWfsStatus, (int)obsId->writeToRm) 
                                        == ERROR )
                  {
                     ERROR_LOG ("Failed to run FG correction");
                  }

                  if ( obsId->threshRealTimeFlag == TRUE )
                  {
                     if ( aoThresholdPerSubapCompute (pImage, obsId->aoCcdId,
                                              obsId->aoCtrlId,
                                              obsId->rateBrightPixThreshComp,
                                              pThresh) == ERROR )
                     {
                        ERROR_LOG (
                             "Failed to compute threshold per sub-aperture");
                     }
                  }

                  if ( obsId->aoFlag == TRUE )
                  {
                     if ( aoModeCompute (pImage, obsId->aoCcdId, 
                                         obsId->aoCtrlId,
                                         nCoadds, pThresh, obsId->aoCbAoCtrlId) 
                          == ERROR )
                     {
                        ERROR_LOG ("Failed to aO correction");
                     }
                  }
               }

               if ( obsId->saveCbFgCtrlClosedLoop == TRUE )
               {
                  obsId->saveFgCbCounter ++;
                  if (obsId->saveFgCbCounter == 
                      obsId->saveCbFgCtrlClosedLoopFrame)
                  {
                     if ( aoCbFgCtrlSave (obsId->pCbPathSeq, obsId->aoCcdId, 
                                          obsId->aoCtrlId, obsId->aoCbFgCtrlId) 
                          == ERROR )
                     {
                        ERROR_LOG ("Failed to save FG control CB\n" );
                     }
                     obsId->saveFgCbCounter = 0;
                  }
               }

               if ( obsId->saveCbAoCtrlClosedLoop == TRUE )
               {
                  obsId->saveAoCbCounter ++;
                  if (obsId->saveAoCbCounter == 
                      obsId->saveCbAoCtrlClosedLoopFrame)
                  {
                     if ( aoCbAoCtrlSave (obsId->pCbPathSeq, obsId->aoCcdId, 
                                          obsId->aoCtrlId, obsId->aoCbAoCtrlId) 
                          == ERROR )
                     {
                        ERROR_LOG ("Failed to save aO control CB\n" );
                     }
                     obsId->saveAoCbCounter = 0;
                  }
               }
               
            break;

            default:
#ifdef DEBUG
               printf ("Signal processing switched off\n");
#endif
            break;
         }
      }

      /*
       * Send the data to the DHS, store it to disk or do nothing, 
       * as appropriate
       */

      if ( (obsId->outOptions == 1) && 
           ((obsId->dhsCounter % obsId->dhsQlRate) == 0) )
      {
         /*printf ( "display frame, obsId->dhsCounter=%d\n", obsId->dhsCounter);*/
         /*
          * Convert the time stamps from Gemini raw time into Universal Time
          * and construct these into character strings.
          */

         if (timeThenC( obsId->rawtEnd, UT1, 2, obsId->timeArrayEnd ) != OK)
         {
            ERROR_SET (0,
            "Failed to convert time stamp at observation end to date/time",
            ERROR_LOG_NOW);
         }

         sprintf (obsId->utEndString, "%04d-%02d-%02d:%02d:%02d:%02d",
                  obsId->timeArrayEnd[0], obsId->timeArrayEnd[1], 
                  obsId->timeArrayEnd[2], obsId->timeArrayEnd[3], 
                  obsId->timeArrayEnd[4], obsId->timeArrayEnd[5]);

         /*
          * Compute the elapsed time
          */

         elapsed = obsId->rawtEnd - obsId->rawtStart ;
         if (epToVxPipeWrite( NULL, (char *)obsId->utEndString, 
                              obsId->pUTendContext) == ERROR)
         {
            ERROR_LOG ("Failed to set UT at end of observation SIR record");
         }

         if (epToVxPipeWrite( NULL, (char *)(int)&elapsed, 
                              obsId->pElapsedContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set elapsed time SIR record");
         }

         /*if ( obsId->stopped != TRUE )
            semGive ( detDhsStartSem);*/

         MESSAGE_LOG (MSG_MINDEBUG, "Sending data to DHS...");

         /* Copy the image into pCurFrame */

         imageSize = obsId->aoCcdId->pixelsNb;
         pMax = (float *)((int)pImage + imageSize*sizeof(float));
         pc = obsId->pCurFrame ;

         for ( pi = pImage ; pi < pMax ; pi ++ )
             *(pc ++) = *pi;

         if ( obsId->totalFrames == 1 )
         {
            dhsBdAttribAdd (obsId->dhsDataFrame, "utend", DHS_DT_STRING,
                            0, NULL, obsId->utEndString, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }

#ifdef DEBUG
         dhsBdDsPrint (obsId->dhsDataset, &dhsErrno);
         CHECK_DHS (dhsErrno);
#endif

         /* Send the data to the dhs */
#ifdef DEBUG
         printf (
       "detObserveEnd: dhsBdPut, dhsConnection=%d, pDataLabel=%s, dataset=%d\n",
         detDhsConnection, obsId->pDataLabel, (int) obsId->dhsDataset);
#endif 

         if ( obsId->dhsOutOptions == 2 ) /* QL only */
         {
            if ( obsId->totalFrames == 1 )
               putTag = dhsBdPut (detDhsConnection, obsId->pDataLabel,
                                  DHS_BD_PT_DS_QL, DHS_TRUE,
                                  obsId->dhsDataset, NULL, &dhsErrno);
            else
               putTag = dhsBdPut (detDhsConnection, obsId->pDataLabel,
                                  DHS_BD_PT_DS_QL, DHS_FALSE,
                                  obsId->dhsDataset, NULL, &dhsErrno);
         }
         else
         {
            if ( obsId->totalFrames == 1 )
            {
               putTag =
               dhsBdPut (detDhsConnection, obsId->pDataLabel,
               DHS_BD_PT_DS, DHS_TRUE, obsId->dhsDataset, NULL, &dhsErrno);
            }
            else
            {
               putTag =
               dhsBdPut (detDhsConnection, obsId->pDataLabel,
                         DHS_BD_PT_DS, DHS_FALSE, obsId->dhsDataset, NULL,
                         &dhsErrno);
            }
         }

         CHECK_DHS (dhsErrno);

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR,
                        "Failed to initiate data transfer (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            goto ERROR_EXIT;
         }

         /* Wait for completion */

#ifdef DEBUG
         printf ("detObserveEnd: dhsWait putTag=%d ...\n", (int) putTag);
#endif 

         dhsWait (1, &putTag, &dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR,
                        "Error during wait for data transfer (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);

            dummyDhsErrno = DHS_S_SUCCESS;
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            goto ERROR_EXIT;
         }

#ifdef DEBUG
         printf ("detObserveEnd: detDhsCheckCmdStatus putTag=%d ...\n",
                 (int) putTag);
#endif 

         if ( detDhsCheckCmdStatus (putTag) == ERROR )
         {
            ERROR_SET (S_detControl_DHS_ERROR, "Data transfer failed",
                       ERROR_LOG_NOW);

            dummyDhsErrno = DHS_S_SUCCESS;
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            goto ERROR_EXIT;
         }

         /*
          * If the last frame has been received free the DHS dataset.
          */

#ifdef DEBUG
         printf ("detObserveEnd: dhsTagFree putTag=%d ...\n", (int) putTag);
#endif 

         dhsErrno = DHS_S_SUCCESS;
         dhsTagFree (putTag, &dhsErrno);

         if ( (frameCount == 1) || (obsId->stopped) )
         {
            dhsBdDsFree (obsId->dhsDataset, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }
      }
      else if ( obsId->outOptions == 2 )
      {
         /*
          * Convert the time stamps from Gemini raw time into Universal Time
          * and construct these into character strings.
          */

         if (timeThenC( obsId->rawtEnd, UT1, 2, obsId->timeArrayEnd ) != OK)
         {
            ERROR_SET (0,
            "Failed to convert time stamp at observation end to date/time",
            ERROR_LOG_NOW);
         }

         sprintf (obsId->utEndString, "%04d-%02d-%02d:%02d:%02d:%02d",
                  obsId->timeArrayEnd[0], obsId->timeArrayEnd[1], 
                  obsId->timeArrayEnd[2], obsId->timeArrayEnd[3], 
                  obsId->timeArrayEnd[4], obsId->timeArrayEnd[5]);

         elapsed = obsId->rawtEnd - obsId->rawtStart ;
         if (epToVxPipeWrite( NULL, (char *)obsId->utEndString, 
                              obsId->pUTendContext) == ERROR)
         {
            ERROR_LOG ("Failed to set UT at end of observation SIR record");
         }

         if (epToVxPipeWrite( NULL, (char *)(int)&elapsed, 
                              obsId->pElapsedContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set elapsed time SIR record");
         }

         /*
          * The DHS is not being used and the data will be saved to FITS files.
          * If this is the first frame of the observation the standard names 
          * will be used.
          * Frames 2 onwards have .2, .3, etc... appended to the names.
          */

         if ( obsId->totalFrames != 1 )
         {
            sprintf( pFileNameString, "%s.%d.fits", obsId->pOutFileName,
                     obsId->outNFrames );
         }
         else
         {
            sprintf( pFileNameString, "%s.fits", obsId->pOutFileName ); 
         }

         MESSAGE_LOG2 (MSG_MINDEBUG, 
         "Saving unscrambled data from %p to directly to file \"%s\"...",
         pImage, pFileNameString);

         if (detWriteFits (pFileNameString, obsId, obsId->aoCcdId->xPixels, 
             obsId->aoCcdId->yPixels, pImage)
             == ERROR)
         {
            ERROR_LOG ("Failed to write FITS file");
            goto ERROR_EXIT;
         }

         MESSAGE_LOG (MSG_MINDEBUG, "... file saved ok");
      }

      /* If obsId->totalFrames > 1 and obsId->outNFrames = obsId->totalFrames */
      /* stop the observation */

#ifdef DEBUG
      printf ( "detObserveEnd : ouNFrames = %d, totalFrames = %d\n" ,
               obsId->outNFrames , obsId->totalFrames ) ;
#endif

      if ( (obsId->totalFrames > 1) && 
           (obsId->outNFrames == obsId->totalFrames) )
         obsId->stopped = TRUE ;

      /* Update the cicular buffers */
 
      if ( ++ obsId->aoCbImId->position == CB_IM_RECORD_NB )
      {
         obsId->aoCbImId->position = 0 ;
         obsId->aoCbImId->counter ++ ;
      }

      if ( (obsId->sigMode == AO_MODE_TOTAL) ||
           (obsId->sigMode == AO_MODE_GG) ||
           (obsId->sigMode == AO_MODE_GG_COADD) ||
           (obsId->sigMode == AO_MODE_FG_FOCUS) ||
           (obsId->sigMode == AO_MODE_FG_FOCUS_COADD) ||
           (obsId->sigMode == AO_MODE_MEAS_IM) ||
           (obsId->sigMode == AO_MODE_GG_AO) ||
           (obsId->sigMode == AO_MODE_FG_FOCUS_AO) ||
           (obsId->sigMode == AO_MODE_CLOSED_LOOP) ||
           ((obsId->sigMode == AO_MODE_THRESH) && 
            (obsId->methodThreshComp == AO_THRESH_SPOTS)) )
      {
         if ( ++ obsId->aoCbFgCtrlId->position == CB_FG_CTRL_RECORD_NB )
         {
            obsId->aoCbFgCtrlId->position = 0 ;
            obsId->aoCbFgCtrlId->counter ++ ;
         }
      }
   }

   /*
    * Abort any further readouts if the observation was stopped prematurely.
    */

   if ( obsId->stopped )
   {
      /* add 27 sept 99 for slow stop pb */
      printf ( "detObserveEnd() -> sdsuReadoutAbort()\n" ) ;
      obsAlreadyAborted = TRUE;
      if (sdsuReadoutAbort (sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to abort readouts on receipt of STOP instruction");
         goto ERROR_EXIT;
      }
      /* add 27 sept 99 for slow stop pb */
      printf ( "detObserveEnd() -> sdsuReadoutAbort() done \n" ) ;
   }

   /*
    * If the last frame has been received, set the observing flag FALSE
    * and set the observeC CAR record to IDLE via the "observing" record.
    * Otherwise set a timeout on the receipt of the next frame.
    */

   if ( (frameCount == 1) || (obsId->stopped) )
   {
      if ( sdsuId->frameErrors <= 0 )
      {
         MESSAGE_LOG1 (MSG_LOG, 
                       "Observation completed successfully, frames lost: %d", 
                       sdsuFrameLost);
      }
      else if ( sdsuId->frameErrors < obsId->nframes )
      {
         MESSAGE_LOG2 (MSG_WARNING, 
         "Observation completed with %d frames lost and %d frames with error",
         sdsuFrameLost , sdsuId->frameErrors);
      }
      else
      {
         ERROR_LOG ("Observation failed - all frames lost");
         goto ERROR_EXIT;
      }

      /* Finally, save the circular buffers */

      if ( obsId->saveCbIm == TRUE )
      {
         if ( aoCbImSave (obsId->pCbPath, obsId->aoCcdId, obsId->aoCtrlId, 
                          obsId->aoCbImId) == ERROR )
         {
            ERROR_LOG ("Failed to save image circular buffer\n" ) ;
         }
      }

      if ( obsId->saveCbAoCtrl == TRUE )
      {
         if ( aoCbAoCtrlSave (obsId->pCbPath, obsId->aoCcdId, obsId->aoCtrlId, 
                            obsId->aoCbAoCtrlId) == ERROR )
         {
            ERROR_LOG ("Failed to save control circular buffer\n" ) ;
         }
      }

      if ( obsId->saveCbFgCtrl == TRUE )
      {
         if ( aoCbFgCtrlSave (obsId->pCbPath, obsId->aoCcdId, obsId->aoCtrlId, 
                              obsId->aoCbFgCtrlId) == ERROR )
         {
            ERROR_LOG ("Failed to save FG control circular buffer\n" ) ;
         }
      }

      /* Reset the observing flag */

      obsId->observing = FALSE;
      observingState = menuCarstatesIDLE;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to IDLE");
      }
      measuringState = 0;
      if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                           obsId->pDetMeasuringContext) == ERROR)
      {
         ERROR_LOG ("Failed to set measuring flag to 0");
      }
   }
   else
   {
#ifdef DEBUG
      printf (
      "detObserveEnd: Further frames are anticipated - obs. not finished.\n");
#endif

      /*
       * Start an alarm timer which will trigger if the frame sync callback 
       * never runs.
       * Set the delay time to the readout timeout plus the largest frame 
       * exposure time obtained earlier.
       *
       * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
       */


      if ( sdsuId->simulate )
      {
         readoutTimeout = 5.0;
         if ( obsId->exposed >= obsId->exposedRQ )
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposed;
            }
         }
         else
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposedRQ/(double) obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposedRQ;
            }
         }

         if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, 
                               detObserveTimeout, (int) obsId) == ERROR )
         {
            ERROR_LOG ("Failed to set alarm timer");
         }
      }
   }

   return;


ERROR_EXIT:

   /*
    * If an error occurred, abort the observation, release the SDSU frame 
    * buffer (if necessary)
    * set the observing flag FALSE and set the observeC CAR record to ERROR,
    * via the "observing" record.
    */

   if ( !obsAlreadyAborted )
   {
      if (sdsuReadoutAbort (sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to abort readouts after error");
      }
      obsAlreadyAborted = TRUE;
   }

   obsId->observing = FALSE;
   observingState = menuCarstatesERROR;
   if (epToVxPipeWrite (NULL, (char *) &observingState, 
                        obsId->pDetObservingContext) == ERROR)
   {
      ERROR_LOG ("Failed to set observing flag to ERROR");
   }
   measuringState = 0;
   if (epToVxPipeWrite (NULL, (char *) &measuringState, 
                        obsId->pDetMeasuringContext) == ERROR)
   {
      ERROR_LOG ("Failed to set measuring flag to 0");
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveTimeout
 *
 *   INVOCATION:
 *   detObserveTimeout (timeId, obsIdInt)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) timeId   (timer_t) Timer ID
 *   (>) obsIdInt (int)     Pointer to observation definition, cast to integer
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Handle an observation timeout.
 *
 *   DESCRIPTION:
 *   Handle the situation when a readout does not complete within the time
 *   when its exposure and readout should have finished.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   UNFINISHED
 *-
 */

void detObserveTimeout
   (
   timer_t     timeId,             /* Timer ID.                               */
   int         obsIdInt            /* Pointer to observation ID cast to int.  */
   )
{
   OBS_ID      obsId = (OBS_ID) obsIdInt;
   SDSU_ID     sdsuId = (SDSU_ID) obsId->sdsuId;

   char *      mainKeywords[] = {"NAXIS1", "NAXIS2", "SECTORS", "OSP_FSZ"};
                              /* Main FITS keywords to read from header.      */
   int         mainValues[4]; /* Values corresponding to main FITS keywords.  */

   /*
    * The following variables will be used to test additional header items in
    * a file of simulated data, but the check has not been implemented yet.
    */

   long        observingState; /* Observation status (busy or idle).          */
   long        measuringState; /* Measuring status (0 or 1).                  */
   int         simOption;      /* Simulation option.                          */

#ifdef DEBUG
   printf ("detObserveTimeout: Observation timed out.\n");
#endif

   /*
    * When sdsuLib is simulating this routine makes the frame look like it has
    * been read out properly by the controller, and calls the frame ISR.
    * Simulating the packet ISRs would be a bit tricky...
    */

   if (sdsuId->simulate)
   {
      SDSU_FRAME *pFrame = sdsuId->readFrame;

      pFrame->header.packetCount   = 0;
      pFrame->header.status        = 0;   /* No errors during readout */
      pFrame->header.parameterId   = 0;   /* Simulated parameter ID   */
      pFrame->header.frameCount    = 1;   /* Simulate just one frame  */

      if ( strcmp(obsId->pSimFileName, "NONE") == 0 )
      {

         /*
          * Simulate the data internally, writing the result to the current
          * SDSU frame. Use option 1 (a simple ramp) for large data frames and
          * option 2 (simulated Shack-Hartmann spots) for small data frames.
          */

         MESSAGE_LOG (MSG_LOG, "Simulating data internally");

         if ( (obsId->aoCcdId->xPixels > 256) || 
              (obsId->aoCcdId->yPixels > 256) )
         {
            simOption = 1;
         }
         else
         {
            simOption = 2;
         }

         if ( detSimulateData (obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels, 
                               simOption, pFrame) == ERROR )
         {
            ERROR_LOG ("Failed to simulate data");
         }
      }
      else
      {

         /*
          * Read simulated data from the specified file.
          * First check the contents of the file correspond to the actual SDSU
          * setup.
          */

         MESSAGE_LOG1 (MSG_LOG,
           "Reading simulated data from %s\n", obsId->pSimFileName);
         if (detReadFitsHeaderInt ( obsId->pSimFileName, 3, mainKeywords, 
                                    mainValues) == ERROR )
         {
            ERROR_SET (0, "Failed to read simulated data header",
                       ERROR_LOG_NOW);
         }

         if ( (mainValues[0] == obsId->aoCcdId->xPixels) &&
              (mainValues[1] == obsId->aoCcdId->yPixels) &&
              (mainValues[2] == obsId->aoCcdId->outputsNb)
            )
         {

            /*
             * The file is acceptable. Now read its contents.
             */

            MESSAGE_LOG (MSG_MINDEBUG, "Simulated data header looks OK");
            if (detReadFitsImageUint16 ((uint16 *)& (pFrame->pixel[0]),
                obsId->pSimFileName,
                (obsId->aoCcdId->xPixels)*(obsId->aoCcdId->yPixels)) == ERROR )
            {
               ERROR_SET (0, "Failed to read simulated data", ERROR_LOG_NOW);
            }
         }
         else
         {
            /*
             * The simulated data contained in the file does not match the
             * simulated data required.
             */

            ERROR_SET4 (S_detControl_BAD_FILE,
               "Required size is %d x %d, simulated data file contains %d x %d",
               ERROR_LOG_SAVE, obsId->aoCcdId->xPixels, obsId->aoCcdId->yPixels,
               mainValues[0], mainValues[1]);
            ERROR_SET2 (0,
            "%ld detector outputs are required, simulated data file assumes %d",
               ERROR_LOG_SAVE, obsId->aoCcdId->outputsNb, mainValues[2]);
            ERROR_LOG ("Mismatch between simulated data file and requirements");
         }
      }

      /* Simulate the packet count reaching the desired value. */

      pFrame->header.packetCount   = sdsuId->packetsPerFrame;

      /*
       * Simulate an SDSU frame sync interrupt. This should cause the
       * detObserveEnd callback to be executed.
       */

/* COMMENTED OUT - ONLY ANY USE WHEN USING INTERRUPTS.
      if ( sdsuSimulateSimpleSync(sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to simulate frame sync interrupt");
      }
*/
   }
   else if ( sdsuId->frameIntNum == 0 )
   {
      /*
       * The observation timed out with SDSU frame interrupts disabled.
       * Assume the data are available in the buffer and simulate an SDSU
       * frame sync interrupt.
       * This should cause the detObserveEnd callback to be executed.
       */

      MESSAGE_LOG (MSG_MINDEBUG,
                   "Observation time completed with interrupts disabled");

      if ( sdsuSimulateSimpleSync(sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to simulate frame sync interrupt");
      }
   }
   else
   {
      /* sysIntDisable(6); */                     /* DEBUG TEST */

      /*
       * The observation completion was supposed to have been signalled by an
       * interrupt and timed out.
       * Set the observing flag FALSE and set the observeC CAR record to ERROR,
       * via the "observing" record.
       */

      MESSAGE_LOG (MSG_WARNING,
      "Observation timed out - trying to read data anyway...");

      obsId->observing = FALSE;
      observingState = menuCarstatesERROR;
      if (epToVxPipeWrite (NULL, (char *) &observingState,
          obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to ERROR");
      }

      measuringState = 0;
      if (epToVxPipeWrite (NULL, (char *) &measuringState,
          obsId->pDetMeasuringContext) == ERROR)
      {
         ERROR_LOG ("Failed to set measuring flag to ERROR");
      }

      /* Try and simulate a frame sync interrupt to force a data readout.
       * This may or may not work.
       */

      if ( sdsuSimulateSimpleSync (sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to simulate frame sync interrupt");
      }
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSimulateData
 *
 *   INVOCATION:
 *   detSimulateData (xPixels, yPixels, option, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels (const int)    Number of pixels in X
 *   (>) yPixels (const int)    Number of pixels in Y
 *   (>) option  (const int)    Simulation option
 *   (!) pFrame  (SDSU_FRAME *) Pointer to SDSU frame
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Fill frame buffer with simulated data (TEMPORARY FUNCTION)
 *
 *   DESCRIPTION:
 *   This function fills a frame buffer with simulated data with the following
 *   options:
 *
 *   Option 1 consists of an incrementing series. The first pixel (output 1)
 *   contains zero, the second pixel (output 2) is one, and so on. Thus within
 *   each quadrant the least significant 2 bits should always be the same for a
 *   4-output device, or the least significant bit the same for 2-output
 *   devices.
 *
 *   Option 2 consists of...
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that pFrame points to an SDSU frame structure (initialised
 *   with sdsuFrameAlloc, sdsuFrameFind and sdsuFrameReserve) containing
 *   sufficient storage space for xPixels * yPixels values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Option 2 is very wasteful of CPU. It should be used for small images only.
 *
 *   At the moment this function only simulates a 2x2 array of spots for a CCD
 *   with 2x2 sectors. It can be extended if necessary.
 *-
 */

STATUS detSimulateData
   (
   const int      xPixels,         /* Number of pixels in X.                  */
   const int      yPixels,         /* Number of pixels in Y.                  */
   const int      option,          /* Simulation option.                      */
   SDSU_FRAME *   pFrame           /* Pointer to frame buffer.                */
   )
{
   const int      nPixels = xPixels * yPixels;
                                   /* Total number of pixels.                 */

   const int      nSectors = 4;    /* Number of sectors/outputs.              */
   int            sector;          /* Sector counter.                         */
   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */
   int            i, j;            /* Column and row counters.                */
   int            is, js;          /* Column and row for a particular sector  */

   const int      nSpots = 4;      /* Number of simulated spots.              */
   int            spot;            /* Spot counter.                           */
   int            spotx[4];        /* X coordinates of simulated spot centres */
   int            spoty[4];        /* y coordinates of simulated spot centres */


   double         dist;            /* Distance between pixel and spot centre. */
   double         dvalue;          /* Double valueto write into frame buffer. */
   uint16         value;           /* Integer value to write into frame buffer*/

   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */


   /* Check the frame buffer pointer and size are valid. */

   if (pFrame == NULL)
   {
      ERROR_SET(S_detControl_INTERNAL, "No frame buffer defined",
                ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ((xPixels <= 0) || (yPixels <= 0 ))
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
                  "Bad number of pixels given, %d x %d", ERROR_LOG_SAVE,
                  xPixels, yPixels);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detSimulateData: Simulating %d x %d pixels of data to buffer at %p - option %d\n",
   xPixels, yPixels, pFrame, option );
#endif   /* DEBUG */

   /* Switch according to the simulation option chosen. */

   switch (option)
   {
      case (1):

         /*
          * An incrementing series of values is required.
          * Note that ptr is initialised to the start of the frame pixels.
          */

         ptr = & pFrame->pixel[0];
         for ( i=0; i<nPixels; i++)
         {
            value = (uint16) i;
            *(ptr) = value;
            ptr++;
         }
         break;


      case (2):

         /*
          * An array of simulated spots is required.
          */

         /* First init the number of pixels per sector, based on the
          * number of sectors.
          */

         if ( nSectors == 2 )
         {
            /* There are two outputs and therefore 2 sectors in a 2x1 pattern */

            xPixelsSector = xPixels / 2;
            yPixelsSector = yPixels;
         }
         else if ( nSectors == 4 )
         {
            /* There are four outputs and therefore 2 sectors in a 2x2 pattern*/

            xPixelsSector = xPixels / 2;
            yPixelsSector = yPixels / 2;
         }

         /* Real positions for 2x2 wavefront sensor */
         spotx[0] = 32;
         spoty[0] = 27;
         spotx[1] = 61;
         spoty[1] = 25;
         spotx[2] = 25;
         spoty[2] = 59;
         spotx[3] = 61;
         spoty[3] = 58;

         /*
          * Initialise ptr to the start of the frame pixels and then step
          * through the rows and columns within each sector.
          */

         ptr = & pFrame->pixel[0];

         for ( j=0; j < yPixelsSector; j++ )
         {
            for ( i=0; i < xPixelsSector; i++ )
            {
               for ( sector=1; sector <= nSectors; sector++ )
               {
                  /*
                   * Calculate the row and column coordinates of this particular
                   * point in this sector.
                   */

                  if ( sector == 1 )
                  {
                     /* Sector 1 */
                     is = i;
                     js = j;
                  }
                  else if ( sector == 2 )
                  {
                     /* Sector 2 */
                     is = xPixels - i;
                     js = j;
                  }
                  else if ( sector == 3 )
                  {
                     /* Sector 3 */
                     is = xPixels - i;
                     js = yPixels - j;
                  }
                  else
                  {
                     /* Sector 4 */
                     is = i;
                     js = yPixels - j;
                  }

                  /*
                   * Use the row and column coordinates calculated above to
                   * determine the the distance of this point from each spot
                   * centre, calculate the sum of the light from each spot
                   * (assuming a Gaussian distribution), and write
                   * this sum to the location pointed to by ptr. The value is
                   * not allowed to exceed 65535 because it needs to be stored
                   * as an unsigned 16 bit integer. Finally, ptr is incremented.
                   *
                   * The constant factors used in the following equations are
                   * arbitrary.
                   */

                  dvalue =
                  (double) ( 1000 * rand() / RAND_MAX ); /* Random background */

                  for ( spot=0; spot < nSpots; spot++ )
                  {
                     dist = 0.25 * (double) ((is-spotx[spot])*(is-spotx[spot]) +
                                             (js-spoty[spot])*(js-spoty[spot]));
                     dvalue +=  30000.0 * exp (-dist);
                  }

                  if ( dvalue <= 65535.0 )
                     value = (uint16) floor(dvalue);
                  else
                     value = 65535;

                  *ptr++ = value;
               }
            }
         }
         break;

      default:

         ERROR_SET( S_detControl_BAD_ATTRIBUTE, "Unknown simulation option",
                    ERROR_LOG_SAVE);
         return (ERROR);
         break;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReadFitsHeaderInt
 *
 *   INVOCATION:
 *   detReadFitsHeaderInt (fileName, nKey, keyName, keyVal)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) fileName     (char *)    Name of FITS file whose header is being read
 *   (>) nKey         (int)       Maximum number of keywords to be read
 *   (>) keyName      (char **)   Name of array of keywords
 *   (<) keyVal       (int *)     Array of integer values of keywords
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the integer values of an array of keywords from a FITS header
 *
 *   DESCRIPTION:
 *   Uses functions from the cfitsio library to open a FITS file for reading,
 *   and read the values of specified keywords with integer values. The keywords
 *   elements of an array, and their values are read into the corresponding
 *   elements of an array of integers. The number of elements of the keyword
 *   array may exceed the number of keywords actually present, but obviously not
 *   the number of array elements allocated. The keywords should be consecutive
 *   elements of their array, as reading of keywords will end when a NULL value
 *   is encountered as a keyword. The file is closed when reading is finished.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Steven Heddle,
 *   UKATC, Edinburgh 18/1/1999
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   Restricted to keywords with integer values.
 *-
 */

STATUS detReadFitsHeaderInt
   (
   char *         fileName,          /* Name of file to be read.              */
   int            nKey,              /* Maximum number of keywords to be read */
   char **        keyName,           /* Name of array of keywords             */
   int *          keyVal             /* Array of integer values of keywords   */
   )
{
   fitsfile *     fp;                /* FITS File descriptor.                 */
   int            fitsStatus;        /* FITS status used by the fits function */
   int            i;                 /* Counter.                              */
   char           comment [80] ;     /* Comment buffer read from FITS file    */

   /*
    * Set to zero the FITS status
    */

   fitsStatus = 0 ;

   /*
    * Open the FITS file
    */

   if ( fits_open_file ( &fp, fileName, FITSIO_READONLY, &fitsStatus) )
   {
      ERROR_SET2(0, "Can't open FITS file %s: %d", ERROR_LOG_SAVE, fileName,
                 fitsStatus);
      return (ERROR);
   }

   /*
    * Read the keywords array and write their values into the corresponding
    * int array
    */

   i = 0 ;


   while ( (keyName[i] != NULL) && (i < nKey) )
   {
      if ( fits_read_key(fp, TINT, keyName[i], keyVal+i, comment, &fitsStatus) )
      {
         ERROR_SET2(0, "Failed on reading %s keyword: %d", ERROR_LOG_SAVE,
                    keyName[i], fitsStatus);
      }
      i++;
   }

   /*
    * Close the FITS file
    */

   if (fits_close_file (fp, &fitsStatus))
   {
      ERROR_SET1(0, "Problem closing FITS file : %d", 
                 ERROR_LOG_SAVE, fitsStatus);
      return (ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReadFitsImageUint16
 *
 *   INVOCATION:
 *   detReadFitsImageUint16 (pImageBuffer, fileName, buffSize)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pImageBuffer (uint16 *) Pointer to buffer for image read in
 *   (>) fileName     (char *)   Name of FITS file whose image is being read
 *   (>) buffSize     (int)      Size of the image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the unsigned short int image into a buffer from a FITS file
 *
 *   DESCRIPTION:
 *   The FITS file is opened and the NAXIS keywords read to get the image
 *   size. If the size is greater than buffsize, only enough of the image to
 *   fill the buffer is read in. If the image is smaller than or equal to the
 *   size of the buffer, the whole image is read in. No padding to fill any
 *   unassigned elements of the buffer takes place, as the image dimensions for
 *   any subsequent processing should be strictly controlled to match the
 *   xframesize and yframesize dimensions specified in the context structure.
 *   The FITS file is then closed.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Steven Heddle,
 *   UKATC, Edinburgh 18/1/1999
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The buffer pointed to by pImageBuffer has been allocated large enough to
 *   accomodate buffSize unsigned short ints
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detReadFitsImageUint16
   (
   uint16 *       pImageBuffer, /* Pointer to buffer for image read in     */
   char *         fileName,     /* Name of file to be read.                */
   int            buffSize      /* Size of the image buffer                */
   )
{
   fitsfile *     fp;           /* FITS File descriptor.                   */
   int            fitsStatus;   /* FITS status used by the fits function   */
   int            nFound;       /* Number of keywords founds               */
   long           nAxes[2];     /* Array of keyword NAXIS values           */
   long           nPixels;      /* Number of pixels of the image           */
   long           nElemRead;    /* Number of pixels read                   */
   long           firstPixel;    /* Number of pixels read                   */
   uint16         nullval;      /* Value for undefined pixels when reading */
   int            anynull;      /* Set to 1 if any values are null; else 0 */

   /*
    * Set to zero the FITS status
    */

   fitsStatus = 0 ;

   /*
    * Open the FITS file
    */

   if ( fits_open_file ( &fp, fileName, FITSIO_READONLY, &fitsStatus) )
   {
      ERROR_SET2(0, "Can't open FITS file %s: %d", ERROR_LOG_SAVE, fileName,
                 fitsStatus);
      return (ERROR);
   }

   /*
    * Read the keywords NAXIS1 and NAXIS2 to get image size
    */

   if ( fits_read_keys_lng(fp, "NAXIS", 1, 2, nAxes, &nFound, &fitsStatus) )
   {
      ERROR_SET1(0, "Failed to read keywords NAXIS: %d", ERROR_LOG_SAVE,
                 fitsStatus);

      if (fits_close_file (fp, &fitsStatus))
      {
         ERROR_SET1(0, "Problem closing FITS file: %d", 
                    ERROR_LOG_SAVE, fitsStatus);
      }

      return (ERROR) ;
   }

   nPixels = nAxes[0] * nAxes[1];

   /*
    * Check the image size in comparison to the buffer size
    */

   if ( buffSize < nPixels )
      nElemRead = buffSize ;
   else
      nElemRead = nPixels ;

   /*
    * Read the image
    */

   firstPixel = 1;
   nullval = 0;           /* don't check for null values in the image */

   /* Note that even though the FITS images contains unsigned integer */
   /* pixel values (or more accurately, signed integer pixels with    */
   /* a bias of 32768),  this routine is reading the values into a    */
   /* float array.Cfitsio automatically performs the datatype         */
   /* conversion in cases like this.                                  */

   if ( fits_read_img (fp, TUSHORT, firstPixel, nElemRead, &nullval,
                       pImageBuffer, &anynull, &fitsStatus) )
   {
      ERROR_SET1(0, "Failed to read image: %d", ERROR_LOG_SAVE,
                 fitsStatus);

      if (fits_close_file (fp, &fitsStatus))
      {
         ERROR_SET1(0, "Problem closing FITS file: %d", 
                    ERROR_LOG_SAVE, fitsStatus);
      }

      return (ERROR) ;
   }

   /*
    * Close the FITS file
    */

   if (fits_close_file (fp, &fitsStatus))
   {
      ERROR_SET1(0, "Problem closing FITS file: %d", ERROR_LOG_SAVE, 
                 fitsStatus) ;
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detWriteFits
 *
 *   INVOCATION:
 *   detWriteFits (filename, obsId, xPixels, yPixels, pImageBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) filename     (char *)   Name of file to contain data.
 *   (>) obsId        (OBS_ID)   Current observation context structure
 *   (>) xPixels      (int)      Number of pixels along X axis
 *   (>) yPixels      (int)      Number of pixels along Y axis
 *   (!) pImageBuffer (float *)  Pointer to image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write floating point data to FITS file
 *
 *   DESCRIPTION:
 *   This function writes the contents of the frame buffer to a FITS file.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that pImageBuffer points to a buffer of memory containing
 *   xPixels*yPixels float pixel values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *-
 */

STATUS detWriteFits
   (
   char *         filename,        /* Name of file to be written.             */
   OBS_ID         obsId,           /* Current observation context structure.  */
   int            xPixels,         /* Number of pixels along X axis.          */
   int            yPixels,         /* Number of pixels along Y axis.          */
   float *        pImageBuffer     /* Pointer to image data.                  */
   )
{
   char           telName[40];
   char           bunit[40];

   int            i;
   int            extra;
   int            counter;
   int            bufferSize;

   float          extraBuffer[2880];
   double         elapsedTime;

   FILE           *pFile; 

   /* 
    * Check the parameters provided.
    */

   if (pImageBuffer == NULL)
   {
      ERROR_SET(S_detControl_INTERNAL, "No image buffer defined",
                ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ( (xPixels <= 0) || (yPixels <= 0) )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
         "Bad number of pixels given, %d X %d", ERROR_LOG_SAVE,
         xPixels, yPixels);
      return (ERROR);
   }

   /*
    * Convert the time stamps from Gemini raw time into Universal Time
    * and construct these into character strings.
    */

   if (timeThenC( obsId->rawtEnd, UT1, 2, obsId->timeArrayEnd ) != OK)
   {
      ERROR_SET (0,
      "Failed to convert time stamp at observation end to date/time",
      ERROR_LOG_NOW);
   }

   sprintf (obsId->utEndString, "%04d-%02d-%02d:%02d:%02d:%02d",
            obsId->timeArrayEnd[0], obsId->timeArrayEnd[1],
            obsId->timeArrayEnd[2], obsId->timeArrayEnd[3],
            obsId->timeArrayEnd[4], obsId->timeArrayEnd[5]);

   elapsedTime =  (obsId->rawtEnd - obsId->rawtStart);

   wfsGetTelName ( telName ) ;

   strcpy ( bunit, DET_BUNIT ) ;

   /* Create the FITS file */

   pFile = fopen ( filename , "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Can't create FITS file %s", ERROR_LOG_SAVE,
                   filename );
      return (ERROR);
   }

   /* Write a complete header */

   counter = 0;

   fprintf ( pFile, "SIMPLE  =                    T /                                                " );
   counter ++;
   fprintf ( pFile, "BITPIX  =                  -32 /                                                " );
   counter ++;
   fprintf ( pFile, "NAXIS   =                    2 /                                                " );
   counter ++;
   fprintf ( pFile, "NAXIS1  =                %5d /                                                ", xPixels );
   counter ++;
   fprintf ( pFile, "NAXIS2  =                %5d /                                                ", yPixels );
   counter ++;
   fprintf ( pFile, "BZERO   =                    0 /                                                " );
   counter ++;
   fprintf ( pFile, "EXTEND  =                    T /                                                " );
   counter ++;
   fprintf ( pFile, "UTEND   ='%20s'/                                                ", obsId->utEndString );
   counter ++;
   fprintf ( pFile, "UTSTART ='%20s'/                                                ", obsId->utStartString );
   counter ++;
   fprintf ( pFile, "EXPTIME =      %15f /                                                ", obsId->exposureTime);
   counter ++;
   fprintf ( pFile, "DARKTIME=      %15f /                                                ", obsId->exposureTime);
   counter ++;
   fprintf ( pFile, "ELAPSED =      %15f /                                                ", elapsedTime);
   counter ++;
   fprintf ( pFile, "TELESCOP='%20s'/                                                ", telName);
   counter ++;
   fprintf ( pFile, "INSTRUME='%20s'/                                                ", obsId->pWfsName);
   counter ++;
   fprintf ( pFile, "OBSERVAT='%20s'/                                                ", telName);
   counter ++;
   fprintf ( pFile, "BUNIT   ='%20s'/                                                ", bunit);
   counter ++;
   fprintf ( pFile, "UNITS   ='%20s'/                                                ", bunit);
   counter ++;
   fprintf ( pFile, "OBSTYPE ='%20s'/                                                ", obsId->pObsType);
   counter ++;

   if ( obsId->wcsStatus == 0 )
   {
      fprintf ( pFile, "CTYPE1  ='%20s'/                                                ", obsId->ctype1);
      counter ++;
      fprintf ( pFile, "CRPIX1  =      %15f /                                                ", obsId->crpix1);
      counter ++;
      fprintf ( pFile, "CRVAL1  =      %15f /                                                ", obsId->crval1);
      counter ++;
      fprintf ( pFile, "CTYPE2  ='%20s'/                                                ", obsId->ctype2);
      counter ++;
      fprintf ( pFile, "CRPIX2  =      %15f /                                                ", obsId->crpix2);
      counter ++;
      fprintf ( pFile, "CRVAL2  =      %15f /                                                ", obsId->crval2);
      counter ++;
      fprintf ( pFile, "CD1_1   =      %15f /                                                ", obsId->cd1_1);
      counter ++;
      fprintf ( pFile, "CD1_2   =      %15f /                                                ", obsId->cd1_2);
      counter ++;
      fprintf ( pFile, "CD2_1   =      %15f /                                                ", obsId->cd2_1);
      counter ++;
      fprintf ( pFile, "CD2_2   =      %15f /                                                ", obsId->cd2_2);
      counter ++;
      fprintf ( pFile, "RADECSYS='%20s'/                                                ", obsId->radecsys);
      counter ++;
   }

   fprintf ( pFile, "RA      =      %15f /                                                ", obsId->RA);
   counter ++;
   fprintf ( pFile, "DEC     =      %15f /                                                ", obsId->Dec);
   counter ++;
   fprintf ( pFile, "EQUINOX =      %15f /                                                ", obsId->equinox);
   counter ++;
   fprintf ( pFile, "MJDOBS  =      %15f /                                                ", obsId->mjdobs);
   counter ++;
   fprintf ( pFile, "XBIN    =                %5d /                                                ", obsId->aoCcdId->xBin);
   counter ++;
   fprintf ( pFile, "YBIN    =                %5d /                                                ", obsId->aoCcdId->yBin);
   counter ++;
   fprintf ( pFile, "DATASEC ='%20s'/                                                ", obsId->dataSec);
   counter ++;
   fprintf ( pFile, "CCDSEC  ='%20s'/                                                ", obsId->ccdSec);
   counter ++;
   fprintf ( pFile, "ORIGSEC ='%20s'/                                                ", obsId->origSec);
   counter ++;
   fprintf ( pFile, "DETTYPE ='%20s'/                                                ", obsId->detType);
   counter ++;
   fprintf ( pFile, "DETID   ='%20s'/                                                ", obsId->detId);
   counter ++;
   fprintf ( pFile, "END                                                                             ");
   counter ++;

   /* Fill the rest of the header with blanks: header 36 * 80 char */

   /*printf ( "counter = %d\n" , counter );*/
   counter = counter % 36 ;
   /*printf ( "counter = %d\n" , counter );*/
   if ( counter != 0 )
   {
      for ( i = counter ; i < 36 ; i ++ )
          fprintf ( pFile, "                                                                                " );
   }

   /* Write the image in 2880 byte blocks to the Fits file */

   bufferSize = xPixels * yPixels;

   extra = (bufferSize*4) % 2880;

   /*printf ( "extra=%d\n", extra );*/

   if ( fwrite ( pImageBuffer, sizeof (float), bufferSize, pFile ) != 
        bufferSize )
   {
      ERROR_SET1 ( 0, "Failed to write image into %s",
                   ERROR_LOG_SAVE, filename );

      fclose ( pFile );
      return (ERROR);
   }

   if ( extra != 0 )
   {
      extra = (2880 - extra)/4;
      /*printf ( "extra=%d\n", extra );*/
      for ( i = 0 ; i < extra ; i ++ )
          extraBuffer[i]=0;

      if ( fwrite ( extraBuffer, sizeof (float), extra, pFile ) != 
           extra )
      {
         ERROR_SET1 ( 0, "Failed to fill with zero image into %s",
                      ERROR_LOG_SAVE, filename );

         fclose ( pFile );
         return (ERROR);
      }
   }

   /* Close the fits file */

   fclose ( pFile ) ;

   return ( OK ) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameSize
 *
 *   INVOCATION:
 *   detFrameSize (cadCmdContext, commandNumber, sdsuId, obsId, 
 *                 pOffsetFullVect, pOffsetBinVect);
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (!) obsId           (OBS_ID)          Observation context structure
 *   (>) pOffsetFullVect (long *)          ADC offset vector - no binning
 *   (>) pOffsetBinVect  (long *)          ADC offset vector - binning
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detFrameSize command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU detector geometry parameters.
 *   Default configuration is (no binning):
 *   T_XRAS = 40 , T_YRAS = 40
 *   T_XSUBAP = 1 , T_YSUBAP = 1
 *   T_XSTART = 0 , T_YSTART = 0
 *   T_XSPACE = 0 , T_YSPACE = 0
 *   T_XBIN = 1 , T_YBIN = 1
 *   Binning configuration is :
 *   T_XRAS = 20 , T_YRAS = 20
 *   T_XSUBAP = 1 , T_YSUBAP = 1
 *   T_XSTART = 0 , T_YSTART = 0
 *   T_XSPACE = 0 , T_YSPACE = 0
 *   T_XBIN = 2 , T_YBIN = 2
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detFrameSize
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId,           /* Observation context structure.         */
   long *          pOffsetFullVect, /* ADC offset vector - no binning         */
   long *          pOffsetBinVect   /* ADC offset vector - binning            */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   /*
    * Variables associated with "Set Frame size"
    */

   long         binFlag;

   long         xReqPixels; /* Number of X pixels in digitised image (AC only)*/
   long         yReqPixels; /* Number of Y pixels in digitised image (AC only)*/
   long         xReqBin;    /* X binning factor (pixels per superpixel)       */
   long         yReqBin;    /* Y binning factor (pixels per superpixel)       */
   long         xReqRas;    /* Size of each subaperture in X direction in     */
                            /* super-pixels (wfs ONLY)                        */
   long         yReqRas;    /* Size of each subaperture in Y direction in     */
                            /* super-pixels (WFS only)                        */
   long         xReqSpace;  /* Spacing between subapertures in X direction in */
                            /* pixels (WFS only)                              */
   long         yReqSpace;  /* Spacing between subapertures in Y direction in */
                            /* pixels (WFS only)                              */
   long         xReqStart;  /* X offset from bottom left corner of array in   */
                            /* pixels.                                        */
   long         yReqStart;  /* Y offset from bottom left corner of array in   */
                            /* pixels.                                        */
   long         xReqTail;   /* Number of trailing X pixels to be discarded on */
   long         reqPixelsNb;/* Total number of digitised pixels.              */
   int          nPackets;   /* Number of packets expected per frame.          */

   /*
    * Parameters to update the aoCtrlId structure 
    */

   int          updateAoCtrlFlag = FALSE;
   long         k;
   char         path[STRING_SIZE];        
   char         darkFileName[STRING_SIZE];
   char         fullDarkFileName[STRING_SIZE];
   char         flatFileName[STRING_SIZE];
   char         fullFlatFileName[STRING_SIZE];
   char         refFileName[STRING_SIZE];
   char         fullRefFileName[STRING_SIZE];
   char         aoImFileName[STRING_SIZE];
   char         fullAoImFileName[STRING_SIZE];
   char         aoCmFileName[STRING_SIZE];
   char         fullAoCmFileName[STRING_SIZE];
   char         fgCmFileName[STRING_SIZE];
   char         fullFgCmFileName[STRING_SIZE];
   char         defFileName[STRING_SIZE];
   char         aoInitFileName[STRING_SIZE];
   double       angleM2;
   double       angleM1;
   double       refX;
   double       refY;
   double       rms;
   double       thresh;
   double       totalThresh;

   /*
    * Parameters to update the ADC offset 
    */

   int          updateOffset = FALSE;
   long         i;                  /* index                                  */
   long         offsetVect[4];      /* ADC offset vector                      */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &binFlag);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is not in progress
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Init xBin and yBin
    */

   if ( binFlag == TRUE )
   {
      xReqBin = 2;
      yReqBin = 2;
   }
   else
   {
      xReqBin = 1;
      yReqBin = 1;
   }

   /* Init the others parameters */

   if ( (binFlag == TRUE) && (obsId->aoCcdId->binningFlag == FALSE) ) 
   {
      /* First time binning */
    
      xReqRas = obsId->aoCcdId->xRaster / xReqBin ;
      yReqRas = obsId->aoCcdId->yRaster / yReqBin ;
      xReqPixels = 2 * xReqRas * obsId->aoCcdId->xSubapNb ;
      yReqPixels = 2 * yReqRas * obsId->aoCcdId->ySubapNb ;
      reqPixelsNb = xReqPixels * yReqPixels ;
      xReqStart = 0;
      yReqStart = 0;
      xReqSpace = 0;
      yReqSpace = 0;
      xReqTail = obsId->aoCcdId->xSize - 
                 (((xReqRas * xReqBin) + xReqSpace) * obsId->aoCcdId->xSubapNb)
                 + xReqSpace - xReqStart - obsId->aoCcdId->uscanNb;

      if ( xReqTail < 0 )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
                     "Xtail is %ld. Should not be less than zero", 
                     ERROR_LOG_NOW, xReqTail);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }
      
      obsId->aoCtrlId->initFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoCtrlInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->darkInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->flatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->aoIntMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->aoContMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized",
                           obsId->pAoContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->fgContMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized",
                           obsId->pFgContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
      }

      /* Read default parameters from par file */

#if (MK)
      strcpy ( defFileName , DET_CONTROL_PWFS2_AO_BIN_CTRL_MK_INIT_FILE ) ;
#else
      strcpy ( defFileName , DET_CONTROL_PWFS2_AO_BIN_CTRL_CP_INIT_FILE ) ;
#endif

      if ( strcmp (defFileName, "NONE") != 0 )
      {
         strcpy ( aoInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
         strcat ( aoInitFileName , "/" ) ;
         strcat ( aoInitFileName , defFileName ) ;

         if ( aoCtrlFileRead ( aoInitFileName, path, darkFileName, flatFileName,
                               refFileName, &refX, &refY, aoImFileName,
                               aoCmFileName, fgCmFileName, &rms, &thresh, 
                               &totalThresh, &angleM2, &angleM1 ) == ERROR )
         {
            ERROR_LOG ("Failed to read ao control file parameters");
         }

         sprintf ( fullDarkFileName, "%s/%s", path, darkFileName );
         sprintf ( fullFlatFileName, "%s/%s", path, flatFileName );
         sprintf ( fullRefFileName, "%s/%s", path, refFileName );
         sprintf ( fullAoImFileName, "%s/%s", path, aoImFileName );
         sprintf ( fullAoCmFileName, "%s/%s", path, aoCmFileName );
         sprintf ( fullFgCmFileName, "%s/%s", path, fgCmFileName );

         updateAoCtrlFlag = TRUE;
      }

      /* Init the ADC offset vector */

      for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = pOffsetBinVect[i];

      updateOffset = TRUE;
   }
   else if ( (binFlag == TRUE) && (obsId->aoCcdId->binningFlag == TRUE) )
   {
      /* Already binning mode - do not change anything */

      xReqRas = obsId->aoCcdId->xRaster ;
      yReqRas = obsId->aoCcdId->yRaster ;
      xReqStart = obsId->aoCcdId->xStart;
      yReqStart = obsId->aoCcdId->yStart;
      xReqSpace = obsId->aoCcdId->xSpace;
      yReqSpace = obsId->aoCcdId->ySpace;
      xReqPixels = obsId->aoCcdId->xPixels;
      yReqPixels = obsId->aoCcdId->yPixels;
      reqPixelsNb = obsId->aoCcdId->pixelsNb;
      xReqTail = obsId->aoCcdId->xTail;
   }
   else if ( (binFlag == FALSE) && (obsId->aoCcdId->binningFlag == FALSE) )
   {
      /* Already not binning mode - do not change anything */

      xReqRas = obsId->aoCcdId->xRaster ;
      yReqRas = obsId->aoCcdId->yRaster ;
      xReqStart = obsId->aoCcdId->xStart;
      yReqStart = obsId->aoCcdId->yStart;
      xReqSpace = obsId->aoCcdId->xSpace;
      yReqSpace = obsId->aoCcdId->ySpace;
      xReqPixels = obsId->aoCcdId->xPixels;
      yReqPixels = obsId->aoCcdId->yPixels;
      reqPixelsNb = obsId->aoCcdId->pixelsNb;
      xReqTail = obsId->aoCcdId->xTail;
   }
   else if ( (binFlag == FALSE) && (obsId->aoCcdId->binningFlag == TRUE) )
   {
      /* cancel binning mode */

      xReqRas = obsId->aoCcdId->xRaster * obsId->aoCcdId->xBin;
      yReqRas = obsId->aoCcdId->yRaster * obsId->aoCcdId->yBin;
      xReqStart = 0;
      yReqStart = 0;
      xReqSpace = 0;
      yReqSpace = 0;
      xReqPixels = 2 * xReqRas * obsId->aoCcdId->xSubapNb;
      yReqPixels = 2 * yReqRas * obsId->aoCcdId->ySubapNb;
      reqPixelsNb = xReqPixels * yReqPixels;
      xReqTail = obsId->aoCcdId->xSize - 
                 (((xReqRas * xReqBin) + xReqSpace) * obsId->aoCcdId->xSubapNb)
                 + xReqSpace - xReqStart - obsId->aoCcdId->uscanNb;

      if ( xReqTail < 0 )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
                     "Xtail is %ld. Should not be less than zero", 
                     ERROR_LOG_NOW, xReqTail);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }
      
      obsId->aoCtrlId->initFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoCtrlInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->darkInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->flatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->aoIntMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized",
                           obsId->pAoIntMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->aoContMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized",
                           obsId->pAoContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
      }
      obsId->aoCtrlId->fgContMatInitFlag = FALSE;
      if (epToVxPipeWrite (NULL, "Not initialized",
                           obsId->pFgContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
      }

      /* Read default parameters from par file */

#if (MK)
      strcpy (defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_MK_INIT_FILE);
#else
      strcpy (defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_CP_INIT_FILE);
#endif

      if ( strcmp (defFileName, "NONE") != 0 )
      {
         strcpy ( aoInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
         strcat ( aoInitFileName , "/" ) ;
         strcat ( aoInitFileName , defFileName ) ;

         if ( aoCtrlFileRead ( aoInitFileName, path, darkFileName, flatFileName,
                               refFileName, &refX, &refY, aoImFileName,
                               aoCmFileName, fgCmFileName, &rms, &thresh,
                               &totalThresh, &angleM2, &angleM1 ) == ERROR )
         {
            ERROR_LOG ("Failed to read ao control file parameters");
         }

         sprintf ( fullDarkFileName, "%s/%s", path, darkFileName );
         sprintf ( fullFlatFileName, "%s/%s", path, flatFileName );
         sprintf ( fullRefFileName, "%s/%s", path, refFileName );
         sprintf ( fullAoImFileName, "%s/%s", path, aoImFileName );
         sprintf ( fullAoCmFileName, "%s/%s", path, aoCmFileName );
         sprintf ( fullFgCmFileName, "%s/%s", path, fgCmFileName );

         updateAoCtrlFlag = TRUE;
      }

      /* Init the ADC offset vector */

      for ( i = 0 ; i < obsId->aoCcdId->outputsNb ; i ++ )
             offsetVect[i] = pOffsetFullVect[i];

      updateOffset = TRUE;
   }
   else 
   {
      /* do not change anything */

      xReqRas = obsId->aoCcdId->xRaster ;
      yReqRas = obsId->aoCcdId->yRaster ;
      xReqStart = obsId->aoCcdId->xStart;
      yReqStart = obsId->aoCcdId->xStart;
      xReqSpace = obsId->aoCcdId->xSpace;
      yReqSpace = obsId->aoCcdId->xSpace;
      xReqPixels = obsId->aoCcdId->xPixels;
      yReqPixels = obsId->aoCcdId->yPixels;
      reqPixelsNb = obsId->aoCcdId->pixelsNb;
      xReqTail = obsId->aoCcdId->xTail;
   }

   if ( (!sdsuId->simulate) && (obsId->aoCcdId->packetSize > 0) )
   {
      obsId->aoCcdId->packetSize =
      xReqRas * obsId->aoCcdId->xSubapNb * (obsId->aoCcdId->outputsNb/2) * 2;

      nPackets = (int) ceil ( (double) (reqPixelsNb) / 
                 (double) obsId->aoCcdId->packetSize );
   }
   else
   {
      nPackets = 1;
   }

   /* Init aoCcdId now */

   obsId->aoCcdId->xBin = xReqBin;
   obsId->aoCcdId->yBin = yReqBin;
   obsId->aoCcdId->xRaster = xReqRas;
   obsId->aoCcdId->yRaster = yReqRas;
   obsId->aoCcdId->xStart = xReqStart;
   obsId->aoCcdId->yStart = yReqStart;
   obsId->aoCcdId->xSpace = xReqSpace;
   obsId->aoCcdId->ySpace = yReqSpace;
   obsId->aoCcdId->xPixels = xReqPixels;
   obsId->aoCcdId->yPixels = yReqPixels;
   obsId->aoCcdId->pixelsNb = reqPixelsNb;
   obsId->aoCcdId->xTail = xReqTail;
   obsId->aoCcdId->packetNb = nPackets;

   if ( binFlag == TRUE )
      obsId->aoCcdId->binningFlag = TRUE ;
   else
      obsId->aoCcdId->binningFlag = FALSE ;

   aoCcdContextShow (obsId->aoCcdId);

   MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
      (obsId->aoCcdId->binningFlag ? "binned":"full"));
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%d, YSIZE=%d, XPIXELS=%d, YPIXELS=%d",
      obsId->aoCcdId->xSize, obsId->aoCcdId->ySize, obsId->aoCcdId->xPixels, 
      obsId->aoCcdId->yPixels);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%d, YSUBAP=%d, XBIN=%d, YBIN=%d",
      obsId->aoCcdId->xSubapNb, obsId->aoCcdId->ySubapNb, obsId->aoCcdId->xBin,
      obsId->aoCcdId->yBin);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%d, YRAS=%d, XSPACE=%d, YSPACE=%d",
      obsId->aoCcdId->xRaster, obsId->aoCcdId->yRaster, obsId->aoCcdId->xSpace,
      obsId->aoCcdId->ySpace);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%d, YSTART=%d, XTAIL=%d, NPIXELS=%d",
      obsId->aoCcdId->xStart, obsId->aoCcdId->yStart, obsId->aoCcdId->xTail, 
      obsId->aoCcdId->pixelsNb);

   MESSAGE_LOG2 (MSG_FULLDEBUG,
      "Each frame will consist of %d packets of %d pixels each",
      nPackets, obsId->aoCcdId->packetSize);

   /*
    * Update the geometry parameters in the SDSU timing DSP. These are all
    * "on-the-fly" parameters and need to be downloaded with sdsuParamWRP()
    * and activated by sending a "LDP" command.
    */

   if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE",
                       obsId->aoCcdId->packetSize) == ERROR)
   {
      ERROR_LOG ("Failed to increase the PWFS packet size");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   (uint32) xReqRas)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   (uint32) yReqRas)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", (uint32) xReqStart)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", (uint32) yReqStart)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", (uint32) xReqSpace)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", (uint32) yReqSpace)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XBIN",   (uint32) xReqBin)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YBIN",   (uint32) yReqBin)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  (uint32) xReqTail)
        == ERROR) ||
        (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", (uint32) reqPixelsNb)
        == ERROR)
     )
   {
      ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }
 
   if ( updateOffset == TRUE )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                        (uint32) offsetVect[0] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                         (uint32) offsetVect[1] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2",
                         (uint32) offsetVect[2] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 2 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3",
                         (uint32) offsetVect[3] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 3 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      /*
       * Update the adc sir records 
       */

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                           obsId->pAdc0Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc0 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                           obsId->pAdc1Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc1 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[2]) ,
                           obsId->pAdc2Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc2 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[3]) ,
                           obsId->pAdc3Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc3 sad record");
         return (ERROR);
      }
   }

   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   /*
    * Update the number of packets per frame in the SDSU context structure.
    */

   sdsuId->packetsPerFrame = nPackets;

   /*
    * Now init the SAD geometry records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xRaster) ,
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yRaster) ,
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xBin) ,
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yBin) ,
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xSpace) ,
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->ySpace) ,
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->xStart) ,
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->aoCcdId->yStart) ,
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if ( updateAoCtrlFlag == TRUE )
   {
      if (aoCtrlContextUpdate ( fullDarkFileName, fullFlatFileName,
                                fullRefFileName, fullAoImFileName, 
                                fullAoCmFileName, fullFgCmFileName,
                                refX, refY, angleM2, angleM1, obsId->aoCcdId, 
                                obsId->aoCtrlId ) == ERROR )
      {
         ERROR_SET (0, "Failed to update AO control context", ERROR_LOG_NOW);
      }

/*
      obsId->aoCtrlId->threshold = thresh;
*/
      if ( obsId->aoCcdId->binningFlag == FALSE )
      {
/*
         obsId->aoCtrlId->rms = obsId->aoCtrlId->rmsDarkFull;
         obsId->aoCtrlId->threshold = obsId->aoCtrlId->thresholdDarkFull;
*/
         obsId->aoCtrlId->rms = rms;
         obsId->aoCtrlId->threshold = thresh;
         if ( obsId->aoCtrlId->rmsDarkFull == 0.0 )
            obsId->aoCtrlId->rmsDarkFull = rms;
         if ( obsId->aoCtrlId->thresholdDarkFull == 0.0 )
            obsId->aoCtrlId->thresholdDarkFull = thresh;
      }
      else
      {
/*
         obsId->aoCtrlId->rms = obsId->aoCtrlId->rmsDarkBin;
         obsId->aoCtrlId->threshold = obsId->aoCtrlId->thresholdDarkBin;
*/
         obsId->aoCtrlId->rms = rms;
         obsId->aoCtrlId->threshold = thresh;

         if ( obsId->aoCtrlId->thresholdDarkBin == 0.0 )
            obsId->aoCtrlId->thresholdDarkBin = thresh;
         if ( obsId->aoCtrlId->rmsDarkBin == 0.0 )
            obsId->aoCtrlId->rmsDarkBin = rms;
      }
      for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
          obsId->aoCtrlId->thresholdVect[k] = obsId->aoCtrlId->threshold;

      obsId->aoCtrlId->totalThreshold = totalThresh;

      angleWithM1 = obsId->aoCtrlId->angleWithM1;
      angleWithM2 = obsId->aoCtrlId->angleWithM2;

      aoCtrlContextShow (obsId->aoCcdId, obsId->aoCtrlId, FALSE);

      if ( obsId->aoCtrlId->initFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, "Initialized", obsId->pAoCtrlInitContext) 
             == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
         }
      }

      if ( obsId->aoCtrlId->darkInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, obsId->aoCtrlId->darkFileName, 
                              obsId->pAoDarkInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
         }
      }

      if ( obsId->aoCtrlId->flatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, obsId->aoCtrlId->flatFileName, 
                              obsId->pAoFlatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
         }
      }

      if ( obsId->aoCtrlId->aoIntMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, obsId->aoCtrlId->aoIntMatFileName,
                              obsId->pAoIntMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
         }
      }

      if ( obsId->aoCtrlId->aoContMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, obsId->aoCtrlId->aoContMatFileName,
                              obsId->pAoContMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
         }
      }

      if ( obsId->aoCtrlId->fgContMatInitFlag == TRUE )
      {
         if (epToVxPipeWrite (NULL, obsId->aoCtrlId->fgContMatFileName,
                              obsId->pFgContMatInitContext) == ERROR)
         {
            ERROR_LOG (
            "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
         }
      }

      if (epToVxPipeWrite (NULL, (char *)(int)& (obsId->aoCtrlId->rms), 
                           obsId->pAoRmsContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_RMS_SIR_NAME record");
      }

      if (epToVxPipeWrite (NULL, (char *)(int)& (obsId->aoCtrlId->threshold), 
                           obsId->pAoThreshContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
      }

      if (epToVxPipeWrite (NULL, 
                           (char *)(int)& (obsId->aoCtrlId->totalThreshold), 
                           obsId->pAoTotalContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsReconnect
 *
 *   INVOCATION:
 *   detDhsReconnect (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detDhsReconnect command
 *
 *   DESCRIPTION:
 *   This function disconnects or reconnects to the dhs .
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detDhsReconnect
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */
   
   DHS_STATUS      dhsErrno;      /* DHS error number.                        */

   long            connect;

   /*int             tid;*/

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &connect);
   
   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is not in progress
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Check wether it is connect or disconnect command 
    */
    
   if ( connect == 0 ) /* disconnect requested */
   {

      if ( detDhsConnected == CONNECTED )
      {
         dhsErrno = 0;
         dhsDisconnect (detDhsConnection, &dhsErrno);
         CHECK_DHS (dhsErrno);
         if ( dhsErrno == DHS_S_SUCCESS )
         {
            detDhsConnected = NOT_CONNECTED;
            MESSAGE_LOG (MSG_LOG, "Disconnected to DHS");
         }
      }

      /*dhsEventLoopEnd(&dhsErrno);
      CHECK_DHS (dhsErrno);
      dhsExit ( &dhsErrno );
      CHECK_DHS (dhsErrno);

      printf ("Disconnect dhs\n");

      if ( (tid = taskNameToId ("ImpMaster")) != ERROR )
      {
         if ( taskDelete ( tid) == ERROR )
         {
            ERROR_SET (0, "Failed to delete ImpMaster task", ERROR_LOG_NOW);
            return (ERROR);
         }
         printf ("task ImpMaster deleted\n" ) ;
      }

      if ( (tid = taskNameToId ("ImpTransmitter")) != ERROR )
      {
         if ( taskDelete ( tid) == ERROR )
         {
            ERROR_SET (0, "Failed to delete ImpTransmitter task", ERROR_LOG_NOW);
            return (ERROR);
         }
         printf ("task ImpTransmitter deleted\n" ) ;
      }

      if ( (tid = taskNameToId ("ImpReceiver")) != ERROR )
      {
         if ( taskDelete ( tid) == ERROR )
         {
            ERROR_SET (0, "Failed to delete ImpReceiver task", ERROR_LOG_NOW);
            return (ERROR);
         }
         printf ("task ImpReceiver deleted\n" ) ;
      }

      detDhsInitialised = FALSE;
      if (semDelete (detDhsStartSem) == ERROR )
      {      
         ERROR_SET (0, "Failed to delete DHS start semaphore", ERROR_LOG_NOW);
         return (ERROR);
      } 
      printf ("sem deleted\n" );*/
   }
   else                /* connect requested */
   {
      /*tid = taskSpawn ("ImpMaster", 100, VX_FP_TASK, 20000, (FUNCPTR) ImpMaster, 
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
      if ( tid == ERROR )
      {
         ERROR_SET (0, "Can't spawn the ImpMaster", ERROR_LOG_NOW);
         return (ERROR);
      }
      printf ( "sp ImpMaster ok\n" ) ;

      taskDelay ( (int) (sysClkRateGet () * 10) );

      if ( detDhsInit ("pwfs2", 16, "10.2.4.56", "dataServerNS") == ERROR)
      {
         ERROR_SET (0, "Can't reinit the dhs", ERROR_LOG_NOW);
         return (ERROR);
      }*/

      if ( detDhsConnected == NOT_CONNECTED )
      {

         if ( detDhsConnect () == ERROR )
         {
            ERROR_SET (0, "Can't reconnect to the dhs", ERROR_LOG_NOW);
            return (ERROR);
         }
      }
   }

   /*
    * Now report to the SIR record
    */

   if ( detDhsConnected == CONNECTED )
   {
      if (epToVxPipeWrite (NULL, "CONNECTED", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_DHS_CON_SIR_NAME record");
         errorNumber = ERROR;
      }
   };

   if ( detDhsConnected == NOT_CONNECTED )
   {
      if (epToVxPipeWrite (NULL, " NOT CONNECTED", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_DHS_CON_SIR_NAME record");
         errorNumber = ERROR;
       }
   };

   return (OK); 
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsDisplay
 *
 *   INVOCATION:
 *   detDhsDisplay (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detDhsDisplay command
 *
 *   DESCRIPTION:
 *   This function sets the parameters to send the data to the QL of the DHS.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detDhsDisplay
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */
   
   long            rate;

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &rate);
   
   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress.
    * Set the obsId parameters.
    */

   obsId->dhsQlRate = rate;

   return (OK); 
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detHeadTempGet
 *
 *   INVOCATION:
 *   detReadFitsImageUint16 (struct sirRecord *psir)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) psir (struct sirRecord *) Pointer to headTemp sir record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write the temperature of the CCD into the SIR record
 *
 *   DESCRIPTION:
 *   For this sir record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 03 Apr 2000
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   external variables :detSdsuIdP2, detObsIdP2
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detHeadTempGet
   (
   struct sirRecord *       psir /* Pointer to "headTemp" sir record       */
   )
{
   uint32   value ;

   int      sample ;

   double   meanValue6, meanValue7;
   double   sdsuTemp6, sdsuTemp7, sdsuTemp;

   if ( detObsIdP2 == NULL )
   {
      return (ERROR);
   }

   if ( detSdsuIdP2 == NULL )
   {
      return (ERROR);
   }

   if ( ( detObsIdP2->observing != TRUE ) && ( readTempReadyFlag != FALSE ) )
   {

      meanValue6 = meanValue7 = 0.0;
      for ( sample=0; sample<1; sample++)
      {
         if (sdsuParamRead (detSdsuIdP2, SDSU_IDENT_UTL, "U_ADC6", &value) == 
             ERROR)
         {
            ERROR_LOG ("Failed to read thermistor 1 temperature parameter");
            return (ERROR);
         }
         else
         {
            meanValue6 += (double) value;
         }

         if (sdsuParamRead (detSdsuIdP2, SDSU_IDENT_UTL, "U_ADC7", &value) 
             == ERROR)
         {
            ERROR_LOG ("Failed to read thermistor 2 temperature parameter");
            return (ERROR);
         }
         else
         {
            meanValue7 += (double) value;
         }
      }

      /*meanValue6 /= 20.0;
      meanValue7 /= 20.0;*/

      sdsuTemp6 = meanValue6 * (-0.01545); 
                                      /* 0.01545 is not quite SDSU_TEMP_UNIT*/
      sdsuTemp7 = meanValue7 * (-0.01545); 
                                      /* 0.01545 is not quite SDSU_TEMP_UNIT*/

      sdsuTemp = (sdsuTemp6 + sdsuTemp7) / 2.0 ;

#ifdef DEBUG
      printf ( "detHeadTempGet() : not observing -> val = %f\n" , sdsuTemp ) ;
#endif
      *(double *)psir->val = sdsuTemp ;
   }
#ifdef DEBUG
   else
   {
      printf ( "detHeadTempGet() observing then wait...\n" ) ;
   }
#endif

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detShow
 *
 *   INVOCATION:
 *   detShow (pWfsName,verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of WFS
 *   (>) verbose  (const BOOL)   Enable verbose printout
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show status of detector control tasks
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the current status of the
 *   detector control task.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdP2  (SDSU_ID)  SDSU context structure for PWFS2
 *   (>) detObsIdP2   (OBS_ID)   Observation context structure for PWFS2
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detShow
   (
   const char *   pWfsName,
   const BOOL     verbose
   )
{
   /*
    * Display the contents of the SDSU context structures for the
    * wavefront sensor: PWFS2
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "p2") != NULL) || (strstr(pWfsName, "pwfs2") != NULL)
      )
   {
      printf ("detShow:          PWFS2\n");
      printf ("detShow:          -----\n");

      if ( detSdsuIdP2 != NULL )
      {
         if ( sdsuShow (detSdsuIdP2, verbose) != ERROR )
         {
            if (detObsIdP2 != NULL)
            {
               detObsShow (detObsIdP2, verbose);
            }
         }
         else
         {
            printf ("detShow: SDSU controller context for PWFS2 invalid.\n");
         }
      }
      else
      {
         printf ("detShow: SDSU controller for PWFS2 not initialised.\n");
      }
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detStatusShow
 *
 *   INVOCATION:
 *   detStatusShow (pWfsName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName   (const char *)   Name of WFS
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show status parameters of detector control task
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the status parameters of the
 *   detector control task.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdP2 (SDSU_ID) SDSU context structure for PWFS2
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detStatusShow
   (
   const char *   pWfsName
   )
{
   /*
    * Display the SDSU status parameters for PWFS2
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "p2") != NULL) || (strstr(pWfsName, "pwfs2") != NULL)
      )
   {
      printf ("detStatusShow:          PWFS2\n");
      printf ("detStatusShow:          -----\n");

      if ( detSdsuIdP2 != NULL )
      {
         if ( sdsuStatusShow (detSdsuIdP2) == ERROR )
         {
            printf (
            "detStatusShow: SDSU controller context for PWFS2 invalid.\n");
         }
      }
      else
      {
         printf ("detStatusShow: SDSU controller for PWFS2 not initialised.\n");
      }
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTempShow
 *
 *   INVOCATION:
 *   detTempShow (pWfsName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of WFS
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show temperature parameters of detector control task
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the temperature parameters
 *   of the detector control task.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdP2 (SDSU_ID) SDSU context structure for PWFS2
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detTempShow
   (
   const char *   pWfsName
   )
{
   /*
    * Display the SDSU status parameters for PWFS2
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "p2") != NULL) || (strstr(pWfsName, "pwfs2") != NULL)
      )
   {

      printf ("detTempShow:          PWFS2\n");
      printf ("detTempShow:          -----\n");

      if ( detSdsuIdP2 != NULL )
      {
         if ( sdsuTempShow (detSdsuIdP2) == ERROR )
         {
           printf ("detTempShow: SDSU controller context for PWFS2 invalid.\n");
         }
      }
      else
      {
         printf ("detTempShow: SDSU controller for PWFS2 not initialised.\n");
      }
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObsShow
 *
 *   INVOCATION:
 *   detObsShow (obsId, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) obsId   (OBS_ID)      Pointer to observation ID
 *   (>) verbose (const BOOL)  Enable verbose mode
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of an observation ID structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises an observation ID structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detObsShow
   (
   OBS_ID      obsId,
   const BOOL   verbose
   )
{
   const char *   outOptionStrings[3] =
      {
         "NONE", "DHS", "FILE"
      };

   const char *   dhsOutOptionStrings[3] =
      {
         "PERM", "TEMP", "QL"
      };

   /* Check the observation context structure is valid. */

   if ( obsId == NULL )
   {
      ERROR_SET (0, "Invalid observation context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Display the contents of the observation context structure. */

   printf ("Contents of observation context structure at %p:\n", obsId);
   printf ("--------------------------------------------------------\n");
   printf ("Associated SDSU context          : %p\n", obsId->sdsuId);
   printf ("Observing?                       : %s\n",
           (obsId->observing ? "YES" : "NO") );
   printf ("stopped                          : %s\n",
           (obsId->stopped ? "TRUE" : "FALSE") );
   printf ("continuous                       : %s\n",
           (obsId->continuous ? "TRUE" : "FALSE") );
   printf ("Total number of frames           : %d\n", obsId->totalFrames);
   printf ("Out number of frames             : %d\n", obsId->outNFrames);
   printf ("Frame counter                    : %d\n", obsId->nframes);

   printf ("Alarm timer ID                   : %d\n", (int) obsId->timeId);

   printf ("Name of wfs                      : %s\n", obsId->pWfsName );
   printf ("Observation type                 : %s\n", obsId->pObsType );

   printf ("Output options                   : %s\n",
           outOptionStrings[obsId->outOptions] );
   printf ("DHS output options               : %s\n",
           dhsOutOptionStrings[obsId->dhsOutOptions] );
   printf ("pCurFrame                        : %p\n",
            obsId->pCurFrame );
   printf ("Size of frame in pixels for DHS (X x Y)  : %d x %d\n", 
           obsId->xPixelsDhs, obsId->yPixelsDhs);
   printf ("dhsQlRate                        : %d\n",
           obsId->dhsQlRate );
   printf ("dhsCounter                       : %d\n",
           obsId->dhsCounter );
   printf ("Number of detector outputs       : %ld\n", obsId->outputs);
   printf ("Data label                       : %s\n", obsId->pDataLabel);
   printf ("Output data file name            : %s\n", obsId->pOutFileName);
   printf ("Simulated data file name         : %s (simulate=%s)\n",
   obsId->pSimFileName,
   ((obsId->sdsuId == NULL) ? "DON'T KNOW" : (obsId->sdsuId->simulate ? " YES" : "NO")) );

   printf ("AO CCD geometry context          : %p\n", obsId->aoCcdId);
   printf ("AO control context               : %p\n", obsId->aoCtrlId);
   printf ("AO control CB context            : %p\n", obsId->aoCbAoCtrlId);
   printf ("AO FG control CB context         : %p\n", obsId->aoCbFgCtrlId);
   printf ("AO image CB context              : %p\n", obsId->aoCbImId);
   printf ("save image CB                    : %s\n",
           (obsId->saveCbIm ? "TRUE" : "FALSE") );
   printf ("save control CB                  : %s\n",
           (obsId->saveCbAoCtrl ? "TRUE" : "FALSE") );
   printf ("save FG control CB               : %s\n",
           (obsId->saveCbFgCtrl ? "TRUE" : "FALSE") );
   printf ("Signal processing mode           : %d\n", (int)obsId->sigMode);
   printf ("Number of frames to coadd        : %d\n", (int)obsId->nCoaddFrames);
   printf ("Coadd counter                    : %d\n", (int)obsId->coaddCounter);
   printf ("saveAoCbCounter                    : %d\n", 
           (int)(obsId->saveAoCbCounter) );
   printf ("saveFgCbCounter                  : %d\n", 
           (int)(obsId->saveFgCbCounter) );
   printf ("UpdateFgScale                    : %s\n",
           (obsId->updateFgScale ? "TRUE" : "FALSE") );
   printf ("UpdateAoScale                    : %s\n",
           (obsId->updateAoScale ? "TRUE" : "FALSE") );
   printf ("SaveCentroids                    : %s\n",
           (obsId->saveCentroids ? "TRUE" : "FALSE") );
   printf ("nMode                            : %d\n", (int)(obsId->nMode) );
   printf ("amplitude                        : %f\n", obsId->amplitude );
   printf ("methodThreshComp                 : %d\n", 
           (int)obsId->methodThreshComp);
   printf ("nAverageDataThreshComp           : %d\n",
           (int)obsId->nAverageDataThreshComp);
   printf ("saveCbFgCtrlClosedLoop           : %s\n",
           (obsId->saveCbFgCtrlClosedLoop ? "TRUE" : "FALSE") );
   printf ("saveCbFgCtrlClosedLoopFrame      : %d\n",
           (int)obsId->saveCbFgCtrlClosedLoopFrame);
   printf ("saveCbAoCtrlClosedLoop             : %s\n",
           (obsId->saveCbAoCtrlClosedLoop ? "TRUE" : "FALSE") );
   printf ("saveCbAoCtrlClosedLoopFrame        : %d\n",
           (int)obsId->saveCbAoCtrlClosedLoopFrame);
   printf ("Number of frames with FG only    : %d\n", 
           (int)(obsId->ggFrame) );
   printf ("methodFluxComp                   : %d\n", 
           (int)obsId->methodFluxComp);
   printf ("Average flux flag                : %s\n", 
           (obsId->averageFluxFlag ? "TRUE" : "FALSE") );
   printf ("Threshold flag                   : %s\n", 
           (obsId->threshFlag ? "TRUE" : "FALSE") );
   printf ("nFramesAverageFlux               : %d\n", 
           (int)(obsId->nFramesAverageFlux) );
   printf ("thresholdin real time flag       : %s\n",
           (obsId->threshRealTimeFlag ? "TRUE" : "FALSE") );
   printf ("writeToRm flag                   : %s\n",
           (obsId->writeToRm ? "TRUE" : "FALSE") );
   printf ("Time with GG only                : %f sec\n", (obsId->ggTime) );
   printf ("Time to average aO data          : %f sec\n", (obsId->aoTime) );
   printf ("saveCbFgCtrlClosedLoopTime       : %f sec\n",
           obsId->saveCbFgCtrlClosedLoopTime);
   printf ("saveCbAoCtrlClosedLoopTime         : %f sec\n",
           obsId->saveCbAoCtrlClosedLoopTime);
   printf ("rateBrightPixThreshComp          : %f\n",
           obsId->rateBrightPixThreshComp);
   printf ("multCoeffRmsThreshComp           : %f\n",
           obsId->multCoeffRmsThreshComp);
   printf ("averageRms                       : %f\n", obsId->averageRms);
   printf ("averageMean                      : %f\n", obsId->averageMean);
   printf ("multCoeffAverageFlux             : %f\n",
           obsId->multCoeffAverageFlux);
   printf ("averageFlux                      : %f\n", obsId->averageFlux);
   printf ("Tip scale                        : %f\n", obsId->tipScale);
   printf ("Tilt scale                       : %f\n", obsId->tiltScale);
   printf ("Focus scale                      : %f\n", obsId->focusScale);
   printf ("Sliding Focus gain               : %f\n", obsId->slidingFocusGain);
   printf ("aoScaleVect                      : %p\n", obsId->aoScaleVect);
   printf ("Coadd file name                  : %s\n", obsId->pCoaddFileName);
   printf ("Centroids file name              : %s\n", obsId->pCentFileName);
   printf ("Centroids comments               : %s\n", obsId->pCentComment);
   printf ("Save circular buffer directory   : %s\n", obsId->pCbPath);
   printf ("Save control CB directory (seq)  : %s\n", obsId->pCbPathSeq);

   printf ("dataSec                          : %s\n", obsId->dataSec);
   printf ("ccdSec                           : %s\n", obsId->ccdSec);
   printf ("origSec                          : %s\n", obsId->origSec);
   printf ("utStartString                    : %s\n", obsId->utStartString);
   printf ("utEndString                      : %s\n", obsId->utEndString);
   printf ("detType                          : %s\n", obsId->detType);
   printf ("detId                            : %s\n", obsId->detId);

   printf ("Time at observation start/end    : %f %f\n", obsId->rawtStart,
           obsId->rawtEnd);
   printf ("Exposure time in seconds         : %f\n", obsId->exposureTime);
   printf ("Cutoff frequency in Hz           : %f\n", obsId->cutoffFrequency);
   printf ("Rate sampling frequency          : %f\n",
           obsId->rateSamplingFrequency);
   printf ("Exposure in seconds reqst/actual : %f %f\n", obsId->exposedRQ,
           obsId->exposed);

   printf ("wcsStatus                        : %d\n", obsId->wcsStatus);
   printf ("nWcsPoints                       : %d\n", obsId->nWcsPoints);
   printf ("Axis 1 world coordinate info.    : %s %f %f\n",
      obsId->ctype1, obsId->crpix1, obsId->crval1);
   printf ("Axis 2 world coordinate info.    : %s %f %f\n",
      obsId->ctype2, obsId->crpix2, obsId->crval2);
   printf ("Rotation/skew matrix             : %f %f %f %f\n",
      obsId->cd1_1, obsId->cd1_2, obsId->cd2_1, obsId->cd2_2);
   printf ("RA, Dec, epoch                   : %f %f %f\n",
      obsId->RA, obsId->Dec, obsId->epoch);
   printf ("Radecsys, equinox, mjd           : %s %f %f\n",
      obsId->radecsys, obsId->equinox, obsId->mjdobs);

   printf ("\n");         /* Blank line for spacing */

   if ( obsId->aoCcdId != NULL )
      aoCcdContextShow (obsId->aoCcdId);
   printf ("\n");         /* Blank line for spacing */

   if ( obsId->aoCtrlId != NULL )
      aoCtrlContextShow (obsId->aoCcdId, obsId->aoCtrlId, FALSE);
   printf ("\n\n");         /* Blank line for spacing */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInit
 *
 *   INVOCATION:
 *   detSigInit (cadCmdContext, commandNumber, sdsuId, obsId);
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInit command
 *
 *   DESCRIPTION:
 *   This function updates the AO control context structure
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInit
   (
   CAD_CMD_CONTEXT cadCmdContext,         /* CAD command context structure    */
   int             commandNumber,         /* Command number                   */
   SDSU_ID         sdsuId,                /* SDSU context structure           */
   OBS_ID          obsId                  /* Observation context structure    */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pDarkFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFlatFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pRefFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pAoIntMatFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pAoContMatFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFgContMatFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullDarkFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pFullFlatFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pFullRefFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pFullAoIntMatFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pFullAoContMatFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pFullFgContMatFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   double       refX, refY;
   double       angleWithM2;
   double       angleWithM1;

   /*
    * Initialise the error number and get the attributes provided with this
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pDarkFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pFlatFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, 
                          (char *)&angleWithM2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&refX);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&refY);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, 
                          (char *)&angleWithM1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, pRefFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, pAoIntMatFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, pAoContMatFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10, pFgContMatFileName);

   /*
    * Check there are valid context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCcdId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL,
                 "AO CCD geometry context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCtrlId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "AO control context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together.
    * Ignore the file path if not provided.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullDarkFileName, pDarkFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullFlatFileName, pFlatFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullRefFileName, pRefFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullAoIntMatFileName, pAoIntMatFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullAoContMatFileName, pAoContMatFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullFgContMatFileName, pFgContMatFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      sprintf (pFullDarkFileName, "%s/%s", pFilePath, pDarkFileName);
      sprintf (pFullFlatFileName, "%s/%s", pFilePath, pFlatFileName);
      sprintf (pFullRefFileName, "%s/%s", pFilePath, pRefFileName);
      sprintf (pFullAoIntMatFileName, "%s/%s", pFilePath, pAoIntMatFileName);
      sprintf (pFullAoContMatFileName, "%s/%s", pFilePath, pAoContMatFileName);
      sprintf (pFullFgContMatFileName, "%s/%s", pFilePath, pFgContMatFileName);
   }

   MESSAGE_LOG (MSG_LOG, "Initialising signal processing ...");

   /*
    * Update the AO control context structure
    */

   if (aoCtrlContextUpdate ( pFullDarkFileName, pFullFlatFileName,
                             pFullRefFileName, pFullAoIntMatFileName,
                             pFullAoContMatFileName, pFullFgContMatFileName,
                             refX, refY, angleWithM2, angleWithM1, 
                             obsId->aoCcdId, obsId->aoCtrlId ) == ERROR )
   {
      ERROR_SET (0, "Failed to update AO control context", ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   angleWithM1 = obsId->aoCtrlId->angleWithM1;
   angleWithM2 = obsId->aoCtrlId->angleWithM2;

   aoCtrlContextShow (obsId->aoCcdId, obsId->aoCtrlId, FALSE);

   if ( obsId->aoCtrlId->initFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, "Initialized", obsId->pAoCtrlInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not Initialized", obsId->pAoCtrlInitContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
      }
   }

   if ( obsId->aoCtrlId->darkInitFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, obsId->aoCtrlId->darkFileName, 
                           obsId->pAoDarkInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
      }
   }

   if ( obsId->aoCtrlId->flatInitFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, obsId->aoCtrlId->flatFileName, 
                           obsId->pAoFlatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
      }
   }

   if ( obsId->aoCtrlId->aoIntMatInitFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, obsId->aoCtrlId->aoIntMatFileName, 
                           obsId->pAoIntMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
      }
   }

   if ( obsId->aoCtrlId->aoContMatInitFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, obsId->aoCtrlId->aoContMatFileName, 
                           obsId->pAoContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not initialized", 
                           obsId->pAoContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
      }
   }

   if ( obsId->aoCtrlId->fgContMatInitFlag == TRUE )
   {
      if (epToVxPipeWrite (NULL, obsId->aoCtrlId->fgContMatFileName, 
                           obsId->pFgContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "Not initialized", 
                           obsId->pFgContMatInitContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitAoGain
 *
 *   INVOCATION:
 *   detSigInitAoGain (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitAoGain command
 *
 *   DESCRIPTION:
 *   This function updates aO gains in open and closed loop
 * 
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitAoGain
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   AO_VECT      aoScaleVect;

   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCtrlId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL,
                 "AO control context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   };

   if ( obsId->observing )
   {

#ifdef DEBUG
      printf ( "Observation in progress, update aO scale factors\n" ) ;
#endif

      /*
       * Get the attributes provided with this command.
       */

      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                             (char *)&(obsId->aoScaleVect[0]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                             (char *)&(obsId->aoScaleVect[1]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                             (char *)&(obsId->aoScaleVect[2]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                             (char *)&(obsId->aoScaleVect[3]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4,
                             (char *)&(obsId->aoScaleVect[4]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5,
                             (char *)&(obsId->aoScaleVect[5]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6,
                             (char *)&(obsId->aoScaleVect[6]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7,
                             (char *)&(obsId->aoScaleVect[7]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8,
                             (char *)&(obsId->aoScaleVect[8]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9,
                             (char *)&(obsId->aoScaleVect[9]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10,
                             (char *)&(obsId->aoScaleVect[10]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 11,
                             (char *)&(obsId->aoScaleVect[11]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 12,
                             (char *)&(obsId->aoScaleVect[12]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 13,
                             (char *)&(obsId->aoScaleVect[13]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 14,
                             (char *)&(obsId->aoScaleVect[14]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 15,
                             (char *)&(obsId->aoScaleVect[15]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 16,
                             (char *)&(obsId->aoScaleVect[16]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 17,
                             (char *)&(obsId->aoScaleVect[17]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 18,
                             (char *)&(obsId->aoScaleVect[18]));
      obsId->updateAoScale = TRUE ;
   }
   else /* observation not in progress */
   {
#ifdef DEBUG
      printf ( "Observation not in progress, update aO scale factors... \n" ) ;
#endif

      MESSAGE_LOG (MSG_LOG, "Update aO scale factors during open loop..." ) ;

      /*
       * Get the attributes provided with this command.
       */

      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                             (char *)&(aoScaleVect[0]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                             (char *)&(aoScaleVect[1]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, 
                             (char *)&(aoScaleVect[2]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, 
                             (char *)&(aoScaleVect[3]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, 
                             (char *)&(aoScaleVect[4]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, 
                             (char *)&(aoScaleVect[5]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, 
                             (char *)&(aoScaleVect[6]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, 
                             (char *)&(aoScaleVect[7]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, 
                             (char *)&(aoScaleVect[8]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, 
                             (char *)&(aoScaleVect[9]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10, 
                             (char *)&(aoScaleVect[10]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 11, 
                             (char *)&(aoScaleVect[11]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 12, 
                             (char *)&(aoScaleVect[12]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 13, 
                             (char *)&(aoScaleVect[13]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 14, 
                             (char *)&(aoScaleVect[14]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 15, 
                             (char *)&(aoScaleVect[15]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 16, 
                             (char *)&(aoScaleVect[16])); 
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 17, 
                             (char *)&(aoScaleVect[17]));
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 18, 
                             (char *)&(aoScaleVect[18]));

      if ( aoScaleUpdate (aoScaleVect, obsId->aoCtrlId) == ERROR )
      {
         ERROR_SET (0, "Failed to update aO scale factors",
                    ERROR_LOG_NOW);
         errorNumber = S_detControl_INTERNAL;
         return (errorNumber);
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitFgGain
 *
 *   INVOCATION:
 *   detSigInitFgGain (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitFgGain command
 *
 *   DESCRIPTION:
 *   This function updates the gains of FG Zernikes mode in open and closed loop
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitFgGain
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32         errorNumber;    /* Error number reported by task.           */
   double         tipScale;
   double         tiltScale;
   double         focusScale;
   double         slidingFocusGain;

   /*
    * Initialise the error number and get the attributes provided with this
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&tipScale);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&tiltScale);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&focusScale);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *)&slidingFocusGain);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId->aoCtrlId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL,
                 "AO control context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   };

   /*
    * The command can be used when an observation is or in not in progress.
    */

   if ( obsId->observing )
   {
#ifdef DEBUG
      printf ( "Init FG Gain when observation in progress\n" ) ;
#endif

      obsId->tipScale = tipScale ;
      obsId->tiltScale = tiltScale ;
      obsId->focusScale = focusScale ;
      obsId->slidingFocusGain = slidingFocusGain ;
      obsId->updateFgScale = TRUE ;
   }
   else
   {
#ifdef DEBUG
      printf ( "Init FG Gain when observation is not in progress\n" ) ;
#endif

      MESSAGE_LOG (MSG_LOG, "Update FG scale factors during open loop..." ) ;

      obsId->aoCtrlId->fgScaleFactorVect[0] = tipScale ;
      obsId->aoCtrlId->fgScaleFactorVect[1] = tiltScale ;
      obsId->aoCtrlId->fgScaleFactorVect[2] = focusScale ;
      obsId->aoCtrlId->slidingFocusGain = slidingFocusGain;
      obsId->aoCtrlId->one_slidingFocusGain = 1.0 - slidingFocusGain ;
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeNone
 *
 *   INVOCATION:
 *   detSigModeNone (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeNone command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to no processing.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeNone
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
                                /* closed loop sequence                       */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_NONE;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG (MSG_LOG, "Switching signal processing off");
   if (epToVxPipeWrite (NULL, "No processing",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   if (detDhsInitialised)
      outOption = 1 ;      /* DHS */
   else
      outOption = 0 ;      /* NO DHS */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeDark
 *
 *   INVOCATION:
 *   detSigModeDark (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeDark command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to dark subtraction
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeDark
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */

   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_DARK;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG (MSG_LOG,
   "Signal processing switched to \"Subtract Dark\" mode");
   if (epToVxPipeWrite (NULL, "Subtract dark",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   if (detDhsInitialised)
      outOption = 1 ;      /* DHS */
   else
      outOption = 0 ;      /* NO DHS */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeGg
 *
 *   INVOCATION:
 *   detSigModeGg (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeGg command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to global guide.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */


uint32 detSigModeGg
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&flag);
   sigMode = AO_MODE_GG;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG1 (MSG_LOG,
                 "Signal processing switched to \"Global Guide\" mode, flag=%d",
                 (int)flag);
   if (epToVxPipeWrite (NULL, "Global Guide",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;

   obsId->writeToRm = flag;

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeGgAo
 *
 *   INVOCATION:
 *   detSigModeGgAo (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeGgAo command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to global guide and 
 *   aO correction.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeGgAo
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   double       aoTime;         /* Time used to average images for aO         */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&aoTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&subapOff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&flag);

   sigMode = AO_MODE_GG_AO;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG3 (MSG_LOG,
       "Signal processing switched to \"Global Guide and aO correction\" mode "
       "aoTime=%f s, allowedSubapOff=%d, flag=%d",
       aoTime, (int)subapOff, (int)flag);
   if (epToVxPipeWrite (NULL, "Global guide and aO",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->aoTime = aoTime;
   obsId->aoCtrlId->allowedSubapOff = subapOff;
   obsId->writeToRm = flag;

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeAo
 *
 *   INVOCATION:
 *   detSigModeAo (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeAo command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to aO correction only.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeAo
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       aoTime;         /* Time used to average images for aO         */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&aoTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&subapOff);

   sigMode = AO_MODE_AO;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG2 (MSG_LOG,
           "Signal processing switched to \"aO correction\" mode "
           "aoTime=%f sec, allowedSubapOff=%d",
           aoTime, (int)subapOff);
   if (epToVxPipeWrite (NULL, "aO",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->aoTime = aoTime;
   obsId->aoCtrlId->allowedSubapOff = subapOff;

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeFgFocus
 *
 *   INVOCATION:
 *   detSigModeFgFocus (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                      obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeFgFocus command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to Fast Guide and Focus 
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeFgFocus
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   long         threshRT;       /* Flag to indicate if the thresholds are     */
                                /* computed in real time                      */
   double       expTime;        /* Exposure time                              */
   double       rateBright;     /* Rate of brightest pixels.                  */
   double       multCoeff;      /* Multiplicative coefficients for rms value  */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&subapOff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&flag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&threshRT);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & rateBright);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4,
                          (char *) & multCoeff);

   sigMode = AO_MODE_FG_FOCUS;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   rateBright = rateBright/100.0; /* in percent */

   MESSAGE_LOG3 (MSG_LOG,
           "Signal processing switched to \"FG and Focus\" mode "
           "allowedSubapOff=%d, threshRT=%d, flag=%d", 
           (int)subapOff, (int) threshRT, (int)flag);
   MESSAGE_LOG2 (MSG_LOG, "rate=%f, coeffRms=%f", 
           (float)rateBright, (float) multCoeff);
   if (epToVxPipeWrite (NULL, "Fast Guide and Focus",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->threshRealTimeFlag = threshRT;
   obsId->aoCtrlId->allowedSubapOff = subapOff;
   obsId->writeToRm = flag;
   if ( threshRT == TRUE )
   {
      obsId->rateBrightPixThreshComp = rateBright;
      obsId->aoCtrlId->thresholdRate = rateBright;
      obsId->multCoeffRmsThreshComp = multCoeff;
      obsId->aoCtrlId->thresholdMultCoeff = multCoeff;
   }

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeFgCoadd
 *
 *   INVOCATION:
 *   detSigModeFgCoadd (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                      obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeFgCoadd command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to fast guide and focus and 
 *   coadd.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeFgCoadd
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         nCoaddFrames;   /* Number of frames to coadd.                 */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   long         threshRT;       /* Flag to indicate if the thresholds are     */
                                /* computed in real time                      */
   double       expTime;        /* Exposure time                              */
   double       rateBright;     /* Rate of brightest pixels.                  */
   double       multCoeff;      /* Multiplicative coefficients for rms value  */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pCoaddFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCoaddFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_FG_FOCUS_COADD;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & subapOff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) & nCoaddFrames);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pCoaddFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&flag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&threshRT);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6,
                          (char *) & rateBright);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7,
                          (char *) & multCoeff);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */
   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   rateBright = rateBright/100.0; /* in percent */

   MESSAGE_LOG1 (MSG_LOG,
   "Signal processing switched to \"FG Focus + Coadd\" mode - nCoaddFrames=%ld",
   nCoaddFrames);
   MESSAGE_LOG2 (MSG_LOG, "flag=%d, Compute thresholds in real time=%d", 
                 (int)flag, (int)threshRT );
   MESSAGE_LOG2 (MSG_LOG, "rate=%f, coeffRms=%f",
                (float)rateBright, (float) multCoeff);

   if (epToVxPipeWrite (NULL, "Fast Guide, Focus and Coadd",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
        "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* Combine file and path name for coadd file name */

   detCreateFileName ( pFilePath ,
                       pCoaddFileName ,
                       pFullCoaddFileName ) ;

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->writeToRm = flag;
   obsId->threshRealTimeFlag = threshRT;
   obsId->aoCtrlId->allowedSubapOff = subapOff;
   obsId->nCoaddFrames = nCoaddFrames;
   strncpy( obsId->pCoaddFileName, pFullCoaddFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );

   if ( threshRT == TRUE )
   {                
      obsId->rateBrightPixThreshComp = rateBright;
      obsId->aoCtrlId->thresholdRate = rateBright;
      obsId->multCoeffRmsThreshComp = multCoeff;
      obsId->aoCtrlId->thresholdMultCoeff = multCoeff;
   }

   /* Init the fields of the observe CAD record */

   nExp = nCoaddFrames; /* nCoaddFrames exposures */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeFgFocusAo
 *
 *   INVOCATION:
 *   detSigModeFgFocusAo (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                        obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeFgFocusAo command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to Fast Guide and Focus and
 *   aO correction.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeFgFocusAo
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   long         threshRT;       /* Flag to indicate if the thresholds are     */
                                /* computed in real time                      */
   double       aoTime;         /* Time used to average images for aO         */
   double       expTime;        /* Exposure time                              */
   double       rateBright;     /* Rate of brightest pixels.                  */
   double       multCoeff;      /* Multiplicative coefficients for rms value  */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&aoTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&subapOff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&flag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&threshRT);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4,
                          (char *) & rateBright);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5,
                          (char *) & multCoeff);

   sigMode = AO_MODE_FG_FOCUS_AO;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   rateBright = rateBright/100.0; /* in percent */

   MESSAGE_LOG3 (MSG_LOG,
           "Signal processing switched to \"FG and Focus and aO\" mode "
           "aoTime=%f s, allowedSubapOff=%d, flag=%d",
           aoTime, (int)subapOff, (int)flag);
   MESSAGE_LOG2 (MSG_LOG, "rate=%f, coeffRms=%f",
                (float)rateBright, (float) multCoeff);

   if (epToVxPipeWrite (NULL, "Fast Guide, Focus and aO",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->writeToRm = flag;

   obsId->threshRealTimeFlag = threshRT;

   obsId->sigMode = sigMode;
   obsId->aoTime = aoTime;
   obsId->aoCtrlId->allowedSubapOff = subapOff;

   if ( threshRT == TRUE )
   {                
      obsId->rateBrightPixThreshComp = rateBright;
      obsId->aoCtrlId->thresholdRate = rateBright;
      obsId->multCoeffRmsThreshComp = multCoeff;
      obsId->aoCtrlId->thresholdMultCoeff = multCoeff;
   }

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeCoadd
 *
 *   INVOCATION:
 *   detSigModeCoadd (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                    obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeCoadd command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to COADD mode.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeCoadd
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         nCoaddFrames;   /* Number of frames to coadd.                 */

   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pCoaddFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCoaddFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];


   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_COADD;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & nCoaddFrames);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pCoaddFileName);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG1 (MSG_LOG,
         "Signal processing switched to \"Coadd Only\" mode - nCoaddFrames=%ld",
         nCoaddFrames);
   if (epToVxPipeWrite (NULL, "Coadd only",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* Combine file and path name for coadd file name */

   detCreateFileName ( pFilePath ,
                       pCoaddFileName ,
                       pFullCoaddFileName ) ;

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->nCoaddFrames = nCoaddFrames;
   strncpy( obsId->pCoaddFileName, pFullCoaddFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );

   /* Init the fields of the observe CAD record */

   nExp = nCoaddFrames ;      /* nCoaddFrames exposures */
   outOption = 0 ;            /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;        /* 10ms */
   else
      expTime = 0.005 ;       /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeThresh
 *
 *   INVOCATION:
 *   detSigModeThresh (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                     obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeThresh command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to threshold computation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeThresh
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         method;         /* Method for threshold computation.          */
   long         i;              /* Index                                      */
   long         nAverageData;   /* Number of data to average.                 */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   double       expTime;        /* Exposure time                              */
   double       rateBright;     /* Rate of brightest pixels.                  */
   double       multCoeff;      /* Multiplicative coefficients for rms value  */
   double       threshold;      /* Threshold value if no computation          */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_THRESH;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &method);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) &nAverageData);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                          (char *) & rateBright);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & multCoeff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4,
                          (char *) & threshold);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&flag);


   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   rateBright = rateBright/100.0; /* in percent */

   MESSAGE_LOG4 (MSG_LOG,
            "Signal processing switched to \"Threshold Computation\" mode - "
            "method=%d, nAverageData=%d, rateBright=%f, multCoeff=%f",
            (int)method, (int)nAverageData, rateBright, multCoeff);
   MESSAGE_LOG1 (MSG_LOG, "flag=%d", (int)flag);


   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->writeToRm = flag;

   obsId->methodThreshComp = method;
   obsId->aoCtrlId->thresholdMethod = method;
   if ( obsId->methodThreshComp == AO_THRESH_VALUE )
   {
      /* no computation requested */
      obsId->aoCtrlId->threshold = threshold;

      for ( i = 0 ; i < obsId->aoCcdId->subapUsedNb ; i ++ )
          obsId->aoCtrlId->thresholdVect[i] = threshold;

      if (epToVxPipeWrite (NULL, (char *)(int)& (obsId->aoCtrlId->threshold),
                           obsId->pAoThreshContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
      }
   }
   else
   {
      obsId->nAverageDataThreshComp = nAverageData;
      obsId->rateBrightPixThreshComp = rateBright;
      obsId->aoCtrlId->thresholdRate = rateBright;
      obsId->multCoeffRmsThreshComp = multCoeff;
      obsId->aoCtrlId->thresholdMultCoeff = multCoeff;
      obsId->sigMode = sigMode;
      if (epToVxPipeWrite (NULL, "Threshold computation",
                           obsId->pAoProcessModeContext) == ERROR)
      {
         ERROR_LOG (
              "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
      }

      /* Init the fields of the observe CAD record */

      nExp = nAverageData ;     /* nAverageData exposure */
      outOption = 0 ;           /* NONE */
      if ( obsId->aoCcdId->binningFlag == FALSE )
         expTime = 0.01 ;       /* 10ms */
      else
         expTime = 0.005 ;      /* 5ms */

      if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
           ERROR )
      {
         ERROR_LOG ( "Failed to init fields of observe record");
      }
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeGgCoadd
 *
 *   INVOCATION:
 *   detSigModeGgCoadd (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                      obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeGgCoadd command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to global guide and coadd.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeGgCoadd
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         nCoaddFrames;   /* Number of frames to coadd.                 */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   double       expTime;        /* Exposure time                              */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pCoaddFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCoaddFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_GG_COADD;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & nCoaddFrames);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pCoaddFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&flag);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */
   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG2 (MSG_LOG,
   "Signal processing switched to \"GG + Coadd\" mode - nCoaddFrames=%ld, flag=%d",
   nCoaddFrames, (int)flag);
   if (epToVxPipeWrite (NULL, "Global Guide and Coadd",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
        "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* Combine file and path name for coadd file name */

   detCreateFileName ( pFilePath ,
                       pCoaddFileName ,
                       pFullCoaddFileName ) ;

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->writeToRm = flag;

   obsId->sigMode = sigMode;
   obsId->nCoaddFrames = nCoaddFrames;
   strncpy( obsId->pCoaddFileName, pFullCoaddFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );

   /* Init the fields of the observe CAD record */

   nExp = nCoaddFrames ;     /* nCoaddFrames exposures */
   outOption = 0 ;           /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;       /* 10ms */
   else
      expTime = 0.005 ;      /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeSeq
 *
 *   INVOCATION:
 *   detSigModeSeq (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeSeq command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to sequence closed loop.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */
uint32 detSigModeSeq
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   double       ggTime;         /* Time when fg over the whole CCD in the     */
                                /* closed loop sequence                       */
   long         saveCbFgCtrlClosedLoopFlag;
                                /* Save FG control circular buffer during     */
                                /* closed loop flag.                          */
   long         saveCbAoCtrlClosedLoopFlag;
                                /* Save aO control circular buffer during     */
                                /* closed loop flag.                          */
   long         threshFlag;     /* Threshold after GG Flag                    */
   long         nFramesThresh;  /* Number of frames to compute the threshold  */
   long         fluxFlag;       /* Average flux after FG Flag                 */
   long         nFramesFlux;    /* Number of frames to average for computing  */
                                /* the average flux                           */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         aoFlag;         /* aO flag (yes or no)                        */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   long         threshRT;       /* Flag to indicate if the thresholds are     */
                                /* computed in real time                      */
   double       aoTime;         /* Time used to average images for aO         */
   double       expTime;        /* Exposure time                              */
   double       rateBright;     /* Rate of brightest pixels.                  */
   double       multCoeffFlux;  /* Multiplicative coefficient for average flux*/
   double       saveCbAoCtrlEveryTime;
                                /* Time when to save the aO control circular  */
                                /* buffer in the closed loop sequence         */
   double       saveCbFgCtrlEveryTime;
                                /* Time when to save the FG control circular  */
                                /* buffer in the closed loop sequence         */
   double       rateBrightThreshRT;
                                /* Rate of brightest pixels.                  */
   double       multCoeffThreshRT;      
                                /* Multiplicative coefficients for rms value  */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                /* Path name for circular buffer.             */


   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_CLOSED_LOOP;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & ggTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                          (char *) & threshFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                          (char *) & nFramesThresh);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & rateBright);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *) & fluxFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5,
                          (char *) & nFramesFlux);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6,
                          (char *) & multCoeffFlux);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, (char *)&subapOff);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, (char *)&aoTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9,
                          (char *) & saveCbAoCtrlClosedLoopFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10,
                          (char *) & saveCbAoCtrlEveryTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 11,
                          (char *) & saveCbFgCtrlClosedLoopFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 12,
                          (char *) & saveCbFgCtrlEveryTime);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 13, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 14, (char *) & aoFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 15, (char *)&flag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 16, (char *)&threshRT);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 17,
                          (char *) & rateBrightThreshRT);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 18,
                          (char *) & multCoeffThreshRT);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   rateBright = rateBright / 100.0 ; /* in percent */
   multCoeffFlux = multCoeffFlux / 100.0 ; /* in percent */

   rateBrightThreshRT = rateBrightThreshRT / 100.0 ; /* in percent */

   MESSAGE_LOG1 (MSG_LOG,
      "Signal processing switched to \"Sequence closed loop\" mode - "
      "ggTime=%f", ggTime );
   MESSAGE_LOG1 (MSG_LOG, "Compute thresholds in real time=%d", (int)threshRT );
   MESSAGE_LOG3 (MSG_LOG, "threshFlag=%d, nFramesThresh=%d, rateBright=%f",
                 (int)threshFlag, (int)nFramesThresh, rateBright);
   MESSAGE_LOG3 (MSG_LOG, "fluxFlag=%d, nFramesFlux=%d, multCoeffFlux=%f",
                 (int)fluxFlag, (int)nFramesFlux, multCoeffFlux);
   MESSAGE_LOG3 (MSG_LOG, 
                 "aoFlag=%d, aoTime=%f s, allowedSubapOff=%d",
                 (int)aoFlag, aoTime, (int)subapOff);
   MESSAGE_LOG2 (MSG_LOG, "saveCbFgCtrlFlag=%d, saveCbFgCtrlEveryTime=%f",
      (int)saveCbFgCtrlClosedLoopFlag, saveCbFgCtrlEveryTime);
   MESSAGE_LOG2 (MSG_LOG, "saveCbAoCtrlFlag=%d, saveCbAoCtrlEveryTime=%f",
      (int)saveCbAoCtrlClosedLoopFlag, saveCbAoCtrlEveryTime);
   MESSAGE_LOG1 (MSG_LOG, "pFilePath=%s", pFilePath);
   MESSAGE_LOG1 (MSG_LOG, "flag=%d", (int)flag);
   MESSAGE_LOG2 (MSG_LOG, "If threshold RT : rate=%f, coeffRms=%f",
                (float)rateBrightThreshRT, (float) multCoeffThreshRT);

   if (epToVxPipeWrite (NULL, "Sequence closed loop",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;

   obsId->writeToRm = flag;

   obsId->ggTime = ggTime;
   obsId->saveCbFgCtrlClosedLoop = saveCbFgCtrlClosedLoopFlag;
   obsId->saveCbFgCtrlClosedLoopTime = saveCbFgCtrlEveryTime;
   obsId->saveCbAoCtrlClosedLoop = saveCbAoCtrlClosedLoopFlag;
   obsId->saveCbAoCtrlClosedLoopTime = saveCbAoCtrlEveryTime;

   obsId->averageFluxFlag = fluxFlag;
   obsId->nFramesAverageFlux = nFramesFlux;
   obsId->multCoeffAverageFlux = multCoeffFlux;

   obsId->threshRealTimeFlag = threshRT;
   obsId->threshFlag = threshFlag;
   obsId->methodThreshComp = AO_THRESH_SPOTS;
   obsId->aoCtrlId->thresholdMethod = AO_THRESH_SPOTS;
   if ( threshRT == TRUE )
   {
      obsId->rateBrightPixThreshComp = rateBrightThreshRT;
      obsId->aoCtrlId->thresholdRate = rateBrightThreshRT;
      obsId->multCoeffRmsThreshComp = multCoeffThreshRT;
      obsId->aoCtrlId->thresholdMultCoeff = multCoeffThreshRT;
   }
   else
   {
      obsId->nAverageDataThreshComp = nFramesThresh;
      obsId->rateBrightPixThreshComp = rateBright;
      obsId->aoCtrlId->thresholdRate = rateBright;
   }

   if ( fluxFlag == TRUE )
      obsId->aoCtrlId->multCoeffTotal= multCoeffFlux;

   obsId->aoTime = aoTime;
   obsId->aoCtrlId->allowedSubapOff = subapOff;

   obsId->aoFlag = aoFlag;

   strcpy ( obsId->pCbPathSeq, pFilePath );

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeTotal
 *
 *   INVOCATION:
 *   detSigModeTotal (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                    obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeTotal command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to average flux computation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigModeTotal
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )

{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         method;         /* Method for average flux computation        */
   long         nFramesFlux;    /* Number of frames to average for computing  */
                                /* the average flux                           */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   long         flag;           /* writeToRm flag                             */
   double       expTime;        /* Exposure time                              */
   double       multCoeffFlux;  /* Multiplicative coefficient for average flux*/
   double       thresholdFlux;  /* threshold for flux value                   */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_TOTAL;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & method);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) & thresholdFlux);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                          (char *) & nFramesFlux);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & multCoeffFlux);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&flag);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   multCoeffFlux = multCoeffFlux / 100.0 ; /* in percent */

   MESSAGE_LOG4 (MSG_LOG,
            "Signal processing switched to \"Average Flux computation\" mode - "
            "method=%d, thresholdFlux=%f, nFramesFlux=%d, multCoeffFlux=%f",
             (int)method, thresholdFlux, (int)nFramesFlux, multCoeffFlux);
   MESSAGE_LOG1 (MSG_LOG, "flag=%d", (int)flag);

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->writeToRm = flag;

   obsId->methodFluxComp = method;
   obsId->aoCtrlId->totalMethod = method;
   if ( ( obsId->methodFluxComp == AO_TOTAL_VALUE ) ||
        ( obsId->methodFluxComp == AO_TOTAL_FORMULA ) )
   {
      /* no computation requested */
      if ( obsId->methodFluxComp == AO_TOTAL_VALUE )
      {
         obsId->aoCtrlId->totalThreshold = thresholdFlux;
      }
      else
      { 
         obsId->aoCtrlId->totalThreshold = 
         aoTotalThresholdCompute (obsId->aoCcdId, obsId->aoCtrlId);
      }

      if (epToVxPipeWrite (NULL,
                           (char *)(int)& (obsId->aoCtrlId->totalThreshold),
                           obsId->pAoTotalContext) == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
      }
   }
   else
   {
      obsId->nFramesAverageFlux = nFramesFlux;
      obsId->multCoeffAverageFlux = multCoeffFlux;
      obsId->aoCtrlId->multCoeffTotal= multCoeffFlux;
      obsId->sigMode = sigMode;
      if (epToVxPipeWrite (NULL, "Average flux computation",
                           obsId->pAoProcessModeContext) == ERROR)
      {
         ERROR_LOG (
            "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
      }

      /* Init the fields of the observe CAD record */

      nExp = nFramesFlux ;     /* nFramesFlux exposures */
      outOption = 0 ;          /* NONE */
      if ( obsId->aoCcdId->binningFlag == FALSE )
         expTime = 0.01 ;      /* 10ms */
      else
         expTime = 0.005 ;     /* 5ms */

      if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
           ERROR )
      {
         ERROR_LOG ( "Failed to init fields of observe record");
      }
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigSaveCb
 *
 *   INVOCATION:
 *   detSigSaveCb (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigSaveCb command
 *
 *   DESCRIPTION:
 *   This function sets the parameters to save the circular buffers.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigSaveCb
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         saveCbImFlag;   /* Save image circular buffer flag.           */
   long         saveCbAoCtrlFlag; /* Save control circular buffer flag.         */
   long         saveCbFgCtrlFlag; /* Save FG control circular buffer flag.    */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                /* Path name for circular buffer.             */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0,
                          (char *) & saveCbImFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1,
                          (char *) & saveCbAoCtrlFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                          (char *) & saveCbFgCtrlFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pFilePath);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
/*
      MESSAGE_LOG (MSG_WARNING,
      "NOTE: Changing AO processing parameters while observation in progress");
*/
   }

   if ( saveCbImFlag == TRUE )
   {
      /*MESSAGE_LOG (MSG_LOG, "Save image circular buffer set to TRUE");*/

      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbImContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME record");
      }
   }
   else
   {
      /*MESSAGE_LOG (MSG_LOG, "Save image circular buffer set to FALSE");*/

      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbImContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME record");
      }
   }

   if ( saveCbAoCtrlFlag == TRUE )
   {
      /*MESSAGE_LOG (MSG_LOG, "Save aO control circular buffer set to TRUE");*/

      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbAoCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME record");
      }
   }
   else
   {
      /*MESSAGE_LOG (MSG_LOG, "Save aO control circular buffer set to FALSE");*/

      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbAoCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME record");
      }
   }

   if ( saveCbFgCtrlFlag == TRUE )
   {
      /*MESSAGE_LOG (MSG_LOG, "Save FG control circular buffer set to TRUE");*/

      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbFgCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME record");
      }
   }
   else
   {
      /*MESSAGE_LOG (MSG_LOG, "Save FG control circular buffer set to FALSE");*/

      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbFgCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AOSAVECBCTRL_SIR_NAME record");
      }
   }

   /*
    * These parameters will be used in detObserveEnd.
    */

   obsId->saveCbIm = saveCbImFlag;
   obsId->saveCbAoCtrl = saveCbAoCtrlFlag;
   obsId->saveCbFgCtrl = saveCbFgCtrlFlag;
   strcpy ( obsId->pCbPath , pFilePath );

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigMeasAoIm
 *
 *   INVOCATION:
 *   detSigMeasAoIm (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigMeasAoIm command
 *
 *   DESCRIPTION:
 *   This function sets the parameters to measure a column of the interaction 
 *   matrix
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigMeasAoIm
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure          */
   int             commandNumber,   /* Command number                         */
   SDSU_ID         sdsuId,          /* SDSU context structure                 */
   OBS_ID          obsId            /* Observation context structure          */
   )
{
   uint32       errorNumber;    /* Error number reported by task              */
   long         sigMode;        /* Signal processing mode                     */
   long         subapOff;       /* Number of subapertures allowed to be off   */
   long         newMat;         /* New matrix flag                            */
   long         nMode;          /* Mode number                                */
   long         nCoaddFrames;   /* Number of frames to coadd                  */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */
   double       amplitude;      /* Amplitude of the mode                      */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files                          */
   char         pCoaddFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCoaddFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pCentFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCentFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & newMat);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & nMode);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2,
                          (char *) & amplitude);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & nCoaddFrames);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, pCoaddFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, pCentFileName);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */
   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG2 (MSG_LOG, "Measurement column IM for mode %d, amplitude=%f",
                 (int)nMode, (float)amplitude);

   MESSAGE_LOG1 (MSG_LOG,
   "Signal processing switched to \"Int Mat measurement\" mode - nCoaddFrames=%ld",
   nCoaddFrames);
   if (epToVxPipeWrite (NULL, "Interaction matrix measurement",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
        "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* Combine file and path name for coadd and centroids file names */

   detCreateFileName ( pFilePath ,
                       pCoaddFileName ,
                       pFullCoaddFileName ) ;

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullCentFileName, pCentFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {

      if ( strcmp(pCentFileName, "NONE") == 0 )
      {
         strncpy (pFullCentFileName, pCentFileName,
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullCentFileName, "%s/%s", pFilePath, pCentFileName );
      }
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   if ( newMat == TRUE )
   {
      aoIntMatStructZero (obsId->aoCtrlId);
   }

   if ( amplitude > 0.0 )
   {
      obsId->aoCtrlId->aoIntMatStruct[nMode - 1].posAmplitude = amplitude;
   }
   else
   {
      obsId->aoCtrlId->aoIntMatStruct[nMode - 1].negAmplitude = amplitude;
   }

   sigMode = AO_MODE_MEAS_IM;
   subapOff = 0;

   obsId->writeToRm = TRUE;
   obsId->sigMode = sigMode;
   obsId->nMode = nMode - 1;
   obsId->amplitude = amplitude;
   obsId->aoCtrlId->allowedSubapOff = subapOff;
   obsId->nCoaddFrames = nCoaddFrames;
   strncpy( obsId->pCoaddFileName, pFullCoaddFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );
   strncpy( obsId->pCentFileName, pFullCentFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );

   sprintf ( obsId->pCentComment, "Mode %d, amplitude %f microns", 
             (int)nMode, (float)amplitude );
 
   /* Init the fields of the observe CAD record */

   nExp = nCoaddFrames ;  
      outOption = 0 ;      /* NO DHS */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = TRUE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigCompAoMat
 *
 *   INVOCATION:
 *   detSigCompAoMat (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigCompAoMat command
 *
 *   DESCRIPTION:
 *   This function computes the interaction matrix, then the control matrix and
 *   saves these matrix into files specified by the user
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigCompAoMat
   (
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure          */
   int             commandNumber,   /* Command number                         */
   SDSU_ID         sdsuId,          /* SDSU context structure                 */
   OBS_ID          obsId            /* Observation context structure          */
   )
{
   uint32       errorNumber;    /* Error number reported by task              */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files                          */
   char         pAoImFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullAoImFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
   char         pAoCmFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullAoCmFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pAoImFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pAoCmFileName);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */
   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG (MSG_LOG, 
                "Start to compute the interaction and control matrixes...");

   (void)aoMatZero (obsId->aoCtrlId);

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
   }
   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoContMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
   }

   if ( aoMatCompute ( obsId->aoCcdId, obsId->aoCtrlId ) == ERROR )
   {
      ERROR_SET (S_detControl_INTERNAL, "Failed to compute matrixes", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /* Combine file and path name for matrixes file names */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullAoImFileName, pAoImFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
      strncpy (pFullAoCmFileName, pAoCmFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      sprintf (pFullAoImFileName, "%s/%s", pFilePath, pAoImFileName );
      sprintf (pFullAoCmFileName, "%s/%s", pFilePath, pAoCmFileName );
   }

   /* Save the matrixes */

   obsId->aoCtrlId->aoIntMatInitFlag = TRUE;
   strcpy ( obsId->aoCtrlId->aoIntMatFileName , pFullAoImFileName);
   
   if ( aoMatWrite (pFullAoImFileName, obsId->aoCtrlId->aoIntMat, 
                    obsId->aoCcdId->centroidsNb, obsId->aoCtrlId->aoModeNb, 
                    AO_INT_MAT_TYPE) == ERROR )
   {
      ERROR_SET (S_detControl_INTERNAL, "Failed to save int matrix", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      obsId->aoCtrlId->aoIntMatInitFlag = FALSE;
      strcpy ( obsId->aoCtrlId->aoIntMatFileName , "");
      return (errorNumber);
   }

   obsId->aoCtrlId->aoContMatInitFlag = TRUE;
   strcpy ( obsId->aoCtrlId->aoContMatFileName , pFullAoCmFileName);
   
   if ( aoMatWrite (pFullAoCmFileName, obsId->aoCtrlId->aoContMat, 
                    obsId->aoCtrlId->aoModeNb, obsId->aoCcdId->centroidsNb, 
                    AO_CONT_MAT_TYPE) == ERROR )
   {
      ERROR_SET (S_detControl_INTERNAL, "Failed to save cont matrix", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      obsId->aoCtrlId->aoContMatInitFlag = FALSE;
      strcpy ( obsId->aoCtrlId->aoContMatFileName , "");
      return (errorNumber);
   }

   if (epToVxPipeWrite (NULL, pFullAoImFileName, obsId->pAoIntMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
   }
   if (epToVxPipeWrite (NULL, pFullAoCmFileName, obsId->pAoContMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
   }

   MESSAGE_LOG (MSG_LOG, 
                "Computation of the interaction and control matrixes done");

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigModeSeqDark
 *
 *   INVOCATION:
 *   detSigModeSeqDark (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *                      obsId) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext      (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber      (int)             Command number
 *   (>) sdsuId             (SDSU_ID)         Current SDSU context structure
 *   (>) obsId              (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigModeSeqDark command
 *
 *   DESCRIPTION:
 *   This function defines the AO processing mode to sequence dark.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */
uint32 detSigModeSeqDark
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,       /* CAD command context structure.     */
   int             commandNumber,       /* Command number.                    */
   SDSU_ID         sdsuId,              /* SDSU context structure.            */
   OBS_ID          obsId                /* Observation context structure.     */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         sigMode;        /* Signal processing mode.                    */
   long         nCoaddFrames;   /* Image number to average                    */
   long         nAverageData;   /* Number of data to average.                 */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */
   double       multCoeff;      /* Multiplicative coefficients for rms value  */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pCoaddFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
   char         pFullCoaddFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];

   char         pDarkFileName [STRING_SIZE];

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   sigMode = AO_MODE_SEQ_DARK;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *) & nCoaddFrames);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pCoaddFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3,
                          (char *) & nAverageData);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&multCoeff);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   MESSAGE_LOG3 (MSG_LOG,
      "Signal processing switched to \"Sequence dark\" mode - "
      "nCoaddFrames=%d, nAverageData=%d, multCoeff=%f", 
      (int)nCoaddFrames, (int)nAverageData, (float)multCoeff);

   if (epToVxPipeWrite (NULL, "Sequence Dark",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* First set the dark to null values */

   if ( obsId->aoCcdId->binningFlag == FALSE )
   {
      strcpy ( pDarkFileName, "./data/zeroFullP2Dark.fits" );
   }
   else
   {   
      strcpy ( pDarkFileName, "./data/zeroBinP2Dark.fits" );
   }

   if ( aoDarkUpdate ( pDarkFileName, obsId->aoCcdId, obsId->aoCtrlId) 
        == ERROR )
   {
      ERROR_SET (S_detControl_INTERNAL, "Can't set the dark to null",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if (epToVxPipeWrite (NULL, pDarkFileName, obsId->pAoDarkInitContext) 
       == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
   }
   

   /* Combine file and path name for coadd file name */

   detCreateFileName ( pFilePath ,
                       pCoaddFileName ,
                       pFullCoaddFileName ) ;

   obsId->nCoaddFrames = nCoaddFrames;
   strncpy( obsId->pCoaddFileName, pFullCoaddFileName,
            (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 );

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   obsId->sigMode = sigMode;
   obsId->nAverageDataThreshComp = nAverageData;
   obsId->multCoeffRmsThreshComp = multCoeff;
   obsId->aoCtrlId->thresholdMultCoeff = multCoeff;

   /* Init the fields of the observe CAD record */

   nExp = nCoaddFrames + nAverageData;
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSimulateImage
 *
 *   INVOCATION:
 *   detSimulateImage (xPixels, yPixels, pImage)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels (int)     x pixels
 *   (>) xPixels (int)     y pixels
 *   (>) pImage  (float *) image
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSimulateImage command
 *
 *   DESCRIPTION:
 *   This function simulate an image
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSimulateImage
   (
   int     xPixels,
   int     yPixels,
   float * pImage
   )
{
   FILE *pFile;
   int itemNb;


   pFile = fopen ( "./spot_image.dat" , "r") ;

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Failed to open spot_image.dat",
                 ERROR_LOG_NOW);

      return ( ERROR );
   }

   itemNb = fread ( (float *)pImage,
                    sizeof (float),
                    xPixels * yPixels,
                    pFile);

   if ( itemNb == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Failed to read spot_image.dat",
                 ERROR_LOG_NOW);

      return ( ERROR );
   }

   (void)fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitObserbeRecord
 *
 *   INVOCATION:
 *   detInitObserbeRecord (pRecordPrefix, nExp, expTime, outOption)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *) Record Name prefix
 *   (>) pNExp         (long *)       Number of exposures
 *   (>) pExpTime      (double *)     Exposure time
 *   (>) pOutOption    (long *)       Output option (NONE, DHS, FILE)
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Initialize the fields of the observe CAD record
 *
 *   DESCRIPTION:
 *   Initialize the fields of the observe CAD record according to the signal 
 *   processing
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detInitObserveRecord
   (
   const char *    pRecordPrefix,   /* Record Name Prefix  */
   long *          pNExp,           /* Number of exposures */
   double *        pExpTime,        /* Exposure time       */
   long *          pOutOption       /* Output option       */
   )
{
   uint32       errorNumber=0;  /* Error number reported by task.             */
   long         value;
   char         message [EPICS_MAX_BYTES_STRING_ATTRIB * 2];
   char         pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                /* String to store record fields.         */
   char         label [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char         path [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char         file [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char         sim [EPICS_MAX_BYTES_RECORD_NAME + 1];

#ifdef DEBUG
   printf ( "detInitObserverecord(%s,%d,%f,%d)\n", pRecordPrefix, *pNExp, 
            *pExpTime, *pOutOption);
#endif

   /* Init the field A of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.A", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   if ( cicsDbPut (pRecordName, message, DBF_LONG, pNExp) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field B of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.B", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   /*if ( cicsDbPut (pRecordName, message, DBF_DOUBLE, pExpTime) == ERROR )*/
   if ( cicsDbPut (pRecordName, message, 8, pExpTime) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field C of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.C", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   if ( cicsDbPut (pRecordName, message, DBF_LONG, pOutOption) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field D of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.D", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   strcpy ( label, "NONE" );
   if ( cicsDbPut (pRecordName, message, DBF_STRING, label) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field E of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.E", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   value = 2;
   if ( cicsDbPut (pRecordName, message, DBF_LONG, &value) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field F of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.F", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   strcpy (path, "." );
   if ( cicsDbPut (pRecordName, message, DBF_STRING, path) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field G of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.G", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   strcpy (file, "pwfs2.fits");
   if ( cicsDbPut (pRecordName, message, DBF_STRING, file) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   /* Init the field H of the observe CAD record */

   sprintf ( pRecordName, "%s%s:%s.H", TOP, pRecordPrefix, 
             DET_CONTROL_OBSERVE_CAD_NAME);
   /*printf ( "record name: %s\n" , pRecordName);*/
   strcpy (sim, "NONE");
   if ( cicsDbPut (pRecordName, message, DBF_STRING, sim) == ERROR )
   {
      ERROR_LOG ( "Failed to init %s field");
      errorNumber = S_detControl_INTERNAL;
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitSigInit
 *
 *   INVOCATION:
 *   detInitSigInit (struct genSubRecord *pgsub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pgsub (struct genSubRecord *) Pointer to initSigInit gsub record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the detSigInit input fields according to the binning status
 *
 *   DESCRIPTION:
 *   For this record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 11 July 2000
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   external variables: detObsIdP2
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detInitSigInit
   (
   struct genSubRecord * pgsub      /* Pointer to "initSigInit" gensub record */
   )
{
   long xbin;
   long ybin;
   char path[STRING_SIZE];
   char darkFileName[STRING_SIZE];
   char flatFileName[STRING_SIZE];
   char refFileName[STRING_SIZE];
   char aoImFileName[STRING_SIZE];
   char aoCmFileName[STRING_SIZE];
   char fgCmFileName[STRING_SIZE];
   char defFileName[STRING_SIZE];
   char aoInitFileName[STRING_SIZE];
   double angleM2;
   double angleM1;
   double refX;
   double refY;
   double thresh;
   double rms;
   double totalThresh;

   if ( detObsIdP2 == NULL )
   {
      return (ERROR);
   }

   xbin = *(long *)pgsub->b;

   ybin = *(long *)pgsub->d;

   if ( (xbin == 1) && (ybin == 1) )
   {
      /* Read default parameters from par file */

#if (MK)
      strcpy ( defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_MK_INIT_FILE );
#else
      strcpy ( defFileName, DET_CONTROL_PWFS2_AO_FULL_CTRL_CP_INIT_FILE );
#endif

      if ( strcmp (defFileName, "NONE") != 0 )
      {
         strcpy ( aoInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
         strcat ( aoInitFileName , "/" ) ;
         strcat ( aoInitFileName , defFileName ) ;

         if ( aoCtrlFileRead ( aoInitFileName, path, darkFileName, flatFileName,
                               refFileName, &refX, &refY, aoImFileName, 
                               aoCmFileName, fgCmFileName, &rms, &thresh,
                               &totalThresh, &angleM2, &angleM1) == ERROR )
         {
            printf ("Failed to read ao control file parameters\n");
            return (ERROR);
         }

         strcpy ( (char *)pgsub->vala, path );
         strcpy ( (char *)pgsub->valb, darkFileName );
         strcpy ( (char *)pgsub->valc, flatFileName );
         *(double *)pgsub->vald = angleM2;
         *(double *)pgsub->vale = refX;
         *(double *)pgsub->valf = refY;
         *(double *)pgsub->valg = angleM1;
         strcpy ( (char *)pgsub->valh, refFileName );
         strcpy ( (char *)pgsub->vali, aoImFileName );
         strcpy ( (char *)pgsub->valj, aoCmFileName );
         strcpy ( (char *)pgsub->valk, fgCmFileName );

         /*strcpy ( (char *)pgsub->vala, "." );
         strcpy ( (char *)pgsub->valb, "data/defFullP2DarkMK.fits" );
         strcpy ( (char *)pgsub->valc, "data/defFullP2FlatMK.fits" );
         *(double *)pgsub->vald = 0.0;
         *(double *)pgsub->vale = 40.5;
         *(double *)pgsub->valf = 40.5;
         *(double *)pgsub->valg = 0.0;
         strcpy ( (char *)pgsub->valh, "data/defFullRefP2.dat" );
         strcpy ( (char *)pgsub->vali, "data/defAoIntMatP2MK.dat" );
         strcpy ( (char *)pgsub->valj, "data/defAoContMatP2MK.dat" );
         strcpy ( (char *)pgsub->valk, "data/defFgContMatP2MK.dat" );*/

         *(long *)pgsub->valu = 0; /* no binning: 0 */
      }
   }
   else if ( (xbin == 2) && (ybin == 2) )
   {
      /* Read default parameters from par file */

#if (MK)
      strcpy ( defFileName , DET_CONTROL_PWFS2_AO_BIN_CTRL_MK_INIT_FILE );
#else
      strcpy ( defFileName , DET_CONTROL_PWFS2_AO_BIN_CTRL_CP_INIT_FILE );
#endif

      if ( strcmp (defFileName, "NONE") != 0 )
      {
         strcpy ( aoInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
         strcat ( aoInitFileName , "/" ) ;
         strcat ( aoInitFileName , defFileName ) ;

         if ( aoCtrlFileRead ( aoInitFileName, path, darkFileName, flatFileName,
                               refFileName, &refX, &refY, aoImFileName, 
                               aoCmFileName, fgCmFileName, &rms, &thresh,
                               &totalThresh, &angleM2, &angleM1) == ERROR )
         {
            ERROR_LOG ("Failed to read ao control file parameters");
            return (ERROR);
         }

         strcpy ( (char *)pgsub->vala, path );
         strcpy ( (char *)pgsub->valb, darkFileName );
         strcpy ( (char *)pgsub->valc, flatFileName );
         *(double *)pgsub->vald = angleM2;
         *(double *)pgsub->vale = refX;
         *(double *)pgsub->valf = refY;
         *(double *)pgsub->valg = angleM1;
         strcpy ( (char *)pgsub->valh, refFileName );
         strcpy ( (char *)pgsub->vali, aoImFileName );
         strcpy ( (char *)pgsub->valj, aoCmFileName );
         strcpy ( (char *)pgsub->valk, fgCmFileName );

         /*strcpy ( (char *)pgsub->vala, "." );
         strcpy ( (char *)pgsub->valb, "data/defBinP2DarkMK.fits" );
         strcpy ( (char *)pgsub->valc, "data/defBinP2FlatMK.fits" );
         *(double *)pgsub->vald = 0.0;
         *(double *)pgsub->vale = 20.5;
         *(double *)pgsub->valf = 20.5;
         *(double *)pgsub->valg = 0.0;
         strcpy ( (char *)pgsub->valh, "data/defBinRefP2.dat" );
         strcpy ( (char *)pgsub->vali, "data/defAoIntMatP2MK.dat" );
         strcpy ( (char *)pgsub->valj, "data/defAoContMatP2MK.dat" );*/
         strcpy ( (char *)pgsub->valk, "data/defFgContMatP2MK.dat" );

         *(long *)pgsub->valu = 1; /* binning: 1 */
      }
   }
   else
   {
#ifdef DEBUG
      ERROR_LOG ( "xbin and ybin sir records should contain 1 or 2" );
#endif
   }

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitBw
 *
 *   INVOCATION:
 *   detSigInitBw (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitBw command
 *
 *   DESCRIPTION:
 *   This function updates the butterworth filter cutoff frequency
 *   in open and closed loop
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitBw
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   double       cutoffFreq;
   double       rateSampFreq;

   /*
    * Initialise the error number and get the attributes provided with this
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *)&rateSampFreq);

   /*
    * Check there are valid SDSU and observation context structures.
    */

#ifdef DEBUG
   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
#endif

   /*
    * Compute the new coefficients for the Butterworth filter 
    */

   rateSampFreq = rateSampFreq / 100.0;
   obsId->rateSamplingFrequency = rateSampFreq;
   cutoffFreq = rateSampFreq / obsId->exposureTime;
   obsId->cutoffFrequency = cutoffFreq;

   if ( detComputeCoeffButterworth ( obsId->exposureTime, cutoffFreq, 
                                     coeffData ) == ERROR )
   {
      ERROR_SET (S_detControl_INTERNAL,
                 "Failed to init butterworth coeff filter",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detComputeCoeffButterworth
 *
 *   INVOCATION:
 *   detComputeCoeffButterworth (expTime, cutoffFreq, pCoeffData)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) expTime    (double)   Exposure time in sec
 *   (>) cutoffFreq (double)   Cutoff frequency of the butterworth filter in Hz
 *   (>) pCoeffData (double *) Coeffcients of the butterworth filter
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detComputeCoeffButterworth command
 *
 *   DESCRIPTION:
 *   This function computes the coefficients of the butterworth filter used for 
 *   probe arm guiding
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detComputeCoeffButterworth
   (
   double     expTime,
   double     cutoffFreq,
   double   * pCoeffData
   )
{
   int        i;

   double     threshFreq;
   double     dt;
   double     omega0;
   double     denom;
   double     coeff[5];

   /*
    * The cutoff frequency should be maximum 1/10 of the sampling frequency.
    * The sampling frequency = 1 / exposure time.
    */
   
   threshFreq = 1.0 / (expTime * 10.0);

   if ( cutoffFreq > threshFreq )
   {
      cutoffFreq = threshFreq;
/*#ifdef DEBUG*/
      printf ( "cutoffFreq = threshFreq = %f\n", threshFreq);
/*#endif*/
   }

   /*
    * Now compute the coefficents 
    */

   dt = expTime;
   omega0 = 2 * PI * cutoffFreq;
   denom = dt*dt*omega0*omega0 + sqrt(8.0)*dt*omega0 + 4.0;

   coeff[0] = (8.0 - 2.0*dt*dt*omega0*omega0)/denom;
   coeff[1] = (sqrt(8.0)*dt*omega0 - dt*dt*omega0*omega0 - 4.0)/denom;
   coeff[2] = dt*dt*omega0*omega0/denom;
   coeff[3] = 2.0 * coeff[2];
   coeff[4] = coeff[2];

/*#ifdef DEBUG*/
   for ( i = 0 ; i < 5 ; i ++ )
      printf ( "coeff[%d]=%f\n", i, coeff[i] );
/*#endif*/

   /*
    * Update the butterworth coefficients
    */

   for ( i = 0 ; i < 5 ; i ++ )
       *(pCoeffData + i) = coeff[i];

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigReset
 *
 *   INVOCATION:
 *   detSigReset (pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigReset command
 *
 *   DESCRIPTION:
 *   This function resets the signal processing: mode to global guide, 
 *   thresholds and save circular buffer flags
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */


uint32 detSigReset
   (
   const char *    pRecordPrefix,   /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32       errorNumber;    /* Error number reported by task.             */
   long         k;              /* Index                                      */
   long         sigMode;        /* Signal processing mode.                    */
   long         nExp;           /* Number of exposure                         */
   long         outOption;      /* Output option                              */
   double       expTime;        /* Exposure time                              */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command cannot be used when an observation is in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Define the signal processing mode and associated parameters.
    * These parameters will be used in detObserveEnd.
    */

   sigMode = AO_MODE_GG;
   obsId->sigMode = sigMode;
   obsId->writeToRm = TRUE;

   MESSAGE_LOG1 (MSG_LOG,
                "Signal processing switched to \"Global Guide\" mode, flag=%d",
                (int)obsId->writeToRm);
   if (epToVxPipeWrite (NULL, "Global Guide",
                        obsId->pAoProcessModeContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
   }

   /* Init the fields of the observe CAD record */

   nExp = -1 ;          /* mode continuous */
   outOption = 0 ;      /* NONE */
   if ( obsId->aoCcdId->binningFlag == FALSE )
      expTime = 0.01 ;  /* 10ms */
   else
      expTime = 0.005 ; /* 5ms */

   if ( detInitObserveRecord (pRecordPrefix, &nExp, &expTime, &outOption) ==
        ERROR )
   {
      ERROR_LOG ( "Failed to init fields of observe record");
   }

   /*
    * Reset the thresholds 
    */

   if ( obsId->aoCcdId->binningFlag == FALSE )
      obsId->aoCtrlId->threshold = obsId->aoCtrlId->thresholdDarkFull;
   else
      obsId->aoCtrlId->threshold = obsId->aoCtrlId->thresholdDarkBin;

   for ( k = 0 ; k < obsId->aoCcdId->subapUsedNb ; k ++ )
       obsId->aoCtrlId->thresholdVect[k] = obsId->aoCtrlId->threshold;

/*
   obsId->aoCtrlId->threshold = obsId->aoCtrlId->thresholdDark;
*/

   if (epToVxPipeWrite (NULL, (char *)(int)& (obsId->aoCtrlId->threshold),
                        obsId->pAoThreshContext) == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_THRESH_SIR_NAME record");
   }

   obsId->aoCtrlId->totalThreshold = 0.0;

   if (epToVxPipeWrite (NULL, (char *)(int)& (obsId->aoCtrlId->totalThreshold),
                        obsId->pAoTotalContext) == ERROR)
   {
      ERROR_LOG ("Failed to init DET_CONTROL_AO_TOTAL_SIR_NAME record");
   }

   /* 
    * Reset the save CB flags 
    */

   obsId->saveCbIm = FALSE;
   obsId->saveCbAoCtrl = FALSE;
   obsId->saveCbFgCtrl = FALSE;

   if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbImContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME record");
   }

   if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbAoCtrlContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME record");
   }

   if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbFgCtrlContext) == ERROR)
   {
      ERROR_LOG (
            "Failed to init DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME record");
   }

   /* Initialise the coadd counter used to decide when to save coadded data
    * to disk.
    */

   obsId->saveCentroids = FALSE;
   obsId->coaddCounter = 0;
   obsId->saveAoCbCounter = 0;
   obsId->saveFgCbCounter = 0;
   obsId->averageRms = 0.0;
   obsId->averageMean = 0.0;
   obsId->averageFlux = 0.0;
   obsId->threshRealTimeFlag = FALSE;

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detContInit
 *
 *   INVOCATION:
 *   detContInit (pInitFileName, pTempCode, pTempCoeff, 
 *                poffsetFulVect, pOffsetBinVect, pCcdSn)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName   (char *)   Init file Name
 *   (>) pTempCode       (uint32 *) Target temperature code
 *   (>) pTempCoeff      (uint32 *) Coefficient for temperature control
 *   (>) pOffsetFullVect (long *)   ADC offset vector when no binning [4]
 *   (>) pOffsetBinVect  (long *)   ADC offset vector when binning [4]
 *   (>) pCcdSn          (char *)   CCD serial number
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Init defaults values for the detector controller
 *
 *   DESCRIPTION:
 *   Init default values for target temperature, ADC offsets and 
 *   the serial number of the CCD from a init file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */


uint32 detContInit
   (
   char *   pInitFileName,          /* Init file Name                         */
   uint32 * pTempCode,              /* Target temperature code                */
   uint32 * pTempCoeff,             /* Coefficient for temperature control    */
   long   * pOffsetFullVect,        /* ADC offset vector [4] - no binning     */
   long   * pOffsetBinVect,         /* ADC offset vector [4] - binning        */
   char *   pCcdSn                  /* CCD serial number                      */
   )
{
   FILE *       pFile;
   char         comment [STRING_SIZE];
   float        tempTarget;
   int          coeff;
   int          offset;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      printf ( "Failed to open the Detector Controller init file %s",
		   pInitFileName );
      ERROR_SET1 ( 0, "Failed to open the Detector Controller init file %s",
		   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
            "Failed to read first line of comments from the DC init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): first line of comments:\n" );
   printf ( "%s\n" , comment );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default target temperature */

   if ( (fscanf (pFile, "%f\n", &tempTarget)) == EOF )
   {
      ERROR_SET1 ( 0,
            "Failed to read the target temperature from the DC init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( tempTarget <= -40.0 )
   {
      ERROR_SET ( 0,
                  "Target temperature should be greater than -40.0C",
                  ERROR_LOG_SAVE);
      fclose (pFile);
      return (ERROR);
   }

   if ( tempTarget <= 0.0 )
   {
      *pTempCode = (uint32) ((SDSU_TEMP_BASE - tempTarget) / SDSU_TEMP_UNIT);
      *pTempCode &= 0xfff; 
			  /* Truncate to 0xfff (which is the maximum allowed) */
   }
   else
   {
      /* Switch off cooling altogether for temperatures above 0C. */
      *pTempCode = 0;
   }

#ifdef DEBUG
   printf ( "detContInit(): target temperature = %d\n", *pTempCode );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default temperature coefficient */

   if ( (fscanf (pFile, "%d\n", &coeff)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the temperature coefficient from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *pTempCoeff = coeff;

#ifdef DEBUG
   printf ( "detContInit(): temperature coefficient = %d\n", coeff );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default ADC offset for output 0 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset0 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 0) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 0 (full) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 1 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset1 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 1) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 1 (full) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 2 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset2 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 2) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 2 (full) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 3 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset3 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 3) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 3 = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default ADC offset for output 0 - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset0 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 0) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 0 (bin)= %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 1 - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset1 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 1) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 1 (bin) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 2 - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset2 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 2) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 2 (bin) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 3  - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset3 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 3) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 3 (bin) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the CCD serial number from the file */

   if ( fgets (pCcdSn, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the CCD SN from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pCcdSn[strlen(pCcdSn) - 1] == '\n' )
   {
      pCcdSn[strlen(pCcdSn) - 1] = '\0';
#ifdef DEBUG
      printf ( "detControlInit(): last character of %s was return\n",
               pCcdSn );
#endif

   }

#ifdef DEBUG
   printf ( "detContInit(): CCD serial number: %s\n", pCcdSn );
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detGetSirContext
 *
 *   INVOCATION:
 *   detGetSirContext (pRecordPrefix, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Get the context structures for the SIR records.
 *
 *   DESCRIPTION:
 *   Get the context structures for the SIR records. 
 *   Each SIR is referenced by its name: first get the name of each SIR,
 *   then call epToVxRecContextGet() in order to look-up the context structure
 *   that has previously been assigned to the SIR during initialisation of the
 *   local record data-base. 
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *   obsId has to be allocated before calling this function.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detGetSirContext
   (
   const char * pRecordPrefix,       /* Record Name Prefix.                   */
   OBS_ID       obsId                /* Observation context structure.        */
   )
{

   uint32       errorNumber;         /* Error number reported by task.        */

   char         pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                     /* String to store record names.         */

   /* Initialize the erroNumber */

   errorNumber = OK;

   /* Get the context of the wfs control "state" sir record */

   sprintf (pRecordName, "%s", WFS_CONTROL_STATE_SIR_NAME );
   if (epToVxRecContextGet (pRecordName, & (obsId->pStateContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get WFS_CONTROL_STATE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "initialising" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetInitContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detInitStatus" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_INIT_STATUS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetInitStatusContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INIT_STATUS SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "testing" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_TEST_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetTestContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_TEST_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "testResults" sir record */

   sprintf (pRecordName, "%s", DET_CONTROL_TEST_RESULTS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pTestResultsContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_TEST_RESULTS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detPrimReply" sir record */
   
   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_PRIM_REPLY_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetPrimReplyContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_PRIM_REPLY_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "observing" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_OBSERVING_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetObservingContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_OBSERVING_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "measuring" sir record */

   sprintf (pRecordName, "%s", DET_CONTROL_MEAS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetMeasuringContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_MEAS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }
   
   /* Get the context of the "aoCtrlInit" sir record */
   
   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_CTRL_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoCtrlInitContext), NULL) 
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_CTRL_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoDarkInit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_DARK_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoDarkInitContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_DARK_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoFlatInit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_FLAT_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoFlatInitContext), NULL) 
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_FLAT_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoIntMatInit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoIntMatInitContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoContMatInit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoContMatInitContext), NULL)
       == ERROR)
   {
      ERROR_LOG (
           "Failed to get DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoFgContMatInit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pFgContMatInitContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG (
           "Failed to get DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoRms" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_RMS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoRmsContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_RMS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoThresh" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_THRESH_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoThreshContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_THRESH_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoTotal" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_TOTAL_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoTotalContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_TOTAL_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoSaveCbIm" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoSaveCbImContext), NULL)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to get DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoSaveCbAoCtrl" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoSaveCbAoCtrlContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG (
      "Failed to get DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoSaveCbFgCtrl" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoSaveCbFgCtrlContext),
                            NULL) == ERROR)
   {
      ERROR_LOG (
      "Failed to get DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "aoProcessMode" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_AO_PROCESS_MODE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAoProcessModeContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_AO_PROCESS_MODE_SIR_NAME context") ;
      errorNumber = ERROR;
   }

   /* Get the context of the "outputs" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_OUTPUTS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pOutputsContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_OUTPUTS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detXsize" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETXSIZE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetXsizeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETXSIZE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detYsize" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETYSIZE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetYsizeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETYSIZE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xsubap" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSUBAP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXsubapContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSUBAP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ysubap" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSUBAP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYsubapContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSUBAP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xstart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ystart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xras" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XRASTER_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXrasterContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XRASTER_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "yras" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YRASTER_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYrasterContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YRASTER_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xspace" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSPACE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXspaceContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSPACE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "yspace" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSPACE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYspaceContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSPACE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xbin" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XBIN_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXbinContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XBIN_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ybin" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YBIN_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYbinContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YBIN_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detType" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DET_TYPE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetTypeContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DET_TYPE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detID" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DET_ID_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetIdContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DET_ID_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "dataLabel" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DATA_LABEL_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDataLabelContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DATA_LABEL_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "intTime" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_INT_TIME_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pIntTimeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INT_TIME_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nexpRQ" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NEXPRQ_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNExpRQContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NEXPRQ_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nexp" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NEXP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNExpContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NEXP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nframes" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NFRAMES_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNFramesContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NFRAMES_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "bunit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_BUNIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pBunitContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_BUNIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "utstart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_UTSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pUTstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_UTSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "utend" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_UTEND_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pUTendContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_UTEND_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "exposed" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_EXPOSED_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pExposedContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_EXPOSED_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "exposedRQ" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_EXPOSEDRQ_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pExposedRQContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_EXPOSEDRQ_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "elapsed" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ELAPSED_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pElapsedContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ELAPSED_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "dhsCon" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DHS_CON_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDhsConContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DHS_CON_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc0" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC0_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc0Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC0_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc1" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC1_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc1Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC1_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc2" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC2_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc2Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC2_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc3" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC3_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc3Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC3_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Return */

   return ( errorNumber );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detWriteDefSirContext
 *
 *   INVOCATION:
 *   detWriteDefSirContext (obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Write Default values to the SIR records.
 *
 *   DESCRIPTION:
 *   Write Default values to the SIR records.
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detWriteDefSirContext
   (
   OBS_ID       obsId                /* Observation context structure.        */
   )
{

   uint32       errorNumber;         /* Error number reported by task.        */

   /* Initialize the erroNumber */

   errorNumber = OK;

   /* Init the "testResults" sir record */

   if (epToVxPipeWrite (NULL, "Not tested", obsId->pTestResultsContext) 
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_TEST_RESULTS_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoCtrlInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoCtrlInitContext) 
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CTRL_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoDarkInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoDarkInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_DARK_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoFlatInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoFlatInitContext) 
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_FLAT_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoIntMatInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoIntMatInitContext) 
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_INT_MAT_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoContMatInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pAoContMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_CONT_MAT_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "aoFgContMatInit" sir record */

   if (epToVxPipeWrite (NULL, "Not initialized", obsId->pFgContMatInitContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_FG_CONT_MAT_INIT_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init aoSaveCbIm, aoSaveCbAoCtrl and aoSaveCbFgCtrl sir records -
      all of them FALSE when booting*/

   if ( obsId->saveCbIm == TRUE )
   {
      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbImContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME record");
         errorNumber = ERROR;
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbImContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_IM_SIR_NAME record");
         errorNumber = ERROR;
      }
   }

   if ( obsId->saveCbAoCtrl == TRUE )
   {
      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbAoCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME record");
         errorNumber = ERROR;
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbAoCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_AO_CTRL_SIR_NAME record");
         errorNumber = ERROR;
      }
   }

   if ( obsId->saveCbFgCtrl == TRUE )
   {
      if (epToVxPipeWrite (NULL, "TRUE", obsId->pAoSaveCbFgCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME record");
         errorNumber = ERROR;
      }
   }
   else
   {
      if (epToVxPipeWrite (NULL, "FALSE", obsId->pAoSaveCbFgCtrlContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to init DET_CONTROL_AO_SAVE_CB_FG_CTRL_SIR_NAME record");
         errorNumber = ERROR;
      }
   }

   /* Init "aoProcessMode" sir record - note sigMode = AO_MODE_NONE */

   if (epToVxPipeWrite (NULL, "No processing", obsId->pAoProcessModeContext)
       == ERROR)
   {
      ERROR_LOG (
      "Failed to init DET_CONTROL_AO_PROCESS_MODE_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "detType" sir record */

   if (epToVxPipeWrite( NULL, DET_TYPE, obsId->pDetTypeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      errorNumber = ERROR;
   }

   /* Init the "bunit" sir record */

   if (epToVxPipeWrite( NULL, DET_BUNIT, obsId->pBunitContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      errorNumber = ERROR;
   }

   /* Init the "dhsCon" sir record */

   if (epToVxPipeWrite( NULL, "NOT CONNECTED", obsId->pDhsConContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init dhs connection sir record");
      errorNumber = ERROR;
   }

   /* return */

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitModAst
 *
 *   INVOCATION:
 *   detSigInitModAst (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitModAst command
 *
 *   DESCRIPTION:
 *   This function updates the external structure astigModel
 * 
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitModAst
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   double       a1;
   double       a2;
   double       a3;
   double       p1;
   double       p2;
   double       p3;
   double       c;
   double       b1;
   double       b2;
   double       b3;
   double       pp1;
   double       pp2;
   double       pp3;
   double       d;
   long         apply;
   double       gain0;
   double       gain45;
   double       offset0;
   double       offset45;


   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Get the attributes provided with this command.
    */
       
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&a1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&a2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&a3);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&p1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&p2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&p3);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *)&c);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, (char *)&b1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, (char *)&b2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, (char *)&b3);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10, (char *)&pp1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 11, (char *)&pp2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 12, (char *)&pp3);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 13, (char *)&d);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 14, (char *)&apply);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 15, (char *)&gain0);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 16, (char *)&gain45);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 17, (char *)&offset0);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 18, (char *)&offset45);

   /* 
    * Check the attributes
    */

   if ( gain0 == 0.0 )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Gain for astig 0 should be different from zero",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( gain45 == 0.0 )
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Gain for astig 45 should be different from zero",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Update the astigModel structure
    */

   if (semTake (accessAstigModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, "Timeout on mutex acess to astigModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      astigModel.a1 = a1;
      astigModel.a2 = a2;
      astigModel.a3 = a3;
      astigModel.p1 = p1;
      astigModel.p2 = p2;
      astigModel.p3 = p3;
      astigModel.c = c;
      astigModel.b1 = b1;
      astigModel.b2 = b2;
      astigModel.b3 = b3;
      astigModel.pp1 = pp1;
      astigModel.pp2 = pp2;
      astigModel.pp3 = pp3;
      astigModel.d = d;
      astigModel.applyModel = apply;
      astigModel.gain0 = gain0;
      astigModel.gain45 = gain45;
      astigModel.offsetAstig0 = offset0;
      astigModel.offsetAstig45 = offset45;

      semGive (accessAstigModel);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitModTref
 *
 *   INVOCATION:
 *   detSigInitModTref (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitModTref command
 *
 *   DESCRIPTION:
 *   This function updates the external structure trefoilModel
 * 
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitModTref
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   double       a;
   double       p;
   double       c;
   double       b;
   double       pp;
   double       d;
   long         apply;


   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Get the attributes provided with this command.
    */
       
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&a);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&p);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&c);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&b);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&pp);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&d);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *)&apply);

   /*
    * Update the trefoilModel structure
    */

   if (semTake (accessTrefoilModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to trefoilModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      trefoilModel.a = a;
      trefoilModel.p = p;
      trefoilModel.c = c;
      trefoilModel.b = b;
      trefoilModel.pp = pp;
      trefoilModel.d = d;
      trefoilModel.applyModel = apply;

      semGive (accessTrefoilModel);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitModComa
 *
 *   INVOCATION:
 *   detSigInitModComa (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitModComa command
 *
 *   DESCRIPTION:
 *   This function updates the external structure comaModel
 * 
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitModComa
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   double       a;
   double       p;
   double       c;
   double       b;
   double       pp;
   double       d;
   long         apply;


   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Get the attributes provided with this command.
    */
       
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&a);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&p);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&c);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&b);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&pp);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&d);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *)&apply);

   /*
    * Update the comaModel structure
    */

   if (semTake (accessComaModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to comaModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      comaModel.a = a;
      comaModel.p = p;
      comaModel.c = c;
      comaModel.b = b;
      comaModel.pp = pp;
      comaModel.d = d;
      comaModel.applyModel = apply;

      semGive (accessComaModel);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSigInitModFoc
 *
 *   INVOCATION:
 *   detSigInitModFoc (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSigInitModFoc command
 *
 *   DESCRIPTION:
 *   This function updates the external structure focusModel
 * 
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSigInitModFoc
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   double       a1;
   double       a2;
   double       p1;
   double       p2;
   double       c;
   long         apply;


   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Get the attributes provided with this command.
    */
       
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *)&a1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *)&a2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *)&p1);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *)&p2);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *)&c);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *)&apply);

   /*
    * Update the focusModel structure
    */

   if (semTake (accessFocusModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to focusModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      focusModel.a1 = a1;
      focusModel.a2 = a2;
      focusModel.p1 = p1;
      focusModel.p2 = p2;
      focusModel.c = c;
      focusModel.applyModel = apply;

      semGive (accessFocusModel);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detZpModShow
 *
 *   INVOCATION:
 *   detZpModShow ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detZpModShow command
 *
 *   DESCRIPTION:
 *   This function shows the contents of the zero point models 
 * 
 *   EXTERNAL VARIABLES:
 *   astigModel
 *   focusModel
 *   comaModel
 *   trefoilModel
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detZpModShow
   (
   )
{
   uint32                   errorNumber; 
   AST_ZP_MODEL_ID_STRUCT   tempAstModel;
   TREF_ZP_MODEL_ID_STRUCT  tempTrefModel;
   COMA_ZP_MODEL_ID_STRUCT  tempComaModel;
   FOCUS_ZP_MODEL_ID_STRUCT tempFocModel;

   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Show the astigModel structure
    */

   if (semTake (accessAstigModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to astigModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      tempAstModel.a1 = astigModel.a1;
      tempAstModel.a2 = astigModel.a2;
      tempAstModel.a3 = astigModel.a3;
      tempAstModel.p1 = astigModel.p1;
      tempAstModel.p2 = astigModel.p2;
      tempAstModel.p3 = astigModel.p3;
      tempAstModel.c = astigModel.c;
      tempAstModel.b1 = astigModel.b1;
      tempAstModel.b2 = astigModel.b2;
      tempAstModel.b3 = astigModel.b3;
      tempAstModel.pp1 = astigModel.pp1;
      tempAstModel.pp2 = astigModel.pp2;
      tempAstModel.pp3 = astigModel.pp3;
      tempAstModel.d = astigModel.d;
      tempAstModel.applyModel = astigModel.applyModel;
      tempAstModel.gain0 = astigModel.gain0;
      tempAstModel.gain45 = astigModel.gain45;
      tempAstModel.offsetAstig0 = astigModel.offsetAstig0;
      tempAstModel.offsetAstig45 = astigModel.offsetAstig45;

      semGive (accessAstigModel);
   }

   printf ( "Zero point model for astigmatism\n\n" ); 

   printf ( "Astig0 = a1*cos(X+p1) + a2*cos(2*X+p2) + a3*cos(4*X+p3) + c\n" );
   printf ("a1=%f microns, a2=%f microns, a3=%f microns\n",
           (float)tempAstModel.a1, (float)tempAstModel.a2, (float)tempAstModel.a3);
   printf ("p1=%f deg, p2=%f deg, p3=%f deg\n",
           (float)tempAstModel.p1, (float)tempAstModel.p2, (float)tempAstModel.p3);
   printf ("c=%f microns\n\n", (float)tempAstModel.c);

   printf ( "Astig45 = b1*sin(X+pp1) + b2*sin(2*X+pp2) + b3*sin(4*X+pp3) + d\n" );
   printf ("b1=%f microns, b2=%f microns, b3=%f microns\n",
           (float)tempAstModel.b1, (float)tempAstModel.b2, (float)tempAstModel.b3);
   printf ("pp1=%f deg, pp2=%f deg, pp3=%f deg\n",
           (float)tempAstModel.pp1, (float)tempAstModel.pp2, (float)tempAstModel.pp3);
   printf ("d=%f microns\n\n", (float)tempAstModel.d);

   printf ("Astig model applied : %s\n\n",
           (tempAstModel.applyModel ? "TRUE" : "FALSE") );

   printf ( "Astig 0 gain: %f, Astig 45 gain: %f\n" ,
            (float)tempAstModel.gain0, (float)tempAstModel.gain45 );
   printf ( "Astig 0 offset: %f, Astig 45 offset: %f\n\n" ,
            (float)tempAstModel.offsetAstig0, (float)tempAstModel.offsetAstig45 );
   
   /*
    * Show the trefoilModel structure
    */

   if (semTake (accessTrefoilModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to trefoilModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      tempTrefModel.a = trefoilModel.a;
      tempTrefModel.p = trefoilModel.p;
      tempTrefModel.c = trefoilModel.c;
      tempTrefModel.b = trefoilModel.b;
      tempTrefModel.pp = trefoilModel.pp;
      tempTrefModel.d = trefoilModel.d;
      tempTrefModel.applyModel = trefoilModel.applyModel;

      semGive (accessTrefoilModel);
   }

   printf ( "Zero point model for trefoil\n\n" ); 

   printf ( "Cos Trefoil = a*cos(3*X+p) + c\n" );
   printf ("a=%f microns, p=%f deg, c=%f microns\n",
           (float)tempTrefModel.a, (float)tempTrefModel.p, (float)tempTrefModel.c);

   printf ( "Sin Trefoil = b*sin(3*X+pp) + d\n" );
   printf ("b=%f microns, pp=%f deg, d=%f microns\n\n",
           (float)tempTrefModel.b, (float)tempTrefModel.pp, (float)tempTrefModel.d);

   printf ("Trefoil model applied : %s\n\n",
           (tempTrefModel.applyModel ? "TRUE" : "FALSE") );

   /*
    * Show the comaModel structure
    */

   if (semTake (accessComaModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to comaModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      tempComaModel.a = comaModel.a;
      tempComaModel.p = comaModel.p;
      tempComaModel.c = comaModel.c;
      tempComaModel.b = comaModel.b;
      tempComaModel.pp = comaModel.pp;
      tempComaModel.d = comaModel.d;
      tempComaModel.applyModel = comaModel.applyModel;

      semGive (accessComaModel);
   }

   printf ( "Zero point model for coma\n\n" ); 

   printf ( "coma X = a*cos(X+p) + c\n" );
   printf ("a=%f microns, p=%f deg, c=%f microns\n",
           (float)tempComaModel.a, (float)tempComaModel.p, (float)tempComaModel.c);

   printf ( "coma Y = b*sin(X+pp) + d\n" );
   printf ("b=%f microns, pp=%f deg, d=%f microns\n\n",
           (float)tempComaModel.b, (float)tempComaModel.pp, (float)tempComaModel.d);

   printf ("Coma model applied : %s\n\n",
           (tempComaModel.applyModel ? "TRUE" : "FALSE") );

   /*
    * Show the focusModel structure
    */

   if (semTake (accessFocusModel, ZP_MODEL_SEM_TIMEOUT) != OK)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "Timeout on mutex acess to focusModel",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }
   else
   {
      tempFocModel.a1 = focusModel.a1;
      tempFocModel.a2 = focusModel.a2;
      tempFocModel.p1 = focusModel.p1;
      tempFocModel.p2 = focusModel.p2;
      tempFocModel.c = focusModel.c;
      tempFocModel.applyModel = focusModel.applyModel;

      semGive (accessFocusModel);
   }

   printf ( "Zero point model for focus\n\n" ); 

   printf ( "focus = a1*cos(X+p1) + a2*cos(2*X+p2) + c\n" );
   printf ("a1=%f microns, p1=%f deg\n" ,
           (float)tempFocModel.a1, (float)tempFocModel.p1);
   printf ("a2=%f microns, p2=%f deg, c=%f microns\n",
           (float)tempFocModel.a2, (float)tempFocModel.p2, 
           (float)tempFocModel.c);

   printf ("Focus model applied : %s\n\n",
           (tempFocModel.applyModel ? "TRUE" : "FALSE") );

   /*
    * Return and exit 
    */

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitSigInitModAst
 *
 *   INVOCATION:
 *   detInitSigInitModAst (struct genSubRecord *pgsub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pgsub (struct genSubRecord *) Pointer to initModAst gsub record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the detSigInitModAst input fields 
 *
 *   DESCRIPTION:
 *   For this record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 11 September 2001
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detInitSigInitModAst
   (
   struct genSubRecord * pgsub      /* Pointer to "initModAst" gensub record */
   )
{
   char defFileName[STRING_SIZE];
   char modInitFileName[STRING_SIZE];
   double a1;
   double a2;
   double a3;
   double p1;
   double p2;
   double p3;
   double c;
   double b1;
   double b2;
   double b3;
   double pp1;
   double pp2;
   double pp3;
   double d;
   double gain0;
   double gain45;
   double offset0;
   double offset45;
   int apply;

   /* Read default parameters from par file */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_MK_INIT_FILE );
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_CP_INIT_FILE );
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( modInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( modInitFileName , "/" ) ;
      strcat ( modInitFileName , defFileName ) ;

      if ( aoModAstFileRead ( modInitFileName, &a1, &a2, &a3, &p1, &p2, 
                              &p3, &c, &b1, &b2, &b3, &pp1, &pp2, &pp3,
                              &d, &gain0, &gain45, &offset0, &offset45,
                              &apply ) == ERROR )
      {
         ERROR_LOG ("Failed to read model file parameters\n");
         return (ERROR);
      }

      *(double *)pgsub->vala = a1;
      *(double *)pgsub->valb = a2;
      *(double *)pgsub->valc = a3;
      *(double *)pgsub->vald = p1;
      *(double *)pgsub->vale = p2;
      *(double *)pgsub->valf = p3;
      *(double *)pgsub->valg = c;
      *(double *)pgsub->valh = b1;
      *(double *)pgsub->vali = b2;
      *(double *)pgsub->valj = b3;
      *(double *)pgsub->valk = pp1;
      *(double *)pgsub->vall = pp2;
      *(double *)pgsub->valm = pp3;
      *(double *)pgsub->valn = d;
      *(long *)pgsub->valo = apply;
      *(double *)pgsub->valp = gain0;
      *(double *)pgsub->valq = gain45;
      *(double *)pgsub->valr = offset0;
      *(double *)pgsub->vals = offset45;
   }

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitSigInitModTref
 *
 *   INVOCATION:
 *   detInitSigInitModTref (struct genSubRecord *pgsub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pgsub (struct genSubRecord *) Pointer to initModTref gsub record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the detSigInitModTref input fields 
 *
 *   DESCRIPTION:
 *   For this record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 11 September 2001
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detInitSigInitModTref
   (
   struct genSubRecord * pgsub      /* Pointer to "initModTref" gensub record */
   )
{
   char defFileName[STRING_SIZE];
   char modInitFileName[STRING_SIZE];
   double a;
   double p;
   double c;
   double b;
   double pp;
   double d;
   int apply;

   /* Read default parameters from par file */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_MK_INIT_FILE );
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_CP_INIT_FILE );
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( modInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( modInitFileName , "/" ) ;
      strcat ( modInitFileName , defFileName ) ;

      if ( aoModTrefFileRead ( modInitFileName, &a, &p, &c, &b, &pp, 
                               &d, &apply ) == ERROR )
      {
         ERROR_LOG ("Failed to read model file parameters\n");
         return (ERROR);
      }

      *(double *)pgsub->vala = a;
      *(double *)pgsub->valb = p;
      *(double *)pgsub->valc = c;
      *(double *)pgsub->vald = b;
      *(double *)pgsub->vale = pp;
      *(double *)pgsub->valf = d;
      *(long *)pgsub->valg = apply;
   }

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitSigInitModComa
 *
 *   INVOCATION:
 *   detInitSigInitModComa (struct genSubRecord *pgsub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pgsub (struct genSubRecord *) Pointer to initModComa gsub record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the detSigInitModComa input fields 
 *
 *   DESCRIPTION:
 *   For this record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 11 September 2001
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detInitSigInitModComa
   (
   struct genSubRecord * pgsub      /* Pointer to "initModComa" gensub record */
   )
{
   char defFileName[STRING_SIZE];
   char modInitFileName[STRING_SIZE];
   double a;
   double p;
   double c;
   double b;
   double pp;
   double d;
   int apply;

   /* Read default parameters from par file */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_MK_INIT_FILE );
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_CP_INIT_FILE );
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( modInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( modInitFileName , "/" ) ;
      strcat ( modInitFileName , defFileName ) ;

      if ( aoModComaFileRead ( modInitFileName, &a, &p, &c, &b, &pp, 
                               &d, &apply ) == ERROR )
      {
         ERROR_LOG ("Failed to read model file parameters\n");
         return (ERROR);
      }

      *(double *)pgsub->vala = a;
      *(double *)pgsub->valb = p;
      *(double *)pgsub->valc = c;
      *(double *)pgsub->vald = b;
      *(double *)pgsub->vale = pp;
      *(double *)pgsub->valf = d;
      *(long *)pgsub->valg = apply;
   }

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInitSigInitModFoc
 *
 *   INVOCATION:
 *   detInitSigInitModFoc (struct genSubRecord *pgsub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pgsub (struct genSubRecord *) Pointer to initModFoc gsub record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the detSigInitModFoc input fields 
 *
 *   DESCRIPTION:
 *   For this record, I have decided to use Epics facilities and not 
 *   epToVxLib. Faster and simpler. CB - 11 September 2001
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detInitSigInitModFoc
   (
   struct genSubRecord * pgsub      /* Pointer to "initModFoc" gensub record */
   )
{
   char defFileName[STRING_SIZE];
   char modInitFileName[STRING_SIZE];
   double a1;
   double p1;
   double a2;
   double p2;
   double c;
   int apply;

   /* Read default parameters from par file */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_MK_INIT_FILE );
#else
   strcpy ( defFileName, DET_CONTROL_PWFS2_AO_MOD_CP_INIT_FILE );
#endif

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( modInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( modInitFileName , "/" ) ;
      strcat ( modInitFileName , defFileName ) ;

      if ( aoModFocFileRead ( modInitFileName, &a1, &p1, &a2, &p2, 
                              &c, &apply ) == ERROR )
      {
         ERROR_LOG ("Failed to read model file parameters\n");
         return (ERROR);
      }

      *(double *)pgsub->vala = a1;
      *(double *)pgsub->valb = a2;
      *(double *)pgsub->valc = p1;
      *(double *)pgsub->vald = p2;
      *(double *)pgsub->vale = c;
      *(long *)pgsub->valf = apply;
   }

   return (OK) ;
}
