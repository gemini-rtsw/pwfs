/*+
 *   MODULE NAME:
 *   detControl
 *
 *   FILENAME:
 *   detControl.h
 *
 *   PURPOSE:
 *   Include file for detControl
 *
 *   IMPORTANT:
 *   *** THIS FILE MUST BE MODIFIED TO REFLECT THE ACTUAL VME ADDRESSES OF
 *   *** THE SDSU CONTROLLERS AT YOUR SITE. SEE DEFINITIONS BELOW.
 *
 *INDENT-OFF*
 *   27 oct 1999: CB - Add a new parameter dhsOutOptions in the structure 
 *                     + fits keywords
 *   8 July 1999: CB - Add a new parameter timeToWaitAo in the structure 
 *                     OBS_ID_STRUCT
 *   21 Apr 1999: CB - Simplified version for PWFS1 only
 *INDENT-ON*
 *-
 */


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <timers.h>
#include "gemModNum.h"
#include "epToVxLib.h"
#include "sdsuLib.h"
#include "osp.h"

#ifndef NO_DHS
#include "dhs.h"
#else
typedef   unsigned long   DHS_CONNECT;
#endif   /* NO_DHS */


/* defines */

#define   DET_CONTROL_TASK_NAME               "detControl"
                                    /* Detector Controller task name.         */

#define   DET_CONTROL_INIT_SIR_NAME           "initialising"
                                    /* Name of SIR record containing          */
                                    /* initialisation state.                  */

#define   DET_CONTROL_INIT_STATUS_SIR_NAME    "detInitStatus"
                                    /* Name of SIR record containing          */
                                    /* SDSU initialisation status.            */

#define   DET_CONTROL_TEST_RESULTS_SIR_NAME   "testResults"
                                    /* Name of SIR record containing          */
                                    /* SDSU test results.                     */

#define   DET_CONTROL_PRIM_REPLY_SIR_NAME     "detPrimReply"
                                    /* Name of SIR record containing          */
                                    /* reply from SDSU primitive cmd.         */

#define   DET_CONTROL_OBSERVING_SIR_NAME      "observing"
                                    /* Name of SIR record containing          */
                                    /* observing state.                       */

   /*
    * Define the VME addresses of the SDSU controllers installed on the bus.
    * If a particular controller is not installed its address should be set
    * to 0x0, and the controller will then be simulated.
    * NOTE: IT IS VERY IMPORTANT THAT THESE ADDRESSES ARE CORRECT.
    */

   /*
    * MVME167 0xc0000010
    * POWERPC 0x08000000
    */

#define   DET_CONTROL_PWFS1_SDSU_ADRS_VME 0x08000000      
                                    /* VME address of SDSU controller         */
                                    /* for PWFS1.                             */

   /*
    * Define the bit masks used to stop detector control processes individually.
    */

#define   DET_CONTROL_PWFS1_MASK          0x1      /* Bit 0 set */

   /*
    * Define the maximum data frame sizes for each of the wavefront sensors.
    */

#define DET_CONTROL_PWFS1_XSIZE           80
#define DET_CONTROL_PWFS1_YSIZE           80

   /*
    * Define the default number of SDSU data buffers allocated for PWFS1
    * sdsuLib expects there to be at least 2 buffers.
    */

#define DET_CONTROL_PWFS1_MAX_FRAMES      1   
                                     /* Was 5 - only 2 needed for simple task */

   /*
    * Define World Coordinate System constants.
    */

#define DET_CONTROL_MAX_WCSPOINTS         40   
                                     /* Max number of WCS calibration points. */

   /*
    * Define the default signal processing initialisation files for PWFS1
    * Set to "NONE" if no default signal processing initialisation is required.
    */

#define DET_CONTROL_PWFS1_OSPAOINI_FILE      "pwfs1ao.ini"  

   /*
    * Define the names of the OMF files containing the DSP code. These files are
    * downloaded automatically on startup. 
    */

