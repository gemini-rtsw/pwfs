#ifndef __INCaoP1Libh
#define __INCaoP1Libh

/*
 * MODULE NAME: 
 * aoP1Lib
 * 
 * FILENAME: 
 * aoP1Lib.h
 *
 * PURPOSE: 
 * Include file for PWFS1 active optics library 
 * Contains all the types and constants definition of this library
 * Note: aO means active optics, FG means fast guide 
 * 
 * AUTHORS:
 * Corinne Boyer
 *
 * HISTORY MODIFICATION:
 * 18 May 2004:AA - Read default TTF gains from file
 * 04 Feb 2003: CB - Add aoThreshold to aoCtrlId
 * 08 Feb 2002: CB - Implement threshold per sub-aperture and in real time
 * 21 Dec 2001: CB - add automatic init of zero point models from par file
 * 06 June 2001: CB - add FOCUS_ZP_MODEL_ID structure 
 * 29 May 2001: CB - add COMA_ZP_MODEL_ID structure 
 * 23 May 2001: CB - replace ZP_MODEL_ID by AST_ZP_MODEL_ID structure
 *                   add TREF_ZP_MODEL_ID structure as well
 * 12 April 2001: CB - Add structure ZP_MODEL_ID_STRUCT
 * 02 Feb 2001: CB - Add aoVectAfterRot and fgVectAfterRot vectors in the 
 *                   circular buffers AO_CB_CTRL_ID and AO_CB_FG_CTRL_ID
 * 31 October 2000: CB - Replace aoRmsNoiseDarkCompute aoRmsNoiseImageCompute
 * 13 May 1999 - CB - Original creation
 *
 */

/***************************************************** Constants definition ***/

#define STRING_SIZE          160       /* Size of a string                    */

#define CCD_XSIZE            80        /* Default value for X size of the CCD */

#define CCD_YSIZE            80        /* Default value for Y size of the CCD */

#define CCD_SIZE             (CCD_XSIZE * CCD_YSIZE)
                                       /* Default value for CCD size          */

#define SUBAP_NB             36        /* Max number of subapertures          */

#define FG_MODE_NB           3         /* Max number of FG modes to correct   */

#define AO_MODE_NB           19        /* Max number of aO mode to correct    */

#define MODE_NB              (FG_MODE_NB + AO_MODE_NB)
                                       /* Total number of modes to correct    */

#define CB_IM_RECORD_NB      500       /* Number of records of the image      */
                                       /* circular buffer                     */

#define CB_AO_CTRL_RECORD_NB 800       /* Number of records of the aO control */
                                       /* circular buffer                     */

#define CB_FG_CTRL_RECORD_NB 8192      /* Number of records of the FG control */
                                       /* circular buffer                     */

#define AO_SUBAP_OFF         32767     /* Indicates there is no light on at   */
                                       /* least one subaperture               */

#define AO_SH_OFF            65536     /* Indicates there is no light on at   */
                                       /* least two subapertures              */

#define AO_MIN_DOUBLE        1.0e-10   /* Mininum double used when comparing  */
                                       /* total counts to threshold           */

#define AO_TIME_NOW_ERROR    -5.55e9   /* If time Now returns an error, time  */
                                       /* is set to this value                */

#define ZP_MODEL_SEM_TIMEOUT 100       /* Timeout for zero point model        */
                                       /* semaphore                           */

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
   AO_MODE_FG_FOCUS,       /* Fast Guide and Focus mode.             */
   AO_MODE_FG_FOCUS_COADD, /* Fast Guide and Focus and coadd mode.   */
   AO_MODE_MEAS_IM,        /* Interaction matrix measurement mode.   */
   AO_MODE_AO,             /* Active optics correction mode only     */
   AO_MODE_GG_AO,          /* Global Guide and aO correction mode    */
   AO_MODE_FG_FOCUS_AO,    /* FG and Focus and aO correction mode    */
   AO_MODE_SEQ_DARK,       /* Sequence dark starting averaging       */
                           /* images and then computing the threshold*/
   AO_MODE_CLOSED_LOOP,    /* Sequence of closed loop starting       */
                           /* with GG and next with FG and Focus and */
                           /* aO correction                          */
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
   AO_TOTAL_SPOTS = 0,  /* Computation of average flux when spots */
   AO_TOTAL_VALUE,      /* No computation, used given value       */
   AO_TOTAL_FORMULA     /* No image, use a function(rms,N)        */
};

