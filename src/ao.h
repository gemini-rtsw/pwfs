/*
 * MODULE NAME: 
 * 
 * FILENAME: 
 * ao.h
 *
 * PURPOSE: 
 * Include file for adaptive optics library 
 * Contains all the types and constants definition of this library
 * 
 * HISTORY MODIFICATION:
 * 13 May 1999 - CB - Original creation
 *
 */

/***************************************************** Constants definition ***/

#define STRING_SIZE      160           /* Size of a string                    */

#define CCD_XSIZE        80            /* Default value for X size of the CCD */

#define CCD_YSIZE        80            /* Default value for Y size of the CCD */

#define CCD_SIZE         (CCD_XSIZE * CCD_YSIZE)
                                       /* Default value for CCD size          */

#define SUBAPERTURE_NB   4             /* Max number of subapertures          */

#define MODE_NB          3             /* Max number of active optics mode to */
                                       /* to be corrected                     */

#define WFS_CB_RECORD_NB 100           /* Number of WFS circular buffer       */
                                       /* records                             */


/***************************************** Definition of vectors and matrix ***/

typedef double IMAGE_VECT [ CCD_SIZE ] ;

typedef double WFS_VECT [ 2 * SUBAPERTURE_NB ] ;

typedef double COMMAND_VECT [ MODE_NB ] ;

typedef double MATRIX [ 2 * SUBAPERTURE_NB * MODE_NB ] ;

/*************************** Definition of the structure describing the WFS ***/

typedef struct 
{

   char       darkFileName [ STRING_SIZE ] ; 
                                       /* Name of the file which contains the */
                                       /* dark image                          */

   char       flatFileName [ STRING_SIZE ] ; 
                                       /* Name of the file which contains the */
                                       /* flat fielding                       */

   IMAGE_VECT darkVector ;             /* Vector containing the dark image for*/
                                       /* the whole CCD                       */

   IMAGE_VECT flatVector ;             /* Vector containing the flat fielding */
                                       /* image for the whole CCD             */

   int        xArraySize ;             /* Columns number in pixels of the CCD */

   int        yArraySize ;             /* Rows number in pixels of the CCD    */

   int        arraySize ;              /* Size in pixels of the whole CCD     */
                                       /* arraySize = xArraySize * yArraySize */

   int        sectorsNb ;              /* Number of sectors of the CCD        */
                                       /* By default 4                        */

   int        xStart ;                 /* Number of columns to be discarded   */
                                       /* before the first subaperture        */

   int        yStart ;                 /* Number of rows to be discarded      */
                                       /* before the first subaperture        */

   int        xBin ;                   /* Binning factor in X (by default 1)  */

   int        yBin ;                   /* Binning factor in Y (by default 1)  */
   
   int        xRaster ;                /* X size of a subaperture in binned   */
                                       /* pixels                              */

   int        yRaster ;                /* Y size of a subaperture in binned   */
                                       /* pixels                              */

   int        xSpace ;                 /* Columns number in pixels to be      */
                                       /* discarded between subapertures      */

   int        ySpace ;                 /* Rows number in pixels to be         */
                                       /* discarded between subapertures      */

   int        xSubapertureNb ;         /* Column number of subapertures       */

   int        ySubapertureNb ;         /* Row number of subapertures          */

   int        subapertureNb ;          /* Total number of subapertures, must  */
                                       /* be equal to xSubapertureNb *        */
                                       /* ySubapertureNb                      */

   int        centroidsNb ;            /* Each subaperture gives two centroids*/
                                       /* must be equal to subapertureNb * 2  */

   int        subapertureNotUsedNb ;   /* Number of subapertures not used     */

   int        unused ;                 /* The structure size must be equal to */
                                       /* a number multiple of a double       */

   int        subapertureUsedVector [ SUBAPERTURE_NB ] ;
                                       /* Vector describing the subapertures  */
                                       /* which are used or not used for the  */
                                       /* correction : TRUE or FALSE          */

} CCD_STRUCT, * CCD_PSTRUCT ;


/************************************ Definition of the WFS circular buffer ***/

typedef struct
{

   IMAGE_VECT imageVector ;            /* CCD image after dark subtraction    */

   WFS_VECT   centroidsVector ;        /* Vector which contains the centroids */

   double guidingVector [ 2 ] ;        /* Vector which contains the guiding   */
                                       /* data                                */

   int    imageStatus ;                /* Status provided by the detector     */
                                       /* controller                          */

   int    wfsStatus ;                  /* Status when computing centroids     */

} WFS_CB_RECORD_STRUCT ;

typedef struct
{

   WFS_CB_RECORD_STRUCT wfsCbRecord [ WFS_CB_RECORD_NB ] ;

   double               exposureTime ; /* Exposure time in second             */

   int                  position ;     /* From 0 to WFS_CB_RECORD_NB -1       */

   int                  offset ;       /* Offset of a record from the         */
                                       /* beginning of the circular buffer    */
 
   int                  counter ;      

   int                  unused ;

} WFS_CB_STRUCT ;

/******************************************* Definition of the ao structure ***/

typedef struct 
{
      /* This structure contains all the data needed to perform active optics */
      /* correction from the centroids to the zernikes modes computation      */ 
   char   refVectFileName [ STRING_SIZE ] ; 
                                       /* Name of the reference vector file   */
                                       /* used for centroids computation      */
   
   char   intMatFileName [ STRING_SIZE ] ; 
                                       /* Name of the file which contains the */
                                       /* interaction matrix                  */

   char   contMatFileName [ STRING_SIZE ] ; 
                                       /* Name of the file which contains the */
                                       /* control matrix                      */

   double referenceVector [ 2 * SUBAPERTURE_NB ] ;
   double inter [ 2 * SUBAPERTURE_NB ] ;
