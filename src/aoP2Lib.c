/*+
 *   MODULE NAME:
 *   aoP2Lib
 *
 *   FILENAME:
 *   aoP2Lib.c
 *
 *   PURPOSE:
 *   Active optics library dedicated to PWFS2 
 *
 *   DESCRIPTION:
 *   This file contains the active optics library dedicated to PWFS2. 
 *   PWFS2 is a Shack Hartmann sensor composed of a CCD 80x80 and 
 *   a matrix of 2x2 lenslet array
 *   Note: aO means active optics, FG means fast guide
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   AUTHORS:
 *   Corinne Boyer
 *
 *   FUNCTIONS:
 *   aoCcdContextCreate() - Create a AO CCD geometry context structure
 *   aoCtrlContextCreate() - Create a AO control context structure
 *   aoCbImContextCreate() - Create a AO image circular buffer context structure
 *   aoCbAoCtrlContextCreate() - Create a AO control circular buffer context 
 *                               structure
 *   aoCbFgCtrlContextCreate() - Create a FG control circular buffer context 
 *                               structure
 *   aoCcdContextShow() - Display a AO CCD geometry context structure
 *   aoRefRead() - Read the SH reference file 
 *   aoScaleRead() - Read the aO scale factor file
 *   aoScaleUpdate() - Update the aO scale factor from a vector
 *   aoFitsImageFloatRead() - Read a float image from a FITS file 
 *   aoFitsImageFloatWrite() - Write a float image to a FITS file 
 *   aoMatRead() - Read an interaction or control matrix from a file
 *   aoMatWrite() - Write an interaction or a control matrix to a file
 *   aoFgContMatRead() - Read a FG control matrix from a file
 *   aoCtrlContextInit() - Init the AO control context structure
 *   aoCtrlContextUpdate() - Update the AO control context structure
 *   aoCtrlContextShow() - Display a AO Control context structure
 *   aoDarkSubtract() - Subtract a dark from an image 
 *   aoGlobalGuide() - Compute tip and tilt modes only over the whole CCD
 *   aoGlobalGuideAndError() - Compute tip and tilt modes only over the whole 
 *                             CCD and the associated errors
 *   aoImageFloatAverage() - Average float images
 *   aoRmsNoiseImageCompute() - To compute the rms of the noise
 *   aoThresholdCompute() - Compute the threshold 
 *   aoCentroidsCompute() - Compute the centroids of an image
 *   aoModeCompute() - Compute the aO modes
 *   aoCbImSave() - Save the image circular buffer
 *   aoCbImZero() - Set to zero the image circular buffer
 *   aoCbAoCtrlZero() - Set to zero the aO control circular buffer
 *   aoCbFgCtrlZero() - Set to zero the FG control circular buffer
 *   aoCbAoCtrlSave() - Save the aO control circular buffer
 *   aoCbFgCtrlSave() - Save the FG control circular buffer

 *   aoGuideAndFocus() - Compute tip, tilt and focus modes
 *   aoModeAnalyze() - Compute the centroids and modes for analyze
 *   aoCentroidsWrite() - Write centroids to a file
 *   aoIntMatStructZero() - Set to zero the aO interaction matrix structure
 *   aoIntMatStructShow() - Display the aO interaction matrix structure
 *   aoMatZero() - Set to zero the aO interaction and the aO control matrix
 *   aoMatCompute() - Compute the aO interaction and the aO control matrix
 *   aoDarkUpdate() - Update the dark buffer of the control context structure
 *   aoCtrlFileRead () - Read parameters from the AO control file
 *   aoModInit () - Init the zero point models
 *   aoModAstFileRead () - Read astigmatism zero point model from model file
 *   aoModTrefFileRead () - Read trefoil zero point model from model file
 *   aoModComaFileRead () - Read coma zero point model from model file
 *   aoModFocFileRead () - Read focus zero point model from model file
 *   aoThresholdPerSubapCompute() - Compute a threshold per subaperture
 * 
 *INDENT-OFF*
 *   13 Sep 2001: CB - Add aoThresholdPerSubapCompute()
 *   08 Aug 2001: CB - Major modifications to have ao correction with P2 also
 *   29 Mar 2001: CB - For guide and focus multiply focus per two when binning
 *                     for TT fix bug in rotation matrix
 *   08 Feb 2001: CB - Add zernikesVectAfterRot in circular buffer AO_CB_CTRL_ID
 *   10 Nov 2000: CB - Put back sliding average for focus computation in
 *                     aoGuideAndFocus and aoGuideAndFocusAndError
 *   31 Oct 2000: CB - Remove sliding average for focus computation in
 *                     aoGuideAndFocus and aoGuideAndFocusAndError
 *   25 Oct 2000: CB - Replace aoRmsNoiseDarkCompute aoRmsNoiseImageCompute
 *   13 Mar 2000: CB - original creation
 *INDENT-ON*
 *-
 */

/***************************************************************** Includes ***/

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "fitsio.h"
#include "timeLib.h"

#include "errorLib.h"
#include "matrixLib.h"
#include "aoP2Lib.h"

/********************************************************External functions ***/

extern STATUS writeWfsToSynchro ();           /* defined into writeZernikes.c */
extern STATUS writeWfsToTcs ();               /* defined into writeZernikes.c */

/****************************************************************** Defines ***/