enum
{
   AO_INT_MAT_TYPE = 0, /* Control matrix type                    */
   AO_CONT_MAT_TYPE     /* No computation, used given value       */
};

/***************************************** Definition of vectors and matrix ***/

typedef float  IMAGE_VECT [ CCD_SIZE ];

typedef double WFS_VECT [ (2 * SUBAP_NB) ];  
                                       /* 2 informations per subaperture +    */

typedef double GUIDE_VECT [ 2 ];       /* only 2 information for the whole CCD*/

typedef double FG_VECT [ FG_MODE_NB ];

typedef double AO_VECT [ AO_MODE_NB ];

typedef double AO_MATRIX [ 2 * SUBAP_NB * AO_MODE_NB ];

typedef double FG_MATRIX [ 2 * SUBAP_NB * FG_MODE_NB ];

typedef struct                         /* Structure needed to measure a column*/
                                       /* of the interaction matrixes         */
{
   double     posAmplitude;            /* Positive amplitude of the measured  */
                                       /* mode                                */

   double     negAmplitude;            /* Negative amplitude of the measured  */
                                       /* mode                                */

   WFS_VECT   posCentroidsVect;        /* Centroids vector corresponding to   */
                                       /* a positive amplitude                */

   WFS_VECT   negCentroidsVect;        /* Centroids vector corresponding to   */
                                       /* a negative amplitude                */

} CIM_STRUCT;

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

   int          aoScaleInitFlag;       /* TRUE or FALSE, if a aO scale factor */
                                       /* vector is init or not               */

   int          aoIntMatInitFlag;      /* TRUE or FALSE, if an aO interaction */
                                       /* matrix is init or not               */

   int          aoContMatInitFlag;     /* TRUE or FALSE, if aO control matrix */
                                       /* is init or not                      */

   int          fgContMatInitFlag;     /* TRUE or FALSE, if FG control matrix */
                                       /* is init or not                      */

   char         darkFileName [ STRING_SIZE ]; 
                                       /* Name of the file which contains the */
                                       /* dark image                          */

   char         flatFileName [ STRING_SIZE ]; 
                                       /* Name of the file which contains the */
                                       /* flat fielding                       */

   char         refVectFileName [ STRING_SIZE ]; 
                                       /* Name of the reference vector file   */
                                       /* used for centroids computation      */
   
   char         aoScaleFileName [ STRING_SIZE ]; 
                                       /* Name of the ao scale factor vector  */
                                       /* file                                */
   
   char         aoIntMatFileName [ STRING_SIZE ]; 
                                       /* Name of the aO interaction matrix   */
                                       /* file                                */

   char         aoContMatFileName [ STRING_SIZE ]; 
                                       /* Name of the aO control matrix file  */

   char         fgContMatFileName [ STRING_SIZE ]; 
                                       /* Name of the FG control matrix file  */

   IMAGE_VECT   darkVect;              /* Vector containing the dark image for*/
                                       /* the whole CCD                       */

   IMAGE_VECT   flatVect;              /* Vector containing the flat fielding */
                                       /* image for the whole CCD             */

   IMAGE_VECT   sumVect;               /* Vector containing a coadd image     */

   WFS_VECT     refWfsVect;            /* Vector containing the center of each*/
                                       /* subapertures                        */

   GUIDE_VECT   refGuideVect;          /* Vector containing the center of the */
                                       /* whole CCD                           */

   AO_VECT      aoScaleFactorVect;     /* Vector containing the scale factor  */
                                       /* for each ao modes                   */

   FG_VECT      fgScaleFactorVect;     /* Vector containing the scale factor  */
                                       /* for each ao modes                   */

   CIM_STRUCT   aoIntMatStruct[AO_MODE_NB]; 
                                       /* Structure needed to compute the     */
                                       /* aO interaction matrix               */
 
   AO_MATRIX    aoIntMat;              /* aO interaction matrix               */

   AO_MATRIX    aoContMat;             /* ao control matrix                   */

   FG_MATRIX    fgContMat;             /* FG control matrix                   */

   int          thresholdMethod;       /* Method to compute threshold         */
                                       /* AO_THRESH_SPOTS, AO_THRESH_NOSPOTS, */
                                       /* AO_THRESH_VALUE                     */

   int          totalMethod;           /* Method to compute average flux      */
                                       /* AO_TOTAL_SPOTS, AO_TOTAL_VALUE      */

   double       rms;                   /* RMS used for the threshold          */
                                       /* computation                         */

   double       threshold;             /* Threshold used for the centroids    */
                                       /* computation                         */ 
   double       thresholdRate;         /* Rate of brighter pixels used to     */
                                       /* compute the threshold               */

   double       thresholdMultCoeff;    /* Multiplicative coefficient for      */
                                       /* threshold computation               */

   double       thresholdDarkFull;     /* Threshold computed during sequence  */
                                       /* dark when no binning - save         */

   double       thresholdDarkBin;      /* Threshold computed during sequence  */
                                       /* dark when binning - save            */

   double       rmsDarkFull;           /* RMS computed during sequence dark   */
                                       /* when no binning - save              */

   double       rmsDarkBin;            /* RMS computed during sequence dark   */
                                       /* when binning - save                 */

   WFS_VECT     thresholdVect;         /* Threshold computed for each         */
                                       /* subaperture                         */

   WFS_VECT     averageThreshVect;     /* Avreage threshold computed for each */
                                       /* subaperture used when aO            */

   double       averageTotal;          /* Average of the total counts for the */
                                       /* whole CCD                           */

   double       totalThreshold;        /* Threshold for the total counts for  */
                                       /* the whole CCD                       */

   double       multCoeffTotal;        /* Multiplicative coefficient (0 to 1) */
                                       /* totalThreshold =                    */
                                       /* averageTotal * multCoeffTotal       */

   double       angleWithM2;           /* Angle between M2 and P1 coordinates */

   double       cosAngleWithM2;        /* Cos of the angleWithM2              */

   double       sinAngleWithM2;        /* Sinus of the angleWithM2            */

   double       angleWithM1;           /* Angle between M1 and P1 coordinates */

   double       cosAngleWithM1;        /* Cos of the angleWithM1              */

   double       sinAngleWithM1;        /* Sinus of the angleWithM1            */

   double       slidingFocusGain;      /* Gain for sliding average for focus  */

   double       one_slidingFocusGain;  /* 1 - slidingFocusGain                */

   double       previousFocus;         /* Previous focus mode value           */

   double       aoThreshold;           /* aO threshold above which the aO     */
                                       /* gains are increased                 */

   double       aoMaxThreshold;        /* aO threshold above which the aO     */
                                       /* values are clamped                  */

   int          allowedSubapOff;       /* Number of subapertures allowed to   */
                                       /* be off when computing the centroids */

   int          focusCounter;          /* Counter used for computing the focus*/


   int          coaddCounter;          /* Counter used for coadd images       */

   int          aoModeNb;              /* Number of aO modes to correct       */

   int          aoModeNotUsedNb;       /* Number of aO modes not used         */

   int          aoModeUsedNb;          /* Total number of used modes, must be */
                                       /* equal to aoModeNb - aoModeNotUsedNb */

   int          aoModeUsedVect [ AO_MODE_NB ];
                                       /* Vector describing the aO modes      */
                                       /* which are used or not used for the  */
                                       /* correction : TRUE or FALSE          */

   int          fgModeNb;              /* Number of FG modes to correct       */

} AO_CTRL_ID_STRUCT, * AO_CTRL_ID;

