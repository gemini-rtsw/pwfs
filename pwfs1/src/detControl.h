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
 *   02 Apr 2001: CB - add sir adc0, adc1, adc2, adc3
 *   20 Feb 2001: CB - add sir dhsCon
 *   31 Jan 2001: CB - Move all the DATREC_CONTEXT structures into the obsId 
 *                     structure
 *   26 Jan 2001: CB - Add DET_CONTROL_PWFS1_CP_INIT_FILE
 *                         DET_CONTROL_PWFS1_MK_INIT_FILE
 *                     Replace/add DET_CONTROL_PWFS1_AO_FULL_CTRL_MK_INIT_FILE
 *                                 DET_CONTROL_PWFS1_AO_FULL_CTRL_CP_INIT_FILE
 *                                 DET_CONTROL_PWFS1_AO_BIN_CTRL_MK_INIT_FILE
 *                                 DET_CONTROL_PWFS1_AO_BIN_CTRL_CP_INIT_FILE
 *   08 Dec 2000: CB - Add aoFlag in sequence closed loop
 *                     add detSigReset
 *   07 Dec 2000: CB - Add aoSaveCbIm, aoSaveCbCtrl, aoSaveCbFgCtrl sir records
 *                     add cutoffFrequency rateSamplingFrequency
 *   20 Apr 2000: CB - Major modifications: replace ospLib with aoP1Lib,
 *                     add 3 SIR records for stae of signal processing,
 *                     add all the geometry sir records,
 *                     include coadd file + save,
 *                     replace detSigMode by several detSigModexxx cad
 *                     add detType, detId, dataLabel, intTime nexpRQ,
 *                     nexp, nframes, bunit, exposedRQ, exposed, utstart,
 *                     utend, elapsed sir recordsadd parameters to measure 
 *                     the average flux during the sequence closed loop
 *   14 Jan 2000: CB - Add focus gains
 *   27 oct 1999: CB - Add a new parameter dhsOutOptions in the structure 
 *                     + fits keywords
 *   8 July 1999: CB - Add a new parameter timeToWaitAo in the structure 
 *                     OBS_ID_STRUCT
 *   21 Apr 1999: CB - Simplified version for PWFS1 only
 *INDENT-ON*
 *-
 */


/***************************************************************** Includes ***/

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <timers.h>
#include "gemModNum.h"
#include "epToVxLib.h"
#include "sdsuLib.h"
# include "aoP1Lib.h"

#include "dhs.h"

/****************************************************************** Defines ***/

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

#define   DET_CONTROL_TEST_SIR_NAME          "testing"
                                    /* Name of SIR record containing          */
                                    /* test state.                            */

#define   DET_CONTROL_PRIM_REPLY_SIR_NAME     "detPrimReply"
                                    /* Name of SIR record containing          */
                                    /* reply from SDSU primitive cmd.         */

#define   DET_CONTROL_OBSERVING_SIR_NAME      "observing"
                                    /* Name of SIR record containing          */
                                    /* observing state.                       */

#define   DET_CONTROL_MEAS_SIR_NAME           "measuring"
                                    /* Name of SIR record containing          */
                                    /* measuring state                        */

#define   DET_CONTROL_AOCTRLINIT_SIR_NAME     "aoCtrlInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the AO control context        */
                                    /* structure                              */

#define   DET_CONTROL_AODARKINIT_SIR_NAME     "aoDarkInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the dark buffer               */

#define   DET_CONTROL_AOFLATINIT_SIR_NAME     "aoFlatInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the flat buffer               */

#define   DET_CONTROL_AOINTMATINIT_SIR_NAME   "aoIntMatInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the interaction matrix        */

#define   DET_CONTROL_AOCONTMATINIT_SIR_NAME  "aoContMatInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the control matrix            */

#define   DET_CONTROL_AOFGCONTMATINIT_SIR_NAME "aoFgContMatInit"
                                    /* Name of SIR record containing the init */
                                    /* state of the theoretical FG control    */
                                    /* matrix                                 */

#define   DET_CONTROL_AOTHRESH_SIR_NAME       "aoThresh"
                                    /* Name of SIR record containing the      */
                                    /* threshold for centroids computation    */

