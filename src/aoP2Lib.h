#ifndef __INCaoP2Libh
#define __INCaoP2Libh

/*
 * MODULE NAME: 
 * aoP2Lib
 * 
 * FILENAME: 
 * aoP2Lib.h
 *
 * PURPOSE: 
 * Include file for PWFS2 active optics library 
 * Contains all the types and constants definition of this library
 * 
 * AUTHORS:
 * Corinne Boyer
 *
 * HISTORY MODIFICATION:
 * 25 October 2000: CB - Replace aoRmsNoiseDarkCompute aoRmsNoiseImageCompute
 * 13 May 1999 - CB - Original creation
 *
 */

/***************************************************** Constants definition ***/

#define STRING_SIZE       160          /* Size of a string                    */

#define CCD_XSIZE         80           /* Default value for X size of the CCD */

#define CCD_YSIZE         80           /* Default value for Y size of the CCD */

#define CCD_SIZE          (CCD_XSIZE * CCD_YSIZE)
                                       /* Default value for CCD size          */

#define SUBAP_NB          4            /* Max number of subapertures          */

#define MODE_NB           19           /* Max number of zernikes mode to be   */
                                       /* corrected - for P2 only 3           */

#define CB_IM_RECORD_NB   100          /* Number of records of the image      */
                                       /* circular buffer                     */

#define CB_CTRL_RECORD_NB 2000         /* Number of records of the control    */
                                       /* circular buffer                     */

#define AO_SUBAP_OFF      32767        /* Indicates there is no light on at   */
                                       /* least one subaperture               */

#define AO_SH_OFF         65536        /* Indicates there is no light on at   */
                                       /* least two subapertures              */

#define AO_MIN_DOUBLE     1.0e-10      /* Mininum double used when comparing  */
                                       /* total counts to threshold           */

/********************************************************************* Enum ***/

enum
{
   AO_MODE_NONE = 0,       /* No signal processing.                  */
   AO_MODE_DARK,           /* Subtract DARK frame.                   */
   AO_MODE_COADD,          /* Coadd Only mode.                       */
   AO_MODE_THRESH,         /* Threshold computation mode             */
   AO_MODE_TOTAL,          /* Average flux computation               */ 
   AO_MODE_GG,             /* Global Guide mode.                     */
   AO_MODE_GG_COADD,       /* Global Guide and Coadd mode.           */
   AO_MODE_FG_FOCUS,       /* FG and focus correction mode           */
   AO_MODE_FG_FOCUS_COADD, /* FG and focus and Coadd mode            */
   AO_MODE_SEQ_DARK,       /* Sequence dark starting averaging       */
                           /* images and then computing the threshold*/
   AO_MODE_CLOSED_LOOP,    /* Sequence of closed loop starting       */
                           /* with FG and next with FG and focus     */
   AO_MODE_MAX             /* Maximum mode marker.                   */
};

enum
{
   AO_THRESH_SPOTS = 0, /* Computation of threshold whith spots   */
   AO_THRESH_NOSPOTS,   /* Computation of threshold without spots */
   AO_THRESH_VALUE      /* No computation, used given value       */
};

enum
{
   AO_TOTAL_SPOTS = 0, /* Computation of average flux when spots */
   AO_TOTAL_VALUE      /* No computation, used given value       */
};

/***************************************** Definition of vectors and matrix ***/

typedef float IMAGE_VECT [ CCD_SIZE ];

typedef double WFS_VECT [ (2 * SUBAP_NB) + 2 ];  
                                       /* 2 informations per subaperture +    */
                                       /* 2 information for the whole CCD     */

typedef double COMMAND_VECT [ MODE_NB ];

/*************************** Definition of the structure describing the WFS ***/