/********************************** Definition of the image circular buffer ***/

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

/***************************** Definition of the aO control circular buffer ***/

typedef struct                         /* Definition of the aO control        */
{                                      /* circular buffer record              */

   WFS_VECT     totalCountsVect;       /* Vector which contains the total of  */
                                       /* counts for each subaperture         */
                                       /* (subapUsedNb values) + the total of */
                                       /* counts for the whole CCD (1 value)  */

   WFS_VECT     thresholdVect;         /* Threshold computed in real time for */
                                       /* each subaperture                    */

   WFS_VECT     centroidsVect;         /* Vector which contains the centroids */

   WFS_VECT     errorCentroidsVect;    /* Vector which contains the errors of */
                                       /* the centroids computation           */

   AO_VECT      aoVect;                /* Vector which contains the zernikes  */
                                       /* modes to send to M1 after a&g and   */
                                       /* cass rotator rotation               */

   AO_VECT      aoVectAfterRot;        /* Vector which contains the zernikes  */
                                       /* modes to send to M1 after a&g and   */
                                       /* cass rotator rotation               */

   AO_VECT      aoErrorsVect;          /* Vector which contains the errors    */
                                       /* associated with the zernikes modes  */

   double       time;                  /* Time stamp of the record            */

   int          wfsStatus;             /* Status when computing centroids     */
                                       /* OK or AO_SUBAP_OFF or AO_SH_OFF     */

   int          unused;                /* The structure size must be equal to */
                                       /* a number multiple of a double       */

} CB_AO_CTRL_RECORD_STRUCT;