#define   DET_CONTROL_AOTOTAL_SIR_NAME        "aoTotal"
                                    /* Name of SIR record containing the      */
                                    /* flux threshold for centroids comp.     */

#define   DET_CONTROL_AOSAVECBIM_SIR_NAME     "aoSaveCbIm"
                                    /* Name of SIR record containing the      */
                                    /* ao Save Image CB Flag                  */

#define   DET_CONTROL_AOSAVECBCTRL_SIR_NAME   "aoSaveCbCtrl"
                                    /* Name of SIR record containing the      */
                                    /* ao Save Control CB Flag                */

#define   DET_CONTROL_AOSAVECBFGCTRL_SIR_NAME "aoSaveCbFgCtrl"
                                    /* Name of SIR record containing the      */
                                    /* ao Save FG Control CB Flag             */

#define   DET_CONTROL_AOPROCESSMODE_SIR_NAME  "aoProcessMode"
                                    /* Name of SIR record containing the      */
                                    /* processing mode                        */

#define   DET_CONTROL_OUTPUTS_SIR_NAME        "outputs"
                                    /* Name of SIR record containing the      */
                                    /* number of ouputs                       */

#define   DET_CONTROL_DETXSIZE_SIR_NAME       "detXsize"
                                    /* Name of SIR record containing the X    */
                                    /* detector size                          */

#define   DET_CONTROL_DETYSIZE_SIR_NAME       "detYsize"
                                    /* Name of SIR record containing the Y    */
                                    /* detector size                          */

#define   DET_CONTROL_XSUBAP_SIR_NAME         "xsubap"
                                    /* Name of SIR record containing the X    */
                                    /* number of subapertures / output        */

#define   DET_CONTROL_YSUBAP_SIR_NAME         "ysubap"
                                    /* Name of SIR record containing the Y    */
                                    /* number of subapertures / output        */

#define   DET_CONTROL_XSTART_SIR_NAME         "xstart"
                                    /* Name of SIR record containing the X    */
                                    /* left offset                            */

#define   DET_CONTROL_YSTART_SIR_NAME         "ystart"
                                    /* Name of SIR record containing the Y    */
                                    /* bottom offset                          */

#define   DET_CONTROL_XRASTER_SIR_NAME        "xras"
                                    /* Name of SIR record containing the X    */
                                    /* subaperture size                       */

#define   DET_CONTROL_YRASTER_SIR_NAME        "yras"
                                    /* Name of SIR record containing the Y    */
                                    /* subaperture size                       */

#define   DET_CONTROL_XSPACE_SIR_NAME         "xspace"
                                    /* Name of SIR record containing the X    */
                                    /* space between subapertures             */

#define   DET_CONTROL_YSPACE_SIR_NAME         "yspace"
                                    /* Name of SIR record containing the Y    */
                                    /* space between subapertures             */

#define   DET_CONTROL_XBIN_SIR_NAME           "xbin"
                                    /* Name of SIR record containing the X    */
                                    /* binning factor                         */

#define   DET_CONTROL_YBIN_SIR_NAME           "ybin"
                                    /* Name of SIR record containing the Y    */
                                    /* binning factor                         */

#define   DET_CONTROL_DETTYPE_SIR_NAME        "detType"
                                    /* Name of SIR record containing the type */
                                    /* of detector controller                 */

#define   DET_CONTROL_DETID_SIR_NAME          "detID"
                                    /* Name of SIR record containing the SN of*/
                                    /* the CCD                                */

#define   DET_CONTROL_DATALABEL_SIR_NAME      "dataLabel"
                                    /* Name of SIR record containing the most */
                                    /* recent DHS data label                  */

#define   DET_CONTROL_INTTIME_SIR_NAME        "intTime"
                                    /* Name of SIR record containing the      */
                                    /* integration time                       */

#define   DET_CONTROL_NEXPRQ_SIR_NAME         "nexpRQ"
                                    /* Name of SIR record containing          */
                                    /* requested nb of exp/dataset            */

#define   DET_CONTROL_NEXP_SIR_NAME           "nexp"
                                    /* Name of SIR record containing current  */
                                    /* nb of exp/dataset                      */