typedef struct 
{

   unsigned long  outputsNb;           /* Number of sectors of the CCD        */
                                       /* By default T_OUTPUTS = 4            */

   int        xSize;                   /* Columns number in pixels of the CCD */
                                       /* per output, T_XSIZE                 */

   int        ySize;                   /* Rows number in pixels of the CCD    */
                                       /* per output, T_YSIZE                 */

   int        xMax;                    /* Max columns number in pixels of the */
                                       /* CCD, xSize * outputsNb / 2          */

   int        yMax;                    /* Max rows number in pixels of the    */
                                       /* CCD, ySize * outputsNb / 2          */

   int        xStart;                  /* Number of columns to be discarded   */
                                       /* before the first subaperture        */
                                       /* T_XSTART                            */

   int        yStart;                  /* Number of rows to be discarded      */
                                       /* before the first subaperture        */
                                       /* T_YSTART                            */

   int        xBin;                    /* Binning factor in X (by default 1)  */
                                       /* T_XBIN                              */

   int        yBin;                    /* Binning factor in Y (by default 1)  */
                                       /* T_YBIN                              */
   
   int        xRaster;                 /* X size of a subaperture in binned   */
                                       /* pixels, T_XRAS                      */

   int        yRaster;                 /* Y size of a subaperture in binned   */
                                       /* pixels, T_YRAS                      */

   int        xSpace;                  /* Columns number in pixels to be      */
                                       /* discarded between subapertures      */
                                       /* T_XSPACE                            */

   int        ySpace;                  /* Rows number in pixels to be         */
                                       /* discarded between subapertures      */
                                       /* T_YSPACE                            */

   int        xSubapNb;                /* Column number of subapertures per   */
                                       /* output, T_XSUBAP                    */

   int        ySubapNb;                /* Row number of subapertures          */
                                       /* output, T_YSUBAP                    */

   int        subapNb;                 /* Total number of subapertures, must  */
                                       /* be equal to xSubapNb * ySubapNb *   */
                                       /* outputsNb                           */

   int        subapNotUsedNb;          /* Number of subapertures not used     */

   int        subapUsedNb;             /* Total number of used subapertures,  */
                                       /* must be equal to subapNb -          */
                                       /* subapNotUsedNb                      */

   int        subapUsedVect [ SUBAP_NB ];
                                       /* Vector describing the subapertures  */
                                       /* which are used or not used for the  */
                                       /* correction : TRUE or FALSE          */

   int        centroidsNb;             /* Each subaperture gives two centroids*/
                                       /* must be equal to subapUsedNb * 2    */

   int        xPixels;                 /* Columns number in pixels of the     */
                                       /* image. Will be equal to xRaster *   */
                                       /* xSubapNb * 2                        */

   int        yPixels;                 /* Rows number in pixels of the image  */
                                       /* Will be equals to yRaster * ySubapNb*/
                                       /* * 2                                 */

   int        pixelsNb;                /* Total number of pixels of the image */
                                       /* Will be equal to xPixels * yPixels  */
                                       /* T_NPIXEL                            */

   int        uscanNb;                 /* Number of underscan pixels, (default*/
                                       /* is 8), T_USCAN                      */

   int        xTail;                   /* Remaining pixels to discard by row  */
                                       /* T_XTAIL                             */

   int        packetSize;              /* Packet Size in pixels, V_PSIZE      */

   int        packetNb;                /* Packet number, will be equal to:    */
                                       /* pixelsNb / VPSIZE                   */

   int        binningFlag;             /* TRUE or FALSE, if binning or not    */

   int        unused;                  /* The structure size must be equal to */
                                       /* a number multiple of a double       */

} AO_CCD_ID_STRUCT, * AO_CCD_ID;

/******************************************* Definition of the ao structure ***/