typedef struct                         /* aO control circular buffer          */
{

   CB_AO_CTRL_RECORD_STRUCT cbAoCtrlRecord [ CB_AO_CTRL_RECORD_NB ];

   double                   exposureTime; 
                                       /* Exposure time in second             */
 
   int                      processingMode;
                                       /* Processing mode used: AO_MODE_NODE  */
                                       /* ... to AO_MODE_FG_AO                */

   int                      averageImageNb;
                                       /* Number of images averaging before   */
                                       /* computing the aO modes              */

   int                      position;  /* From 0 to CB_AO_CTRL_RECORD_NB - 1  */

   int                      offset;    /* Offset of a record from the         */
                                       /* beginning of the circular buffer    */
 
   int                      counter;      

   int                      unused;    /* The structure size must be equal to */
                                       /* a number multiple of a double       */

} AO_CB_AO_CTRL_ID_STRUCT , * AO_CB_AO_CTRL_ID;

/***************************** Definition of the FG control circular buffer ***/

typedef struct                         /* Definition of the FG control        */
{                                      /* circular buffer record              */

   GUIDE_VECT   guidesVect;            /* Vector which contains the guides    */
                                       /* values                              */

   GUIDE_VECT   errorGuidesVect;       /* Vector which contains the errors of */
                                       /* the guide values computation        */

   WFS_VECT     totalCountsVect;       /* Vector which contains the total of  */
                                       /* counts for each subaperture         */
                                       /* (subapUsedNb values) + 1 value      */
                                       /* equivalent to the sum of the        */
                                       /* subapUsedNb values or to the total  */
                                       /* of the whole CCD                    */

   WFS_VECT     centroidsVect;         /* Vector which contains the centroids */

   WFS_VECT     errorCentroidsVect;    /* Vector which contains the errors of */
                                       /* the centroids computation           */

   WFS_VECT     thresholdVect;         /* Threshold computed in real time for */
                                       /* each subaperture                    */

   FG_VECT      fgVect;                /* Vector which contains the zernikes  */
                                       /* modes to send to M2 before a&G and  */
                                       /* cass rotator rotation               */

   FG_VECT      fgVectAfterRot;        /* Vector which contains the zernikes  */
                                       /* modes to send to M2 after a&g and   */
                                       /* cass rotator rotation               */

   FG_VECT      fgErrorsVect;          /* Vector which contains the errors    */
                                       /* associated with the zernikes modes  */

   double       time;                  /* Time stamp of the record            */

   int          wfsStatus;             /* Status when computing guide values  */
                                       /* OK or AO_SH_OFF                     */

   int          unused;                /* The structure size must be equal to */
                                       /* a number multiple of a double       */

} CB_FG_CTRL_RECORD_STRUCT;