#define   DET_CONTROL_NFRAMES_SIR_NAME        "nframes"
                                    /* Name of SIR record containing nb of    */
                                    /* frames/dataset                         */

#define   DET_CONTROL_BUNIT_SIR_NAME          "bunit"
                                    /* Name of SIR record containing the data */
                                    /* unit                                   */

#define   DET_CONTROL_UTSTART_SIR_NAME        "utstart"
                                    /* Name of SIR record containing the ut   */
                                    /* at start of observation                */

#define   DET_CONTROL_UTEND_SIR_NAME          "utend"
                                    /* Name of SIR record containing the ut   */
                                    /* at end of observation                  */

#define   DET_CONTROL_EXPOSED_SIR_NAME        "exposed"
                                    /* Name of SIR record containing the      */
                                    /* total integration time                 */

#define   DET_CONTROL_EXPOSEDRQ_SIR_NAME      "exposedRQ"
                                    /* Name of SIR record containing the      */
                                    /* requested total integration            */

#define   DET_CONTROL_ELAPSED_SIR_NAME        "elapsed"
                                    /* Name of SIR record containing the      */
                                    /* elapsed time                           */

#define   DET_CONTROL_DHSCON_SIR_NAME         "dhsCon"
                                    /* Name of SIR record containing the      */
                                    /* status of the dhs connection           */

#define   DET_CONTROL_ADC0_SIR_NAME           "adc0"
                                    /* Name of SIR record containing the      */
                                    /* ADC of the output 0                    */

#define   DET_CONTROL_ADC1_SIR_NAME           "adc1"
                                    /* Name of SIR record containing the      */
                                    /* ADC of the output 1                    */

#define   DET_CONTROL_ADC2_SIR_NAME           "adc2"
                                    /* Name of SIR record containing the      */
                                    /* ADC of the output 2                    */

#define   DET_CONTROL_ADC3_SIR_NAME           "adc3"
                                    /* Name of SIR record containing the      */
                                    /* ADC of the output 3                    */

#define   DET_CONTROL_OBSERVE_CAD_NAME        "observe"
                                    /* Name of observe CAD record       */

#define   DET_DHS_TASK_PRIORITY               210
                                    /* Priority of the dhs task               */

#define   DET_DHS_TASK_STACK_SIZE             0x100000
                                    /* Stack size needed by the dhs task      */

#define   DET_CONTROL_PWFS1_SDSU_ADRS_VME     0x08000000
                                    /* VME address of PWFS1 SDSU controller   */
                                    /* If the controller is not installed its */
                                    /* address should be set to 0x0, and the  */
                                    /* controller will then be simulated      */
                                    /* MVME167 0xc0000020, POWERPC 0x08000000 */

#define   DET_CONTROL_PWFS1_MASK              0x1      
                                    /* Define the bit masks used to stop      */
                                    /* the detector control process, Bit 1 set*/


#define DET_CONTROL_PWFS1_MAX_FRAMES          1   
                                    /* Define the default number of SDSU data */
                                    /* buffers allocated for PWFS1            */

#define DET_CONTROL_MAX_WCSPOINTS             40   
                                     /* Max number of WCS calibration points. */

#define DET_CONTROL_PWFS1_MK_INIT_FILE        "defDetContP1MK.dat"
                                    /* Define the MK default init file for    */
                                    /* PWFS1 detector controller. Set to      */
                                    /* "NONE" if no default settings is       */
                                    /* required.                              */

#define DET_CONTROL_PWFS1_CP_INIT_FILE        "defDetContP1CP.dat"
                                    /* Define the CP default init file for    */
                                    /* PWFS1 detector controller. Set to      */
                                    /* "NONE" if no default settings is       */
                                    /* required.                              */

#define DET_CONTROL_PWFS1_AO_FULL_CTRL_MK_INIT_FILE   "defFullCtrlP1MK.dat"  
                                    /* Define the MK default ao control init  */
                                    /* file for PWFS1 when no binning. Set to */
                                    /* "NONE" if no default ao control        */
                                    /* initialisation is required.            */

#define DET_CONTROL_PWFS1_AO_BIN_CTRL_MK_INIT_FILE    "defBinCtrlP1MK.dat"  
                                    /* Define the MK default ao control init  */
                                    /* file for PWFS1 when binning. Set to    */
                                    /* "NONE" if no default ao control        */
                                    /* initialisation is required.            */