typedef struct 
{
      /* This structure contains all the data needed to perform active optics */
      /* correction from the centroids to the zernikes modes computation      */ 

   int          initFlag;              /* TRUE or FALSE, if the current       */
                                       /* structure is init or not            */

   int          darkInitFlag;          /* TRUE or FALSE, if dark is init or   */
                                       /* not                                 */

   int          flatInitFlag;          /* TRUE or FALSE, if flat is init or   */
                                       /* not                                 */

   int          refInitFlag;           /* TRUE or FALSE, if a reference is    */
                                       /* init or not                         */

   char         darkFileName [ STRING_SIZE ]; 
                                       /* Name of the file which contains the */
                                       /* dark image                          */

   char         flatFileName [ STRING_SIZE ]; 
                                       /* Name of the file which contains the */
                                       /* flat fielding                       */

   char         refVectFileName [ STRING_SIZE ]; 
                                       /* Name of the reference vector file   */
                                       /* used for centroids computation      */
   
   IMAGE_VECT   darkVect;              /* Vector containing the dark image for*/
                                       /* the whole CCD                       */

   IMAGE_VECT   flatVect;              /* Vector containing the flat fielding */
                                       /* image for the whole CCD             */

   IMAGE_VECT   sumVect;               /* Vector containing a coadd image     */

   WFS_VECT     refVect;               /* Vector containing the center of each*/
                                       /* subapertures                        */

   COMMAND_VECT scaleFactorVect;       /* Vector containing the scale factor  */
                                       /* for each modes                      */

   int          thresholdMethod;       /* Method to compute threshold         */
                                       /* AO_THRESH_SPOTS, AO_THRESH_NOSPOTS, */
                                       /* AO_THRESH_VALUE                     */

   int          totalMethod;           /* Method to compute average flux      */
                                       /* AO_TOTAL_SPOTS, AO_TOTAL_VALUE      */

   double       threshold;             /* Threshold used for the centroids    */
                                       /* computation                         */ 
   double       thresholdRate;         /* Rate of brighter pixels used to     */
                                       /* compute the threshold               */

   double       thresholdMultCoeff;    /* Multiplicative coefficient for      */
                                       /* threshold computation               */

   double       thresholdDark;         /* Threshold computed during sequence  */
                                       /* dark - save                         */ 
   double       averageTotal;          /* Average of the total counts for the */
                                       /* whole CCD                           */

   double       totalThreshold;        /* Threshold for the total counts for  */
                                       /* the whole CCD                       */

   double       multCoeffTotal;        /* Multiplicative coefficient (0 to 1) */
                                       /* totalThreshold =                    */
                                       /* averageTotal * multCoeffTotal       */

   double       angleWithM2;           /* Angle between M2 and P2 coordinates */

   double       cosAngle;              /* Cos of the angleWithM2              */

   double       sinAngle;              /* Sinus of the angleWithM2            */

   double       slidingFocusGain;      /* Gain for sliding average for focus  */

   double       one_slidingFocusGain;  /* 1 - slidingFocusGain                */

   double       previousFocus;         /* Previous focus mode value           */

   int          coaddCounter;          /* Counter used for coadd images       */

   int          focusCounter;          /* Counter used for computing the focus*/

   int          modeNb;                /* Number of modes to correct          */

   int          allowedSubapOff;       /* Number of subapertures allowed to   */
                                       /* be off when computing the centroids */

} AO_CTRL_ID_STRUCT, * AO_CTRL_ID;

/*********************************** Definition of the WFS circular buffers ***/

typedef struct                         /* Definition of the image circular    */
{                                      /* buffer record                       */

   IMAGE_VECT   imageVect;             /* CCD image after dark subtraction    */

   int          imageStatus;           /* Status provided by the detector     */
                                       /* controller                          */
} CB_IM_RECORD_STRUCT;

typedef struct                         /* Image circular buffer               */
{
   CB_IM_RECORD_STRUCT  cbImRecord [ CB_IM_RECORD_NB ];

   double               exposureTime;  /* Exposure time in second             */
 
   int                  processingMode;/* Processing mode used: AO_MODE_NODE  */
                                       /* ... to AO_MODE_FG_FOCUS             */

   int                  position;      /* From 0 to CB_IM_RECORD_NB - 1       */

   int                  offset;        /* Offset of a record from the         */
                                       /* beginning of the circular buffer    */
 
   int                  counter;

} AO_CB_IM_ID_STRUCT , * AO_CB_IM_ID;