typedef struct                         /* FG control circular buffer          */
{

   CB_FG_CTRL_RECORD_STRUCT cbFgCtrlRecord [ CB_FG_CTRL_RECORD_NB ];

   double                   exposureTime; 
                                       /* Exposure time in second             */

   int                      processingMode;
                                       /* Processing mode used: AO_MODE_NODE  */
                                       /* ... to AO_MODE_FG_AO                */

   int                      position;  /* From 0 to CB_FG_CTRL_RECORD_NB - 1  */

   int                      offset;    /* Offset of a record from the         */
                                       /* beginning of the circular buffer    */

   int                      counter;

} AO_CB_FG_CTRL_ID_STRUCT , * AO_CB_FG_CTRL_ID;

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
   char         cbAoCtrlFileName [ STRING_SIZE ];
                                       /* Name of the aO control circular     */
                                       /* buffer save file                    */

   int          recordNb;              /* Saved record number                 */

   int          processingMode;        /* Processing mode                     */

   int          centroidsNb;           /* Number of centroid values           */

   int          aoModeNb;              /* Number of aO modes                  */

   int          averageImageNb;        /* Number of images averaged before    */
                                       /* computing the centroids             */

   int          unused;                /* The size of the structure should be */
                                       /* a multiple of a double              */

   double       exposureTime;          /* Exposure time in second             */

   double       rms;                   /* RMS used for the threshold          */
                                       /* computation                         */

   double       threshold;             /* Threshold used for the centroids    */
                                       /* computation                         */

   double       totalThreshold;        /* Threshold for total count used for  */
                                       /* the centroids computation           */

   double       angleWithM1;           /* Angle between M1 and P1 coordinates */

   double       flipXWithM1;           /* Flip in X of PWFS2 tip measurements */
                                       /* according to M1 referential         */
                                       /* Not used always = 0                 */

   double       flipYWithM1;           /* Flip in Y of PWFS2 Tilt measurements*/
                                       /* according to M1 referential         */
                                       /* Not used always = 0                 */

   WFS_VECT     thresholdVect;         /* Threshold computed for each         */
                                       /* subaperture                         */

   WFS_VECT     refWfsVect;            /* Vector containing the center of each*/
                                       /* subapertures                        */

   AO_VECT      aoScaleFactorVect;     /* Vector containing the scale factor  */
                                       /* for each modes                      */

} AO_HEADER_CB_AO_CTRL_ID_STRUCT, * AO_HEADER_CB_AO_CTRL_ID;

typedef struct
{
   char         cbFgCtrlFileName [ STRING_SIZE ];
                                       /* Name of the FG control circular     */
                                       /* buffer save file                    */

   int          recordNb;              /* Saved record number                 */

   int          processingMode;        /* Processing mode                     */

   int          guidesNb;              /* Number of guiding values            */

   int          centroidsNb;           /* Number of centroid values           */

   int          fgModeNb;              /* Number of FG modes                  */

   int          unused;                /* The size of the structure should be */
                                       /* a multiple of a double              */

   double       exposureTime;          /* Exposure time in second             */

   double       rms;                   /* RMS used for the threshold          */
                                       /* computation                         */

   double       threshold;             /* Threshold used for the centroids    */
                                       /* computation                         */

   double       totalThreshold;        /* Threshold for total count used for  */
                                       /* the centroids computation           */

   double       angleWithM2;           /* Angle between M1 and P1 coordinates */

   double       flipXWithM2;           /* Flip in X of PWFS2 tip measurements */
                                       /* according to M2 referential         */
                                       /* Not used always = 0                 */

   double       flipYWithM2;           /* Flip in Y of PWFS2 Tilt measurements*/
                                       /* according to M2 referential         */
                                       /* Not used always = 0                 */

   double       slidingFocusGain;      /* Gain for sliding average for focus  */

   WFS_VECT     thresholdVect;         /* Threshold computed for each         */
                                       /* subaperture                         */

   WFS_VECT     refWfsVect;            /* Vector containing the center of each*/

   GUIDE_VECT   refGuideVect;          /* Vector containing the center of each*/
                                       /* subapertures                        */

   FG_VECT      fgScaleFactorVect;     /* Vector containing the scale factor  */
                                       /* for each modes                      */

} AO_HEADER_CB_FG_CTRL_ID_STRUCT, * AO_HEADER_CB_FG_CTRL_ID;