#define   DET_CONTROL_OMF_FILE_PATH         "./bin/asm56000"
                                      /* Directory containing OMF files.      */

#define   DET_CONTROL_OMF_VME_FILE          "vme-39.lod"
                                      /* OMF file to download to VME DSP.     */

#define   DET_CONTROL_GBD_OMF_TIM_FILE      "tim-39.lod"
                                      /* OMF file to download to TIMING DSP   */
                                      /* for PWFS and OIWFS.                  */

#define   DET_CONTROL_OMF_UTL_FILE         "util.lod"
                                      /* OMF file to download to UTILITY DSP  */

   /* Define the name of the directory containing parameter files. */

#define   DET_CONTROL_PAR_FILE_PATH         "./data"

   /* Define the default directory to contain engineering data files. */

#define   DET_CONTROL_DATA_FILE_PATH         "."


typedef   struct      /* Context structure used to describe an observation.   */
{
                           /* AGWPS context information.                      */
                           /* --------------------------                      */
   SDSU_ID      sdsuId;    /* SDSU context.                                   */
   BOOL         observing; /* Flag set TRUE when observing.                   */
   BOOL         stopped;   /* Flag set TRUE when observation stopped.         */
   BOOL         continuous;/* BUG WORK AROUND: Set TRUE whenever the SDSU     */
                           /* controller is in continuous mode.               */
   DATREC_CONTEXT   pDetObservingContext;
                           /* Observing record context.                       */
   int          totalFrames;/* Total frames for observation.                  */
   int          outNFrames;/* Frame counter for output display.               */
   int          nframes;   /* Frame counter for this observation.             */
   SEM_ID       syncSem;   /* Observation synchronsisation semaphore.         */

                           /* Timer information.                              */
                           /* ------------------                              */
   timer_t      timeId;    /* ID of timer used to time observation in simu.   */

                           /* High level information.                         */
                           /* -----------------------                         */
   char         pWfsName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Name of wavefront sensor.                       */
   char         pObsType [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Type of observation.                            */

                           /* Data handling information.                      */
                           /* --------------------------                      */
   int          outOptions;/* Output options (0=none, 1=DHS, 2=file).         */
   int          dhsOutOptions;/* DHS Output options (0=PERM, 1=TEMP, 2=QL).   */
   DHS_CONNECT  dhsConnection;/* DHS connection ID.                           */
   DHS_BD_DATASET dhsDataset; /* DHS dataset ID.                              */
   DHS_BD_FRAME dhsDataFrame; /* DHS data frame ID.                           */
   float *      pCurFrame; /* Pointer to current unscrambled data frame.      */
   int          xPixels;   /* Number of columns in frame, in pixels.          */
   int          yPixels;   /* Number of rows in frame, in pixels.             */
   uint32       outputs;   /* Number of detector outputs.                     */

   char         pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* DHS data label.                                 */
   char         pOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for processed  */
                           /* data.                                           */
   char         pSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for simulated  */
                           /* data.                                           */
                           /* (This file is used for engineering only).       */

                           /* Fits keywords                                   */
                           /* -------------                                   */

   char          dataSec[22];
   char          ccdSec[22];
   char          origSec[22];
   int           timeArrayStart[7];/* Array of year/month/day/hour/min/sec    */
   char          utStartString[20];/* String which contains UTSTART data      */
   int           timeArrayEnd[7];  /* Array of year/month/day/hour/min/sec    */
   char          utEndString[20];  /* String to contain UTEND data            */
   char          detType[16];
   char          detId[16];

                           /* Signal processing information.                  */
                           /* ------------------------------                  */
   struct OSP_CONTEXT * ospAOContext;   
                           /* Pointer to AO signal processing context struct. */
   struct OSP_GEOMETRY * ospGeometry;   
                           /* Pointer to signal processing geometry structure */
   long         sigMode;   /* Signal processing mode.                         */
   long         nCoaddFrames;   
                           /* Number of frames to coadd.                      */
   long         timeToWaitAo;   
                           /* Time to wait in secs for AO mode                */
   int          coaddCounter;   
                           /* Counter used to decide when to save coadded data*/
   BOOL         updateAOGain;   
                           /* Flag to indicate if AO gains have been updated  */
                           /* when closed loop                                */
   double       pGain[OSP_ZMAX];   
                           /* Vector which contains new AO closed loop gains  */
   BOOL         updateFGGain;   
                           /* Flag to indicate if FG gains have been updated  */
                           /* when closed loop                                */
   double       tipGain;   /* FG tip gain, new value                          */
   double       tiltGain;  /* FG tilt gain, new value                         */

                           /* Time stamps.                                    */
                           /* ------------                                    */
   double       rawtStart; /* Raw Gemini time at start of observation.        */
   double       rawtEnd;   /* Raw Gemini time at end of observation.          */
   double       exposedRQ; /* Requested total exposure time.                  */
   double       exposed;   /* Actual total exposure time.                     */
   double       frameTime; /* Frame time.                                     */

                           /* World Coordinate System (WCS) information.      */
                           /* ------------------------------------------      */
   int          wcsStatus; /* WCS status (0 if WCS information OK).           */
   int          nWcsPoints;/* Number of WCS calibration points.               */
   double       fpxy[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of defined focal plane XY coordinates     */
   double       pixij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of measured pixel IJ coordinates          */
   double       detij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of pixel IJ coordinates applying to data  */
                           /* read from the detector.                         */
   double       cij[6];    /* XY to IJ transformation matrix.                 */
   char         ctype1[9]; /* WCS projection type for axis 1.                 */
   double       crpix1;    /* Pixel coordinate reference for axis 1.          */
   double       crval1;    /* World coordinate reference for axis 1.          */
   char         ctype2[9]; /* WCS projection type for axis 2.                 */
   double       crpix2;    /* Pixel coordinate reference for axis 2.          */
   double       crval2;    /* World coordinate reference for axis 2.          */
   double       cd1_1;     /* xi rotation/skew matrix element.                */
   double       cd1_2;     /* xj rotation/skew matrix element.                */
   double       cd2_1;     /* yi rotation/skew matrix element.                */
   double       cd2_2;     /* yj rotation/skew matrix element.                */
   double       RA;        /* Right Ascension in hours.                       */
   double       Dec;       /* Declination in degrees.                         */

   char         radecsys[9];/* Type of RA/Dec (for celestial coordinate).     */
   double       equinox;   /* Epoch of mean equator & equinox (celestial      */
                           /* coords).                                        */
   double       epoch;     /* Epoch of observation as a year.                 */
   double       mjdobs;    /* Epoch of observation as a modified Julian date. */
} OBS_ID_STRUCT, * OBS_ID;

   /*
    * Error number codes used by detControl.
    * These are designed to be processed using the vxWorks "makeStatTbl" 
    * utility.
    */

#define S_detControl_BAD_COMMAND   (M_detControl | 1) /* Unrecognised command */
#define S_detControl_BAD_WFS_NAME  (M_detControl | 2) /* Unrecognised WFS name*/
#define S_detControl_BAD_ATTRIBUTE (M_detControl | 3) /* Bad attribute value  */
#define S_detControl_BAD_FILE      (M_detControl | 4) /* Bad parameter file   */
#define S_detControl_SDSU_ERROR    (M_detControl | 5) /* Error from SDSU      */
                                                      /* controller           */
#define S_detControl_DHS_ERROR     (M_detControl | 6) /* Error from DHS       */
#define S_detControl_INTERNAL      (M_detControl | 7) /* detControl internal  */
                                                      /* failure              */
#define S_detControl_BUSY          (M_detControl | 8) /* Controller busy      */
#define S_detControl_NO_ACCESS     (M_detControl | 9) /* No access to hardware*/
#define S_detControl_WCS_ERROR     (M_detControl | 10)/* Error in WCS         */
                                                      /* calculation          */

   /*
    * Define the commands recognised by the detector controller task.
    */

enum
   {

   /* GBDS commands. */

   DET_CONTROL_CMD_SETUP = 0,  /* Set up SDSU controller parameters.          */
   DET_CONTROL_CMD_CHOP,       /* Specify chop states mask.                   */
   DET_CONTROL_CMD_EXPOSURE,   /* Specify exposure time.                      */
   DET_CONTROL_CMD_OBSTYPE,    /* Specify observation type.                   */
   DET_CONTROL_CMD_SETDHS,     /* Set Data Handling System parameters.        */
   DET_CONTROL_CMD_SETWCS,     /* Set World Coordinate System parameters.     */
   DET_CONTROL_CMD_OBSERVE,    /* Make observation.                           */
   DET_CONTROL_CMD_PAUSE,      /* Pause observation.                          */
   DET_CONTROL_CMD_CONTINUE,   /* Continue observation.                       */
   DET_CONTROL_CMD_STOP,       /* Stop observation.                           */
   DET_CONTROL_CMD_ABORT,      /* Abort observation.                          */
   DET_CONTROL_CMD_SIGINIT,    /* Initialise signal processing.               */
   DET_CONTROL_CMD_SIGINITGAIN,/* Initialise signal processing gains.         */
   DET_CONTROL_CMD_SIGINITSH,  /* Initialise WFS geometry for signal process. */
   DET_CONTROL_CMD_SIGMODE,    /* Configure signal processing.                */
   DET_CONTROL_CMD_SIGINITFGGAIN,/* Initialize FG gains                       */

   /* genSub commands. */

   DET_CONTROL_CMD_TTFZERO,    /* ttfZero.                                    */
   DET_CONTROL_CMD_AOZERO,     /* aoZero.                                     */
   DET_CONTROL_CMD_PROBEOFFSET,/* probeOffset.                                */

   /* Engineering commands. */

   DET_CONTROL_CMD_INITIALISE, /* Initialise SDSU controller.                 */
   DET_CONTROL_CMD_RESET,      /* Reset SDSU controller.                      */
   DET_CONTROL_CMD_TEST,       /* Test SDSU controller.                       */
   DET_CONTROL_CMD_GIVEUP,     /* Give up control of hardware (HRWFS/OIWFS).  */
   DET_CONTROL_CMD_SAVE,       /* Save SDSU controller parameters.            */
   DET_CONTROL_CMD_GEOMETRY,   /* Set detector readout geometry.              */
   DET_CONTROL_CMD_PRIMITIVE,  /* Execute SDSU primitive command.             */
   DET_CONTROL_CMD_DOWNLOAD,   /* Download DSP code.                          */
   DET_CONTROL_CMD_MODE,       /* Set detector readout mode.                  */
   DET_CONTROL_CMD_OFFSET,     /* Set detector ADC offsets.                   */
   DET_CONTROL_CMD_TEMP        /* Define temperature control params.          */
   };

   /* Public variables */

IMPORT BOOL        detDhsInitialised;
IMPORT SEM_ID      detDhsSem;

   /* Public functions */

IMPORT void   detShow (const char * pWfsNames, const BOOL verbose);
IMPORT void   detStatusShow (const char * pWfsNames);
IMPORT void   detTempShow (const char * pWfsNames);
IMPORT STATUS detObsShow (OBS_ID obsId, const BOOL verbose);
IMPORT void   detDhsErrorCallback (DHS_CONNECT connect, 
                                   DHS_STATUS errorNum,
                                   DHS_ERR_LEVEL errorLev, 
                                   char * msg, DHS_TAG tag, void * userData);
IMPORT STATUS detDhsInit (const char * pClientName, const int numConnect,
                          const char * pHostName, const char * pServerName);