typedef struct                         /* Definition of the control circular  */
{                                      /* buffer record                       */

   WFS_VECT     totalCountsVect;       /* Vector which contains the total of  */
                                       /* counts for each subaperture (4      */
                                       /* values) + the total of counts for   */
                                       /* the whole CCD (1 value)             */

   WFS_VECT     centroidsVect;         /* Vector which contains the centroids */
                                       /* + the whole guiding value           */

   WFS_VECT     errorCentroidsVect;    /* Vector which contains the errors of */
                                       /* the centroids computation           */

   COMMAND_VECT zernikesVect;          /* Vector which contains the zernikes  */
                                       /* modes to send to the SCS            */

   COMMAND_VECT errorsVect;            /* Vector which contains the errors    */
                                       /* associated with the zernikes modes  */

   double       time;                  /* Time stamp of the record            */

   int          wfsStatus;             /* Status when computing centroids     */
                                       /* OK or AO_SUBAP_OFF or AO_SH_OFF     */

   int          unused;                /* The structure size must be equal to */
                                       /* a number multiple of a double       */

} CB_CTRL_RECORD_STRUCT;

typedef struct                         /* Control circular buffer             */
{

   CB_CTRL_RECORD_STRUCT cbCtrlRecord [ CB_CTRL_RECORD_NB ];

   double                exposureTime; /* Exposure time in second             */
 
   int                   processingMode;
                                       /* Processing mode used: AO_MODE_NODE  */
                                       /* ... to AO_MODE_FG_FOCUS             */

   int                   position;     /* From 0 to CB_CTRL_RECORD_NB - 1     */

   int                   offset;       /* Offset of a record from the         */
                                       /* beginning of the circular buffer    */
 
   int                   counter;      

} AO_CB_CTRL_ID_STRUCT , * AO_CB_CTRL_ID;

/************************* Header structures for circular buffer save files ***/

typedef struct
{
   char         cbImFileName [ STRING_SIZE ];
                                       /* Name of the image circular buffer   */
                                       /* save file                           */

   int          recordNb;              /* Saved record number                 */

   int          processingMode;        /* Processing mode                     */

   double       exposureTime;          /* Exposure time in second             */

   IMAGE_VECT   darkVect;              /* Vector containing the dark image for*/
                                       /* the whole CCD                       */

   IMAGE_VECT   flatVect;              /* Vector containing the flat fielding */
                                       /* image for the whole CCD             */

} AO_HEADER_CB_IM_ID_STRUCT, * AO_HEADER_CB_IM_ID;

typedef struct
{
   char         cbCtrlFileName [ STRING_SIZE ];
                                       /* Name of the control circular buffer */
                                       /* save file                           */

   int          recordNb;              /* Saved record number                 */

   int          processingMode;        /* Processing mode                     */

   int          centroidsNb;           /* Number of centroid and guiding      */
                                       /* values                              */
                                       
   int          modeNb;                /* Number of modes                     */
   
   double       exposureTime;          /* Exposure time in second             */

   WFS_VECT     refVect;               /* Vector containing the center of each*/
                                       /* subapertures                        */

   COMMAND_VECT scaleFactorVect;       /* Vector containing the scale factor  */
                                       /* for each modes                      */
                                       
   double       threshold;             /* Threshold used for the centroids    */
                                       /* computation                         */

   double       totalThreshold;        /* Threshold for total count used for  */
                                       /* the centroids computation           */

   double       angleWithM2;           /* Angle between M2 and P2 coordinates */

   double       slidingFocusGain;      /* Gain for sliding average for focus  */

} AO_HEADER_CB_CTRL_ID_STRUCT, * AO_HEADER_CB_CTRL_ID;

/**************************************************************** Functions ***/