/****************** Structure for zero point model for astigmatism off axis ***/

typedef struct
{

   double   a1;                        /* Scale factor of cos (theta)         */
   double   a2;                        /* Scale factor of cos (2*theta)       */
   double   a3;                        /* Scale factor of cos (4*theta)       */
   double   p1;                        /* Phase of cos (theta)                */
   double   p2;                        /* Phase of cos (2*theta)              */
   double   p3;                        /* Phase of cos (4*theta)              */
   double   c;                         /* Constant term for astig0            */
   double   b1;                        /* Scale factor of sin (theta)         */
   double   b2;                        /* Scale factor of sin (2*theta)       */
   double   b3;                        /* Scale factor of sin (4*theta)       */
   double   pp1;                       /* Phase of sin (theta)                */
   double   pp2;                       /* Phase of sin (2*theta)              */
   double   pp3;                       /* Phase of sin (4*theta)              */
   double   d;                         /* Constant term for astig45           */
   double   astig0;                    /* Zero point model for astig 0        */
   double   astig45;                   /* Zero point model for astig45        */
   int      applyModel;                /* Apply the astigmatism zero point    */
                                       /* model TRUE|FALSE                    */
   double   gain0;                     
   double   gain45;                   
   double   offsetAstig0;            
   double   offsetAstig45;          

} AST_ZP_MODEL_ID_STRUCT, *AST_ZP_MODEL_ID;

/********************** Structure for zero point model for trefoil off axis ***/

typedef struct
{

   double   a;                         /* Scale factor of cos (3*theta)       */
   double   p;                         /* Phase of cos (3*theta)              */
   double   c;                         /* Constant term for cos trefoil       */
   double   b;                         /* Scale factor of sin (3*theta)       */
   double   pp;                        /* Phase of sin (3*theta)              */
   double   d;                         /* Constant term for sin trefoil       */
   double   costref;                   /* Zero point model for cos trefoil    */
   double   sintref;                   /* Zero point model for sin trefoil    */
   int      applyModel;                /* Apply the trefoil zero point model  */
                                       /* TRUE|FALSE                          */

} TREF_ZP_MODEL_ID_STRUCT, *TREF_ZP_MODEL_ID;

/************************* Structure for zero point model for coma off axis ***/

typedef struct
{

   double   a;                         /* Scale factor of cos (theta)         */
   double   p;                         /* Phase of cos (theta)                */
   double   c;                         /* Constant term for comaX             */
   double   b;                         /* Scale factor of sin (theta)         */
   double   pp;                        /* Phase of sin (theta)                */
   double   d;                         /* Constant term for comaY             */
   double   comaX;                     /* Zero point model for comaX          */
   double   comaY;                     /* Zero point model for comaY          */
   int      applyModel;                /* Apply the coma zero point model     */
                                       /* TRUE|FALSE                          */

} COMA_ZP_MODEL_ID_STRUCT, *COMA_ZP_MODEL_ID;

/************************ Structure for zero point model for focus off axis ***/

typedef struct
{

   double   a1;                        /* Scale factor of cos (theta)         */
   double   a2;                        /* Scale factor of cos (2*theta)       */
   double   p1;                        /* Phase of cos (theta)                */
   double   p2;                        /* Phase of cos (2*theta)              */
   double   c;                         /* Constant term for focus             */
   double   focus;                     /* Zero point model for comaX          */
   int      applyModel;                /* Apply the focus zero point model    */
                                       /* TRUE|FALSE                          */

} FOCUS_ZP_MODEL_ID_STRUCT, *FOCUS_ZP_MODEL_ID;