/*#define DEBUG*/               /* Define this macro to enable debug messages */

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCcdContextCreate
 *
 *   INVOCATION:
 *   aoCcdContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CCD_ID) Pointer to AO CCD geometry context structure, or NULL if 
 *               unsuccessful.
 *
 *   PURPOSE:
 *   Create a AO CCD geometry context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a AO CCD geometry context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CCD_ID aoCcdContextCreate (void)
{
   AO_CCD_ID   aoCcdId;

   /* Allocate memory for the AO CCD geometry context structure, initialising 
    * its contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCcdContextCreate: Allocating %d bytes for AO_CCD_ID.\n",
            sizeof (AO_CCD_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCcdId = (AO_CCD_ID) calloc ( (size_t) 1, sizeof (AO_CCD_ID_STRUCT) )) 
       == NULL)
   {
      ERROR_SET ( 0, "Memory allocation for AO CCD geometry context failed",
                  ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCcdId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCtrlContextCreate
 *
 *   INVOCATION:
 *   aoCtrlContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CTRL_ID) Pointer to AO control context structure, or NULL if 
 *                unsuccessful.
 *
 *   PURPOSE:
 *   Create a AO control context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a AO control context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CTRL_ID aoCtrlContextCreate (void)
{
   AO_CTRL_ID   aoCtrlId;

   /* Allocate memory for the AO control context structure, initialising its
    * contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCtrlContextCreate: Allocating %d bytes for AO_CTRL_ID\n",
            sizeof (AO_CTRL_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCtrlId = (AO_CTRL_ID) calloc ((size_t) 1, sizeof (AO_CTRL_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET ( 0, "Memory allocation for AO control context failed",
                  ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCtrlId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbImContextCreate
 *
 *   INVOCATION:
 *   aoCbImContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CB_IM_ID) Pointer to AO image circular buffer context structure, or 
 *                 NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create a AO image circular buffer context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a AO image circular buffer context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CB_IM_ID aoCbImContextCreate (void)
{
   AO_CB_IM_ID   aoCbImId;

   /* Allocate memory for the AO image circular buffer context structure, 
    * initialising its contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCbImContextCreate: Allocating %d bytes for AO_CB_IM_ID\n",
            sizeof (AO_CB_IM_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCbImId = (AO_CB_IM_ID) calloc ((size_t) 1, 
                                         sizeof (AO_CB_IM_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET ( 0, 
            "Memory allocation for AO image circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbImId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbAoCtrlContextCreate
 *
 *   INVOCATION:
 *   aoCbAoCtrlContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CB_AO_CTRL_ID) Pointer to AO control circular buffer context structure,
 *                      or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create a AO control circular buffer context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a AO control circular buffer context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CB_AO_CTRL_ID aoCbAoCtrlContextCreate (void)
{
   AO_CB_AO_CTRL_ID   aoCbAoCtrlId;

   /* Allocate memory for the AO control circular buffer context structure, 
    * initialising its contents to zero.
    */

#ifdef DEBUG
   printf ( 
     "aoCbAoCtrlContextCreate: Allocating %d bytes for AO_CB_AO_CTRL_ID\n",
     sizeof (AO_CB_AO_CTRL_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCbAoCtrlId = (AO_CB_AO_CTRL_ID) calloc ((size_t) 1, 
                                          sizeof (AO_CB_AO_CTRL_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET ( 0, 
            "Memory allocation for AO control circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbAoCtrlId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbFgCtrlContextCreate
 *
 *   INVOCATION:
 *   aoCbFgCtrlContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CB_FG_CTRL_ID) Pointer to FG control circular buffer context structure,
 *                      or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create a FG control circular buffer context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a FG control circular buffer context
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CB_FG_CTRL_ID aoCbFgCtrlContextCreate (void)
{
   AO_CB_FG_CTRL_ID   aoCbFgCtrlId;

   /* Allocate memory for the FG control circular buffer context structure,
    * initialising its contents to zero.
    */

#ifdef DEBUG
   printf (
      "aoCbFgCtrlContextCreate: Allocating %d bytes for AO_CB_FG_CTRL_ID\n",
      sizeof (AO_CB_FG_CTRL_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCbFgCtrlId = (AO_CB_FG_CTRL_ID) calloc ((size_t) 1,
                                          sizeof (AO_CB_FG_CTRL_ID_STRUCT)))
       == NULL)
   {
      ERROR_SET ( 0,
            "Memory allocation for FG control circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbFgCtrlId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCcdContextShow
 *
 *   INVOCATION:
 *   aoCcdContextShow (aoCcdId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) aoCcdId (AO_CCD_ID) Pointer to the AO CCD geometry context structure 
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the AO CCD geometry context structure
 *
 *   DESCRIPTION:
 *   This function displays the content of the AO CCD geometry context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCcdContextShow (
   AO_CCD_ID aoCcdId
   )
{
   int    i;

   /* Check the CCD geometry context structure is valid. */

   if ( aoCcdId == NULL )
   {
      ERROR_SET ( 0, "Invalid CCD geometry context", ERROR_LOG_SAVE );
      return (ERROR);
   }

   /* Display the contents of the CCD geometry context structure. */

   printf ( "CCD geometry context structure :\n" );
   printf ( "Outputs number : %d\n" , (int) aoCcdId->outputsNb );
   printf ( "xSize (per output) : %d\n" , aoCcdId->xSize );
   printf ( "ySize (per output) : %d\n" , aoCcdId->ySize );
   printf ( "xMax : %d\n" , aoCcdId->xMax );
   printf ( "yMax : %d\n" , aoCcdId->yMax );
   printf ( "xStart : %d\n" , aoCcdId->xStart );
   printf ( "yStart : %d\n" , aoCcdId->yStart );
   printf ( "xBin : %d\n" , aoCcdId->xBin );
   printf ( "yBin : %d\n" , aoCcdId->yBin );
   printf ( "xRaster : %d\n" , aoCcdId->xRaster );
   printf ( "yRaster : %d\n" , aoCcdId->yRaster );
   printf ( "xSpace : %d\n" , aoCcdId->xSpace );
   printf ( "ySpace : %d\n" , aoCcdId->ySpace );
   printf ( "xSubap number : %d\n" , aoCcdId->xSubapNb );
   printf ( "ySubap number : %d\n" , aoCcdId->ySubapNb );
   printf ( "Subaperture number : %d\n" , aoCcdId->subapNb );
   printf ( "Subaperture number not used : %d\n" , aoCcdId->subapNotUsedNb );
   printf ( "Subaperture number used : %d\n" , aoCcdId->subapUsedNb );
   for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
       printf ( "subapUsedVector[%d] : %s\n" , 
                i+1, (aoCcdId->subapUsedVect[i] ? "TRUE" : "FALSE") );
   printf ( "Centroids number : %d\n" , aoCcdId->centroidsNb );
   printf ( "xPixels : %d\n" , aoCcdId->xPixels );
   printf ( "yPixels : %d\n" , aoCcdId->yPixels );
   printf ( "Pixels number : %d\n" , aoCcdId->pixelsNb );
   printf ( "uscan number : %d\n" , aoCcdId->uscanNb );
   printf ( "xTail : %d\n" , aoCcdId->xTail );
   printf ( "Packet size : %d\n" , aoCcdId->packetSize );
   printf ( "Packet number : %d\n" , aoCcdId->packetNb );
   printf ( "binningFlag : %s\n" ,
            (aoCcdId->binningFlag ? "TRUE" : "FALSE") );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoRefRead
 *
 *   INVOCATION:
 *   aoRefRead (pRefFileName, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRefFileName (char *) Pointer to the SH reference file name
 *   (!) aoCcdId (AO_CCD_ID)   Pointer to the AO CCD geometry context structure 
 *   (!) aoCtrlId (AO_CTRL_ID) Pointer to the AO control context structure 
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the content of a SH reference file 
 *
 *   DESCRIPTION:
 *   This function reads the content of the reference file given by pRefFileName
 *   and stores the values into aoCtrlId. 
 *   Note: P2 CCD is a 80x80 pixels. The coordinates of the first pixel are 
 *   0.5,0.5. The reference file contains the coordinates of the ideal center
 *   of each subapertures of the SH WFS.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pRefFilename is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoRefRead (
   char *     pRefFileName,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{

   int      i, j;                 /* Index                      */
   float    x, y;                 /* Coordinates of each center */
   char     comment[STRING_SIZE]; /* First line of comments     */
   FILE *   pFile;                /* File Id                    */
   WFS_VECT refVectRead;          /* Temporary reference vector */              

   /* Open the file in read mode */

   pFile = fopen ( pRefFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the WFS reference file %s",
                   ERROR_LOG_SAVE, pRefFileName );
      aoCtrlId->refInitFlag = FALSE;
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the WFS reference file %s",
      ERROR_LOG_SAVE, pRefFileName );

      fclose (pFile);

      aoCtrlId->refInitFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoRefRead(): first line of comments:\n" );
   printf ( "%s\n\n" , comment );
#endif

   /* The next lines contain the center of each subapertures */

   for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
   {
       if ( (fscanf (pFile, "%f %f", &x, &y)) != EOF ) 
       {
          refVectRead[2*i] = (double)x;
          refVectRead[2*i+1] = (double)y;
       }
       else
       {
          ERROR_SET2 ( 0, 
          "Failed to read center of subaperture %d in WFS reference file %s",
          ERROR_LOG_SAVE, i, pRefFileName );
          fclose (pFile);
          aoCtrlId->refInitFlag = FALSE;
          return (ERROR);
       }
   }

   /* Close the file */

   fclose (pFile);

   /* Init the aoCtrlId structure and modify if needed the aoCcdId structure */

   strcpy ( aoCtrlId->refVectFileName, pRefFileName );

   j = 0;
   aoCcdId->subapNotUsedNb = 0;
   for ( i = 0 ; i < aoCcdId->subapNb ; i ++ )
   {
       if ( (refVectRead[2*i] < 0.0) || (refVectRead[2*i+1] < 0.0) )
       {
          aoCcdId->subapUsedVect[i] = FALSE;
          aoCcdId->subapNotUsedNb += 1;
       }
       else
       {
          aoCcdId->subapUsedVect[i] = TRUE;
          aoCtrlId->refWfsVect[2*j] = refVectRead[2*i];
          aoCtrlId->refWfsVect[2*j+1] = refVectRead[2*i+1];
          j ++;
       }
   }

   aoCcdId->subapUsedNb = aoCcdId->subapNb - aoCcdId->subapNotUsedNb;
   aoCcdId->centroidsNb = 2 * (aoCcdId->subapUsedNb);
   
   aoCtrlId->refInitFlag = TRUE;

#ifdef DEBUG
   printf ( "aoRefRead(): centers\n" );
   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       printf ( "subaperture %d: %f, %f\n" , i+1, aoCtrlId->refWfsVect[2*i],
                aoCtrlId->refWfsVect[2*i+1] );
#endif   

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoScaleRead
 *
 *   INVOCATION:
 *   aoScaleRead (pAoScaleFileName, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pAoScaleFileName (char *) Pointer to the aO scale factor file name
 *   (!) aoCtrlId (AO_CTRL_ID) Pointer to the AO control context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the content of a aO scale factor file
 *
 *   DESCRIPTION:
 *   This function reads the content of the aO scale factor file given by
 *   pAoScaleFileName and stores the values into aoCtrlId.
 *   Note: There are aoModeNb scale factors to read. A scale factor to 0
 *   means the mode is not used.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pAoScaleFilename is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoScaleRead (
   char *     pAoScaleFileName,
   AO_CTRL_ID aoCtrlId
   )
{

   int      i;                    /* Index                            */
   float    value;                /* Scale factor                     */
   char     comment[STRING_SIZE]; /* First line of comments           */
   FILE *   pFile;                /* File Id                          */
   AO_VECT  aoScaleVectRead;      /* Temporary aO scale factor vector */

   /* Open the file in read mode */

   pFile = fopen ( pAoScaleFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the aO scale factor file %s",
                   ERROR_LOG_SAVE, pAoScaleFileName );
      aoCtrlId->aoScaleInitFlag = FALSE;
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read first line of comments from the aO scale factor file %s",
      ERROR_LOG_SAVE, pAoScaleFileName );

      fclose (pFile);

      aoCtrlId->aoScaleInitFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoScaleRead(): first line of comments:\n" );
   printf ( "%s\n\n" , comment );
#endif

   /* The next lines contain the scale factor for the aO modes */

   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       if ( (fscanf (pFile, "%f", &value)) != EOF )
       {
          aoScaleVectRead[i] = (double)value;
       }
       else
       {
          ERROR_SET2 ( 0,
          "Failed to read scale factor for mode %d in aO scale factor file %s",
          ERROR_LOG_SAVE, i, pAoScaleFileName );
          fclose (pFile);
          aoCtrlId->aoScaleInitFlag = FALSE;
          return (ERROR);
       }
   }

   /* Close the file */

   fclose (pFile);

   /* Init the aoCtrlId structure */

   strcpy ( aoCtrlId->aoScaleFileName, pAoScaleFileName );

   aoCtrlId->aoModeNotUsedNb = 0;
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       aoCtrlId->aoScaleFactorVect[i] = aoScaleVectRead[i];
       if ( aoScaleVectRead[i] == 0.0 )
       {
          aoCtrlId->aoModeUsedVect[i] = FALSE;
          aoCtrlId->aoModeNotUsedNb += 1;
       }
       else
       {
          aoCtrlId->aoModeUsedVect[i] = TRUE;
       }
   }

   aoCtrlId->aoModeUsedNb = aoCtrlId->aoModeNb - aoCtrlId->aoModeNotUsedNb;

   aoCtrlId->aoScaleInitFlag = TRUE;

#ifdef DEBUG
   printf ( "aoScaleRead(): \n" );
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       printf ( "aO mode %d: %f\n" , i+1, aoCtrlId->aoScaleFactorVect[i]);
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoScaleUpdate
 *
 *   INVOCATION:
 *   aoScaleUpdate (pAoScaleVect, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pAoScaleVect (double *)   Pointer to the aO scale factor vector
 *   (!) aoCtrlId     (AO_CTRL_ID) Pointer to the AO control context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Update the aO scale factors from a vector
 *
 *   DESCRIPTION:
 *   This function updates the aO scale factor vector of aoCtrlId with a
 *   vector given by pAoScaleVect and stores the values into aoCtrlId.
 *   Note: There are aoModeNb scale factors to update. A scale factor to 0
 *   means the mode is not used.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoScaleUpdate (
   double *   pAoScaleVect,
   AO_CTRL_ID aoCtrlId
   )
{

   int      i;                            /* Index                            */

   /* Update the aoCtrlId structure */

   strcpy ( aoCtrlId->aoScaleFileName, "Through dm" );

   aoCtrlId->aoModeNotUsedNb = 0;
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       aoCtrlId->aoScaleFactorVect[i] = pAoScaleVect[i];
       if ( pAoScaleVect[i] == 0.0 )
       {
          aoCtrlId->aoModeUsedVect[i] = FALSE;
          aoCtrlId->aoModeNotUsedNb += 1;
       }
       else
       {
          aoCtrlId->aoModeUsedVect[i] = TRUE;
       }
   }

   aoCtrlId->aoModeUsedNb = aoCtrlId->aoModeNb - aoCtrlId->aoModeNotUsedNb;

   aoCtrlId->aoScaleInitFlag = TRUE;

#ifdef DEBUG
   printf ( "aoScaleUpdate(): \n" );
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       printf ( "aO mode %d: %f\n" , i+1, aoCtrlId->aoScaleFactorVect[i]);
#endif

   return (OK);
}
   
/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoFitsImageFloatWrite
 *
 *   INVOCATION:
 *   aoFitsImageFloatWrite (pFileName, pImageBuffer, xBufferSize, yBufferSize)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFitsFileName (char *)  Pointer to the name of the FITS file
 *   (<) pImageBuffer  (float *) Pointer to the buffer where to store the image
 *   (>) xBufferSize   (int)     X size of the image buffer
 *   (>) yBufferSize   (int)     Y size of the image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write a float image into a FITS file
 *
 *   DESCRIPTION:
 *   A 2-d float image is written as a FITS primary array, with a minimal
 *   header.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pFileName is the full name of the file including the path.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Andrew Johnson.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoFitsImageFloatWrite (
   char *     pFitsFileName,
   float *    pImageBuffer,
   int        xBufferSize,
   int        yBufferSize
   )
{
   int        i;                /* index                                      */
   int        bufferSize;       /* Size of the buffer to write                */
   FILE       *pFile;           /* File descriptor                            */

   /* Create the FITS file */

   pFile = fopen ( pFitsFileName , "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Can't create FITS file %s", ERROR_LOG_SAVE,
                   pFitsFileName );
      return (ERROR);
   }

   /* Write a minimal header */

   fprintf ( pFile, "SIMPLE  =                    T /                                                " );
   fprintf ( pFile, "BITPIX  =                  -32 /                                                " );
   fprintf ( pFile, "NAXIS   =                    2 /                                                " );
   fprintf ( pFile, "NAXIS1  =                %5d /                                                ", xBufferSize );
   fprintf ( pFile, "NAXIS2  =                %5d /                                                ", yBufferSize );
   fprintf ( pFile, "BZERO   =                    0 /                                                " );
   fprintf ( pFile, "EXTEND  =                    T /                                                " );
   fprintf ( pFile, "END                                                                             ");

   /* Fill the rest of the header with blanks: header 36 * 80 char */

   for ( i = 0 ; i < 28 ; i ++ )
       fprintf ( pFile, "                                                                                " );

   /* Write the image to the Fits file */

   bufferSize = xBufferSize * yBufferSize;

   if ( fwrite ( pImageBuffer, sizeof (float), bufferSize, pFile ) != 
        bufferSize )
   {
      ERROR_SET1 ( 0, "Failed to write image into %s",
                   ERROR_LOG_SAVE, pFitsFileName );

      fclose ( pFile );
      return (ERROR);
   }

   /* Close the fits file */

   fclose ( pFile ) ;

   return ( OK ) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoFitsImageFloatRead
 *
 *   INVOCATION:
 *   aoFitsImageFloatRead (pFileName, pImageBuffer, xBufferSize, yBufferSize)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFitsFileName (char *)  Pointer to the name of the FITS file
 *   (<) pImageBuffer  (float *) Pointer to the buffer where to store the image
 *   (>) xBufferSize   (int)     X size of the image buffer
 *   (>) yBufferSize   (int)     Y size of the image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read a float image from a FITS file
 *
 *   DESCRIPTION:
 *   This routine allows to read simple FITS file which have been written with
 *   aoFitsImageFloatWrite.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Marianne Takamiya
 *   and Mark Chun. 
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pFileName is the full name of the file including the path.
 *   The buffer pointed to by pImageBuffer has been allocated large enough to
 *   accomodate xBufferSize*yBufferSize float.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */
STATUS aoFitsImageFloatRead (
   char *     pFitsFileName,
   float *    pImageBuffer,
   int        xBufferSize,
   int        yBufferSize
   )
{
   char       header[2880];
   char       line[81];
   char       restHeader[2880];
   char       keyword[8];
   char       *token;
   char       *delim1 = "=";
   char       *delim2 = "\0";

   int        flag;
   int        bufferSize;
   int        pixelsNb;
   int        bitpix;
   int        naxis;
   int        naxis1;
   int        naxis2;

   long       restSize;
   long       lineSize=80;
   long       nChar;
   long       headerSize=0;

   FILE       *pFile;

#ifdef DEBUG
   int        i;
#endif

   /* Open the FITS file */

   pFile = fopen ( pFitsFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Can't open FITS file %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      return (ERROR);
   }

   /* Read the first line */

   nChar = fread ( header, sizeof (char), lineSize, pFile );

   if ( nChar != lineSize )
   {
      ERROR_SET1 ( 0, "Can't read the first line of %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

   strncpy ( line, header, lineSize );
   line[81]='\0';

   /* Check this line contains SIMPLE keyword */

   if ( strncmp ( "SIMPLE  ", line, 8 ) != 0 )
   {
      ERROR_SET1 ( 0, "File %s doesn't contain SIMPLE keyword", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

   headerSize += 80;

   flag = TRUE;
   while ( flag )
   {
      nChar = fread ( header, sizeof (char), lineSize, pFile );

      if ( nChar != lineSize )
      {
         ERROR_SET1 ( 0, "Can't read the next line of %s", ERROR_LOG_SAVE,
                      pFitsFileName);
         fclose ( pFile );
         return (ERROR);
      }

      strncpy ( line, header, lineSize );
      line[81]= '\0';

      strncpy ( keyword, line, 8 );

      token = strtok ( line, delim1);
      token = strtok ( NULL, delim2);

      if ( strncmp ( "END     ", keyword, 8) == 0 ) 
         flag = FALSE;
      if ( strncmp ( "BITPIX  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &bitpix);
      if ( strncmp ( "NAXIS   ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis);
      if ( strncmp ( "NAXIS1  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis1);
      if ( strncmp ( "NAXIS2  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis2);

      headerSize += 80;
   }

   if ( headerSize % 2880 != 0 )
   {
      restSize = 2880 - (headerSize % 2880);
      nChar = fread ( restHeader, sizeof (char), restSize, pFile );
      if ( nChar != restSize )
      {
         ERROR_SET1 ( 0, "Can't read the rest of the header of %s", 
                      ERROR_LOG_SAVE, pFitsFileName);
         fclose ( pFile );
         return (ERROR);
      }
   }

   /* Now read the data */

   bufferSize = xBufferSize * yBufferSize;
   pixelsNb = naxis1 * naxis2;

   if ( pixelsNb != bufferSize )
   {
      ERROR_SET2 ( 0, "Dark Image size %d not as expected %d",
                   ERROR_LOG_SAVE, (int)pixelsNb, bufferSize);
      fclose ( pFile );
      return (ERROR);
   }

   if ( fread ( pImageBuffer, sizeof (float), bufferSize, pFile ) != 
        bufferSize )
   {
      ERROR_SET1 ( 0, "Failed to read image from %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

#ifdef DEBUG
   for ( i = 0 ; i < 10 ; i ++ )
       printf ( "pixel %d = %f\n", i, *(pImageBuffer + i) );
#endif

   /* Close the FITS file */

   fclose ( pFile );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoMatRead
 *
 *   INVOCATION:
 *   aoMatRead (pMatFileName, typeExpected, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pMatFileName (char *) Pointer to the matrix file name
 *   (>) typeExpected (int)    Type of the matrix expected AO_INT_MAT_TYPE or
 *                             AO_CONT_MAT_TYPE
 *   (>) aoCcdId (AO_CCD_ID)   Pointer to the AO CCD geometry context structure
 *   (!) aoCtrlId (AO_CTRL_ID) Pointer to the AO control context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read a matrix from a file
 *
 *   DESCRIPTION:
 *   This function reads a aO control or aO interaction matrix of various 
 *   dimensions from a file given by pMatFileName and stores the matrix into 
 *   aoCtrlId.
 *   Note: The first comment line indicates if it is an interaction or a
 *   control matrix, the second line indicates the dimension.
 *   If the type of the matrix is not equivalent to the typeExpected, no matrix
 *   is read and the routine exit with an error.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pMatFilename is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */


STATUS aoMatRead (
   char *     pMatFileName,
   int        typeExpected,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{

   int        type;                 /* Type of the matrix         */
   int        row, col;             /* Dimension of the matrix    */
   int        i, j;                 /* Index                      */
   float      value;                /* Element of the matrix      */
   AO_MATRIX  mat;                  /* Matrix read                */
   char       comment[STRING_SIZE]; /* First line of comments     */
   FILE *     pFile;                /* File Id                    */

   /* Open the file in read mode */

   pFile = fopen ( pMatFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );

      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n" , comment );
#endif

   /* The next line contains the type of the matrix */

   if ( (fscanf (pFile, "%d\n", &type)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the type of the matrix in the file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( (type != AO_INT_MAT_TYPE) && (type != AO_CONT_MAT_TYPE) )
   {
      ERROR_SET1 ( 0,
            "Type of the matrix is unrecognized: %d (should be 0 or 1)",
            ERROR_LOG_SAVE, type );
      fclose (pFile);
      return (ERROR);
   }

   if ( type != typeExpected )
   {
      ERROR_SET2 ( 0,
            "Type of the matrix is not the one expected: %d (should be %d)",
            ERROR_LOG_SAVE, type, typeExpected);
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): type of the matrix %s\n" ,
            (type ? "CONTROL" : "INTERACTION") );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n", comment );
#endif

   /* The next line contains the dimensions of the matrix */

   if ( (fscanf (pFile, "%d %d\n", &row, &col)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the dimension of the matrix in the file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      if (type == AO_INT_MAT_TYPE)
         aoCtrlId->aoIntMatInitFlag = FALSE;
      else
         aoCtrlId->aoContMatInitFlag = FALSE;
      return (ERROR);
   }

   if ( type == AO_INT_MAT_TYPE ) /* interaction matrix */
   {
      if ( (row != aoCcdId->centroidsNb) || (col != aoCtrlId->aoModeNb) )
      {
         ERROR_SET4 ( 0,
            "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
            ERROR_LOG_SAVE, row, col, aoCcdId->centroidsNb, aoCtrlId->aoModeNb);
         aoCtrlId->aoIntMatInitFlag = FALSE;
         fclose (pFile);
         return (ERROR);
      }
   }
   else             /* control matrix */
   {
      if ( (row != aoCtrlId->aoModeNb) || (col != aoCcdId->centroidsNb) )
      {
         ERROR_SET4 ( 0,
            "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
            ERROR_LOG_SAVE, row, col, aoCtrlId->aoModeNb, aoCcdId->centroidsNb);
         aoCtrlId->aoContMatInitFlag = FALSE;
         fclose (pFile);
         return (ERROR);
      }
   }

#ifdef DEBUG
   printf ( "aoMatRead(): dimensions of the matrix %d, %d\n", row, col );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      if (type == AO_INT_MAT_TYPE)
         aoCtrlId->aoIntMatInitFlag = FALSE;
      else
         aoCtrlId->aoContMatInitFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n", comment );
#endif

   /* Now read the matrix */

   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
       {
           if ( (fscanf (pFile, "%f", &value)) != EOF )
           {
              *(mat + i*col + j) = (double)(value);
           }
           else
           {
              ERROR_SET1 ( 0, "Failed to read matrix from file %s",
                           ERROR_LOG_SAVE, pMatFileName );
              if (type == AO_INT_MAT_TYPE)
                 aoCtrlId->aoIntMatInitFlag = FALSE;
              else
                 aoCtrlId->aoContMatInitFlag = FALSE;
              fclose (pFile);
              return (ERROR);
           }
       }
   }

   /* Close the file */

   fclose (pFile);

   /* Init the aoCtrlId structure */

   if ( type == AO_INT_MAT_TYPE )
   {
      strcpy ( aoCtrlId->aoIntMatFileName, pMatFileName );
      aoCtrlId->aoIntMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->aoIntMat, row, col);
      aoCtrlId->aoContMatInitFlag = FALSE;
   }
   else
   {
      strcpy ( aoCtrlId->aoContMatFileName, pMatFileName );
      aoCtrlId->aoContMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->aoContMat, row, col);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): matrix\n" );
   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
           printf ( "%f ", mat[i*col +j]);
       printf ( "\n" );
   }
#endif
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoMatWrite
 *
 *   INVOCATION:
 *   aoMatWrite (pMatFileName, pMat, rowNb, colNb, type)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pMatFileName (char *)   Pointer to the matrix file name
 *   (>) pMat         (double *) Pointer to the matrix to write
 *   (>) rowNb        (int)      Dimension of the matrix to write
 *   (>) colNb        (int)      Dimension of the matrix to write
 *   (>) type         (int)      Type of the matrix to write
 *
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write a matrix to a file
 *
 *   DESCRIPTION:
 *   This function writes a aO control or aO interaction matrix of various 
 *   dimensions to a file given by pMatFileName.
 *   Note: The first comment line indicates if it is an interaction or a
 *   control matrix, the second line indicates the dimension.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pMatFilename is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoMatWrite (
   char *     pMatFileName,
   double *   pMat,
   int        rowNb,
   int        colNb,
   int        type
   )
{
   int      i, j;                 /* Index                      */
   FILE *   pFile;                /* File Id                    */

   /* Open the file in write mode */

   pFile = fopen ( pMatFileName, "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      return (ERROR);
   }

   /* Write the first line: should be a comment line */

   (void) fprintf (pFile,
          "# Type of the matrix (0: Interaction, 1: Control)\n");

   /* Write the type of the matrix */

   (void) fprintf (pFile, "%d\n", type);

   /* Write the next line of comment */

   (void) fprintf (pFile, "# Dimensions\n" );

   /* Write the dimensions */

   (void) fprintf (pFile, "%d %d\n", rowNb, colNb );

   /* Write the next line of comments */

   (void) fprintf (pFile, "# Matrix\n" );

   /* Now write the matrix */

   for ( i = 0 ; i < rowNb ; i ++ )
   {
       for ( j = 0 ; j < colNb ; j ++ )
           (void) fprintf (pFile, "%f ", *(pMat + i*colNb + j) );
       (void) fprintf (pFile, "\n" );
   }
     
   /* Close the file */

   fclose (pFile);


#ifdef DEBUG
   printf ( "aoMatWrite: Write matrix into %s done \n" , pMatFileName );
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoFgContMatRead
 *
 *   INVOCATION:
 *   aoFgContMatRead (pFgContMatFileName, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFgContMatFileName (char *) Pointer to the FG control matrix file name
 *   (>) aoCcdId (AO_CCD_ID)         Pointer to the AO CCD geometry context
 *                                   structure
 *   (!) aoCtrlId (AO_CTRL_ID)       Pointer to the AO control context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read a FG control matrix from a file
 *
 *   DESCRIPTION:
 *   This function reads a FG control matrix of various dimensions
 *   from a file given by pFgContMatFileName and stores the matrix into ctrlId.
 *   Note: The first comment line indicates if it is an interaction or a
 *   control matrix, the second line indicates the dimension.
 *   If the type of the matrix is not equivalent to a control matrix, no matrix
 *   is read and the routine exit with an error.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pFgContMatFilename is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoFgContMatRead (
   char *     pFgContMatFileName,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{

   int       type;                 /* Type of the matrix         */
   int       row, col;             /* Dimension of the matrix    */
   int       i, j;                 /* Index                      */
   float     value;                /* Element of the matrix      */
   FG_MATRIX mat;                  /* Matrix read                */
   char      comment[STRING_SIZE]; /* First line of comments     */
   FILE *    pFile;                /* File Id                    */

   /* Open the file in read mode */

   pFile = fopen ( pFgContMatFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the FG control matrix file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );

      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoFgContMatRead(): %s\n" , comment );
#endif

   /* The next line contains the type of the matrix */

   if ( (fscanf (pFile, "%d\n", &type)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the type of the matrix in the file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( type != AO_CONT_MAT_TYPE )
   {
      ERROR_SET1 ( 0,
            "Type of the matrix is not the one expected: %d (should be 1)",
            ERROR_LOG_SAVE, type );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoFgContMatRead(): type of the matrix %s\n" ,
            (type ? "CONTROL" : "INTERACTION") );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoFgContMatRead(): %s\n", comment );
#endif

   /* The next line contains the dimensions of the matrix */

   if ( (fscanf (pFile, "%d %d\n", &row, &col)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the dimension of the matrix in the file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );
      fclose (pFile);
      aoCtrlId->fgContMatInitFlag = FALSE;
      return (ERROR);
   }

   if ( (row != aoCtrlId->fgModeNb) || (col != aoCcdId->centroidsNb) )
   {
      ERROR_SET4 ( 0,
         "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
         ERROR_LOG_SAVE, row, col, aoCtrlId->fgModeNb, aoCcdId->centroidsNb);
      aoCtrlId->fgContMatInitFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoFgContMatRead(): dimensions of the matrix %d, %d\n", row, col );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pFgContMatFileName );
      aoCtrlId->fgContMatInitFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoFgContMatRead(): %s\n", comment );
#endif

   /* Now read the matrix */

   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
       {
           if ( (fscanf (pFile, "%f", &value)) != EOF )
           {
              *(mat + i*col + j) = (double)(value);
           }
           else
           {
              ERROR_SET1 ( 0, "Failed to read matrix from file %s",
                           ERROR_LOG_SAVE, pFgContMatFileName );
              aoCtrlId->fgContMatInitFlag = FALSE;
              fclose (pFile);
              return (ERROR);
           }
       }
   }

   /* Close the file */

   fclose (pFile);

   /* Init the aoCtrlId structure */

   strcpy ( aoCtrlId->fgContMatFileName, pFgContMatFileName );
   aoCtrlId->fgContMatInitFlag = TRUE;
   (void) copyMat ( mat, aoCtrlId->fgContMat, row, col);

#ifdef DEBUG
   printf ( "aoFgContMatRead(): matrix\n" );
   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
           printf ( "%f ", mat[i*col +j]);
       printf ( "\n" );
   }
#endif
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCtrlContextInit
 *
 *   INVOCATION:
 *   aoCtrlContextInit (pInitFileName, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)     Pointer to the AO init file name 
 *   (>) aoCcdId       (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *                                  structure 
 *   (<) aoCtrlId      (AO_CTRL_ID) Pointer to the AO control context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the AO control context structure
 *
 *   DESCRIPTION:
 *   Init the AO control context structure with defaults contained 
 *   into pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCtrlContextInit (
   char *     pInitFileName,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{

   int        i;
   int        mode;
   FILE *     pFile;
   char       comment [STRING_SIZE];
   char       fileName [STRING_SIZE];
   double     value; 
   double     max; 
   IMAGE_VECT image;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the AO init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): first line of comments:\n" );
   printf ( "%s\n" , comment );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the second line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read the name of the dark fits file and init the dark vector */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the name of the dark fits file from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n", 
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): dark file name: %s\n", fileName );
#endif

   if ( aoFitsImageFloatRead (fileName, image, aoCcdId->xPixels, 
                              aoCcdId->yPixels) == ERROR )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the dark image from the dark fits file %s",
      ERROR_LOG_SAVE, fileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->darkInitFlag = FALSE;
      return (ERROR);
   }

   strcpy ( aoCtrlId->darkFileName , fileName );
   
   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoCtrlId->darkVect[i] = image[i];
   
   aoCtrlId->darkInitFlag = TRUE;

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read the name of the flat fits file and init the flat vector */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the name of the flat fits file from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n", 
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): flat file name: %s\n", fileName );
#endif

   if ( aoFitsImageFloatRead (fileName, image, aoCcdId->xPixels, 
                              aoCcdId->yPixels) == ERROR )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the flat image from the flat fits file %s",
      ERROR_LOG_SAVE, fileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->flatInitFlag = FALSE;
      return (ERROR);
   }

   strcpy ( aoCtrlId->flatFileName , fileName );
   
   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoCtrlId->flatVect[i] = image[i];

   aoCtrlId->flatInitFlag = TRUE;
   
   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read the name of the WFS reference file */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the WFS reference file name from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n", 
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): WFS reference file name: %s\n", fileName );
#endif

   if ( aoRefRead ( fileName, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0, 
                   "Failed when reading WFS reference file %s" , 
                   ERROR_LOG_SAVE, fileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read value of xcenter for the whole CCD */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read x center of the whole CCD from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   max = (double)(aoCcdId->xPixels);
   if ( value > max )
   {
      ERROR_SET2 ( 0, 
                   "xCenter %f is greater than max pixels %f",
                   ERROR_LOG_SAVE, value, max );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->refGuideVect[0] = value;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): x center for whole CCD=%f\n", 
            aoCtrlId->refGuideVect[0] );
#endif   

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read value of ycenter for the whole CCD */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
         "Failed to read y center for the whole CCD from the AO init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   max = (double)(aoCcdId->yPixels);
   if ( value > max )
   {
      ERROR_SET2 ( 0, 
                   "yCenter %f is greater than max pixels %f",
                   ERROR_LOG_SAVE, value, max );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->refGuideVect[1] = value;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): y center for whole CCD=%f\n", 
            aoCtrlId->refGuideVect[1] );
#endif   

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

   /* Read value of ao zernikes mode to be corrected */

   if ( (fscanf (pFile, "%d\n", &mode)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read the ao mode number from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( mode > AO_MODE_NB )
   {
      ERROR_SET2 ( 0, 
                   "aoModeNb %d is greater than max mode %d",
                   ERROR_LOG_SAVE, mode, (int)(AO_MODE_NB) );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->aoModeNb = mode;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): aO mode number=%d\n", aoCtrlId->aoModeNb );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read value of fg zernikes mode to be corrected */

   if ( (fscanf (pFile, "%d\n", &mode)) == EOF )
   {
      ERROR_SET1 ( 0,
            "Failed to read the FG mode number from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( mode > FG_MODE_NB )
   {
      ERROR_SET2 ( 0,
                   "fgModeNb %d is greater than max mode %d",
                   ERROR_LOG_SAVE, mode, (int)(FG_MODE_NB) );
      aoCtrlId->initFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

   aoCtrlId->fgModeNb = mode;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): FG mode number=%d\n", aoCtrlId->fgModeNb );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read the name of the ao scale factor file */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the aO scale vector file name from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n",
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): aO scale factor file name: %s\n", fileName );
#endif

   if ( aoScaleRead ( fileName, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading aO scale factor file %s" ,
                   ERROR_LOG_SAVE, fileName );
      aoCtrlId->initFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read the name of the interaction matrix file */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the inter. matrix file name from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n",
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): interaction matrix file name: %s\n",
            fileName );
#endif

   if ( aoMatRead ( fileName, AO_INT_MAT_TYPE, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the interaction matrix file %s" ,
                   ERROR_LOG_SAVE, fileName );
      aoCtrlId->initFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read the name of the control matrix file */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the control matrix file name from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n",
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): control matrix file name: %s\n",
            fileName );
#endif

   if ( aoMatRead ( fileName, AO_CONT_MAT_TYPE, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the control matrix file %s" ,
                   ERROR_LOG_SAVE, fileName );
      aoCtrlId->initFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read the name of the FG control matrix file */

   if ( fgets (fileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the FG control matrix file name from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( fileName[strlen(fileName) - 1] == '\n' )
   {
      fileName[strlen(fileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlContextInit(): last character of %s was return\n",
               fileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): FG control matrix file name: %s\n",
            fileName );
#endif

   if ( aoFgContMatRead ( fileName, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the FG control matrix file %s" ,
                   ERROR_LOG_SAVE, fileName );
      aoCtrlId->initFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

   /* Read the FG scaleFactorVect from the init file */

   for ( i = 0 ; i < aoCtrlId->fgModeNb ; i ++ )
   {
      /* Skip the next line of comment */

      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the next line of comments from the AO init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         aoCtrlId->initFlag = FALSE;
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoCtrlContextInit(): %s\n", comment );
#endif   

      /* Read value of the scale factor */

      if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
      {
         ERROR_SET2 ( 0, 
               "Failed to read scale factor [%d] from the AO init file %s",
               ERROR_LOG_SAVE, i, pInitFileName );
         fclose (pFile);
         aoCtrlId->initFlag = FALSE;
         return (ERROR);
      }

      aoCtrlId->fgScaleFactorVect[i] = value;

#ifdef DEBUG
      printf ( "aoCtrlContextInit(): fgScaleFactorVect[%d]=%f\n", i, 
               aoCtrlId->fgScaleFactorVect[i] );
#endif   
   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read gain for focus sliding average */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read focus gain from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->slidingFocusGain = value;
   aoCtrlId->one_slidingFocusGain = 1.0 - aoCtrlId->slidingFocusGain;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): slidingFocusGain = %f\n", 
            aoCtrlId->slidingFocusGain );
   printf ( "aoCtrlContextInit(): one_slidingFocusGain = %f\n", 
            aoCtrlId->one_slidingFocusGain );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read threshold value for centroid computation */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read threshold from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->threshold = value;
   aoCtrlId->thresholdDarkFull = value;
   aoCtrlId->thresholdDarkBin = value;
   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       aoCtrlId->thresholdVect[i] = value;
   aoCtrlId->thresholdMethod = AO_THRESH_VALUE;
   aoCtrlId->thresholdMultCoeff = 0.0;
   aoCtrlId->thresholdRate = 0.0;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): threshold = %f\n", aoCtrlId->threshold );
   printf ( "aoCtrlContextInit(): threshold method = %d\n",
            aoCtrlId->thresholdMethod );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read average flux value for centroid computation */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read average total counts from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->totalThreshold = value;
   aoCtrlId->thresholdMethod = AO_TOTAL_VALUE;
   aoCtrlId->thresholdMultCoeff = 0.0;
   aoCtrlId->averageTotal = 0.0;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): Total threshold = %f\n", 
            aoCtrlId->totalThreshold );
   printf ( "aoCtrlContextInit(): Total count computation method = %d\n",
            aoCtrlId->totalMethod );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read angle between M2 and P2 */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read fangle from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->angleWithM2 = value;

   aoCtrlId->cosAngleWithM2 = cos ( aoCtrlId->angleWithM2 );
   aoCtrlId->sinAngleWithM2 = sin ( aoCtrlId->angleWithM2 );

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): angleWithM2 = %f\n", aoCtrlId->angleWithM2 );
   printf ( "aoCtrlContextInit(): cos(angleWithM2) = %f\n", 
            aoCtrlId->cosAngleWithM2 );
   printf ( "aoCtrlContextInit(): sin(angleWithM2) = %f\n", 
            aoCtrlId->sinAngleWithM2 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the AO init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): %s\n", comment );
#endif

   /* Read angle between M1 and P2 */

   if ( (fscanf (pFile, "%lf\n", &value)) == EOF )
   {
      ERROR_SET1 ( 0,
            "Failed to read angle from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->angleWithM1 = value;

   aoCtrlId->cosAngleWithM1 = cos ( aoCtrlId->angleWithM1 );
   aoCtrlId->sinAngleWithM1 = sin ( aoCtrlId->angleWithM1 );

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): angleWithM1 = %f\n", aoCtrlId->angleWithM1 );
   printf ( "aoCtrlContextInit(): cos(angleWithM1) = %f\n",
            aoCtrlId->cosAngleWithM1 );
   printf ( "aoCtrlContextInit(): sin(angleWithM1) = %f\n",
            aoCtrlId->sinAngleWithM1 );
#endif

   /* End - close and return */

   aoCtrlId->initFlag = TRUE;
   aoCtrlId->coaddCounter = 0;
   aoCtrlId->focusCounter = 0;
   aoCtrlId->previousFocus = 0;
   aoCtrlId->allowedSubapOff = 0;

   for ( i = 0 ; i < CCD_SIZE ; i ++ )
       aoCtrlId->sumVect[i] = 0.0;

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCtrlContextUpdate
 *
 *   INVOCATION:
 *   aoCtrlContextUpdate (pDarkFileName, pFlatFileName, pRefFileName, 
 *                        pAoIntMatFileName, pAoContMatFileName, 
 *                        pFgContMatFileName, xCenter, yCenter, 
 *                        angleWithM2, angleWithM1, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDarkFileName      (char *)     Pointer to the dark file name 
 *   (>) pFlatFileName      (char *)     Pointer to the flat file name 
 *   (>) pRefFileName       (char *)     Pointer to the reference file name 
 *   (>) pAoIntMatFileName  (char *)     Pointer to the aO interaction matrix 
 *                                       file name
 *   (>) pAoContMatFileName (char *)     Pointer to the aO control matrix file 
 *                                       name
 *   (>) pFgContMatFileName (char *)     Pointer to the FG control matrix file
 *                                       name
 *   (>) xCenter            (double)     New xCenter value for whole CCD
 *   (>) yCenter            (double)     New yCenter value for whole CCD
 *   (>) angleWithM2        (double)     New angle between M2 and P2 
 *   (>) angleWithM1        (double)     New angle between M1 and P2 
 *   (>) aoCcdId            (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *                                       structure 
 *   (<) aoCtrlId           (AO_CTRL_ID) Pointer to the AO control context 
 *                                       structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Update the AO control context structure from a DM screen
 *
 *   DESCRIPTION:
 *   Update the AO control context structure 
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pDarkFileName is the full name of the file including the path.
 *   The pFlatFileName is the full name of the file including the path.
 *   The pRefFileName is the full name of the file including the path.
 *   The pAoIntMatFileName is the full name of the file including the path.
 *   The pAoContMatFileName is the full name of the file including the path.
 *   The pFgContMatFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCtrlContextUpdate (
   char *     pDarkFileName,
   char *     pFlatFileName, 
   char *     pRefFileName,
   char *     pAoIntMatFileName,
   char *     pAoContMatFileName,
   char *     pFgContMatFileName,
   double     xCenter,
   double     yCenter, 
   double     angleWithM2,
   double     angleWithM1,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{
   int        i;
   double     max; 
   IMAGE_VECT image;

   /* Init the new dark image */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): dark file name: %s\n", pDarkFileName );
#endif

   if ( aoFitsImageFloatRead (pDarkFileName, image, aoCcdId->xPixels, 
                              aoCcdId->yPixels) == ERROR )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the dark image from the dark fits file %s",
      ERROR_LOG_SAVE, pDarkFileName );
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->darkInitFlag = FALSE;
      return (ERROR);
   }

   strcpy ( aoCtrlId->darkFileName , pDarkFileName );
   
   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoCtrlId->darkVect[i] = image[i];
   
   aoCtrlId->darkInitFlag = TRUE;

   /* Init the new flat image */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): flat file name: %s\n", pFlatFileName );
#endif

   if ( aoFitsImageFloatRead (pFlatFileName, image, aoCcdId->xPixels, 
                              aoCcdId->yPixels) == ERROR )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the flat image from the flat fits file %s",
      ERROR_LOG_SAVE, pFlatFileName );
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->flatInitFlag = FALSE;
      return (ERROR);
   }

   strcpy ( aoCtrlId->flatFileName , pFlatFileName );
   
   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoCtrlId->flatVect[i] = image[i];

   aoCtrlId->flatInitFlag = TRUE;
   
   /* Init the new WFS reference vector */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): WFS reference file name: %s\n", 
            pRefFileName );
#endif

   if ( aoRefRead ( pRefFileName, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0, 
                   "Failed when reading WFS reference file %s" , 
                   ERROR_LOG_SAVE, pRefFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Init the new aO interaction matrix */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): interaction matrix file name: %s\n",
            pAoIntMatFileName );
#endif

   if ( aoMatRead ( pAoIntMatFileName, AO_INT_MAT_TYPE, aoCcdId, aoCtrlId )
        == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the interaction matrix file %s" ,
                   ERROR_LOG_SAVE, pAoIntMatFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Init the new aO control matrix */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): aO control matrix file name: %s\n",
            pAoContMatFileName );
#endif

   if ( aoMatRead ( pAoContMatFileName, AO_CONT_MAT_TYPE, aoCcdId, aoCtrlId )
        == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the control matrix file %s" ,
                   ERROR_LOG_SAVE, pAoContMatFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Init the new FG control matrix */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): FG control matrix file name: %s\n",
            pFgContMatFileName );
#endif

   if ( aoFgContMatRead ( pFgContMatFileName, aoCcdId, aoCtrlId ) == ERROR )
   {
      ERROR_SET1 ( 0,
                   "Failed when reading the FG control matrix file %s" ,
                   ERROR_LOG_SAVE, pFgContMatFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Init the new xcenter for the whole CCD */

   max = (double)(aoCcdId->xPixels);
   if ( xCenter > max )
   {
      ERROR_SET2 ( 0, 
                   "xCenter %f is greater than max pixels %f",
                   ERROR_LOG_SAVE, xCenter, max );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->refGuideVect[0] = xCenter;

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): x center for whole CCD=%f\n", 
            aoCtrlId->refGuideVect[0] );
#endif   

   /* Init the new ycenter for the whole CCD */

   max = (double)(aoCcdId->yPixels);
   if ( yCenter > max )
   {
      ERROR_SET2 ( 0, 
                   "yCenter %f is greater than max pixels %f",
                   ERROR_LOG_SAVE, yCenter, max );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->refGuideVect[1] = yCenter;

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): y center for whole CCD=%f\n", 
            aoCtrlId->refGuideVect[1] );
#endif   

   /* Init the new angle between M2 and P2 */

   aoCtrlId->angleWithM2 = angleWithM2;

   aoCtrlId->cosAngleWithM2 = cos ( aoCtrlId->angleWithM2 );
   aoCtrlId->sinAngleWithM1 = sin ( aoCtrlId->angleWithM2 );

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): angleWithM2 = %f\n", 
            aoCtrlId->angleWithM2 );
   printf ( "aoCtrlContextUpdate(): cos(angleWithM2) = %f\n", 
            aoCtrlId->cosAngleWithM2 );
   printf ( "aoCtrlContextUpdate(): sin(angleWithM2) = %f\n", 
            aoCtrlId->sinAngleWithM2 );
#endif

   /* Init the new angle between M1 and P2 */

   aoCtrlId->angleWithM1 = angleWithM1;

   aoCtrlId->cosAngleWithM1 = cos ( aoCtrlId->angleWithM1 );
   aoCtrlId->sinAngleWithM1 = sin ( aoCtrlId->angleWithM1 );

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): angleWithM1 = %f\n",
            aoCtrlId->angleWithM1 );
   printf ( "aoCtrlContextUpdate(): cos(angleWithM1) = %f\n",
            aoCtrlId->cosAngleWithM1 );
   printf ( "aoCtrlContextUpdate(): sin(angleWithM1) = %f\n",
            aoCtrlId->sinAngleWithM1 );
#endif

   /* End */

   aoCtrlId->initFlag = TRUE;

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCtrlContextShow
 *
 *   INVOCATION:
 *   aoCtrlContextShow (aoCcdId, aoCtrlId, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) aoCcdId (AO_CCD_ID) Pointer to the AO CCD geometry context structure 
 *   (>) aoCtrlId (AO_CTRLID) Pointer to the AO control context structure 
 *   (>) verbose (int)        Verbose mode (TRUE OR FALSE)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the control context structure
 *
 *   DESCRIPTION:
 *   This function displays the content of the control context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCtrlContextShow (
   AO_CCD_ID aoCcdId,
   AO_CTRL_ID aoCtrlId,
   int verbose
   )
{
   int    i, j;

   /* Check the control context structure is valid. */

   if ( aoCcdId == NULL )
   {
      ERROR_SET ( 0, "Invalid AO CCD geometry context", ERROR_LOG_SAVE );
      return (ERROR);
   }

   if ( aoCtrlId == NULL )
   {
      ERROR_SET ( 0, "Invalid AO control context", ERROR_LOG_SAVE );
      return (ERROR);
   }

   /* Display the contents of the control context structure. */

   printf ( "AO control context structure:\n" );
   printf ( "Init flag: %s\n" , 
            (aoCtrlId->initFlag ? "TRUE" : "FALSE") );
   printf ( "Dark init flag: %s\n" , 
            (aoCtrlId->darkInitFlag ? "TRUE" : "FALSE") );
   printf ( "Flat init flag: %s\n" , 
            (aoCtrlId->flatInitFlag ? "TRUE" : "FALSE") );
   printf ( "Reference init flag: %s\n" , 
            (aoCtrlId->refInitFlag ? "TRUE" : "FALSE") );
   printf ( "aO scale factor init flag: %s\n" ,
            (aoCtrlId->aoScaleInitFlag ? "TRUE" : "FALSE") );
   printf ( "Interaction matrix init flag: %s\n" ,
            (aoCtrlId->aoIntMatInitFlag ? "TRUE" : "FALSE") );
   printf ( "aO Control matrix init flag: %s\n" ,
            (aoCtrlId->aoContMatInitFlag ? "TRUE" : "FALSE") );
   printf ( "FG Control matrix init flag: %s\n" ,
            (aoCtrlId->fgContMatInitFlag ? "TRUE" : "FALSE") );

   printf ( "Dark file name: %s\n", aoCtrlId->darkFileName );
   printf ( "Flat file name: %s\n", aoCtrlId->flatFileName );
   printf ( "Reference file name: %s\n", aoCtrlId->refVectFileName );
   printf ( "aO scale factor file name: %s\n", aoCtrlId->aoScaleFileName );
   printf ( "aO Interaction matrix file name: %s\n", 
            aoCtrlId->aoIntMatFileName );
   printf ( "aO Control matrix file name: %s\n" , aoCtrlId->aoContMatFileName );
   printf ( "FG Control matrix file name: %s\n" , aoCtrlId->fgContMatFileName );

   if ( verbose == TRUE )
   {
      printf ( "darkVect= %f %f %f %f %f\n" , 
               aoCtrlId->darkVect[0], aoCtrlId->darkVect[1],
               aoCtrlId->darkVect[2], aoCtrlId->darkVect[3],
               aoCtrlId->darkVect[4]); 
      printf ( "darkVect= %f %f %f %f %f ...\n" , 
               aoCtrlId->darkVect[5], aoCtrlId->darkVect[6], 
               aoCtrlId->darkVect[7], aoCtrlId->darkVect[8], 
               aoCtrlId->darkVect[9]);
      printf ( "darkVect= %f %f %f %f %f \n" , 
               aoCtrlId->darkVect[aoCcdId->pixelsNb-5], 
               aoCtrlId->darkVect[aoCcdId->pixelsNb-4], 
               aoCtrlId->darkVect[aoCcdId->pixelsNb-3],
               aoCtrlId->darkVect[aoCcdId->pixelsNb-2], 
               aoCtrlId->darkVect[aoCcdId->pixelsNb-1]);

      printf ( "flatVect= %f %f %f %f %f\n" , 
               aoCtrlId->flatVect[0], aoCtrlId->flatVect[1],
               aoCtrlId->flatVect[2], aoCtrlId->flatVect[3],
               aoCtrlId->flatVect[4]);
      printf ( "flatVect= %f %f %f %f %f...\n" , 
               aoCtrlId->flatVect[5],
               aoCtrlId->flatVect[6], aoCtrlId->flatVect[7],
               aoCtrlId->flatVect[8], aoCtrlId->flatVect[9]);
      printf ( "flatVect= %f %f %f %f %f \n" , 
               aoCtrlId->flatVect[aoCcdId->pixelsNb-5], 
               aoCtrlId->flatVect[aoCcdId->pixelsNb-4], 
               aoCtrlId->flatVect[aoCcdId->pixelsNb-3],
               aoCtrlId->flatVect[aoCcdId->pixelsNb-2], 
               aoCtrlId->flatVect[aoCcdId->pixelsNb-1]);
   }

   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       printf ( "subaperture %d: %f, %f\n" , i+1, aoCtrlId->refWfsVect[2*i],
                aoCtrlId->refWfsVect[2*i+1] );

   printf ( "X center = %f\n" , aoCtrlId->refGuideVect[0] );
   printf ( "Y center = %f\n" , aoCtrlId->refGuideVect[1] );

   printf ( "aO mode number: %d\n" , aoCtrlId->aoModeNb );
   printf ( "aO mode number not used: %d\n" , aoCtrlId->aoModeNotUsedNb );
   printf ( "aO mode number used: %d\n" , aoCtrlId->aoModeUsedNb );

   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       printf ( "aoModeUsedVector[%d] : %s\n" ,
                i+1, (aoCtrlId->aoModeUsedVect[i] ? "TRUE" : "FALSE") );

   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       printf ( "aoScaleFactorVect[%d]= %f\n", i,
                aoCtrlId->aoScaleFactorVect[i] );

   printf ( "FG mode number: %d\n" , aoCtrlId->fgModeNb );

   for ( i = 0 ; i < aoCtrlId->fgModeNb ; i ++ )
       printf ( "fgScaleFactorVect[%d]= %f\n", i,
                aoCtrlId->fgScaleFactorVect[i] );

   if ( verbose == TRUE )
   {
      (void) aoIntMatStructShow (aoCcdId, aoCtrlId);

      printf ( "Interaction matrix: \n" );
      for ( i = 0 ; i < aoCcdId->centroidsNb ; i ++ )
      {
          for ( j = 0 ; j < aoCtrlId->aoModeNb ; j ++ )
              printf ( "%f ", aoCtrlId->aoIntMat[i*aoCtrlId->aoModeNb + j] );
          printf ( "\n" );
      }

      printf ( "aO Control matrix: \n" );
      for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
      {
          for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
              printf ( "%f ", aoCtrlId->aoContMat[i*aoCcdId->centroidsNb + j] );
          printf ( "\n" );
      }

      printf ( "FG Control matrix: \n" );
      for ( i = 0 ; i < aoCtrlId->fgModeNb ; i ++ )
      {
          for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
              printf ( "%f " ,
                       aoCtrlId->fgContMat[i*aoCcdId->centroidsNb + j] );
          printf ( "\n" );
      }
   }

   printf ( "Threshold method: %d\n" , aoCtrlId->thresholdMethod );
   printf ( "Threshold: %f\n" , aoCtrlId->threshold );
   printf ( "Threshold dark (no bin): %f\n" , aoCtrlId->thresholdDarkFull );
   printf ( "Threshold dark (bin): %f\n" , aoCtrlId->thresholdDarkBin );
   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       printf ( "ThresholdVect[%d] = %f\n" , i , aoCtrlId->thresholdVect[i]);
   printf ( "Threshold rate: %f\n" , aoCtrlId->thresholdRate );
   printf ( "Threshold mult coeff: %f\n" , aoCtrlId->thresholdMultCoeff );
   printf ( "Average total counts method: %d\n" , aoCtrlId->totalMethod );
   printf ( "Average total counts: %f\n" , aoCtrlId->averageTotal );
   printf ( "Threshold for total counts: %f\n" , aoCtrlId->totalThreshold );
   printf ( "Total mult coeff : %f\n" , aoCtrlId->multCoeffTotal );
   printf ( "Angle with M2 (rad): %f\n" , aoCtrlId->angleWithM2 );
   printf ( "Cos Angle with M2: %f\n" , aoCtrlId->cosAngleWithM2 );
   printf ( "Sin Angle with M2: %f\n" , aoCtrlId->sinAngleWithM2 );
   printf ( "Angle with M1 (rad): %f\n" , aoCtrlId->angleWithM1 );
   printf ( "Cos Angle with M1: %f\n" , aoCtrlId->cosAngleWithM1 );
   printf ( "Sin Angle with M1: %f\n" , aoCtrlId->sinAngleWithM1 );
   printf ( "Sliding focus gain: %f\n" , aoCtrlId->slidingFocusGain );
   printf ( "One - Sliding focus gain: %f\n" , aoCtrlId->one_slidingFocusGain );
   printf ( "coaddCounter: %d\n" , aoCtrlId->coaddCounter );
   printf ( "focusCounter: %d\n" , aoCtrlId->focusCounter );
   printf ( "Allowed subapertures to be off: %d\n", aoCtrlId->allowedSubapOff);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoDarkSubtract
 *
 *   INVOCATION:
 *   aoDarkSubtract (pImage, pDark, xPixels, yPixels)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage  (float *) Pointer to the image from which frame is to be 
 *                         subtracted
 *   (>) pDark   (float *) Pointer to the dark to be subtracted
 *   (>) xPixels (int)     x dimension of the image
 *   (>) yPixels (int)     y dimension of the image
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To subtract a dark from an image of the same size, pixel by pixel
 *
 *   DESCRIPTION:
 *   Subtracts a dark pointed to by pDark from an image pointed to by pImage.
 *   The images are both assumed to be (xPixels, yPixels).
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoDarkSubtract (
   float * pImage,
   float * pDark,
   int     xPixels,
   int     yPixels
   )
{
   float * pi, * pd, * pMax;
   int imageSize;

   imageSize = xPixels * yPixels;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   pd = pDark;

   for ( pi = pImage ; pi < pMax ; pi ++ )
   {
       *pi = *(pi) - *(pd++);
   }

   return(OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoGlobalGuide
 *
 *   INVOCATION:
 *   aoGlobalGuide (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, pGuidesVect,
 *                  pFgVect, pFgVectAfterRot, pFgErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage           (float *)    Pointer to the image from which to 
 *                                     compute the centroids
 *   (>) aoCcdId          (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *                                     structure
 *   (>) aoCtrlId         (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect (double *)   Pointer to the total counts vector
 *   (<) pGuidesVect      (double *)   Pointer to the guides vector
 *   (<) pFgVect          (double *)   Pointer to the zernikes vector to send 
 *                                     to M2
 *   (<) pFgVectAfterRot  (double *)   Pointer to the zernikes vector to send 
 *                                     to M2 after rotation
 *   (<) pFgErrorsVect    (double *)   Pointer to the associated errors vector
 *   (<) pTime            (double *)   Pointer to the time associated to the
 *                                     vectors
 *   (<) pWfsStatus       (int *)      Pointer to the status flag when 
 *                                     computing the centroids 
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute tip and tilt modes only over the whole CCD - Associated errors 
 *   are not computed and are set to 0.
 *
 *   DESCRIPTION:
 *   This routine computes the centroids for the whole CCD, basically a tip and
 *   tilt information. This is very useful at the beginning of an observation 
 *   to acquire all the spots of the WFS and to have a global centering.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoGlobalGuide (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   double *     pTotalCountsVect,
   double *     pGuidesVect,
   double *     pFgVect,
   double *     pFgVectAfterRot,
   double *     pFgErrorsVect,
   double *     pTime,
   int *        pWfsStatus
   )
{
   int          imageSize;
   int          i, j;
   float *      pi;
   float *      pd;
   float *      pMax;
   double       total;
   double       x;
   double       y;
   double       pixelVal;
   double       xCenter;
   double       yCenter;
   double       *pTotal;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refGuideVect[0];
   yCenter = aoCtrlId->refGuideVect[1];

#ifdef DEBUG
   printf ( "aoGlobalGuide(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   pTotal = pTotalCountsVect + aoCcdId->subapUsedNb;

   /* Dark subtraction */

   for ( pi = pImage ; pi < pMax ; pi ++ )
       *pi = ( *pi - *(pd ++) );

   /* Thresholding and centroiding */

   total = 0;
   x = 0;
   y = 0;
   pi = pImage;
   for ( j = 1 ; j <= aoCcdId->yPixels ; j ++ )
   {
       for ( i = 1 ; i <= aoCcdId->xPixels ; i ++ )
       {
           pixelVal = (double)(*(pi + aoCcdId->xPixels*(j-1) + i-1)) - 
                      aoCtrlId->threshold;

           if ( pixelVal > (double)(0.0) )
           {
              x += pixelVal*i;
              y += pixelVal*j;
              total += pixelVal;
#ifdef DEBUG
              printf ( "aoGloabalGuide(): i=%d, j=%d, pixelVal=%f\n",
                       i , j, pixelVal );
              printf ( "aoGloabalGuide(): x=%f, y=%f, total=%f\n" ,
                       x, y, total );
#endif
           }
       }
   }

   *pTotal = total;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
#ifdef DEBUG
      printf ( "aoGlobalGuide(): x=%f, y=%f, total=%f\n" , x , y , total );
#endif

      *pWfsStatus = OK;

      *(pGuidesVect) = xCenter - (x / total);
      *(pGuidesVect + 1) = yCenter - (y / total);

/*
      *(pFgVect) = ( aoCtrlId->cosAngleWithM2 * (*pGuidesVect) + 
                     aoCtrlId->sinAngleWithM2 * (*(pGuidesVect+1)) );

      *(pFgVect + 1) = ( aoCtrlId->cosAngleWithM2 * (*(pGuidesVect+1)) -
                         aoCtrlId->sinAngleWithM2 * (*pGuidesVect) ); 
*/
      *(pFgVect) = (*pGuidesVect);

      *(pFgVect + 1) = *(pGuidesVect+1);

      *(pFgVect + 2) = 0.0;

      *(pFgErrorsVect) = 0.0;
      *(pFgErrorsVect + 1) = 0.0;
      *(pFgErrorsVect + 2) = 0.0; 

#ifdef DEBUG
      printf ( "aoGlobalGuide(): z[0]=%f, z[1]=%f\n" ,
               *pFgVect, *(pFgVect +1) );
#endif
   }
   else
   {
      *pWfsStatus = AO_SH_OFF;

      *(pGuidesVect) = 0.0;
      *(pGuidesVect + 1) = 0.0;

      *(pFgVect) = 0.0;
      *(pFgVect + 1) = 0.0;
      *(pFgVect + 2) = 0.0;

      *(pFgErrorsVect) = 0.0;
      *(pFgErrorsVect + 1) = 0.0;
      *(pFgErrorsVect + 2) = 0.0;
   }

   if ( timeNow (pTime) != OK )
   {
#ifdef DEBUG
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
#endif
      *pTime = (double)AO_TIME_NOW_ERROR;
   };

 
   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgVectAfterRot, pFgErrorsVect, 
                          pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoGlobalGuideAndError
 *
 *   INVOCATION:
 *   aoGlobalGuideAndError (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, 
 *                          pGuidesVect, pFgVect, pFgVectAfterRot, 
 *                          pFgErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage           (float *)    Pointer to the image from which to 
 *                                     compute the centroids
 *   (>) aoCcdId          (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                     context structure
 *   (>) aoCtrlId         (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect (double *)   Pointer to the total counts vector
 *   (<) pGuidesVect      (double *)   Pointer to the guides vector
 *   (<) pFgVect          (double *)   Pointer to the zernikes vector
 *   (<) pFgVectAfterRot  (double *)   Pointer to the zernikes vector after
 *                                     rotation
 *   (<) pFgErrorsVect    (double *)   Pointer to the associated errors 
 *                                     vector
 *   (<) pTime            (double *)   Pointer to the time associated to 
 *                                     the vectors
 *   (<) pWfsStatus       (int *)      Pointer to the status flag when 
 *                                     computing the centroids 
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute tip and tilt modes only over the whole CCD - Associated errors
 *   are also computed.
 *
 *   DESCRIPTION:
 *   This routine computes the centroids for the whole CCD, basically a tip and
 *   tilt information. This is very useful at the beginning of an observation 
 *   to acquire all the spots of the WFS and to have a global centering.
 *   Associated errors are computed based on the centroiding errors.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoGlobalGuideAndError (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   double *     pTotalCountsVect,
   double *     pGuidesVect,
   double *     pFgVect,
   double *     pFgVectAfterRot,
   double *     pFgErrorsVect,
   double *     pTime,
   int *        pWfsStatus
   )
{
   int          imageSize;
   int          i, j;
   float *      pi;
   float *      pd;
   float *      pMax;
   double       total;
   double       x;
   double       y;
   double       xTemp;
   double       yTemp;
   double       xErr;
   double       yErr;
   double       xSigma;
   double       ySigma;
   double       cos2;
   double       sin2;
   double       pixelVal;
   double       xCenter;
   double       yCenter;
   double       *pTotal;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refGuideVect[0];
   yCenter = aoCtrlId->refGuideVect[1];

#ifdef DEBUG
   printf ( "aoGlobalGuideAndError(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   cos2 = aoCtrlId->cosAngleWithM2 * aoCtrlId->cosAngleWithM2;
   sin2 = aoCtrlId->sinAngleWithM2 * aoCtrlId->sinAngleWithM2;

   pTotal = pTotalCountsVect + aoCcdId->subapUsedNb;

   /* Dark subtraction */

   for ( pi = pImage ; pi < pMax ; pi ++ )
       *pi = ( *pi - *(pd ++) );

   /* Thresholding and centroiding */

   total = 0;
   x = 0;
   y = 0;
   xErr = 0;
   yErr = 0;
   pi = pImage;
   for ( j = 1 ; j <= aoCcdId->yPixels ; j ++ )
   {
       for ( i = 1 ; i <= aoCcdId->xPixels ; i ++ )
       {
           pixelVal = (double)(*(pi + aoCcdId->xPixels*(j-1) + i-1)) - 
                      aoCtrlId->threshold;

           if ( pixelVal > (double)(0.0) )
           {
              xTemp = pixelVal*i;
              yTemp = pixelVal*j;
              x += xTemp;
              y += yTemp;
              xErr += xTemp*i;
              yErr += yTemp*j;
              total += pixelVal;
#ifdef DEBUG
              printf ( "aoGloabalGuide(): i=%d, j=%d, pixelVal=%f\n",
                       i , j, pixelVal );
              printf ( "aoGloabalGuide(): x=%f, y=%f, total=%f\n" ,
                       x, y, total );
#endif
           }
       }
   }

   *pTotal = total;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
#ifdef DEBUG
      printf ( "aoGlobalGuideAndError(): x=%f, y=%f, total=%f\n" , x , y , total );
#endif

      *pWfsStatus = OK;

      xTemp = (x / total);
      yTemp = (y / total);

      *(pGuidesVect) = xCenter - xTemp;
      *(pGuidesVect + 1) = yCenter - yTemp;
/*
      *(pFgVect) = ( aoCtrlId->cosAngleWithM2 * (*pGuidesVect) + 
                     aoCtrlId->sinAngleWithM2 * (*(pGuidesVect+1)) );

      *(pFgVect + 1) = ( aoCtrlId->cosAngleWithM2 * (*(pGuidesVect +1)) -
                         aoCtrlId->sinAngleWithM2 * (*pGuidesVect) ); 
*/
      *(pFgVect) = *(pGuidesVect) ;

      *(pFgVect + 1) = *(pGuidesVect +1);

      *(pFgVect + 2) = 0.0;

      xSigma = (((xErr / total) - (xTemp * xTemp))/total);
      ySigma = (((yErr / total) - (yTemp * yTemp))/total);

      if ( xSigma < AO_MIN_DOUBLE )
         xSigma = 0.0;
      if ( ySigma < AO_MIN_DOUBLE )
         ySigma = 0.0;

      *(pFgErrorsVect) = sqrt(cos2*xSigma + sin2*ySigma);
      *(pFgErrorsVect + 1) = sqrt(sin2*xSigma + cos2*ySigma);
      *(pFgErrorsVect + 2) = 0.0; 

#ifdef DEBUG
      printf ( "aoGlobalGuideAndError(): z[0]=%f, z[1]=%f\n" ,
               *pFgVect, *(pFgVect +1) );
#endif
   }
   else
   {
      *pWfsStatus = AO_SH_OFF;

      *(pGuidesVect) = 0.0;
      *(pGuidesVect + 1) = 0.0;

      *(pFgVect) = 0.0;
      *(pFgVect + 1) = 0.0;
      *(pFgVect + 2) = 0.0;

      *(pFgErrorsVect) = 0.0;
      *(pFgErrorsVect + 1) = 0.0;
      *(pFgErrorsVect + 2) = 0.0;
   }

   if ( timeNow (pTime) != OK )
   {
#ifdef DEBUG
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
#endif
      *pTime = (double)AO_TIME_NOW_ERROR;
   };

 
   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgVectAfterRot, 
                          pFgErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoImageFloatAverage
 *
 *   INVOCATION:
 *   aoImageFloatAverage (pImage, aoCcdId, aoCtrlId, imageNb)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage    (float *)    Pointer to the float buffer which contains the 
 *                              image to be coadded
 *   (>) aoCcdId   (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *   (>) imageNb   (int)        Number of images to average
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To average float images
 *
 *   DESCRIPTION:
 *   This routine coadds images into sumVect and when the number of images 
 *   coadded reaches imageNb, the sumVect buffer is averaged.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoImageFloatAverage (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   int          imageNb
   )
{
   int          imageSize;
   float *      p;
   float *      pi;
   float *      ps;
   float *      pMax;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   ps = aoCtrlId->sumVect;
   pMax = (float *)((int)ps + imageSize*sizeof(float));
  
   /* Coadd images */

   if ( aoCtrlId->coaddCounter == 0 )
   {
      for ( p = ps ; p < pMax ; )
      {
          *(p++) = *(pi++);
      }
      aoCtrlId->coaddCounter ++;
#ifdef DEBUG
      printf ( "Pixel[0]=%f, Sum[0]=%f\n" , *pImage, aoCtrlId->sumVect[0]);
#endif

   }
   else
   {
      if ( aoCtrlId->coaddCounter < imageNb )
      {
         for ( p = ps ; p < pMax ; p ++ )
         {
             *p = ( *(p) + *(pi++) );
         }
         aoCtrlId->coaddCounter ++;
#ifdef DEBUG
         printf ( "Pixel[0]=%f, Sum[0]=%f\n" , *pImage, aoCtrlId->sumVect[0]);
#endif

      }

      if  ( aoCtrlId->coaddCounter == imageNb )
      {
          for ( p = ps ; p < pMax; p ++ )
          {
               *p = (*(p) / imageNb);
          }
          aoCtrlId->coaddCounter = 0;
#ifdef DEBUG
          printf ( "Sum[0]=%f\n" , aoCtrlId->sumVect[0]);
#endif

      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoRmsNoiseImageCompute
 *
 *   INVOCATION:
 *   aoRmsNoiseImageCompute (pImage, aoCcdId, pRmsNoise) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage         (float *)    Pointer to the image from which to compute 
 *                                   the rms of the noise
 *   (>) aoCcdId        (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *                                   structure
 *   (<) pRmsNoise      (double *)   Pointer to the rms of the noise
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute the rms of the noise
 *
 *   DESCRIPTION:
 *   This routine computes for a dedicated image pImage the rms of the 
 *   noise. 
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoRmsNoiseImageCompute (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   double *     pRmsNoise
   )
{
   int          imageSize;
   float *      p;
   float *      pi;
   float *      pMax;
   double       value;
   double       meanPixel;
   double       variance;
   double       rmsrms;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   pMax = (float *)((int)pi + imageSize*sizeof(float));

   /* Compute mean and variance */

   meanPixel = 0.0;
   variance = 0.0;

   for ( p = pi ; p < pMax ; p ++ )
   {
       value = (double)(*p);

       meanPixel += value;
   
       variance += (value * value);
   }

   meanPixel = meanPixel / (double)(aoCcdId->pixelsNb);

   variance = variance / (double)(aoCcdId->pixelsNb);

   rmsrms = variance - (meanPixel*meanPixel);

   /* Compute the rms of the noise */

   if ( rmsrms < 0.0 )
   {
      ERROR_SET (0, "Variance of the noise is negative" , ERROR_LOG_SAVE);
      *pRmsNoise = 0.0;
      return (ERROR);
   }

   *pRmsNoise = sqrt ( rmsrms );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoThresholdCompute
 *
 *   INVOCATION:
 *   aoThresholdCompute (pImage, aoCcdId, ratePixel, pThreshold)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage         (float *)    Pointer to the image from which to compute 
 *                                   the centroids
 *   (>) aoCcdId        (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *                                   structure
 *   (>) ratePixel      (double)     Rate of the brightest pixels to determine 
 *                                   the threshold should between 0 and 1
 *   (<) pThreshold     (double *)   Pointer to the threshold value to compute
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute the threshold 
 *
 *   DESCRIPTION:
 *   This routine allows to compute the optimized threshold pThreshold from 
 *   a 4 spots image pImage according to the following criteria: ratePixel% 
 *   of the brightest pixels. 
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoThresholdCompute (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   double       ratePixel,
   double *     pThreshold
   )
{
   int          index;
   int          imageSize;
   int          gap;
   int          i, j;
   float        temp;
   float *      p;
   float *      pi;
   float *      pn;
   float *      pMax;
   IMAGE_VECT   newImageVect; 

   /* Check range of ratePixel: should be between 0 and 1 */

   if ( (ratePixel < 0.0) || (ratePixel > 1.0) )
   {
      ERROR_SET1 ( 0 , "ratePixel (%f) should be comprised between 0 and 1",
                   ERROR_LOG_SAVE, ratePixel );
      return (ERROR);
   }

   /* Some initializations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   pMax = (float *)((int)pi + imageSize*sizeof(float));
   pn = newImageVect;
   
   for ( p = pi ; p < pMax ; )
       *(pn ++) = *(p ++);
      
   /* Sort newImageVect */

   pn = newImageVect;
   for ( gap = aoCcdId->pixelsNb/2 ; gap > 0 ; gap /= 2 )
   {
       for ( i = gap ; i < aoCcdId->pixelsNb ; i ++ )
       {
           for ( j = i - gap ; j >= 0 && (*(pn+j)>*(pn+j+gap)) ; j -= gap)
           {
               temp = *(pn+j);
               *(pn+j) = *(pn+j+gap);
               *(pn+j+gap) = temp;
           }
       }
   }

   /* Determine the threshold: corresponds to ratePixel% of brightest pixels */

   index = (int) ceil ((double)(aoCcdId->pixelsNb) * (1.0 - ratePixel));
   printf ( "index = %d\n" ,index);

   *pThreshold = *(pn + index);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCentroidsCompute
 *
 *   INVOCATION:
 *   aoCentroidsCompute (pImage, aoCcdId, aoCtrlId, pTotalCountsVect,
 *                       pCentroidsVect, pErrorCentroidsVect, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage              (float *)    Pointer to the image from which to
 *                                        compute the centroids
 *   (>) aoCcdId             (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId            (AO_CTRL_ID) Pointer to the AO control structure
 *   (!) pTotalCountsVect    (double *)   Pointer to the total counts vector
 *   (!) pCentroidsVect      (double *)   Pointer to the centroids vector
 *   (!) pErrorCentroidsVect (double *)   Pointer to the error centroids vector
 *   (!) pWfsStatus          (int *)      Pointer to the wfs status when
 *                                        computing the centroids
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute the centroids
 *
 *   DESCRIPTION:
 *   This routine computes only the centroids of the image given by pImage.
 *   The dark subtraction is done elsewhere. The centroids are stored into
 *   pCentroidsVect and no associated errors are computed. The
 *   pErrorCentroidsVect elements are set to 0.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCentroidsCompute (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   double *     pTotalCountsVect,
   double *     pCentroidsVect,
   double *     pErrorCentroidsVect,
   int *        pWfsStatus
   )
{
   int          i, j;
   int          k, l;
   int          m;
   int          subapNb;
   int          subapOffNb;
   float *      pi;
   float *      pMin;
   float *      pMax;
   double       thresh;
   double       totalSubap;
   double       total;
   double       xSubap;
   double       ySubap;
   double       pixelVal;
   double       xSubapCenter;
   double       ySubapCenter;

   m = 0;
   subapOffNb = 0;
   total = (double)0.0;
   *pWfsStatus = OK;
   for ( k = 0 ; k < 2 * aoCcdId->ySubapNb ; k ++ )
   {
       for ( l = 0 ; l < 2 * aoCcdId->xSubapNb ; l ++ )
       {
           subapNb = 2*k*aoCcdId->xSubapNb + l;

           if ( aoCcdId->subapUsedVect[subapNb] == TRUE)
           {
#ifdef DEBUG
              printf ( "subaperture NB = %d is used\n" , subapNb );
#endif

              xSubap = (double)(0.0);
              ySubap = (double)(0.0);
              totalSubap = (double)(0.0);
              thresh = aoCtrlId->thresholdVect[m];

              xSubapCenter = aoCtrlId->refWfsVect[2*m] - aoCcdId->xRaster*l;
              ySubapCenter = aoCtrlId->refWfsVect[2*m+1] -
                             aoCcdId->yRaster*k;

#ifdef DEBUG
              printf ( "xSubapCenter = %f, ySubapCenter= %f\n" ,
                       xSubapCenter, ySubapCenter );
#endif
              for ( i = 1 ; i <= aoCcdId->yRaster ; i ++ )
              {
                  j = 1;
                  pMin = pImage + ((i-1)*aoCcdId->xPixels) +
                         (l*aoCcdId->xRaster) +
                         (k * aoCcdId->xPixels * aoCcdId->yRaster);
                  pMax = pMin + aoCcdId->xRaster;

                  for ( pi = pMin ; pi < pMax ; pi ++)
                  {
                      /*pixelVal = (double)(*pi) - aoCtrlId->threshold;*/
                      pixelVal = (double)(*pi) - thresh;
                      if ( pixelVal > (double)(0.0) )
                      {
                         xSubap += pixelVal*j;
                         ySubap += pixelVal*i;
                         totalSubap += pixelVal;
#ifdef DEBUG
                         printf ( "SUBAP%d i=%d, j=%d, x=%f, y=%f, total=%f\n",
                                  subapNb, i, j, xSubap, ySubap, totalSubap);
#endif
                      }
                      j ++;
                  }
              }

              *(pTotalCountsVect + m) = totalSubap;

              if ( totalSubap > 0.0 )
              {
                 *(pCentroidsVect + 2*m) = (xSubap/totalSubap) - xSubapCenter;
                 *(pCentroidsVect + 2*m + 1) =
                 (ySubap/totalSubap) - ySubapCenter;

                 *(pErrorCentroidsVect + 2*m) = 0.0;
                 *(pErrorCentroidsVect + 2*m + 1) = 0.0;
#ifdef DEBUG
                 printf ( "SUBAP%d centX=%f, centY=%f\n",
                          subapNb, *(pCentroidsVect + 2*m),
                          *(pCentroidsVect + 2*m + 1));
#endif
              }
              else
              {
                 *pWfsStatus = AO_SUBAP_OFF;
                 subapOffNb ++;
                 *(pCentroidsVect + 2*m) = (double)(0.0);
                 *(pCentroidsVect + 2*m + 1) = (double)(0.0);

                 *(pErrorCentroidsVect + 2*m) = 0.0;
                 *(pErrorCentroidsVect + 2*m + 1) = 0.0;
#ifdef DEBUG
                 printf ( "SUBAP%d OFF centX=%f, centY=%f\n",
                          subapNb, *(pCentroidsVect + 2*m),
                          *(pCentroidsVect + 2*m + 1));
#endif
              }

              total += totalSubap;
#ifdef DEBUG
              printf ( "TOTAL =%f\n", total);
#endif
              m ++;
           }
           else
           {
#ifdef DEBUG
              printf ( "subaperture NB = %d is not used\n" , subapNb );
#endif
           }
       }
   }

   *(pTotalCountsVect + m) = total;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
      if ( *pWfsStatus == AO_SUBAP_OFF)
      {
         if ( subapOffNb > aoCtrlId->allowedSubapOff )
         {
            *pWfsStatus = AO_SH_OFF;
         };
      };
   }
   else
   {
      *pWfsStatus = AO_SH_OFF;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModeCompute
 *
 *   INVOCATION:
 *   aoModeCompute (pImage, aoCcdId, aoCtrlId, imageNb, aoCbAoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage    (float *)    Pointer to the float buffer which contains the
 *                              image to be coadded
 *   (>) aoCcdId   (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *   (>) imageNb   (int)        Number of images to average
 *   (!) aoCbAoCtrlId (AO_CB_AO_CTRL_ID) Pointer to the aO control circular 
 *                                       buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute aO modes
 *
 *   DESCRIPTION:
 *   First, this routine coadds images into sumVect and when the number of
 *   images coadded reaches imageNb, the sumVect buffer is averaged. Then the
 *   centroids of this average image are computed and multiplied by the control
 *   matrix and by the scale factors to give the aO modes and all these data are
 *   saved into the aoCbAoCtrlId circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModeCompute (
   float *          pImage,
   AO_CCD_ID        aoCcdId,
   AO_CTRL_ID       aoCtrlId,
   int              imageNb,
   AO_CB_AO_CTRL_ID aoCbAoCtrlId
   )
{
   int          imageSize;
   int          indexCtrl;
   int *        pWfsStatus;
   float *      p;
   float *      pi;
   float *      ps;
   float *      pMax;
   double *     pTotalCountsVect;
   double *     pCentroidsVect;
   double *     pErrorCentroidsVect;
   double *     pAoVect;
   double *     pAoVectAfterRot;
   double *     pAoErrorsVect;
   double *     pAo;
   double *     pErrorAo;
   double *     pCent;
   double *     pMaxAo;
   double *     pMaxCent;
   double *     pMat;
   double *     pTime;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   ps = aoCtrlId->sumVect;
   pMax = (float *)((int)ps + imageSize*sizeof(float));

   /* Coadd images */

   if ( aoCtrlId->coaddCounter < imageNb )
   {
      if ( aoCtrlId->coaddCounter == 0 )
      {
         for ( p = ps ; p < pMax ; )
         {
             *(p++) = *(pi++);
         }
      }
      else
      {
         for ( p = ps ; p < pMax ; p ++ )
         {
             *p = ( *(p) + *(pi++) );
         }
      }
      aoCtrlId->coaddCounter ++;

      if  ( aoCtrlId->coaddCounter == imageNb )
      {
          for ( p = ps ; p < pMax; p ++ )
          {
               *p = (*(p) / imageNb);
          }
          aoCtrlId->coaddCounter = 0;

         /* Compute the centroids */

         indexCtrl = aoCbAoCtrlId->position;
         pTotalCountsVect = 
         aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].totalCountsVect;
         pCentroidsVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].centroidsVect;
         pErrorCentroidsVect =
         aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].errorCentroidsVect;
         pAoVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoVect;
         pAoVectAfterRot = 
         aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoVectAfterRot;
         pAoErrorsVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoErrorsVect;
         pWfsStatus = &(aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].wfsStatus);
         pTime = &(aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].time);

         pErrorAo = pAoErrorsVect;
         pMaxAo = pAoVect + aoCtrlId->aoModeNb;
         pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;
         pMat = aoCtrlId->aoContMat;

         for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
             *pAo = 0.0;

         if ( aoCentroidsCompute ( aoCtrlId->sumVect, aoCcdId, aoCtrlId,
                                   pTotalCountsVect,
                                   pCentroidsVect, pErrorCentroidsVect,
                                   pWfsStatus) == ERROR )
         {
            ERROR_SET (0, "Error when computing centroids" , ERROR_LOG_SAVE);
            return (ERROR);
         }

         /* Apply the control matrix and the scale factor */

         if ( *pWfsStatus != AO_SH_OFF )
         {
            for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
                for ( pCent = pCentroidsVect ; pCent < pMaxCent ; )
                    *pAo += (*(pMat ++)) * (*(pCent ++));

            if ( aoCcdId->binningFlag == TRUE )
            {
               for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
               {
                   *pAo *= (2.0);
                   *(pErrorAo ++) = 0.0;
               }
            }
            else
            {
               for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
               {
                   *(pErrorAo ++) = 0.0;
               }
            }
         }
         else
         {
            for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
            {
                *pAo = 0.0;
                *(pErrorAo ++) = 0.0;
            }
         }

         /* Get a timestamp to record at which time ao data are sent to TCS */

         if ( timeNow (pTime) != OK )
         {
#ifdef DEBUG
            ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
#endif
            *pTime = (double)AO_TIME_NOW_ERROR;
         };

         if ( writeWfsToTcs(aoCtrlId, pAoVect, pAoVectAfterRot, pAoErrorsVect,
                            pTime) != OK )
         {
            ERROR_SET ( 0, "Failed to write data to the TCS", ERROR_LOG_SAVE);
            return (ERROR);
         };

         /* Update the aoCbAoCtrlId circular buffer */

         if ( ++ aoCbAoCtrlId->position == CB_AO_CTRL_RECORD_NB )
         {
            aoCbAoCtrlId->position = 0;
            aoCbAoCtrlId->counter ++;
         }
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbImSave
 *
 *   INVOCATION:
 *   aoCbImSave (pCbImFilePath, aoCcdId, aoCtrlId, aoCbImId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbImFilePath (char *)      Directory where to save the circular buffer
 *                                   image
 *   (>) aoCcdId       (AO_CCD_ID)   Pointer to the CCD geometry context 
 *                                   structure
 *   (>) aoCtrlId      (AO_CTRL_ID)  Pointer to the control context structure
 *   (>) aoCbImId      (AO_CB_IM_ID) Pointer to the image circular buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Save the image circular buffer.
 *
 *   DESCRIPTION:
 *   This routine save the contents of the image circular buffer into a file
 *   for further purposes. The contents of the circular buffer is sorted in
 *   order to save the older record in first and to have the more recent one 
 *   at the end.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbImSave
   (
   char *          pCbImFilePath,   /* Image circular buffer directory        */
   AO_CCD_ID       aoCcdId,         /* Pointer to the CCD geometry structure  */
   AO_CTRL_ID      aoCtrlId,        /* Pointer to the control structure       */
   AO_CB_IM_ID     aoCbImId         /* Pointer to the image circular buffer   */
   )
{
   int                       i;
   int                       index;
   int                       defNameFlag; 
   int                       itemNb; 
   int                       timeArray[7]; 
   double                    timeSave;
   AO_HEADER_CB_IM_ID_STRUCT aoHeaderCbIm;

   FILE *          pFile;

   /*
    * Create the file name where to save the circular buffer
    */

   defNameFlag = FALSE;

   if ( timeNow (&timeSave) != OK )
   {
      ERROR_SET (0, "Failed to get time to determine name of cb save files",
                 ERROR_LOG_NOW);

      defNameFlag = TRUE;
   }

   if (timeThenC (timeSave, UT1, 2, timeArray) != OK)
   {
      ERROR_SET (0, "Failed to convert raw time into date and time", 
                 ERROR_LOG_NOW);
      defNameFlag = TRUE;
   }

   if ( defNameFlag != TRUE )
   {
      if ( ( strcmp (pCbImFilePath, "") == 0 ) || 
           ( strcmp (pCbImFilePath, "NONE") == 0 ) )
      {
         sprintf ( aoHeaderCbIm.cbImFileName, 
                   "./D%04d%02d%02dT%02d%02d%02dP2.cbi",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5] );
      }
      else
      {
         sprintf ( aoHeaderCbIm.cbImFileName, 
                   "%s/D%04d%02d%02dT%02d%02d%02dP2.cbi",
                   pCbImFilePath, timeArray[0], timeArray[1], timeArray[2], 
                   timeArray[3], timeArray[4], timeArray[5] );
      }
   }
   else
   {
      if ( ( strcmp (pCbImFilePath, "") == 0 ) || 
           ( strcmp (pCbImFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbIm.cbImFileName, "./defaultP2.cbi" );
      }
      else
      {
         sprintf ( aoHeaderCbIm.cbImFileName, "%s/defaultP2.cbi" ,
                   pCbImFilePath );
      }
   }

#ifdef DEBUG
   printf ( "File name: %s\n" , aoHeaderCbIm.cbImFileName );
#endif

   /* 
    * Init the Header of the file
    */

   aoHeaderCbIm.processingMode = aoCbImId->processingMode;
   aoHeaderCbIm.exposureTime = aoCbImId->exposureTime;

   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoHeaderCbIm.darkVect[i] = aoCtrlId->darkVect[i]; 

   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoHeaderCbIm.flatVect[i] = aoCtrlId->flatVect[i]; 
   
   index = aoCbImId->position;

   if ( aoCbImId->counter == 0 )
   {
      aoHeaderCbIm.recordNb = index;
   }
   else
   {
      aoHeaderCbIm.recordNb = CB_IM_RECORD_NB;
   }

   /* 
    * Open the image circular buffer save file
    */

   pFile = fopen ( aoHeaderCbIm.cbImFileName, "w" );

   if ( pFile == (FILE *)(NULL) )
   {
      ERROR_SET1 (0, "Failed to open in write mode file %s", 
                 ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
   }
   else
   {
      /* 
       * First save the header
       */

      itemNb = fwrite ( (char *)&aoHeaderCbIm, 
                        sizeof (char), 
                        sizeof (AO_HEADER_CB_IM_ID_STRUCT), 
                        pFile); 

      if ( itemNb == NULL )
      {
         ERROR_SET1 (0, "Failed to write header in file %s",
                     ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
         (void)fclose (pFile);
         return (ERROR);
      }
     
#ifdef DEBUG
      printf ( "Save the header done \n");
#endif

      /*
       * Now save the CCD geometry stucture
       */

      itemNb = fwrite ( (char *)aoCcdId, 
                        sizeof (char), 
                        sizeof (AO_CCD_ID_STRUCT), 
                        pFile); 

      if ( itemNb == NULL )
      {
         ERROR_SET1 (0, "Failed to write CCD geometry structure in file %s",
                     ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
         (void)fclose (pFile);
         return (ERROR);
      }
     
#ifdef DEBUG
      printf ( "Save the CCD geometry structure done \n");
#endif

      /*
       * Now save the records
       */

#ifdef DEBUG
      printf ( "index: %d, counter: %d\n", index, aoCbImId->counter);
#endif
      if ( aoCbImId->counter == 0 )
      {
#ifdef DEBUG
         printf ( "Counter= 0 -> save records from 0 to %d\n", index - 1);
#endif 
         for ( i = 0 ; i < index ; i ++ )
         {
             itemNb = fwrite ( (char *)&(aoCbImId->cbImRecord[i]), 
                               sizeof (char), 
                               sizeof (CB_IM_RECORD_STRUCT), 
                               pFile); 

             if ( itemNb == NULL )
             {
                ERROR_SET1 (0, "Failed to write record in file %s",
                            ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
                (void)fclose (pFile);
                return (ERROR);
             }
         }
      }
      else
      {
         if ( index != 0 )
         {
#ifdef DEBUG
            printf ( "Counter# 0, index # 0 -> save records from %d to %d\n", 
                     index , CB_IM_RECORD_NB - 1 );
#endif 
            for ( i = index ; i < CB_IM_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbImId->cbImRecord[i]), 
                                  sizeof (char), 
                                  sizeof (CB_IM_RECORD_STRUCT), 
                                  pFile); 

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }

#ifdef DEBUG
            printf ( "then -> save records from 0 to %d\n", index - 1 );
#endif
            for ( i = 0 ; i < index ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbImId->cbImRecord[i]), 
                                  sizeof (char), 
                                  sizeof (CB_IM_RECORD_STRUCT), 
                                  pFile); 

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
         else
         {
#ifdef DEBUG
            printf ( "Counter# 0, index = 0 -> save records from 0 to %d\n", 
                     CB_IM_RECORD_NB - 1 );
#endif 
            for ( i = 0 ; i < CB_IM_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbImId->cbImRecord[i]), 
                                  sizeof (char), 
                                  sizeof (CB_IM_RECORD_STRUCT), 
                                  pFile); 

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbIm.cbImFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
      }

      (void) fclose (pFile);
   }

#ifdef DEBUG
   printf ( "Save image cb done\n" );
#endif
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbImZero
 *
 *   INVOCATION:
 *   aoCbImZero (aoCbImId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCbImId (AO_CB_IM_ID)  Pointer to the image circular buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the image circular buffer.
 *
 *   DESCRIPTION:
 *   This routine set to zero all the records of the image circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbImZero
   (
   AO_CB_IM_ID   aoCbImId     /* Pointer to the image circular buffer */
   )
{
   int           index;
   int           i;

   for ( index = 0 ; index < CB_IM_RECORD_NB ; index ++ )
   {
       aoCbImId->cbImRecord[index].imageStatus = 0;
       for ( i = 0 ; i < CCD_SIZE ; i ++ )
           aoCbImId->cbImRecord[index].imageVect[i] = 0.0;
   }
  
   aoCbImId->exposureTime = 0.0;
   aoCbImId->processingMode = 0;
   aoCbImId->position = 0;
   aoCbImId->offset = 0;
   aoCbImId->counter = 0;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbAoCtrlZero
 *
 *   INVOCATION:
 *   aoCbAoCtrlZero (aoCbAoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCbAoCtrlId (AO_CB_AO_CTRL_ID) Pointer to the aO control circular 
 *                                       buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the aO control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine set to zero all the records of the aO control circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbAoCtrlZero
   (
   AO_CB_AO_CTRL_ID aoCbAoCtrlId /* Pointer to the aO control circular buffer */
   )
{
   int             index;
   int             i;

   for ( index = 0 ; index < CB_AO_CTRL_RECORD_NB ; index ++ )
   {
       aoCbAoCtrlId->cbAoCtrlRecord[index].time = 0.0;
       aoCbAoCtrlId->cbAoCtrlRecord[index].wfsStatus = 0;
       for ( i = 0 ; i < 2*SUBAP_NB ; i ++ )
       {
           aoCbAoCtrlId->cbAoCtrlRecord[index].totalCountsVect[i] = 0.0;
           aoCbAoCtrlId->cbAoCtrlRecord[index].centroidsVect[i] = 0.0;
           aoCbAoCtrlId->cbAoCtrlRecord[index].errorCentroidsVect[i] = 0.0;
       }
       for ( i = 0 ; i < AO_MODE_NB ; i ++ )
       {
           aoCbAoCtrlId->cbAoCtrlRecord[index].aoVect[i] = 0.0;
           aoCbAoCtrlId->cbAoCtrlRecord[index].aoVectAfterRot[i] = 0.0;
           aoCbAoCtrlId->cbAoCtrlRecord[index].aoErrorsVect[i] = 0.0;
       }
   }

   aoCbAoCtrlId->exposureTime = 0.0;
   aoCbAoCtrlId->processingMode = 0;
   aoCbAoCtrlId->averageImageNb = 0;
   aoCbAoCtrlId->position = 0;
   aoCbAoCtrlId->offset = 0;
   aoCbAoCtrlId->counter = 0;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbFgCtrlZero
 *
 *   INVOCATION:
 *   aoCbFgCtrlZero (aoCbFgCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCbFgCtrlId (AO_CB_FG_CTRL_ID)  Pointer to the FG control circular 
 *                                        buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the FG control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine set to zero all the records of the FG control circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbFgCtrlZero
   (
   AO_CB_FG_CTRL_ID aoCbFgCtrlId /* Pointer to the FG control circular buffer */
   )
{
   int             index;
   int             i;

   for ( index = 0 ; index < CB_FG_CTRL_RECORD_NB ; index ++ )
   {
       aoCbFgCtrlId->cbFgCtrlRecord[index].time = 0.0;
       aoCbFgCtrlId->cbFgCtrlRecord[index].wfsStatus = 0;
       for ( i = 0 ; i < 2*SUBAP_NB ; i ++ )
       {
           aoCbFgCtrlId->cbFgCtrlRecord[index].totalCountsVect[i] = 0.0;
           aoCbFgCtrlId->cbFgCtrlRecord[index].centroidsVect[i] = 0.0;
           aoCbFgCtrlId->cbFgCtrlRecord[index].errorCentroidsVect[i] = 0.0;
       }
       for ( i = 0 ; i < 2 ; i ++ )
       {
           aoCbFgCtrlId->cbFgCtrlRecord[index].guidesVect[i] = 0.0;
           aoCbFgCtrlId->cbFgCtrlRecord[index].errorGuidesVect[i] = 0.0;
       }
       for ( i = 0 ; i < FG_MODE_NB ; i ++ )
       {
           aoCbFgCtrlId->cbFgCtrlRecord[index].fgVect[i] = 0.0;
           aoCbFgCtrlId->cbFgCtrlRecord[index].fgVectAfterRot[i] = 0.0;
           aoCbFgCtrlId->cbFgCtrlRecord[index].fgErrorsVect[i] = 0.0;
       }
   }
  
   aoCbFgCtrlId->exposureTime = 0.0;
   aoCbFgCtrlId->processingMode = 0;
   aoCbFgCtrlId->position = 0;
   aoCbFgCtrlId->offset = 0;
   aoCbFgCtrlId->counter = 0;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbAoCtrlSave
 *
 *   INVOCATION:
 *   aoCbAoCtrlSave (pCbAoCtrlFilePath, aoCcdId, aoCtrlId, aoCbAoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbAoCtrlFilePath (char *)           Directory where to save the aO 
 *                                            circular buffer control
 *   (>) aoCcdId           (AO_CCD_ID)        Pointer to the CCD geometry 
 *                                            structure
 *   (>) aoCtrlId          (AO_CTRL_ID)       Pointer to the control context
 *                                            structure
 *   (>) aoCbAoCtrlId      (AO_CB_AO_CTRL_ID) Pointer to the aO control 
 *                                            circular buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Save the aO control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine save the contents of the aO control circular buffer into a 
 *   file for further purposes. The contents of the circular buffer is sorted in
 *   order to save the older record in first and to have the more recent ones
 *   at the end.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbAoCtrlSave
   (
   char *           pCbAoCtrlFilePath, /* aO control circular buffer directory*/
   AO_CCD_ID        aoCcdId,           /* Pointer to the CCD geometry         */
                                       /* structure                           */
   AO_CTRL_ID       aoCtrlId,          /* Pointer to the control structure    */
   AO_CB_AO_CTRL_ID aoCbAoCtrlId       /* Pointer to the aO control circular  */
                                       /* buffer                              */
   )
{
   int                            i;
   int                            index;
   int                            defNameFlag;
   int                            itemNb;
   int                            timeArray[7];
   double                         timeSave;

   AO_HEADER_CB_AO_CTRL_ID_STRUCT aoHeaderCbAoCtrl;

   FILE *                         pFile;

   /*
    * Create the file name where to save the aO circular buffer
    */

   defNameFlag = FALSE;

   if ( timeNow (&timeSave) != OK )
   {
      ERROR_SET (0, "Failed to get time to determine name of cb save files",
                 ERROR_LOG_NOW);

      defNameFlag = TRUE;
   }

   if (timeThenC (timeSave, UT1, 2, timeArray) != OK)
   {
      ERROR_SET (0, "Failed to convert raw time into date and time",
                 ERROR_LOG_NOW);
      defNameFlag = TRUE;
   }

   if ( defNameFlag != TRUE )
   {
      if ( ( strcmp (pCbAoCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbAoCtrlFilePath, "NONE") == 0 ) )
      {
         sprintf ( aoHeaderCbAoCtrl.cbAoCtrlFileName,
                   "./D%04d%02d%02dT%02d%02d%02dP2.cbcao",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5]);
      }
      else
      {
         sprintf ( aoHeaderCbAoCtrl.cbAoCtrlFileName,
                   "%s/D%04d%02d%02dT%02d%02d%02dP2.cbcao",
                   pCbAoCtrlFilePath, timeArray[0], timeArray[1], timeArray[2],
                   timeArray[3], timeArray[4], timeArray[5]);
      }
   }
   else
   {
      if ( ( strcmp (pCbAoCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbAoCtrlFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbAoCtrl.cbAoCtrlFileName, "./defaultP2.cbcao" );
      }
      else
      {
         sprintf ( aoHeaderCbAoCtrl.cbAoCtrlFileName, "%s/defaultP2.cbcao",
                   pCbAoCtrlFilePath );
      }
   }

#ifdef DEBUG
   printf ( "File name: %s\n" , aoHeaderCbAoCtrl.cbAoCtrlFileName );
#endif

#ifdef DEBUG
   printf ( "File name: %s\n" , aoHeaderCbAoCtrl.cbAoCtrlFileName );
#endif

   /*
    * Init the header of the file
    */

   index = aoCbAoCtrlId->position;

   if ( aoCbAoCtrlId->counter == 0 )
   {
      aoHeaderCbAoCtrl.recordNb = index;
   }
   else
   {
      aoHeaderCbAoCtrl.recordNb = CB_AO_CTRL_RECORD_NB;
   }

   aoHeaderCbAoCtrl.processingMode = aoCbAoCtrlId->processingMode;
   aoHeaderCbAoCtrl.centroidsNb = aoCcdId->centroidsNb;
   aoHeaderCbAoCtrl.aoModeNb = aoCtrlId->aoModeNb;
   aoHeaderCbAoCtrl.averageImageNb = aoCbAoCtrlId->averageImageNb;
   aoHeaderCbAoCtrl.exposureTime = aoCbAoCtrlId->exposureTime;
   for ( i = 0 ; i < aoCcdId->centroidsNb ; i ++ )
       aoHeaderCbAoCtrl.refWfsVect[i] = aoCtrlId->refWfsVect[i];
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       aoHeaderCbAoCtrl.aoScaleFactorVect[i] = aoCtrlId->aoScaleFactorVect[i];
   aoHeaderCbAoCtrl.threshold = aoCtrlId->threshold;
   aoHeaderCbAoCtrl.totalThreshold = aoCtrlId->totalThreshold;
   aoHeaderCbAoCtrl.angleWithM1 = aoCtrlId->angleWithM1;

   /*
    * Open the image circular buffer
    */

   pFile = fopen ( aoHeaderCbAoCtrl.cbAoCtrlFileName, "w" );

   if ( pFile == (FILE *)(NULL) )
   {
      ERROR_SET1 (0, "Failed to open in write mode file %s",
                 ERROR_LOG_NOW, aoHeaderCbAoCtrl.cbAoCtrlFileName);
   }
   else
   {
      /*
       * First save the header of the file
       */

      itemNb = fwrite ( (char *)&aoHeaderCbAoCtrl,
                        sizeof (char),
                        sizeof (AO_HEADER_CB_AO_CTRL_ID_STRUCT),
                        pFile);

      if ( itemNb == NULL )
      {
         ERROR_SET1 (0, "Failed to write header in file %s",
                     ERROR_LOG_NOW, aoHeaderCbAoCtrl.cbAoCtrlFileName);
         (void)fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "Save header cb done\n" );
#endif
      /*
       * Now save the records
       */
#ifdef DEBUG
      printf ( "index: %d, counter: %d\n", index, aoCbAoCtrlId->counter);
#endif
      if ( aoCbAoCtrlId->counter == 0 )
      {
#ifdef DEBUG
         printf ( "Counter= 0 -> save records from 0 to %d\n", index - 1);
#endif
         for ( i = 0 ; i < index ; i ++ )
         {
             itemNb = fwrite ( (char *)&(aoCbAoCtrlId->cbAoCtrlRecord[i]),
                               sizeof (char),
                               sizeof (CB_AO_CTRL_RECORD_STRUCT),
                               pFile);

             if ( itemNb == NULL )
             {
                ERROR_SET1 (0, "Failed to write record in file %s",
                            ERROR_LOG_NOW, aoHeaderCbAoCtrl.cbAoCtrlFileName);
                (void)fclose (pFile);
                return (ERROR);
             }
         }
      }
      else
      {
         if ( index != 0 )
         {
#ifdef DEBUG
            printf ( "Counter# 0, index # 0 -> save records from %d to %d\n",
                     index , CB_AO_CTRL_RECORD_NB - 1 );
#endif
            for ( i = index ; i < CB_AO_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbAoCtrlId->cbAoCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_AO_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, 
                               aoHeaderCbAoCtrl.cbAoCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }

#ifdef DEBUG
            printf ( "then -> save records from 0 to %d\n", index - 1 );
#endif
            for ( i = 0 ; i < index ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbAoCtrlId->cbAoCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_AO_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, 
                               aoHeaderCbAoCtrl.cbAoCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
         else
         {
#ifdef DEBUG
            printf ( "Counter# 0, index = 0 -> save records from 0 to %d\n",
                     CB_AO_CTRL_RECORD_NB - 1 );
#endif
            for ( i = 0 ; i < CB_AO_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbAoCtrlId->cbAoCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_AO_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, 
                               aoHeaderCbAoCtrl.cbAoCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
      }

      (void) fclose (pFile);
   }

#ifdef DEBUG
   printf ( "Save ao ctrl cb done\n" );
#endif
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbFgCtrlSave
 *
 *   INVOCATION:
 *   aoCbFgCtrlSave (pCbFgCtrlFilePath, aoCcdId, aoCtrlId, aoCbFgCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbFgCtrlFilePath (char *)      Directory where to save the circular
 *                                       buffer FG control
 *   (>) aoCcdId      (AO_CCD_ID)        Pointer to the CCD geometry structure
 *   (>) aoCtrlId     (AO_CTRL_ID)       Pointer to the control context 
 *                                       structure
 *   (>) aoCbFgCtrlId (AO_CB_FG_CTRL_ID) Pointer to the FG control circular
 *                                       buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Save the FG control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine save the contents of the FG control circular buffer into a
 *   file for further purposes. The contents of the circular buffer is sorted in
 *   order to save the older record in first and to have the more recent ones
 *   at the end.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbFgCtrlSave
   (
   char *           pCbFgCtrlFilePath, /* FG Control circular buffer directory*/
   AO_CCD_ID        aoCcdId,      /* Pointer to the CCD geometry structure    */
   AO_CTRL_ID       aoCtrlId,     /* Pointer to the control structure         */
   AO_CB_FG_CTRL_ID aoCbFgCtrlId  /* Pointer to the FG control circular buffer*/
   )
{
   int                            i;
   int                            index;
   int                            defNameFlag;
   int                            itemNb;
   int                            timeArray[7];
   double                         timeSave;

   AO_HEADER_CB_FG_CTRL_ID_STRUCT aoHeaderCbFgCtrl;

   FILE *                         pFile;

   /*
    * Create the file name where to save the FG circular buffer
    */

   defNameFlag = FALSE;

   if ( timeNow (&timeSave) != OK )
   {
      ERROR_SET (0, "Failed to get time to determine name of cb save files",
                 ERROR_LOG_NOW);

      defNameFlag = TRUE;
   }

   if (timeThenC (timeSave, UT1, 2, timeArray) != OK)
   {
      ERROR_SET (0, "Failed to convert raw time into date and time",
                 ERROR_LOG_NOW);
      defNameFlag = TRUE;
   }

   if ( defNameFlag != TRUE )
   {
      if ( ( strcmp (pCbFgCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbFgCtrlFilePath, "NONE") == 0 ) )
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName,
                   "./D%04d%02d%02dT%02d%02d%02dP2.cbfgc",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5]);
      }
      else
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName,
                   "%s/D%04d%02d%02dT%02d%02d%02dP2.cbfgc",
                   pCbFgCtrlFilePath, timeArray[0], timeArray[1], timeArray[2],
                   timeArray[3], timeArray[4], timeArray[5]);
      }
   }
   else
   {
      if ( ( strcmp (pCbFgCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbFgCtrlFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbFgCtrl.cbFgCtrlFileName, "./defaultP2.cbfgc" );
      }
      else
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName, "%s/defaultP2.cbfgc",
                   pCbFgCtrlFilePath );
      }
   }

#ifdef DEBUG
   printf ( "File name: %s\n" , aoHeaderCbFgCtrl.cbFgCtrlFileName );
#endif

   /*
    * Init the header of the file
    */

   index = aoCbFgCtrlId->position;

   if ( aoCbFgCtrlId->counter == 0 )
   {
      aoHeaderCbFgCtrl.recordNb = index;
   }
   else
   {
      aoHeaderCbFgCtrl.recordNb = CB_FG_CTRL_RECORD_NB;
   }

   aoHeaderCbFgCtrl.processingMode = aoCbFgCtrlId->processingMode;
   aoHeaderCbFgCtrl.centroidsNb = aoCcdId->centroidsNb;
   aoHeaderCbFgCtrl.guidesNb = 2;
   aoHeaderCbFgCtrl.fgModeNb = aoCtrlId->fgModeNb;
   aoHeaderCbFgCtrl.exposureTime = aoCbFgCtrlId->exposureTime;

   for ( i = 0 ; i < 2 ; i ++ )
       aoHeaderCbFgCtrl.refGuideVect[i] = aoCtrlId->refGuideVect[i];

   for ( i = 0 ; i < aoCcdId->centroidsNb ; i ++ )
       aoHeaderCbFgCtrl.refWfsVect[i] = aoCtrlId->refWfsVect[i];

   for ( i = 0 ; i < aoCtrlId->fgModeNb ; i ++ )
       aoHeaderCbFgCtrl.fgScaleFactorVect[i] = aoCtrlId->fgScaleFactorVect[i];

   aoHeaderCbFgCtrl.threshold = aoCtrlId->threshold;
   aoHeaderCbFgCtrl.totalThreshold = aoCtrlId->totalThreshold;
   aoHeaderCbFgCtrl.angleWithM2 = aoCtrlId->angleWithM2;
   aoHeaderCbFgCtrl.slidingFocusGain = aoCtrlId->slidingFocusGain;

   /*
    * Open the image circular buffer
    */

   pFile = fopen ( aoHeaderCbFgCtrl.cbFgCtrlFileName, "w" );

   if ( pFile == (FILE *)(NULL) )
   {
      ERROR_SET1 (0, "Failed to open in write mode file %s",
                 ERROR_LOG_NOW, aoHeaderCbFgCtrl.cbFgCtrlFileName);
   }
   else
   {
      /*
       * First save the header of the file
       */

      itemNb = fwrite ( (char *)&aoHeaderCbFgCtrl,
                        sizeof (char),
                        sizeof (AO_HEADER_CB_FG_CTRL_ID_STRUCT),
                        pFile);

      if ( itemNb == NULL )
      {
         ERROR_SET1 (0, "Failed to write header in file %s",
                     ERROR_LOG_NOW, aoHeaderCbFgCtrl.cbFgCtrlFileName);
         (void)fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "Save header cb done\n" );
#endif
      /*
       * Now save the records
       */

#ifdef DEBUG
      printf ( "index: %d, counter: %d\n", index, aoCbFgCtrlId->counter);
#endif
      if ( aoCbFgCtrlId->counter == 0 )
      {
#ifdef DEBUG
         printf ( "Counter= 0 -> save records from 0 to %d\n", index - 1);
#endif
         for ( i = 0 ; i < index ; i ++ )
         {
             itemNb = fwrite ( (char *)&(aoCbFgCtrlId->cbFgCtrlRecord[i]),
                               sizeof (char),
                               sizeof (CB_FG_CTRL_RECORD_STRUCT),
                               pFile);

             if ( itemNb == NULL )
             {
                ERROR_SET1 (0, "Failed to write record in file %s",
                            ERROR_LOG_NOW, aoHeaderCbFgCtrl.cbFgCtrlFileName);
                (void)fclose (pFile);
                return (ERROR);
             }
         }
      }
      else
      {
         if ( index != 0 )
         {
#ifdef DEBUG
            printf ( "Counter# 0, index # 0 -> save records from %d to %d\n",
                     index , CB_FG_CTRL_RECORD_NB - 1 );
#endif
            for ( i = index ; i < CB_FG_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbFgCtrlId->cbFgCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_FG_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbFgCtrl.cbFgCtrlFileName)
;
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }

#ifdef DEBUG
            printf ( "then -> save records from 0 to %d\n", index - 1 );
#endif
            for ( i = 0 ; i < index ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbFgCtrlId->cbFgCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_FG_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW,
                               aoHeaderCbFgCtrl.cbFgCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
         else
         {
#ifdef DEBUG
            printf ( "Counter# 0, index = 0 -> save records from 0 to %d\n",
                     CB_FG_CTRL_RECORD_NB - 1 );
#endif
            for ( i = 0 ; i < CB_FG_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbFgCtrlId->cbFgCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_FG_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW,
                               aoHeaderCbFgCtrl.cbFgCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
      }

      (void) fclose (pFile);
   }

#ifdef DEBUG
   printf ( "Save FG cb done\n" );
#endif
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoGuideAndFocus
 *
 *   INVOCATION:
 *   aoGuideAndFocus (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, 
 *                    pCentroidsVect, pErrorCentroidsVect, pFgVect, 
 *                    pFgVectAfterRot, pFgErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage              (float *)    Pointer to the image from which to 
 *                                        compute the centroids
 *   (>) aoCcdId             (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                        context structure
 *   (>) aoCtrlId            (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCounts        (double *)   Pointer to the total counts vector
 *   (<) pCentroidsVect      (double *)   Pointer to the centroids vector
 *   (<) pErrorCentroidsVect (double *)   Pointer to the errors centroids 
 *                                        vector
 *   (<) pFgVect             (double *)   Pointer to the zernikes vector to 
 *                                        send to M2
 *   (<) pFgVectAfterRot     (double *)   Pointer to the zernikes vector after 
 *                                        to send to M2 rotation
 *   (<) pFgErrorsVect       (double *)   Pointer to the associated errors 
 *                                        vector
 *   (<) pTime               (double *)   Pointer to the time associated to 
 *                                        the vectors
 *   (<) pWfsStatus          (int *)      Pointer to the status flag when 
 *                                        computing the centroids 
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute tip, tilt and focus modes - Associated errors are not computed 
 *   and set to 0.
 *
 *   DESCRIPTION:
 *   This routine computes the centroids for each subapertures, basically a tip 
 *   and tilt information. The centroids information are then used to compute 
 *   the average tip, tilt and focus modes to send to the secondary mirror. 
 *   This routine is the standard routine for FG on PWFS2 and should be used 
 *   after centering all the spots.
 *   Note also that a temporal filter is used for the focus mode. This filter 
 *   consists to a sliding average.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoGuideAndFocus (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   double *     pTotalCountsVect,
   double *     pCentroidsVect,
   double *     pErrorCentroidsVect,
   double *     pFgVect,
   double *     pFgVectAfterRot,
   double *     pFgErrorsVect,
   double *     pTime,
   int *        pWfsStatus
   )
{
   int          imageSize;
   float *      pi;
   float *      pd;
   float *      pMax;
   double *     pFg;
   double *     pMaxFg;
   double *     pMaxCent;
   double *     pCent;
   double *     pMat;
   double *     pErrorFg;
   FG_VECT      fg;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));

   pMaxFg = fg + aoCtrlId->fgModeNb;
   pErrorFg = pFgErrorsVect;
   pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;

   pMat = aoCtrlId->fgContMat;

   for ( pFg = fg ; pFg < pMaxFg ; pFg ++ )
       *pFg = 0.0;

   /* Dark subtraction */

   for ( pi = pImage ; pi < pMax ; pi ++ )
       *pi = ( *pi - *(pd ++) );

   /* First compute the centroids of the image */

   if ( aoCentroidsCompute ( pImage, aoCcdId, aoCtrlId, pTotalCountsVect,
                             pCentroidsVect, pErrorCentroidsVect,
                             pWfsStatus) == ERROR )
   {
      ERROR_SET (0, "Error when computing centroids" , ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Compute the TTF */

   if ( *pWfsStatus != AO_SH_OFF )
   {
      for ( pFg = fg ; pFg < pMaxFg ; pFg ++ )
          for ( pCent = pCentroidsVect ; pCent < pMaxCent ; )
              *pFg += (*(pMat ++)) * (*(pCent ++));

/*
      *(pFgVect) = ( aoCtrlId->cosAngleWithM2 * (*fg) +
                   aoCtrlId->sinAngleWithM2 * (*(fg + 1)) );
      *(pFgVect + 1) = ( aoCtrlId->cosAngleWithM2 * (*(fg + 1)) -
                       aoCtrlId->sinAngleWithM2 * (*fg) );
*/
      *(pFgVect) = *(fg);
      *(pFgVect + 1) = *(fg + 1);

      if ( aoCcdId->binningFlag == TRUE )
         *(fg+2) *= 2.0;

      *(pFgVect + 2) = *(fg+2);

      *(pErrorFg + 0) = 0.0;
      *(pErrorFg + 1) = 0.0;
      *(pErrorFg + 2) = 0.0;
   }
   else
   {
      *(pFgVect + 0) = 0.0;
      *(pFgVect + 1) = 0.0;
      *(pFgVect + 2) = 0.0;

      *(pErrorFg + 0) = 0.0;
      *(pErrorFg + 1) = 0.0;
      *(pErrorFg + 2) = 0.0;
   }

   if ( timeNow (pTime) != OK )
   {
#ifdef DEBUG
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
#endif
      *pTime = (double)AO_TIME_NOW_ERROR;
   };
 
   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgVectAfterRot, 
                          pFgErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModeAnalyze
 *
 *   INVOCATION:
 *   aoModeAnalyze (pImage, aoCcdId, aoCtrlId, aoCbAoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage    (float *)    Pointer to the float buffer which contains the
 *                              image coadded
 *   (>) aoCcdId   (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *   (!) aoCbAoCtrlId (AO_CB_AO_CTRL_ID) Pointer to the ao control circular 
 *                                       buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute centroids and aO modes of a averaged image and store this into
 *   the circular buffer
 *
 *   DESCRIPTION:
 *   First, this routine computes the centroids of this average image and then
 *   multiplies this vector by the control matrix and by scale factors to obtain
 *   the aO modes and centroids and modes are saved into the aoCbAoCtrlId
 *   circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModeAnalyze (
   float *       pImage,
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId,
   AO_CB_AO_CTRL_ID aoCbAoCtrlId
   )
{
   int          indexCtrl;
   int *        pWfsStatus;
   double *     pTotalCountsVect;
   double *     pCentroidsVect;
   double *     pErrorCentroidsVect;
   double *     pAoVect;
   double *     pAoVectAfterRot;
   double *     pAoErrorsVect;
   double *     pAo;
   double *     pErrorAo;
   double *     pCent;
   double *     pMaxAo;
   double *     pMaxCent;
   double *     pMat;
   double *     pTime;

   /* Some initialisations */

   indexCtrl = aoCbAoCtrlId->position;
   pTotalCountsVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].totalCountsVect;
   pCentroidsVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].centroidsVect;
   pErrorCentroidsVect = 
   aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].errorCentroidsVect;
   pAoVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoVect;
   pAoVectAfterRot = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoVectAfterRot;
   pAoErrorsVect = aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].aoErrorsVect;
   pWfsStatus = &(aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].wfsStatus);
   pTime = &(aoCbAoCtrlId->cbAoCtrlRecord[indexCtrl].time);

   pErrorAo = pAoErrorsVect;
   pMaxAo = pAoVect + aoCtrlId->aoModeNb;
   pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;
   pMat = aoCtrlId->aoContMat;

   for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
       *pAo = 0.0;

   /* Compute the centroids */


   if ( aoCentroidsCompute ( pImage, aoCcdId, aoCtrlId, pTotalCountsVect,
                             pCentroidsVect, pErrorCentroidsVect,
                             pWfsStatus) == ERROR )
   {
      ERROR_SET (0, "Error when computing centroids" , ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Apply the control matrix and the scale factor */

   if ( *pWfsStatus != AO_SH_OFF )
   {
      for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
          for ( pCent = pCentroidsVect ; pCent < pMaxCent ; )
              *pAo += (*(pMat ++)) * (*(pCent ++));

      if ( aoCcdId->binningFlag == TRUE )
      {
         for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
         {
             *pAo *= (2.0);
             *(pErrorAo ++) = 0.0;
         }
      }
      else
      {
         for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
         {
             *(pErrorAo ++) = 0.0;
         }
      }
   }
   else
   {
      for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
      {
          *pAo = 0.0;
          *(pErrorAo ++) = 0.0;
      }
   }

   /* Get a timestamp to record at which time ao data are sent to TCS */

   if ( timeNow (pTime) != OK )
   {
#ifdef DEBUG
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
#endif
      *pTime = (double)AO_TIME_NOW_ERROR;
   };

   if ( writeWfsToTcs(aoCtrlId, pAoVect, pAoVectAfterRot, pAoErrorsVect,
                      pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the TCS", ERROR_LOG_SAVE);
      return (ERROR);
   };

   /* Update the aoCbAoCtrlId circular buffer */

   if ( ++ aoCbAoCtrlId->position == CB_AO_CTRL_RECORD_NB )
   {
      aoCbAoCtrlId->position = 0;
      aoCbAoCtrlId->counter ++;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCentroidsWrite
 *
 *   INVOCATION:
 *   aoCentroidsWrite (pCentroidsFileName, pCentroids, centNb, pComment)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCentroidsFileName (char *)   Pointer to the centroids file name
 *   (>) pCentroids         (double *) Pointer to the centroids vector to write
 *   (>) centNb             (int)      Dimension of the vector to write
 *   (>) pComment           (char *)   Pointer to a comment string to write
 *
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write a centroids vector to a file
 *
 *   DESCRIPTION:
 *   This function writes a centroids vector of dimension centNb to a file
 *   given by pCentroidsFileName.
 *   Note: The first comment line indicates in which conditions this vector
 *   has been measured, the second line indicates the dimension.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pCentroidsFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoCentroidsWrite (
   char *     pCentroidsFileName,
   double *   pCentroids,
   int        centNb,
   char *     pComment
   )
{
   int      i;                    /* Index                      */
   FILE *   pFile;                /* File Id                    */

   /* Open the file in write mode */

   pFile = fopen ( pCentroidsFileName, "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the centroids file %s",
                   ERROR_LOG_SAVE, pCentroidsFileName );
      return (ERROR);
   }

   /* Write the first line: should be a comment line */

   (void) fprintf (pFile,
          "# Comments \n");

   /* Write the comment line */

   (void) fprintf (pFile, "%s\n", pComment);

   /* Write the next line of comment */

   (void) fprintf (pFile, "# Dimension\n" );

   /* Write the dimension */

   (void) fprintf (pFile, "%d\n", centNb );

   /* Write the next line of comments */

   (void) fprintf (pFile, "# Centroids\n" );

   /* Now write the centroids */

   for ( i = 0 ; i < centNb ; i ++ )
   {
       (void) fprintf (pFile, "%f\n", *(pCentroids + i) );
   }

   /* Close the file */

   fclose (pFile);

#ifdef DEBUG
   printf ( "aoCentroidsWrite: Write centroids into %s done \n" ,
            pCentroidsFileName );
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoIntMatStructZero
 *
 *   INVOCATION:
 *   aoIntMatStructZero (aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the aO interaction matrix structure
 *
 *   DESCRIPTION:
 *   This function sets to zero the aO interaction matrix structure of the AO
 *   control structure given by aoCtrlId.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoIntMatStructZero (
   AO_CTRL_ID     aoCtrlId
   )
{
   int      i, j; /* Index */

   /* Set to the zero the aO interaction matrix structure */

   for ( i = 0 ; i < AO_MODE_NB ; i ++ )
   {
       aoCtrlId->aoIntMatStruct[i].posAmplitude = 0.0;
       aoCtrlId->aoIntMatStruct[i].negAmplitude = 0.0;

       for ( j = 0 ; j < 2*SUBAP_NB ; j ++ )
       {
           aoCtrlId->aoIntMatStruct[i].posCentroidsVect[j] = 0.0;
           aoCtrlId->aoIntMatStruct[i].negCentroidsVect[j] = 0.0;
       }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoIntMatStructShow
 *
 *   INVOCATION:
 *   aoIntMatStructShow (aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) aoCcdId  (AO_CCD_ID)  Pointer to the AO CCD geometry structure
 *   (>) aoCtrlId (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the aO interaction matrix structure
 *
 *   DESCRIPTION:
 *   This function displays the contents of the aO interaction matrix structure
 *   of the AO control structure given by aoCtrlId.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoIntMatStructShow (
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId
   )
{
   int      i, j; /* Index */

   /* Display the aO interaction matrix structure */

   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       printf ( "Mode : %d\n" , i + 1 );
       printf ( "Amplitude positive in microns: %f\n" ,
                (float)(aoCtrlId->aoIntMatStruct[i].posAmplitude) );

       for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
           printf ( "%f " , aoCtrlId->aoIntMatStruct[i].posCentroidsVect[j]);

       printf ( "\n" );

       printf ( "Amplitude negative in microns: %f\n" ,
                (float)(aoCtrlId->aoIntMatStruct[i].negAmplitude) );

       for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
           printf ( "%f " , aoCtrlId->aoIntMatStruct[i].negCentroidsVect[j] );

       printf ( "\n" );
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoMatZero
 *
 *   INVOCATION:
 *   aoMatZero (aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the aO interaction matrix and the aO control matrix of the 
 *   aoCtrlId context structure
 *
 *   DESCRIPTION:
 *   This function sets to zero the aO interaction matrix and the aO control 
 *   matrix of the AO control structure given by aoCtrlId.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoMatZero (
   AO_CTRL_ID     aoCtrlId
   )
{
   int      i; /* Index */

   /* Set to the zero the aO interaction matrix and aO control matrix */

   aoCtrlId->aoIntMatInitFlag = FALSE;
   aoCtrlId->aoContMatInitFlag = FALSE;

   strcpy ( aoCtrlId->aoIntMatFileName, "");
   strcpy ( aoCtrlId->aoContMatFileName, "");

   for ( i = 0 ; i < 2*AO_MODE_NB*SUBAP_NB ; i ++ )
   {
       aoCtrlId->aoIntMat[i] = 0.0;
       aoCtrlId->aoContMat[i] = 0.0;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoMatCompute
 *
 *   INVOCATION:
 *   aoMatCompute (aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCcdId  (AO_CCD_ID)  Pointer to the AO CCD geometry structure
 *   (!) aoCtrlId (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Compute the aO interaction and the aO control matrixes
 *
 *   DESCRIPTION:
 *   This function computes the aO interaction matrix from the aO interaction 
 *   matrix structure:
 *   For all the modes:
 *   column aO interaction matrix [mode] =
 *   (posCentroidsVect - negCentroidsVect)/(posAmplitude - negAmplitude)
 *   Then, the matrix is reduced to the non null columns, tip, tilt modes
 *   are filtered and the this result matrix is inverted by using the svd
 *   routine. Then the inverse matrix is extended with null line corresponding
 *   to the null column of the interaction matrix and multiply by -1 to give
 *   the final and so well known aO control matrix.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   Piston to be filtered !!
 *-
 */

STATUS aoMatCompute (
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId
   )
{
   int         row, col, k;  /* Index                                         */
   int         redColNb;     /* Column number of the reduced interaction      */
                             /* matrix                                        */

   double      tip, tilt;    /* Tip and tilt values to filter from the        */
                             /* interaction matrix                            */

   AO_MATRIX   redIntMat;    /* Reduced interaction matrix                    */
   AO_MATRIX   invRedIntMat; /* Inverse of the reduced interaction matrix     */

   /* Compute each column of the aO interaction matrix */

   k = 0;
   for ( col = 0 ; col < aoCtrlId->aoModeNb ; col ++ )
   {
       if ( (aoCtrlId->aoIntMatStruct[col].posAmplitude != 0.0) &&
            (aoCtrlId->aoIntMatStruct[col].negAmplitude != 0.0) )
       {
          for ( row = 0 ; row < aoCcdId->centroidsNb ; row ++ )
          {
            aoCtrlId->aoIntMat[row*aoCtrlId->aoModeNb + col] =
            (aoCtrlId->aoIntMatStruct[col].posCentroidsVect[row] -
             aoCtrlId->aoIntMatStruct[col].negCentroidsVect[row])/
            (aoCtrlId->aoIntMatStruct[col].posAmplitude -
             aoCtrlId->aoIntMatStruct[col].negAmplitude);

            *(redIntMat + row*aoCtrlId->aoModeNb + k) =
            aoCtrlId->aoIntMat[row*aoCtrlId->aoModeNb + col];
          }

          k ++;
       }
       else
       {
          for ( row = 0 ; row < aoCcdId->centroidsNb ; row ++ )
              aoCtrlId->aoIntMat[row*aoCtrlId->aoModeNb + col] = 0.0;
       }
   }

   redColNb = k;

   if ( redColNb == 0 )
   {
      ERROR_SET ( 0, "Can't inverse a null matrix",
                  ERROR_LOG_SAVE );
      return ( ERROR );
   }

   /* Filter tip, tilt from the reduced interaction matrix */

   for ( col = 0 ; col < redColNb ; col ++ )
   {
       tip = 0.0;
       tilt = 0.0;
       for ( row = 0 ; row < aoCcdId->subapUsedNb ; row ++ )
       {
           tip += *(redIntMat + (2*row)*redColNb + col);
           tilt += *(redIntMat + (2*row+1)*redColNb + col);
       }

       tip = tip / (double)(aoCcdId->subapUsedNb);
       tilt = tilt / (double)(aoCcdId->subapUsedNb);

       for ( row = 0 ; row < aoCcdId->subapUsedNb ; row ++ )
       {
           *(redIntMat + (2*row)*redColNb + col) -= tip;
           *(redIntMat + (2*row+1)*redColNb + col) -= tilt;
       }
   }

   /* Inverse this reduced matrix */

   if ( invRectMat ( redIntMat, aoCcdId->centroidsNb, redColNb, invRedIntMat,
                     1.0e-20, 1.0e-40) != OK )
   {
      ERROR_SET ( 0, "Failed to inverse the reduced interaction matrix",
                  ERROR_LOG_SAVE );
      return ( ERROR );
   }

   /* Determine the aO control matrix */

   k = 0;
   for ( row = 0 ; row < aoCtrlId->aoModeNb ; row ++ )
   {
       if ( (aoCtrlId->aoIntMatStruct[row].posAmplitude != 0.0) &&
            (aoCtrlId->aoIntMatStruct[row].negAmplitude != 0.0) )
       {
          for ( col = 0 ; col < aoCcdId->centroidsNb ; col ++)
          {
              aoCtrlId->aoContMat[row*aoCcdId->centroidsNb + col] =
              (-1.0) * (*(invRedIntMat + k*aoCcdId->centroidsNb + col));
          }

          k ++;
       }
       else
       {
          for ( col = 0 ; col < aoCcdId->centroidsNb ; col ++)
              aoCtrlId->aoContMat[row*aoCcdId->centroidsNb + col] = 0.0;
       }
   }

   /* End of the routine */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoDarkUpdate
 *
 *   INVOCATION:
 *   aoDarkUpdate (pDarkFileName, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDarkFileName      (char *)     Pointer to the dark file name
 *   (>) aoCcdId            (AO_CCD_ID)  Pointer to the CCD geometry context
 *                                       structure
 *   (<) aoCtrlId           (AO_CTRL_ID) Pointer to the control context
 *                                       structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Update the dark buffer of the control context structure
 *
 *   DESCRIPTION:
 *   Update the dark buffer of the control context structure
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pDarkFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoDarkUpdate (
   char *     pDarkFileName,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{
   int        i;
   IMAGE_VECT image;

   /* Init the new dark image */

#ifdef DEBUG
   printf ( "aoDarkUpdate(): dark file name: %s\n", pDarkFileName );
#endif

   if ( aoFitsImageFloatRead (pDarkFileName, image, aoCcdId->xPixels,
                              aoCcdId->yPixels) == ERROR )
   {
      ERROR_SET1 ( 0,
      "Failed to read the dark image from the dark fits file %s",
      ERROR_LOG_SAVE, pDarkFileName );
      aoCtrlId->darkInitFlag = FALSE;
      return (ERROR);
   }

   strcpy ( aoCtrlId->darkFileName , pDarkFileName );

   for ( i = 0 ; i < aoCcdId->pixelsNb ; i ++ )
       aoCtrlId->darkVect[i] = image[i];

   aoCtrlId->darkInitFlag = TRUE;

   /* End */

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCtrlFileRead
 *
 *   INVOCATION:
 *   aoCtrlFileRead (pInitFileName, pPath, pDarkFileName, pFlatFileName, 
 *                   pRefFileName, pRefX, pRefY, pAoImFileName, pAoCmFileName,
 *                   pFgCmFileName, pThresh, pTotalThresh, pAngleM2, pAngleM1)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)   Pointer to the AO init file name 
 *   (<) pPath         (char *)   Pointer to the path
 *   (<) pDarkFileName (char *)   Pointer to the dark file name
 *   (<) pFlatFileName (char *)   Pointer to the dark file name
 *   (<) pRefFileName  (char *)   Pointer to the SH reference file name
 *   (<) pRefX         (double *) Pointer to the X center for the whole CCD
 *   (<) pRefY         (double *) Pointer to the X center for the whole CCD
 *   (<) pAoImFileName (char *)   Pointer to the aO interaction matrix file name
 *   (<) pAoCmFileName (char *)   Pointer to the aO control matrix file name
 *   (<) pFgCmFileName (char *)   Pointer to the FG control matrix file name
 *   (<) pThresh       (double *) Pointer to the threshold
 *   (<) pTotalThresh  (double *) Pointer to the total flux threshold
 *   (<) pAngleM2      (double *) Pointer to the angle with M2
 *   (<) pAngleM1      (double *) Pointer to the angle with M1
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the AO control file
 *
 *   DESCRIPTION:
 *   Read the parameters of the AO control file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *
 *   DEFICIENCIES:
 *   Can't use ERROR_SET1, replace by printf for now - CB 6 july 2000
 *-
 */

STATUS aoCtrlFileRead (
   char *   pInitFileName,
   char *   pPath,
   char *   pDarkFileName,
   char *   pFlatFileName,
   char *   pRefFileName,
   double * pRefX,
   double * pRefY,
   char *   pAoImFileName,
   char *   pAoCmFileName,
   char *   pFgCmFileName,
   double * pThresh,
   double * pTotalThresh,
   double * pAngleM2,
   double * pAngleM1
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      printf ( "Failed to open the AO init file %s\n", pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read first line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): first line of comments:\n" );
   printf ( "%s\n" , comment );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the second line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif 

   /* Read the name of the dark fits file */

   if ( fgets (pDarkFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the name of the dark file from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pDarkFileName[strlen(pDarkFileName) - 1] == '\n' )
   {
      pDarkFileName[strlen(pDarkFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n", 
               pDarkFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): dark file name: %s\n", pDarkFileName );
#endif

   /* Path will always be "." */

   strcpy ( pPath , "." ) ;

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif 

   /* Read the name of the flat fits file */

   if ( fgets (pFlatFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the name of the flat file from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pFlatFileName[strlen(pFlatFileName) - 1] == '\n' )
   {
      pFlatFileName[strlen(pFlatFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n", 
               pFlatFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): flat file name: %s\n", pFlatFileName );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif 

   /* Read the name of the WFS reference file */

   if ( fgets (pRefFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the WFS reference file name from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pRefFileName[strlen(pRefFileName) - 1] == '\n' )
   {
      pRefFileName[strlen(pRefFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n", 
               pRefFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): WFS reference file name: %s\n", pRefFileName );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read value of xcenter for the whole CCD */

   if ( (fscanf (pFile, "%lf\n", pRefX)) == EOF )
   {
      printf ( 
      "Failed to read x center of the whole CCD from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): x center for whole CCD=%f\n", *pRefX );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read value of ycenter for the whole CCD */

   if ( (fscanf (pFile, "%lf\n", pRefY)) == EOF )
   {
      printf ( 
         "Failed to read y center for the whole CCD from the AO init file %s\n",
         pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): y center for whole CCD=%f\n", *pRefY);
#endif

   /* Read the next lines (comments) from the init file */

   for ( i = 0 ; i < 6 ; i ++ )
   {
      /* Skip the next line of comment */

      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         printf ( 
         "Failed to read the next line of comments from the AO init file %s\n",
         pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoCtrlFileRead(): %s\n", comment );
#endif 

   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read the name of the aO interaction matrix file */

   if ( fgets (pAoImFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the name of the aO IM from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pAoImFileName[strlen(pAoImFileName) - 1] == '\n' )
   {
      pAoImFileName[strlen(pAoImFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n",
               pAoImFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): aO IM file name: %s\n", pAoImFileName );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

  /* Read the name of the aO control matrix file */

   if ( fgets (pAoCmFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the name of the aO CM from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pAoCmFileName[strlen(pAoCmFileName) - 1] == '\n' )
   {
      pAoCmFileName[strlen(pAoCmFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n",
               pAoCmFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): aO CM file name: %s\n", pAoCmFileName );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read the name of the FG control matrix file */

   if ( fgets (pFgCmFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the name of the FG CM from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pFgCmFileName[strlen(pFgCmFileName) - 1] == '\n' )
   {
      pFgCmFileName[strlen(pFgCmFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n",
               pFgCmFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): FG CM file name: %s\n", pFgCmFileName );
#endif

   /* Skip the next lines of comments */

   for ( i = 0 ; i < 8 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         printf (
         "Failed to read the next line of comments from the AO init file %s\n",
         pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   }

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   /* Read threshold */

   if ( (fscanf (pFile, "%lf\n", pThresh)) == EOF )
   {
      printf ( "Failed to read threshold from the AO init file %s\n",
               pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): threshold = %f\n", *pThresh );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read total threshold */

   if ( (fscanf (pFile, "%lf\n", pTotalThresh)) == EOF )
   {
      printf ( "Failed to read total threshold from the AO init file %s\n",
               pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): total threshold = %f\n", *pTotalThresh );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf ( 
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read angle between M2 and P2 */

   if ( (fscanf (pFile, "%lf\n", pAngleM2)) == EOF )
   {
      printf ( "Failed to read angle from the AO init file %s\n",
               pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): angleWithM2 = %f\n", *pAngleM2 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (
      "Failed to read the next line of comments from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read angle between M1 and P2 */

   if ( (fscanf (pFile, "%lf\n", pAngleM1)) == EOF )
   {
      printf ( "Failed to read angleM1 from the AO init file %s\n",
               pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): angleWithM1 = %f\n", *pAngleM1 );
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModInit
 *
 *   INVOCATION:
 *   aoModInit (pInitFileName, astModelId, trefModelId, comaModelId, focModelId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)          Pointer to the model file name 
 *   (>) astModelId  (AST_ZP_MODEL_ID)   Pointer to the astigmatism zero point
 *                                       model
 *   (>) trefModelId (TREF_ZP_MODEL_ID)  Pointer to the trefoil zero point model
 *   (>) comaModelId (COMA_ZP_MODEL_ID)  Pointer to the coma zero point model
 *   (>) focModelId  (FOCUS_ZP_MODEL_ID) Pointer to the focus zero point model
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the zero point models
 *
 *   DESCRIPTION:
 *   Init the zero point models with defaults contained into pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModInit (
   char *            pInitFileName,
   AST_ZP_MODEL_ID   astModelId,
   TREF_ZP_MODEL_ID  trefModelId,
   COMA_ZP_MODEL_ID  comaModelId,
   FOCUS_ZP_MODEL_ID focModelId
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the model init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "%s\n" , comment );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the second line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif   
   }

   /* Read the values for the astigmatism 0 model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf %lf %lf\n", 
                &(astModelId->a1), &(astModelId->a2), 
                &(astModelId->a3), &(astModelId->p1), 
                &(astModelId->p2), &(astModelId->p3),
                &(astModelId->c)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the astigmatism 0 model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): astig 0 a1: %f\n", (float)astModelId->a1 );
   printf ( "aoModInit(): astig 0 a2: %f\n", (float)astModelId->a2 );
   printf ( "aoModInit(): astig 0 a3: %f\n", (float)astModelId->a3 );
   printf ( "aoModInit(): astig 0 p1: %f\n", (float)astModelId->p1 );
   printf ( "aoModInit(): astig 0 p2: %f\n", (float)astModelId->p2 );
   printf ( "aoModInit(): astig 0 p3: %f\n", (float)astModelId->p3 );
   printf ( "aoModInit(): astig 0 c: %f\n", (float)astModelId->c );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif   
   }

   /* Read the values for the astigmatism 45 model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf %lf %lf\n", 
                &(astModelId->b1), &(astModelId->b2), 
                &(astModelId->b3), &(astModelId->pp1), 
                &(astModelId->pp2), &(astModelId->pp3),
                &(astModelId->d)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the astig 45 model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): astig 45 b1: %f\n", (float)astModelId->b1 );
   printf ( "aoModInit(): astig 45 b2: %f\n", (float)astModelId->b2 );
   printf ( "aoModInit(): astig 45 b3: %f\n", (float)astModelId->b3 );
   printf ( "aoModInit(): astig 45 pp1: %f\n", (float)astModelId->pp1 );
   printf ( "aoModInit(): astig 45 pp2: %f\n", (float)astModelId->pp2 );
   printf ( "aoModInit(): astig 45 pp3: %f\n", (float)astModelId->pp3 );
   printf ( "aoModInit(): astig 45 d: %f\n", (float)astModelId->d );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif   

   /* Read the values for the gain for astig model */

   if ( fscanf (pFile, "%lf %lf\n", 
                &(astModelId->gain0), 
                &(astModelId->gain45)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read gains for astig model in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): astig 0 gain0: %f\n", (float)astModelId->gain0 );
   printf ( "aoModInit(): astig 0 gain45: %f\n", (float)astModelId->gain45 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif   

   /* Read the values for the offsets for astig model */

   if ( fscanf (pFile, "%lf %lf\n", 
                &(astModelId->offsetAstig0), 
                &(astModelId->offsetAstig45)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read offsets for astig model in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): astig 0 offset0: %f\n", 
            (float)astModelId->offsetAstig0 );
   printf ( "aoModInit(): astig 0 offset45: %f\n", 
            (float)astModelId->offsetAstig45 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif   

   /* Read the apply astig model flag */

   if ( (fscanf (pFile, "%d\n", &astModelId->applyModel)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply astig model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): astig model apply flag=%d\n", astModelId->applyModel);
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif
   }

   /* Read the values for the cos trefoil model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                &(trefModelId->a), &(trefModelId->p), 
                &(trefModelId->c)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the cos trefoil model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): cos tref a: %f\n", (float)trefModelId->a );
   printf ( "aoModInit(): cos tref p: %f\n", (float)trefModelId->p );
   printf ( "aoModInit(): cos tref c: %f\n", (float)trefModelId->c );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif
   }

   /* Read the values for the sin trefoil model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                &(trefModelId->b), &(trefModelId->pp), 
                &(trefModelId->d)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the sin trefoil model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): sin tref b: %f\n", (float)trefModelId->b );
   printf ( "aoModInit(): sin tref pp: %f\n", (float)trefModelId->pp );
   printf ( "aoModInit(): sin tref d: %f\n", (float)trefModelId->d );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif

   /* Read the apply trefoil model flag */

   if ( (fscanf (pFile, "%d\n", &trefModelId->applyModel)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply trefoil model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): trefoil model apply flag=%d\n", 
            trefModelId->applyModel);
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif
   }

   /* Read the values for the coma X model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                &(comaModelId->a), &(comaModelId->p), 
                &(comaModelId->c)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the coma X model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): coma a: %f\n", (float)comaModelId->a );
   printf ( "aoModInit(): coma p: %f\n", (float)comaModelId->p );
   printf ( "aoModInit(): coma c: %f\n", (float)comaModelId->c );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif
   }

   /* Read the values for the coma Y model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                &(comaModelId->b), &(comaModelId->pp), 
                &(comaModelId->d)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the coma Y model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): coma Y b: %f\n", (float)comaModelId->b );
   printf ( "aoModInit(): coma Y pp: %f\n", (float)comaModelId->pp );
   printf ( "aoModInit(): coma Y d: %f\n", (float)comaModelId->d );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif

   /* Read the apply coma model flag */

   if ( (fscanf (pFile, "%d\n", &comaModelId->applyModel)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply coma model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): coma model apply flag=%d\n", 
            comaModelId->applyModel);
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModInit(): %s\n", comment );
#endif
   }

   /* Read the values for the focus model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf\n", 
                &(focModelId->a1), &(focModelId->p1), 
                &(focModelId->a2), &(focModelId->p2), 
                &(focModelId->c)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the focus model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): focus a1: %f\n", (float)focModelId->a1 );
   printf ( "aoModInit(): focus p1: %f\n", (float)focModelId->p1 );
   printf ( "aoModInit(): focus a2: %f\n", (float)focModelId->a2 );
   printf ( "aoModInit(): focus p2: %f\n", (float)focModelId->p2 );
   printf ( "aoModInit(): focus c: %f\n", (float)focModelId->c );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): %s\n", comment );
#endif

   /* Read the apply focus model flag */

   if ( (fscanf (pFile, "%d\n", &focModelId->applyModel)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply focus model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModInit(): focus model apply flag=%d\n", 
            focModelId->applyModel);
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModAstFileRead
 *
 *   INVOCATION:
 *   aoModAstFileRead (pInitFileName, pA1, pA2, pA3, pP1, pP2, pP3, pC, 
 *                     pB1, pB2, pB3, pPp1, pPp2, pPp3, pD,
 *                     pGain0, pGain45, pOffset0, pOffset45, pApply)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)   Pointer to the model file name 
 *   (<) pA1           (double *) a1
 *   (<) pA2           (double *) a2
 *   (<) pA3           (double *) a3
 *   (<) pP1           (double *) p1
 *   (<) pP2           (double *) p2
 *   (<) pP3           (double *) p3
 *   (<) pC            (double *) c
 *   (<) pB1           (double *) a1
 *   (<) pB2           (double *) a2
 *   (<) pB3           (double *) a3
 *   (<) pPp1          (double *) p1
 *   (<) pPp2          (double *) p2
 *   (<) pPp3          (double *) p3
 *   (<) pD            (double *) d
 *   (<) pGain0        (double *) gain0
 *   (<) pGain45       (double *) gain45
 *   (<) pOffset0      (double *) offsetAstig0
 *   (<) pOffset45     (double *) offsetAstig45
 *   (<) pApply        (int *)    applyModel
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the astigmatism zero point model from the model file
 *
 *   DESCRIPTION:
 *   Read the parameters of the astigmatism zero point model from 
 *   the model file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModAstFileRead (
   char * pInitFileName,
   double * pA1,
   double * pA2,
   double * pA3,
   double * pP1,
   double * pP2,
   double * pP3,
   double * pC,
   double * pB1,
   double * pB2,
   double * pB3,
   double * pPp1,
   double * pPp2,
   double * pPp3,
   double * pD,
   double * pGain0,
   double * pGain45,
   double * pOffset0,
   double * pOffset45,
   int    * pApply
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the model init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "%s\n" , comment );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the second line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModAstFileRead(): %s\n", comment );
#endif   
   }

   /* Read the values for the astigmatism 0 model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf %lf %lf\n", 
                pA1, pA2, pA3, pP1, pP2, pP3, pC) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the astigmatism 0 model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): astig 0 a1: %f\n", (float)*pA1 );
   printf ( "aoModAstFileRead(): astig 0 a2: %f\n", (float)*pA2 );
   printf ( "aoModAstFileRead(): astig 0 a3: %f\n", (float)*pA3 );
   printf ( "aoModAstFileRead(): astig 0 p1: %f\n", (float)*pP1 );
   printf ( "aoModAstFileRead(): astig 0 p2: %f\n", (float)*pP2 );
   printf ( "aoModAstFileRead(): astig 0 p3: %f\n", (float)*pP3 );
   printf ( "aoModAstFileRead(): astig 0 c: %f\n", (float)*pC );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModAstFileRead(): %s\n", comment );
#endif   
   }

   /* Read the values for the astigmatism 45 model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf %lf %lf\n", 
                pB1, pB2, pB3, pPp1, pPp2, pPp3, pD) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the astig 45 model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): astig 45 b1: %f\n", (float)*pB1 );
   printf ( "aoModAstFileRead(): astig 45 b2: %f\n", (float)*pB2 );
   printf ( "aoModAstFileRead(): astig 45 b3: %f\n", (float)*pB3 );
   printf ( "aoModAstFileRead(): astig 45 pp1: %f\n", (float)*pP1 );
   printf ( "aoModAstFileRead(): astig 45 pp2: %f\n", (float)*pP2 );
   printf ( "aoModAstFileRead(): astig 45 pp3: %f\n", (float)*pP3 );
   printf ( "aoModAstFileRead(): astig 45 d: %f\n", (float)*pD );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModastFileRead(): %s\n", comment );
#endif   

   /* Read the values for the gains for astig model */

   if ( fscanf (pFile, "%lf %lf\n", 
                pGain0, pGain45 ) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read gains for astig model in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): astig 0 gain0: %f\n", (float)*pGain0 );
   printf ( "aoModAstFileRead(): astig 0 gain45: %f\n", (float)*pGain45 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): %s\n", comment );
#endif   

   /* Read the values for the offsets for astig model */

   if ( fscanf (pFile, "%lf %lf\n", 
                pOffset0, pOffset45) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read offsets for astig model in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): astig 0 offset0: %f\n", 
            (float)*pOffset0 );
   printf ( "aoModastFileRead(): astig 0 offset45: %f\n", 
            (float)*pOffset45 );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): %s\n", comment );
#endif   

   /* Read the apply astig model flag */

   if ( (fscanf (pFile, "%d\n", pApply)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply astig model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModAstFileRead(): astig model apply flag=%d\n", *pApply);
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModTrefFileRead
 *
 *   INVOCATION:
 *   aoModTrefFileRead (pInitFileName, pA, pP, pC, pB, pPp, pD, pApply)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)          Pointer to the model file name 
 *   (<) pA            (double *) a
 *   (<) pP            (double *) p
 *   (<) pC            (double *) c
 *   (<) pB            (double *) b
 *   (<) pPp           (double *) pp
 *   (<) pD            (double *) d
 *   (<) pApply        (int *) applyModel
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the trefoil zero point model from the model file
 *
 *   DESCRIPTION:
 *   Read the parameters of the trefoil zero point model from
 *   the model file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModTrefFileRead (
   char * pInitFileName,
   double * pA,
   double * pP,
   double * pC,
   double * pB,
   double * pPp,
   double * pD,
   int * pApply
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the model init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "%s\n" , comment );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 14 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the second line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModTrefFileRead(): %s\n", comment );
#endif   
   }

   /* Read the values for the cos trefoil model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                pA, pP, pC) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the cos trefoil model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModTrefFileRead(): cos tref a: %f\n", (float)*pA );
   printf ( "aoModTrefFileRead(): cos tref p: %f\n", (float)*pP );
   printf ( "aoModTrefFileRead(): cos tref c: %f\n", (float)*pC );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModTrefFileRead(): %s\n", comment );
#endif
   }

   /* Read the values for the sin trefoil model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                pB, pPp, pD) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the sin trefoil model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModTrefFileRead(): sin tref b: %f\n", (float)*pB );
   printf ( "aoModTrefFileRead(): sin tref pp: %f\n", (float)*pPp );
   printf ( "aoModTrefFileRead(): sin tref d: %f\n", (float)*pD );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModTrefFileRead(): %s\n", comment );
#endif

   /* Read the apply trefoil model flag */

   if ( (fscanf (pFile, "%d\n", pApply)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply trefoil model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModTrefFileRead(): trefoil model apply flag=%d\n", 
            *pApply);
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModComaFileRead
 *
 *   INVOCATION:
 *   aoModComaFileRead (pInitFileName, pA, pP, pC, pB, pPp, pD, pApply)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)          Pointer to the model file name 
 *   (<) pA            (double *) a
 *   (<) pP            (double *) p
 *   (<) pC            (double *) c
 *   (<) pB            (double *) b
 *   (<) pPp           (double *) pp
 *   (<) pD            (double *) d
 *   (<) pApply        (int *) applyModel
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the coma zero point model from the model file
 *
 *   DESCRIPTION:
 *   Read the parameters of the coma zero point model from
 *   the model file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModComaFileRead (
   char * pInitFileName,
   double * pA,
   double * pP,
   double * pC,
   double * pB,
   double * pPp,
   double * pD,
   int * pApply
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the model init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "%s\n" , comment );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 22 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the second line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModComaFileRead(): %s\n", comment );
#endif   
   }

   /* Read the values for the coma X model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                pA, pP, pC) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the coma X model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModComaFileRead(): coma a: %f\n", (float)*pA );
   printf ( "aoModComaFileRead(): coma p: %f\n", (float)*pP );
   printf ( "aoModComaFileRead(): coma c: %f\n", (float)*pC );
#endif

   /* Skip the next line of comment */

   for ( i = 0 ; i < 2 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0,
         "Failed to read the next line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModComaFileRead(): %s\n", comment );
#endif
   }

   /* Read the values for the coma Y model */

   if ( fscanf (pFile, "%lf %lf %lf\n", 
                pB, pPp, pD) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the coma Y model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModComaFileRead(): coma Y b: %f\n", (float)*pB );
   printf ( "aoModComaFileRead(): coma Y pp: %f\n", (float)*pPp );
   printf ( "aoModComaFileRead(): coma Y d: %f\n", (float)*pD );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModComaFileRead(): %s\n", comment );
#endif

   /* Read the apply coma model flag */

   if ( (fscanf (pFile, "%d\n", pApply)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply coma model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModComaFileRead(): coma model apply flag=%d\n", *pApply);
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoModFocFileRead
 *
 *   INVOCATION:
 *   aoModFocFileRead (pInitFileName, pA1, pP1, pA2, pP2, pC, pApply)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)   Pointer to the model file name 
 *   (<) pA1            (double *) a1
 *   (<) pP1            (double *) p1
 *   (<) pA2            (double *) a2
 *   (<) pP2            (double *) p2
 *   (<) pC             (double *) c
 *   (<) pApply         (int *) applyModel
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the focus zero point model from the model file
 *
 *   DESCRIPTION:
 *   Read the parameters of the focus zero point model from
 *   the model file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModFocFileRead (
   char * pInitFileName,
   double * pA1,
   double * pP1,
   double * pA2,
   double * pP2,
   double * pC,
   int * pApply
   )
{
   FILE *     pFile;
   char       comment [STRING_SIZE];
   int        i;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the model init file %s",
                   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0, 
      "Failed to read first line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "%s\n" , comment );
#endif

   /* Skip the next lines of comment */

   for ( i = 0 ; i < 30 ; i ++ )
   {
      if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
      {
         ERROR_SET1 ( 0, 
         "Failed to read the second line of comments from the model init file %s",
         ERROR_LOG_SAVE, pInitFileName );
         fclose (pFile);
         return (ERROR);
      }

#ifdef DEBUG
      printf ( "aoModFocFileRead(): %s\n", comment );
#endif   
   }

   /* Read the values for the focus model */

   if ( fscanf (pFile, "%lf %lf %lf %lf %lf\n", 
                pA1, pP1, pA2, pP2, pC) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the focus model values in the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModFocFileRead(): focus a1: %f\n", (float)*pA1 );
   printf ( "aoModFocFileRead(): focus p1: %f\n", (float)*pP1 );
   printf ( "aoModFocFileRead(): focus a2: %f\n", (float)*pA2 );
   printf ( "aoModFocFileRead(): focus p2: %f\n", (float)*pP2 );
   printf ( "aoModFocFileRead(): focus c: %f\n", (float)*pC );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
      "Failed to read the next line of comments from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModFocFileRead(): %s\n", comment );
#endif

   /* Read the apply focus model flag */

   if ( (fscanf (pFile, "%d\n", pApply)) == EOF )
   {
      ERROR_SET1 ( 0, 
      "Failed to read the apply focus model flag from the model init file %s",
      ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoModFocFileRead(): focus model apply flag=%d\n", 
            *pApply);
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoThresholdPerSubapCompute
 *
 *   INVOCATION:
 *   aoThresholdPerSubapCompute (pImage, aoCcdId, ratePixel, pThreshold)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage         (float *)    Pointer to the image from which to compute
 *                                   the centroids
 *   (>) aoCcdId        (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *                                   structure
 *   (>) ratePixel      (double)     Rate of the brightest pixels to determine
 *                                   the thresholds - should be between 0 and 1
 *   (<) pThreshold     (double *)   Pointer to the threshold vector 
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute the threshold per subaperture
 *
 *   DESCRIPTION:
 *   This routine allows to compute the optimized threshold per subaperture 
 *   from a PWFS2 spots image pImage according to the following criteria: 
 *   ratePixel % of the brightest pixels of the subaperture.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP2Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoThresholdPerSubapCompute (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   double       ratePixel,
   double *     pThreshold
   )
{
   int          index;
   int          gap;
   int          pixelsNb;
   int          subapNb;
   int          i, j;
   int          l, k;
   int          m;
   float        temp;
   float *      pn;
   float *      pi;
   float *      pMin;
   float *      pMax;
   IMAGE_VECT   newImageVect;

   /* Check range of ratePixel: should be between 0 and 1 */

   if ( (ratePixel < 0.0) || (ratePixel > 1.0) )
   {
      ERROR_SET1 ( 0 , "ratePixel (%f) should be comprised between 0 and 1",
                   ERROR_LOG_SAVE, ratePixel );
      return (ERROR);
   }

   /* Store the pixels of the subaperture into newImageVect */

   m=0;
   for ( k = 0 ; k < 2 * aoCcdId->ySubapNb ; k ++ )
   {
       for ( l = 0 ; l < 2 * aoCcdId->xSubapNb ; l ++ )
       {
           subapNb = 2*k*aoCcdId->xSubapNb + l;
           pn = newImageVect;

           if ( aoCcdId->subapUsedVect[subapNb] == TRUE)
           {
              pixelsNb = aoCcdId->xRaster * aoCcdId->yRaster; 

#ifdef DEBUG
              printf ( "subaperture NB = %d is used\n" , subapNb );
#endif

              for ( i = 1 ; i <= aoCcdId->yRaster ; i ++ )
              {
                  pMin = pImage + ((i-1)*aoCcdId->xPixels) +
                         (l*aoCcdId->xRaster) +
                         (k * aoCcdId->xPixels * aoCcdId->yRaster);
                  pMax = pMin + aoCcdId->xRaster;

                  for ( pi = pMin ; pi < pMax ; pi ++)
                      *(pn ++) = *pi;
              }

#ifdef DEBUG
              pn = newImageVect;
              printf ( "Pixels = " );
              for ( i = 0; i < pixelsNb ; i ++ )
                  printf ( "%f " , *(pn + i));
              printf ( "\n" );
#endif

              /* Now sort newImageVector */

              pn = newImageVect;

              for ( gap = pixelsNb/2 ; gap > 0 ; gap /= 2 )
              {
                  for ( i = gap ; i < pixelsNb ; i ++ )
                  {
                      for ( j = i - gap ; j >= 0 && (*(pn+j)>*(pn+j+gap)) ; 
                            j -= gap)
                      {
                          temp = *(pn+j);
                          *(pn+j) = *(pn+j+gap);
                          *(pn+j+gap) = temp;
                      }
                  }
              }

#ifdef DEBUG
              pn = newImageVect;
              printf ( "Pixels = " );
              for ( i = 0; i < pixelsNb ; i ++ )
                  printf ( "%f " , *(pn + i));
              printf ( "\n" );
#endif

              /* Now compute the threshold for this subaperture */

              index = (int) ceil ((double)(pixelsNb) * (1.0 - ratePixel));

              *(pThreshold + m) = *(pn + index);

#ifdef DEBUG
              printf ( "index = %d\n" ,index);
              printf ( "threshold[%d] = %f\n" , m , *(pThreshold + m));
#endif
              m ++;
           }
       }
   }

   return (OK);
}