#ifdef vxWorks
AO_CCD_ID aoCcdContextCreate (void);
AO_CTRL_ID aoCtrlContextCreate (void);
AO_CB_IM_ID aoCbImContextCreate (void);
AO_CB_CTRL_ID aoCbCtrlContextCreate (void);
STATUS aoCcdContextShow (AO_CCD_ID aoCcdId);
STATUS aoRefRead (char * pRefFileName, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoFitsImageFloatRead (char * pFitsFileName, float * pImageBuffer,
                              int xBufferSize, int yBufferSize);
STATUS aoFitsImageFloatWrite (char * pFitsFileName, float * pImageBuffer,
                               int xBufferSize, int yBufferSize);
STATUS aoCtrlContextInit (char * pInitFileName, AO_CCD_ID aoCcdId,
                          AO_CTRL_ID aoCtrlId);
STATUS aoCtrlContextUpdate (char * pDarkFileName, char * pFlatFileName,
                            char * pRefFileName, double xCenter, double yCenter,
                            double angle, AO_CCD_ID aoCcdId,
                            AO_CTRL_ID aoCtrlId);
STATUS aoDarkSubtract (float * pImage, float * pDark, int xPixels,
                       int yPixels);
STATUS aoGlobalGuide (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, 
                      double * pTotalCountsVect, double * pCentroidsVect, 
                      double * pZernikesVect, double * pErrorsVect, 
                      double * pTime, int * pWfsStatus);
STATUS aoGlobalGuideAndError (float * pImage, AO_CCD_ID aoCcdId, 
                              AO_CTRL_ID aoCtrlId, double * pTotalCountsVect, 
                              double * pCentroidsVect, double * pZernikesVect, 
                              double * pErrorsVect, double * pTime, 
                              int * pWfsStatus);
STATUS aoImageFloatAverage (float * pImage, AO_CCD_ID aoCcdId, 
                            AO_CTRL_ID aoCtrlId, int imageNb);
STATUS aoRmsNoiseImageCompute (float * pImage, AO_CCD_ID aoCcdId, 
                               double * pRmsNoise);
STATUS aoThresholdCompute (float * pImage, AO_CCD_ID aoCcdId, double ratePixel, 
                           double * pThreshold);
STATUS aoCtrlContextShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoGuideAndFocus (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, 
                        double *pTotalCountsVect, double * pCentroidsVect, 
                        double * pErrorCentroidsVect, double * pZernikesVect, 
                        double * pErrorsVect, double * pTime, int * pWfsStatus);
STATUS aoGuideAndFocusAndError (float * pImage, AO_CCD_ID aoCcdId, 
                                AO_CTRL_ID aoCtrlId, double *pTotalCountsVect, 
                                double * pCentroidsVect, 
                                double * pErrorCentroidsVect, 
                                double * pZernikesVect, double * pErrorsVect, 
                                double * pTime, int * pWfsStatus);
STATUS aoCbImSave (char *pCbImFilePath, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, 
                   AO_CB_IM_ID aoCbImId);
STATUS aoCbCtrlSave (char *pCbCtrlFilePath, AO_CCD_ID aoCcdId, 
                     AO_CTRL_ID aoCtrlId, AO_CB_CTRL_ID aoCbCtrlId);
STATUS aoCbImZero (AO_CB_IM_ID aoCbImId);
STATUS aoCbCtrlZero (AO_CB_CTRL_ID aoCbCtrlId);
STATUS aoDarkUpdate (char * pDarkFileName, AO_CCD_ID aoCcdId,
                     AO_CTRL_ID aoCtrlId);
STATUS aoCtrlFileRead (char * pInitFileName, char * pPath,
                       char * pDarkFileName,
                       char * pFlatFileName, double * pAngle, double * pRefX,
                       double * pRefY, char * pRefFileName, double * pThresh,
                       double * pTotalThresh);
#endif

#endif /* __INCaoP2Libh */