#define DET_CONTROL_PWFS1_AO_FULL_CTRL_CP_INIT_FILE   "defFullCtrlP1CP.dat"  
                                    /* Define the CP default ao control init  */
                                    /* file for PWFS1 when no binning. Set to */
                                    /* "NONE" if no default ao control        */
                                    /* initialisation is required.            */

#define DET_CONTROL_PWFS1_AO_BIN_CTRL_CP_INIT_FILE    "defBinCtrlP1CP.dat"  
                                    /* Define the CP default ao control init  */
                                    /* file for PWFS1 when binning. Set to    */
                                    /* "NONE" if no default ao control        */
                                    /* initialisation is required.            */

#define   DET_CONTROL_OMF_FILE_PATH           "./bin/asm56000"
                                    /* Directory containing OMF files for the */
                                    /* DSP code                               */

#define   DET_CONTROL_OMF_VME_FILE            "vme-39.lod"
                                    /* OMF file to download to VME DSP.       */

#define   DET_CONTROL_GBD_OMF_TIM_FILE        "tim-39.lod"
                                    /* OMF file to download to TIMING DSP     */

#define   DET_CONTROL_OMF_UTL_FILE            "util.lod"
                                    /* OMF file to download to UTILITY DSP    */

#define   DET_CONTROL_PAR_FILE_PATH           "./data"
                                    /* Name of the directory containing par   */
                                    /* files.                                 */

#define   DET_CONTROL_DATA_FILE_PATH          "."
                                    /* Define the default directory to contain*/
                                    /* engineering data files.                */

#define   DET_TYPE "CCD39+SDSUII"

#define   DET_CCD_SN "a5209-5-11"

#define   DET_BUNIT "SDSU ADC units"

/********************************************************************* Enum ***/

enum
{
   NOT_INIT = 0,           /* DHS is not initialized                          */
   CONNECTED,              /* DHS is connected                                */
   NOT_CONNECTED           /* DHS is not connected                            */
};

/****************************************************************** Typedef ***/