/**************************************************************** Functions ***/

#ifdef vxWorks
AO_CCD_ID aoCcdContextCreate (void);
AO_CTRL_ID aoCtrlContextCreate (void);
AO_CB_IM_ID aoCbImContextCreate (void);
AO_CB_AO_CTRL_ID aoCbAoCtrlContextCreate (void);
AO_CB_FG_CTRL_ID aoCbFgCtrlContextCreate (void);
STATUS aoCcdContextShow (AO_CCD_ID aoCcdId);
STATUS aoRefRead (char * pRefFileName, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoScaleRead (char * pAoScaleFileName, AO_CTRL_ID aoCtrlId);
STATUS aoScaleUpdate (double * pAoScaleVect, AO_CTRL_ID aoCtrlId);
STATUS aoFitsImageFloatRead (char * pFitsFileName, float * pImageBuffer, 
                             int xBufferSize, int yBufferSize);
STATUS aoFitsImageFloatWrite (char * pFitsFileName, float * pImageBuffer, 
                              int xBufferSize, int yBufferSize);
STATUS aoMatRead (char * pMatFileName, int typeExpected, AO_CCD_ID aoCcdId, 
                  AO_CTRL_ID aoCtrlId);
STATUS aoMatWrite (char * pMatFileName, double * pMat, int rowNb, int colNb, 
                   int type);
STATUS aoFgContMatRead (char * pFgContMatFileName, AO_CCD_ID aoCcdId, 
                        AO_CTRL_ID aoCtrlId);
STATUS aoCtrlContextInit (char * pInitFileName, AO_CCD_ID aoCcdId, 
                          AO_CTRL_ID aoCtrlId);
STATUS aoCtrlContextUpdate (char * pDarkFileName, char * pFlatFileName, 
                            char * pRefFileName, char * pAoIntMatFileName, 
                            char * pAoContMatFileName, 
                            char * pFgContMatFileName, double xCenter, 
                            double yCenter, double angleWithM2,
                            double angleWithM1, 
                            AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoCtrlContextShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, int verbose);
STATUS aoDarkSubtract (float * pImage, float * pDark, int xPixels, int yPixels);
STATUS aoGlobalGuide (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, 
                      double * pTotalCountsVect, double * pGuidesVect, 
                      double * pFgVect, double * pFgVectAfterRot, 
                      double * pFgErrorsVect, double * pTime, int * pWfsStatus,
                      int writeToRm);
STATUS aoGlobalGuideAndError (float * pImage, AO_CCD_ID aoCcdId, 
                              AO_CTRL_ID aoCtrlId, double * pTotalCountsVect, 
                              double * pGuidesVect, double * pFgVect, 
                              double * pFgVectAfterRot, double * pFgErrorsVect, 
                              double * pTime, int * pWfsStatus, int writeToRm);
STATUS aoImageFloatAverage (float * pImage, AO_CCD_ID aoCcdId, 
                            AO_CTRL_ID aoCtrlId, int imageNb);
STATUS aoRmsNoiseImageCompute (float * pImage, AO_CCD_ID aoCcdId, 
                               double * pRmsNoise, double * pMeanNoise);
STATUS aoThresholdCompute (float * pImage, AO_CCD_ID aoCcdId, 
                           AO_CTRL_ID aoCtrlId, double ratePixel, 
                           double * pThreshold);
STATUS aoCentroidsCompute (float * pImage, AO_CCD_ID aoCcdId, 
                           AO_CTRL_ID aoCtrlId, double * pThreshVect,
                           double * pTotalCountsVect, double * pCentroidsVect, 
                           double * pErrorCentroidsVect, int * pWfsStatus);
STATUS aoModeCompute (float * pImage, int imageStatus, AO_CCD_ID aoCcdId, 
                      AO_CTRL_ID aoCtrlId, int imageNb, int pauseNb,
                      double * pThreshVect, AO_CB_AO_CTRL_ID aoCbAoCtrlId);
STATUS aoCbImSave (char * pCbImFilePath, AO_CCD_ID aoCcdId, 
                   AO_CTRL_ID aoCtrlId, AO_CB_IM_ID aoCbImId);
STATUS aoCbImZero (AO_CB_IM_ID aoCbImId);
STATUS aoCbAoCtrlZero (AO_CB_AO_CTRL_ID aoCbAoCtrlId);
STATUS aoCbFgCtrlZero (AO_CB_FG_CTRL_ID aoCbFgCtrlId);
STATUS aoCbAoCtrlSave (char * pCbAoCtrlFilePath, AO_CCD_ID aoCcdId, 
                     AO_CTRL_ID aoCtrlId, AO_CB_AO_CTRL_ID aoCbAoCtrlId );
STATUS aoCbFgCtrlSave (char * pCbFgCtrlSave, AO_CCD_ID aoCcdId, 
                       AO_CTRL_ID aoCtrlId, AO_CB_FG_CTRL_ID aoCbFgCtrlId );
STATUS aoGuideAndFocus (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                        double *pThreshVect, double *pTotalCountsVect, 
                        double *pCentroidsVect, double *pErrorCentroidsVect, 
                        double *pFgVect, double *pFgVectAfterRot, 
                        double *pFgErrorsVect, double *pTime, 
                        int *pWfsStatus, int writeToRm);
STATUS aoModeAnalyze (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, 
                      double * pThreshVect, AO_CB_AO_CTRL_ID aoCbAoCtrlId);
STATUS aoCentroidsWrite (char * pCentroidsFileName, double * pCentroids, 
                         int centNb, char * pComment);
STATUS aoIntMatStructZero (AO_CTRL_ID aoCtrlId);
STATUS aoIntMatStructShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoMatZero (AO_CTRL_ID aoCtrlId);
STATUS aoMatCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoDarkUpdate (char * pDarkFileName, AO_CCD_ID aoCcdId, 
                     AO_CTRL_ID aoCtrlId);
STATUS aoCtrlFileRead (char * pInitFileName, char * pPath, char * pDarkFileName,
                       char * pFlatFileName, char * pRefFileName, 
                       double * pRefX, double * pRefY, char * pAoImFileName,
                       char * pAoCmFileName, char * pFgCmFileName,
                       double * pRms, double * pThresh, double * pTotalThresh,
                       double * pAngleM2, double * pAngleM1,
                       double *pFgGain, double * pSlidingFocusGain);
STATUS aoModInit (char * pInitFileName, AST_ZP_MODEL_ID astModelId,
                  TREF_ZP_MODEL_ID trefModelId, COMA_ZP_MODEL_ID comaModelId,
                  FOCUS_ZP_MODEL_ID focModelId);
STATUS aoModAstFileRead (char * pInitFileName, double * pA1, double * pA2,
                         double * pA3, double * pP1, double * pP2,
                         double * pP3, double * pC, double * pB1,
                         double * pB2, double * pB3, double * pPp1,
                         double * pPp2, double * pPp3, double * pD,
                         double * pGain0, double * pGain45,
                         double * pOffset0, double * pOffset45, int * pApply);
STATUS aoModTrefFileRead (char * pInitFileName, double * pA, double * pP,
                          double * pC, double * pB, double * pPp, double * pD,
                          int * pApply);
STATUS aoModComaFileRead (char * pInitFileName, double * pA, double * pP,
                          double * pC, double * pB, double * pPp,
                          double * pD, int * pApply);
STATUS aoModFocFileRead (char * pInitFileName, double * pA1, double * pP1,
                         double * pA2, double * pP2, double * pC,
                         int * pApply);
STATUS aoThresholdPerSubapCompute (float * pImage, AO_CCD_ID aoCcdId,
                                   AO_CTRL_ID aoCtrlId, double ratePixel,
                                   double * pThreshold);
double aoTotalThresholdCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
#endif

#endif /* __INCaoP1Libh */