typedef   struct      /* Context structure used to describe an observation.   */
{
                           /* AGWPS context information.                      */
                           /* --------------------------                      */
   SDSU_ID      sdsuId;    /* SDSU context.                                   */
   BOOL         observing; /* Flag set TRUE when observing.                   */
   BOOL         stopped;   /* Flag set TRUE when observation stopped.         */
   BOOL         continuous;/* BUG WORK AROUND: Set TRUE whenever the SDSU     */
                           /* controller is in continuous mode.               */
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
   int          xPixelsDhs;/* Number of columns in frame, in pixels.          */
   int          yPixelsDhs;/* Number of rows in frame, in pixels.             */
   int          dhsCounter;/* Counter for frames to be sent to the QL         */
   int          dhsQlRate; /* Number of frames send to the DHS QL             */
   DHS_BD_DATASET dhsDataset; /* DHS dataset ID.                              */
   DHS_BD_FRAME dhsDataFrame; /* DHS data frame ID.                           */
   float *      pCurFrame; /* Pointer to current unscrambled data frame.      */
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

                           /* AO parameters                                   */
                           /* -------------                                   */
   AO_CCD_ID    aoCcdId;   /* AO CCD geometry context structure               */
   AO_CTRL_ID   aoCtrlId;  /* AO control context structure                    */
   AO_CB_IM_ID  aoCbImId;  /* AO image circular buffer context structure      */
   AO_CB_CTRL_ID aoCbCtrlId;
                           /* aO control circular buffer context structure    */
   AO_CB_FG_CTRL_ID aoCbFgCtrlId;
                           /* FG control circular buffer context structure    */
   int          coaddCounter;
                           /* Counter of coadding images                      */
   int          saveCbCounter;
                           /* Counter of used in closed loop sequence to save */
                           /* the aO control circular buffer                  */
   int          saveFgCbCounter;
                           /* Counter of used in closed loop sequence to save */
                           /* the FG control circular buffer                  */
   int          updateFgScale;
                           /* Flag to indicate if the FG scale factors have   */
                           /* been updated                                    */
   int          updateAoScale;
                           /* Flag to indicate if the aO scale factors have   */
                           /* been updated                                    */
   long         saveCentroids;
                           /* Flag to indicate if we want to save centroids   */
                           /* data when FG FOCUS and COADD mode               */
   long         nMode;     /* Mode number when computing a column of the      */
                           /* interaction matrix                              */
   long         saveCbIm;  /* Save the image circular buffer flag TRUE/FALSE. */
   long         saveCbCtrl;/* Save the control circular buffer flag TRUE/FALSE*/
   long         saveCbFgCtrl;
                           /* Save the FG control CB flag TRUE/FALSE          */
   long         sigMode;   /* Signal processing mode.                         */
   long         nCoaddFrames;
                           /* Number of frames to coadd.                      */
   long         methodThreshComp;
                           /* Method for threshold computation                */
   long         nAverageDataThreshComp;
                           /* Number of data to average for threshold         */
                           /* computation                                     */
   long         saveCbFgCtrlClosedLoop;
                           /* Save FG control circular buffer during closed   */
                           /* loop sequence                                   */
   long         saveCbFgCtrlClosedLoopFrame;
                           /* Save FG control circular buffer during closed   */
                           /* loop sequence every this number of frames       */
   long         saveCbCtrlClosedLoop;
                           /* Save aO control circular buffer during closed   */
                           /* loop sequence                                   */
   long         saveCbCtrlClosedLoopFrame;
                           /* Save aO control circular buffer during closed   */
                           /* loop sequence every this number of frames       */
   long         fgFrame;   /* Number of frames with FG only over the whole CCD*/
                           /* in the closed loop sequence                     */
   long         methodFluxComp;
                           /* Method for average flux computation             */
   long         averageFluxFlag;
                           /* Average flux after FG Flag                      */
   long         threshFlag;/* Threshold after FG Flag                         */
   long         aoFlag;    /* aO Flag in sequence closed loop                 */
   long         nFramesAverageFlux;
                           /* Number of frames to average for computing the   */
                           /* average flux                                    */
   double       fgTime;    /* Time with FG only over the whole CCD in the     */
                           /* closed loop sequence                            */
   double       saveCbFgCtrlClosedLoopTime;
                           /* Save FG control circular  buffer during closed  */
                           /* loop sequence every this time                   */
   double       saveCbCtrlClosedLoopTime;
                           /* Save aO control circular  buffer during closed  */
                           /* loop sequence every this time                   */
   double       rateBrightPixThreshComp;
                           /* Rate for brightest pixels for threshold         */
                           /* computation                                     */
   double       multCoeffRmsThreshComp;
                           /* Multiplicative coeff for threshold computation  */
   double       averageRms;
                           /* Average rms for threshold computation           */
   double       multCoeffAverageFlux;
                           /* Multiplicative coefficient for average flux     */
   double       averageFlux;
                           /* Average flux                                    */
   double       tipScale;  /* Scale factor of the tip mode                    */
   double       tiltScale; /* Scale factor of the tilt mode                   */
   double       focusScale; /* Scale factor of the focus mode                 */
   double       slidingFocusGain; 
                           /* Gain for the sliding average for the focus mode */
   double       amplitude; /* Amplitude of the mode when computing a column   */
                           /* of the interaction matrix                       */
   AO_VECT      aoScaleVect; 
                           /* Scale factor vector for aO modes                */
   char         pCoaddFileName[(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for coadd data */
   char         pCentFileName[(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for centroids  */
   char         pCentComment[EPICS_MAX_BYTES_STRING_ATTRIB];
   char         pCbPath[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Directory where to save the Circular Buffers    */
   char         pCbPathSeq[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Directory where to save the control circular    */
                           /* buffer during closed loop                       */

                           /* Fits keywords                                   */
                           /* -------------                                   */

   char          dataSec[22];
   char          ccdSec[22];
   char          origSec[22];
   char          utStartString[20];/* String which contains UTSTART data      */
   char          utEndString[20];  /* String to contain UTEND data            */
   char          detType[16];
   char          detId[16];
   int           timeArrayStart[7];/* Array of year/month/day/hour/min/sec    */
   int           timeArrayEnd[7];  /* Array of year/month/day/hour/min/sec    */

                           /* Time stamps.                                    */
                           /* ------------                                    */
   double       rawtStart; /* Raw Gemini time at start of observation.        */
   double       rawtEnd;   /* Raw Gemini time at end of observation.          */
   double       exposureTime;
                           /* Current exposure time                           */
   double       cutoffFrequency;
                           /* Cuttof frequency (bandwidth ) of the system     */
   double       rateSamplingFrequency;
                           /* Rate of sampling frequency (between 0 and 1)    */
   double       exposedRQ; /* Requested total exposure time.                  */
   double       exposed;   /* Actual total exposure time.                     */

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
   double       crpix1;    /* Pixel coordinate reference for axis 1.          */
   double       crval1;    /* World coordinate reference for axis 1.          */
   double       crpix2;    /* Pixel coordinate reference for axis 2.          */
   double       crval2;    /* World coordinate reference for axis 2.          */
   double       cd1_1;     /* xi rotation/skew matrix element.                */
   double       cd1_2;     /* xj rotation/skew matrix element.                */
   double       cd2_1;     /* yi rotation/skew matrix element.                */
   double       cd2_2;     /* yj rotation/skew matrix element.                */
   double       RA;        /* Right Ascension in hours.                       */
   double       Dec;       /* Declination in degrees.                         */
   double       equinox;   /* Epoch of mean equator & equinox (celestial      */
                           /* coords).                                        */
   double       epoch;     /* Epoch of observation as a year.                 */
   double       mjdobs;    /* Epoch of observation as a modified Julian date. */
   char         ctype1[9]; /* WCS projection type for axis 1.                 */
   char         ctype2[9]; /* WCS projection type for axis 2.                 */
   char         radecsys[9];/* Type of RA/Dec (for celestial coordinate).     */

                           /* SAD information                                 */
                           /* ---------------                                 */

   DATREC_CONTEXT pStateContext;      /* Context structure for state SIR      */
                                      /* record                               */
   DATREC_CONTEXT pDetInitContext;    /* Context structure for initialising   */
                                      /* state SIR record.                    */
   DATREC_CONTEXT pDetInitStatusContext;
                                      /* Context structure for SDSU           */
                                      /* initialisation status SIR record.    */
   DATREC_CONTEXT pDetTestContext;    /* Context structure for                */
                                      /* testing state SIR record.            */
   DATREC_CONTEXT pTestResultsContext;
                                      /* Context structure for SDSU test      */
                                      /* results SIR record.                  */
   DATREC_CONTEXT pDetPrimReplyContext;
                                      /* Context structure for SDSU           */
                                      /* primitive reply string SIR record.   */
   DATREC_CONTEXT pDetObservingContext;
                                      /* Observing record context.            */
   DATREC_CONTEXT pDetMeasuringContext;
                                      /* Measuring record context.            */
   DATREC_CONTEXT pOutputsContext ;   /* Number of outputs SIR record         */
                                      /* context structure                    */
   DATREC_CONTEXT pDetXsizeContext ;  /* X detector size SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pDetYsizeContext ;  /* Y detector size SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pXsubapContext ;    /* X subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pYsubapContext ;    /* Y subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pXstartContext ;    /* X left offset SIR record context     */
                                      /* structure                            */
   DATREC_CONTEXT pYstartContext ;    /* Y bottom offset SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pXrasterContext ;   /* X subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pYrasterContext ;   /* Y subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pXspaceContext ;    /* X space between subapertures SIR     */
                                      /* record context structure             */
   DATREC_CONTEXT pYspaceContext ;    /* Y space between subapertures SIR     */
                                      /* record context structure             */
   DATREC_CONTEXT pXbinContext ;      /* X binning factor SIR record context  */
                                      /* structure                            */
   DATREC_CONTEXT pYbinContext ;      /* Y binning factor SIR  record context */
                                      /* structure                            */
   DATREC_CONTEXT pAoCtrlInitContext; /* Context structure for aoCtrlInit     */
                                      /* SIR record.                          */
   DATREC_CONTEXT pAoFlatInitContext; /* Context structure for aoFlatInit     */
                                      /* SIR record.                          */
   DATREC_CONTEXT pAoContMatInitContext;
                                      /* Context structure for                */
                                      /* aoContMatInit SIR record.            */
   DATREC_CONTEXT pAoIntMatInitContext; 
                                      /* Context structure for                */
                                      /* aoIntMatInit SIR record.             */
   DATREC_CONTEXT pAoFgContMatInitContext;
                                      /* Context structure for                */
                                      /* aoFgContMatInit SIR record.          */
   DATREC_CONTEXT pAoDarkInitContext; /* Context structure for aoDarkInit     */
                                      /* SIR record.                          */
   DATREC_CONTEXT pAoThreshContext;   /* Context structure for aoThresh SIR   */
                                      /* record.                              */
   DATREC_CONTEXT pAoProcessModeContext;
                                      /* Context structure for aoProcessMode  */
                                      /* SIR record.                          */
   DATREC_CONTEXT pAoTotalContext;    /* Context structure for aoTotal SIR    */
                                      /* record.                              */
   DATREC_CONTEXT pAoSaveCbImContext; /* Context structure for aoSaveCbIm SIR */
                                      /* record.                              */
   DATREC_CONTEXT pAoSaveCbCtrlContext;
                                      /* Context structure for aoSaveCbCtrl   */
                                      /* SIR record.                          */
   DATREC_CONTEXT pAoSaveCbFgCtrlContext;
                                      /* Context structure for aoSaveCbFgCtrl */
                                      /* SIR record.                          */
   DATREC_CONTEXT pDetTypeContext;    /* Context structure for detector       */
                                      /* controller type.                     */
   DATREC_CONTEXT pDetIdContext;      /* Context structure for detector       */
                                      /* Id or SN                             */
   DATREC_CONTEXT pBunitContext ;     /* Data unit SIR record context         */
                                      /* structure                            */
   DATREC_CONTEXT pDataLabelContext ; /* Data Label SIR record context        */
                                      /* structure                            */
   DATREC_CONTEXT pIntTimeContext ;   /* Integration time SIR record context  */
                                      /* structure                            */
   DATREC_CONTEXT pNExpRQContext ;    /* Requested number of exp/data set SIR */
                                      /* record context structure             */
   DATREC_CONTEXT pNExpContext ;      /* Actual number of exp/data set SIR    */
                                      /* record context structure             */
   DATREC_CONTEXT pNFramesContext ;   /* Number of frames/data set SIR        */
                                      /* record context structure             */
   DATREC_CONTEXT pUTstartContext ;   /* UT at start of observation SIR record*/
                                      /* context structure                    */
   DATREC_CONTEXT pUTendContext ;     /* UT at end of observation SIR record  */
                                      /* context structure                    */
   DATREC_CONTEXT pExposedRQContext ; /* Requested total integration time SIR */
                                      /* record context structure             */
   DATREC_CONTEXT pExposedContext ;   /* Actual total integration time SIR    */
                                      /* record context structure             */
   DATREC_CONTEXT pElapsedContext ;   /* Actual elapsed time SIR record       */
   DATREC_CONTEXT pDhsConContext ;    /* dhs connection status SIR record     */
                                      /* context structure                    */
   DATREC_CONTEXT pAdc0Context ;      /* ADC 0 SIR record context structure   */
   DATREC_CONTEXT pAdc1Context ;      /* ADC 1 SIR record context structure   */
   DATREC_CONTEXT pAdc2Context ;      /* ADC 2 SIR record context structure   */
   DATREC_CONTEXT pAdc3Context ;      /* ADC 3 SIR record context structure   */
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

   DET_CONTROL_CMD_CHOP = 0,   /* Specify chop states mask.                   */
   DET_CONTROL_CMD_FRAME_SIZE, /* Specify frame size (binning or not)         */
   DET_CONTROL_CMD_DHS_RECONNECT,/* Set connection with DHS                   */
   DET_CONTROL_CMD_DHS_DISPLAY,/* Set display parameters for DHS QL           */
   DET_CONTROL_CMD_EXPOSURE,   /* Specify exposure time.                      */
   DET_CONTROL_CMD_OBSTYPE,    /* Specify observation type.                   */
   DET_CONTROL_CMD_SETDHS,     /* Set Data Handling System parameters.        */
   DET_CONTROL_CMD_SETWCS,     /* Set World Coordinate System parameters.     */
   DET_CONTROL_CMD_OBSERVE,    /* Make observation.                           */
   DET_CONTROL_CMD_PAUSE,      /* Pause observation.                          */
   DET_CONTROL_CMD_CONTINUE,   /* Continue observation.                       */
   DET_CONTROL_CMD_STOP,       /* Stop observation.                           */
   DET_CONTROL_CMD_ABORT,      /* Abort observation.                          */
   DET_CONTROL_CMD_SIGRESET,   /* Reset signal processing.                    */
   DET_CONTROL_CMD_SIGINIT,    /* Initialise signal processing.               */
   DET_CONTROL_CMD_SIGINITGAIN,/* Initialise signal processing gains.         */
   DET_CONTROL_CMD_SIGINITSH,  /* Initialise WFS geometry for signal process. */
   DET_CONTROL_CMD_SIGINITFGGAIN,/* Initialize FG gains                       */
   DET_CONTROL_CMD_SIGINITBW,  /* Init Butterworth filter.                    */
   DET_CONTROL_CMD_SIGMODE_NONE, /* Configure to no signal processing.        */
   DET_CONTROL_CMD_SIGMODE_DARK, /* Configure to dark subtraction only.       */
   DET_CONTROL_CMD_SIGMODE_COADD, /* Configure to coadd only.                 */
   DET_CONTROL_CMD_SIGMODE_THRESH,/* Configure to compute threshold.          */
   DET_CONTROL_CMD_SIGMODE_TOTAL,
                               /* Configure to average flux computation mode. */
   DET_CONTROL_CMD_SIGMODE_GG, /* Configure to global guide only.             */
   DET_CONTROL_CMD_SIGMODE_GG_COADD,
                               /* Configure to fast guide and coadd mode.     */
   DET_CONTROL_CMD_SIGMODE_FG_FOCUS, /* Configure to fast guide and focus.    */
   DET_CONTROL_CMD_SIGMODE_FG_FOCUS_COADD, 
                               /* Configure to fast guide and focus and coadd */
                               /* mode.                                       */
   DET_CONTROL_CMD_SIGMODE_AO, /* Configure to aO only.                       */
   DET_CONTROL_CMD_SIGMODE_GG_AO,
                               /* Configure to global guide and aO.           */
   DET_CONTROL_CMD_SIGMODE_FG_FOCUS_AO, /* Configure to FG and focus and aO.  */
   DET_CONTROL_CMD_SIGMODE_SEQ_DARK,/* Configure sequence dark mode.          */
   DET_CONTROL_CMD_SIGMODE_SEQ,/* Configure sequence closed loop mode.        */
   DET_CONTROL_CMD_SIGINIT_CB, /* Save circular buffers.                      */
   DET_CONTROL_CMD_SIGMEAS_IM, /* Measure column of interaction matrix.       */
   DET_CONTROL_CMD_SIGCOMP_MAT,/* Compute control and interaction matrixes.   */


   /* genSub commands. */

   DET_CONTROL_CMD_TTFZERO,    /* ttfZero.                                    */
   DET_CONTROL_CMD_AOZERO,     /* aoZero.                                     */
   DET_CONTROL_CMD_PROBEOFFSET,/* probeOffset.                                */

   /* Engineering commands. */

   DET_CONTROL_CMD_INITIALISE, /* Initialise SDSU controller.                 */
   DET_CONTROL_CMD_RESET,      /* Reset SDSU controller.                      */
   DET_CONTROL_CMD_TEST,       /* Test SDSU controller.                       */
   DET_CONTROL_CMD_SAVE,       /* Save SDSU controller parameters.            */
   DET_CONTROL_CMD_GEOMETRY,   /* Set detector readout geometry.              */
   DET_CONTROL_CMD_PRIMITIVE,  /* Execute SDSU primitive command.             */
   DET_CONTROL_CMD_MODE,       /* Set detector readout mode.                  */
   DET_CONTROL_CMD_OFFSET,     /* Set detector ADC offsets.                   */
   DET_CONTROL_CMD_TEMP        /* Define temperature control params.          */
   };

   /* Public variables */

IMPORT BOOL        detDhsInitialised;

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
