/*+
 *   MODULE NAME:
 *   aoP1Lib
 *
 *   FILENAME:
 *   aoP1Lib.c
 *
 *   PURPOSE:
 *   Active optics library dedicated to PWFS1 
 *
 *   DESCRIPTION:
 *   This file contains the active optics library dedicated to PWFS1. 
 *   PWFS1 is a Shack Hartmann sensor composed of a CCD 80x80 and 
 *   a matrix of 6x6 lenslet array
 *   Note: aO means active optics, FG means fast guide
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   AUTHORS:
 *   Corinne Boyer
 *
 *   FUNCTIONS:
 *   aoCcdContextCreate() - Create a CCD geometry context structure
 *   aoCtrlContextCreate() - Create a control context structure
 *   aoCbImContextCreate() - Create a image circular buffer context structure
 *   aoCbAoCtrlContextCreate() - Create a aO control circular buffer context 
 *                               structure
 *   aoCbFgCtrlContextCreate() - Create a FG control circular buffer context 
 *                               structure
 *   aoCcdContextShow() - Display a CCD geometry context structure
 *   aoRefRead() - Read the SH reference file
 *   aoScaleRead() - Read the aO scale factor file
 *   aoScaleUpdate() - Update the aO scale factor from a vector
 *   aoFitsImageFloatRead() - Read a float image from a FITS file
 *   aoFitsImageFloatWrite() - Write a float image to a FITS file
 *   aoMatRead() - Read an interaction or control matrix from a file 
 *   aoMatWrite() - Write an interaction or a control matrix to a file 
 *   aoFgContMatRead() - Read a FG control matrix from a file 
 *   aoCtrlContextInit() - Init the control context structure
 *   aoCtrlContextUpdate() - Update the control context structure
 *   aoCtrlContextShow() - Display the control context structure
 *   aoDarkSubtract() - Subtract a dark from an image
 *   aoGlobalGuide() - Compute tip and tilt modes only over the whole CCD
 *   aoGlobalGuideAndError() - Compute tip and tilt modes only over the whole
 *                             CCD and the associated errors
 *   aoImageFloatAverage() - Average float images
 *   aoRmsNoiseDarkCompute() - To compute the rms of the noise
 *   aoThresholdCompute() - Compute the threshold
 *   aoCentroidsCompute() - Compute the centroids of an image
 *   aoModeCompute() - Compute the aO modes
 *   aoCbImSave() - Save the image circular buffer
 *   aoCbImZero() - Set to zero the image circular buffer
 *   aoCbCtrlZero() - Set to zero the control circular buffer
 *   aoCbFgCtrlZero() - Set to zero the FG control circular buffer
 *   aoCbCtrlSave() - Save the control circular buffer
 *   aoCbFgCtrlSave() - Save the FG control circular buffer
 *   aoGuideAndFocus() - Compute FG modes
 *   aoModeAnalyze() - Compute the centroids and modes for analyze
 *   aoCentroidsWrite() - Write centroids to a file
 *   aoColImStructZero() - Set to zero the interaction matrix structure
 *   aoColImStructShow() - Display the interaction matrix structure
 *   aoMatZero() - Set to zero the interaction and the control matrix 
 *   aoMatCompute() - Compute the interaction and the control matrix
 *   aoDarkUpdate() - Update the dark buffer of the control context structure
 *   aoCtrlFileRead () - Read parameters from the AO control file
 * 
 *INDENT-OFF*
 *   21 April 2000: CB - original creation
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
#include "aoP1Lib.h"

/******************************************************* External functions ***/

extern STATUS writeWfsToSynchro ();           /* defined into writeZernikes.c */
extern STATUS writeWfsToTcs ();               /* defined into writeZernikes.c */

/****************************************************************** Defines ***/

/*#define DEBUG*/                   /* Define this macro to enable debug messages */

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
 *   (AO_CCD_ID) Pointer to CCD geometry context structure, or NULL if 
 *               unsuccessful.
 *
 *   PURPOSE:
 *   Create a CCD geometry context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a CCD geometry context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CCD_ID aoCcdContextCreate (void)
{
   AO_CCD_ID   aoCcdId;

   /* Allocate memory for the CCD geometry context structure, initialising 
    * its contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCcdContextCreate: Allocating %d bytes for AO_CCD_ID.\n",
            sizeof (AO_CCD_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCcdId = (AO_CCD_ID) calloc ( (size_t) 1, sizeof (AO_CCD_ID_STRUCT) )) 
       == NULL)
   {
      ERROR_SET ( 0, "Memory allocation for CCD geometry context failed",
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
 *   (AO_CTRL_ID) Pointer to control context structure, or NULL if 
 *                unsuccessful.
 *
 *   PURPOSE:
 *   Create a control context structure for the aO library
 *
 *   DESCRIPTION:
 *   This function creates and initialises a control context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CTRL_ID aoCtrlContextCreate (void)
{
   AO_CTRL_ID   aoCtrlId;

   /* Allocate memory for the control context structure, initialising its
    * contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCtrlContextCreate: Allocating %d bytes for AO_CTRL_ID\n",
            sizeof (AO_CTRL_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCtrlId = (AO_CTRL_ID) calloc ((size_t) 1, sizeof (AO_CTRL_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET ( 0, "Memory allocation for aO control context failed",
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
 *   (AO_CB_IM_ID) Pointer to image circular buffer context structure, or 
 *                 NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create a image circular buffer context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a image circular buffer context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CB_IM_ID aoCbImContextCreate (void)
{
   AO_CB_IM_ID   aoCbImId;

   /* Allocate memory for the image circular buffer context structure, 
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
            "Memory allocation for image circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbImId);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoCbCtrlContextCreate
 *
 *   INVOCATION:
 *   aoCbCtrlContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CB_CTRL_ID) Pointer to aO control circular buffer context structure, 
 *                   or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create a aO control circular buffer context structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises a aO control circular buffer context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

AO_CB_CTRL_ID aoCbCtrlContextCreate (void)
{
   AO_CB_CTRL_ID   aoCbCtrlId;

   /* Allocate memory for the aO control circular buffer context structure, 
    * initialising its contents to zero.
    */

#ifdef DEBUG
   printf ( "aoCbCtrlContextCreate: Allocating %d bytes for AO_CB_CTRL_ID\n",
            sizeof (AO_CB_CTRL_ID_STRUCT) );
#endif /* DEBUG */

   if ((aoCbCtrlId = (AO_CB_CTRL_ID) calloc ((size_t) 1, 
                                             sizeof (AO_CB_CTRL_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET ( 0, 
            "Memory allocation for aO control circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbCtrlId);
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
 *   (AO_CB_CTRL_ID) Pointer to FG control circular buffer context structure,
 *                   or NULL if unsuccessful.
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
 *   aoP1Lib.h
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
 *   (>) aoCcdId (AO_CCD_ID) Pointer to the CCD geometry context structure 
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the CCD geometry context structure
 *
 *   DESCRIPTION:
 *   This function displays the content of the CCD geometry context 
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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
 *   and stores the values into ctrlId.
 *   Note: P1 CCD is a 80x80 pixels. The coordinates of the first pixel are
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
 *   aoP1Lib.h
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
 *   aoP1Lib.h
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
 *   aoP1Lib.h
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

   int      i;                    /* Index                            */

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
 *   aoP1Lib.h
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
 *   aoP1Lib.h
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
 *   This function reads a control or interaction matrix of various dimensions
 *   from a file given by pMatFileName and stores the matrix into ctrlId.
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
 *   aoP1Lib.h
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

   int      type;                 /* Type of the matrix         */
   int      row, col;             /* Dimension of the matrix    */
   int      i, j;                 /* Index                      */
   float    value;                /* Element of the matrix      */
   MATRIX   mat;                  /* Matrix read                */
   char     comment[STRING_SIZE]; /* First line of comments     */
   FILE *   pFile;                /* File Id                    */

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
         aoCtrlId->intMatInitFlag = FALSE;
      else
         aoCtrlId->contMatInitFlag = FALSE;
      return (ERROR);
   }

   if ( type == AO_INT_MAT_TYPE ) /* interaction matrix */
   {
      if ( (row != aoCcdId->centroidsNb) || (col != aoCtrlId->aoModeNb) )
      {
         ERROR_SET4 ( 0,
            "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
            ERROR_LOG_SAVE, row, col, aoCcdId->centroidsNb, aoCtrlId->aoModeNb);
         aoCtrlId->intMatInitFlag = FALSE;
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
         aoCtrlId->contMatInitFlag = FALSE;
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
         aoCtrlId->intMatInitFlag = FALSE;
      else
         aoCtrlId->contMatInitFlag = FALSE;
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
                 aoCtrlId->intMatInitFlag = FALSE;
              else
                 aoCtrlId->contMatInitFlag = FALSE;
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
      strcpy ( aoCtrlId->intMatFileName, pMatFileName );
      aoCtrlId->intMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->intMat, row, col);
      aoCtrlId->contMatInitFlag = FALSE;
   }
   else
   {
      strcpy ( aoCtrlId->contMatFileName, pMatFileName );
      aoCtrlId->contMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->contMat, row, col);
   }

#ifdef DEBUG
   printf ( "aoRefRead(): matrix\n" );
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
 *   This function writes a control or interaction matrix of various dimensions
 *   to a file given by pMatFileName.
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
 *   aoP1Lib.h
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
 *   aoP1Lib.h
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

   int      type;                 /* Type of the matrix         */
   int      row, col;             /* Dimension of the matrix    */
   int      i, j;                 /* Index                      */
   float    value;                /* Element of the matrix      */
   MATRIX   mat;                  /* Matrix read                */
   char     comment[STRING_SIZE]; /* First line of comments     */
   FILE *   pFile;                /* File Id                    */

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
 *   aoP1Lib.h
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

      for ( i = 0 ; i < aoCcdId->xPixels*aoCcdId->yPixels ; i ++ )
          image[i] = 0.0;
      /*if ( aoFitsImageFloatWrite (fileName, image, aoCcdId->xPixels,
                                  aoCcdId->yPixels) == ERROR )
      {
         ERROR_SET1 ( 0, "Failed to write %s\n" , ERROR_LOG_SAVE, fileName );
         fclose (pFile);
         aoCtrlId->initFlag = FALSE;
         return ( ERROR );
      };*/
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->darkInitFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
      for ( i = 0 ; i < aoCcdId->xPixels*aoCcdId->yPixels ; i ++ )
          image[i] = 1.0;
      /*if ( aoFitsImageFloatWrite (fileName, image, aoCcdId->xPixels,
                                  aoCcdId->yPixels) == ERROR )
      {
         ERROR_SET1 ( 0, "Failed to write %s\n" , ERROR_LOG_SAVE, fileName );
         fclose (pFile);
         aoCtrlId->initFlag = FALSE;
         return ( ERROR );
      };*/
      aoCtrlId->initFlag = FALSE;
      aoCtrlId->flatInitFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
      aoCtrlId->initFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
      aoCtrlId->initFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
      aoCtrlId->initFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
                   "modeNb %d is greater than max mode %d",
                   ERROR_LOG_SAVE, mode, (int)(AO_MODE_NB) );
      aoCtrlId->initFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
                   "modeNb %d is greater than max mode %d",
                   ERROR_LOG_SAVE, mode, (int)(FG_MODE_NB) );
      aoCtrlId->initFlag = FALSE;
      /*fclose (pFile);
      return (ERROR);*/
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
      /*fclose (pFile);
      return (ERROR);*/
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
      /*fclose (pFile);
      return (ERROR);*/
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
      /*fclose (pFile);
      return (ERROR);*/
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
      /*fclose (pFile);
      return (ERROR);*/
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
               "Failed to read FG scale factor [%d] from the AO init file %s",
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
   aoCtrlId->thresholdDark = value;
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

   /* Read angle between M2 and P1 */

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

   /* Read angle between M1 and P1 */

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

   aoCtrlId->allowedSubapOff = 1;
   aoCtrlId->coaddCounter = 0;
   aoCtrlId->focusCounter = 0;
   aoCtrlId->previousFocus = 0.0;

   for ( i = 0 ; i < 2*SUBAP_NB ; i ++ )
       aoCtrlId->sumVect[i] = 0.0;

   aoCtrlId->initFlag = TRUE;
   
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
 *                        pIntMatFileName, pContMatFileName, pFgContMatFileName,
 *                        xCenter, yCenter, angleWithM2, angleWithM1, 
 *                        aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDarkFileName      (char *)     Pointer to the dark file name
 *   (>) pFlatFileName      (char *)     Pointer to the flat file name
 *   (>) pRefFileName       (char *)     Pointer to the reference file name
 *   (>) pIntMatFileName    (char *)     Pointer to the interaction matrix file 
 *                                       name
 *   (>) pContMatFileName   (char *)     Pointer to the control matrix file name
 *   (>) pFgContMatFileName (char *)     Pointer to the FG control matrix file 
 *                                       name
 *   (>) xCenter            (double)     New xCenter value for whole CCD
 *   (>) yCenter            (double)     New yCenter value for whole CCD
 *   (>) angleWithM2        (double)     New angle between M2 and P1
 *   (>) angleWithM1        (double)     New angle between M1 and P1
 *   (>) aoCcdId            (AO_CCD_ID)  Pointer to the CCD geometry context
 *                                       structure
 *   (<) aoCtrlId           (AO_CTRL_ID) Pointer to the control context 
 *                                       structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Update the control context structure from a DM screen
 *
 *   DESCRIPTION:
 *   Update the control context structure
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pDarkFileName is the full name of the file including the path.
 *   The pFlatFileName is the full name of the file including the path.
 *   The pRefFileName is the full name of the file including the path.
 *   The pIntMatFileName is the full name of the file including the path.
 *   The pContMatFileName is the full name of the file including the path.
 *   The pFgContMatFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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
   char *     pIntMatFileName,
   char *     pContMatFileName,
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
      /*for ( i = 0 ; i < aoCcdId->xPixels*aoCcdId->yPixels ; i ++ )
          image[i] = 0.0;
      if ( aoFitsImageFloatWrite (pDarkFileName, image, aoCcdId->xPixels,
                                  aoCcdId->yPixels) == ERROR )
      {
         ERROR_SET1 ( 0, "Failed to write %s\n" , ERROR_LOG_SAVE, 
                      pDarkFileName );
         aoCtrlId->initFlag = FALSE;
         return ( ERROR );
      };*/
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
      /*for ( i = 0 ; i < aoCcdId->xPixels*aoCcdId->yPixels ; i ++ )
          image[i] = 1.0;
      if ( aoFitsImageFloatWrite (pFlatFileName, image, aoCcdId->xPixels,
                                  aoCcdId->yPixels) == ERROR )
      {
         ERROR_SET1 ( 0, "Failed to write %s\n" , ERROR_LOG_SAVE, 
                      pFlatFileName );
         aoCtrlId->initFlag = FALSE;
         return ( ERROR );
      };*/
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

   /* Init the new interaction matrix */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): interaction matrix file name: %s\n", 
            pIntMatFileName );
#endif

   if ( aoMatRead ( pIntMatFileName, AO_INT_MAT_TYPE, aoCcdId, aoCtrlId ) 
        == ERROR )
   {
      ERROR_SET1 ( 0, 
                   "Failed when reading the interaction matrix file %s" , 
                   ERROR_LOG_SAVE, pIntMatFileName );
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   /* Init the new control matrix */

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): control matrix file name: %s\n", 
            pContMatFileName );
#endif

   if ( aoMatRead ( pContMatFileName, AO_CONT_MAT_TYPE, aoCcdId, aoCtrlId ) 
        == ERROR )
   {
      ERROR_SET1 ( 0, 
                   "Failed when reading the control matrix file %s" , 
                   ERROR_LOG_SAVE, pContMatFileName );
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

   /* Init the new angle between M2 and P1 */

   aoCtrlId->angleWithM2 = angleWithM2;

   aoCtrlId->cosAngleWithM2 = cos ( aoCtrlId->angleWithM2 );
   aoCtrlId->sinAngleWithM2 = sin ( aoCtrlId->angleWithM2 );

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): angleWithM2 = %f\n", 
            aoCtrlId->angleWithM2 );
   printf ( "aoCtrlContextUpdate(): cos(angleWithM2) = %f\n",
            aoCtrlId->cosAngleWithM2 );
   printf ( "aoCtrlContextUpdate(): sin(angleWithM2) = %f\n",
            aoCtrlId->sinAngleWithM2 );
#endif

   /* Init the new angle between M1 and P1 */

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
 *   (>) aoCcdId (AO_CCD_ID)  Pointer to the CCD geometry context structure
 *   (>) aoCtrlId (AO_CTRLID) Pointer to the control context structure
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
 *   aoP1Lib.h
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
      ERROR_SET ( 0, "Invalid CCD geometry context", ERROR_LOG_SAVE );
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
            (aoCtrlId->intMatInitFlag ? "TRUE" : "FALSE") );
   printf ( "aO Control matrix init flag: %s\n" ,
            (aoCtrlId->contMatInitFlag ? "TRUE" : "FALSE") );
   printf ( "FG Control matrix init flag: %s\n" ,
            (aoCtrlId->fgContMatInitFlag ? "TRUE" : "FALSE") );

   printf ( "Allowed subapertures to be off: %d\n", aoCtrlId->allowedSubapOff);
   printf ( "Focus counter : %d\n", aoCtrlId->focusCounter);

   printf ( "Dark file name: %s\n" , aoCtrlId->darkFileName );
   printf ( "Flat file name: %s\n" , aoCtrlId->flatFileName );
   printf ( "Reference file name: %s\n" , aoCtrlId->refVectFileName );
   printf ( "aO scale factor file name: %s\n" , aoCtrlId->aoScaleFileName );
   printf ( "Interaction matrix file name: %s\n" , aoCtrlId->intMatFileName );
   printf ( "aO Control matrix file name: %s\n" , aoCtrlId->contMatFileName );
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
      (void) aoColImStructShow (aoCcdId, aoCtrlId);

      printf ( "Interaction matrix: \n" );
      for ( i = 0 ; i < aoCcdId->centroidsNb ; i ++ )
      {
          for ( j = 0 ; j < aoCtrlId->aoModeNb ; j ++ )
              printf ( "%f " , aoCtrlId->intMat[i*aoCtrlId->aoModeNb + j] );
          printf ( "\n" );
      }

      printf ( "aO Control matrix: \n" );
      for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
      {
          for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
              printf ( "%f " , aoCtrlId->contMat[i*aoCcdId->centroidsNb + j] );
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
   printf ( "Threshold dark: %f\n" , aoCtrlId->thresholdDark );
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
   printf ( "coaddCounter: %d\n" , aoCtrlId->coaddCounter );
   printf ( "Sliding focus gain: %f\n" , aoCtrlId->slidingFocusGain );
   printf ( "One - Sliding focus gain: %f\n" , aoCtrlId->one_slidingFocusGain );

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
 *   aoP1Lib.h
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
 *                  pFgVect, pFgErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage           (float *)    Pointer to the image from which to 
 *                                     compute centroids
 *   (>) aoCcdId          (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *                                     structure
 *   (>) aoCtrlId         (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect (double *)   Pointer to the vector of total counts
 *   (<) pGuidesVect      (double *)   Pointer to the guides vector
 *   (<) pFgVect          (double *)   Pointer to the zernikes vector to send 
 *                                     to M2
 *   (<) pFgErrorsVect    (double *)   Pointer to the associated errors vector
 *   (<) pTime            (double *)   Pointer to the time associated to the
 *                                     vectors
 *   (<) pWfsStatus       (int *)      Pointer to the status flag when computing
 *                                     the centroids
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
 *   aoP1Lib.h
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
   double       tipScale;
   double       tiltScale;
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

   tipScale = aoCtrlId->fgScaleFactorVect[0];
   tiltScale = aoCtrlId->fgScaleFactorVect[1];

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

      *(pGuidesVect) = (x / total) - xCenter;
      *(pGuidesVect + 1) = (y / total) - yCenter;

      *(pFgVect) = tipScale *
      ( aoCtrlId->cosAngleWithM2 * (*pGuidesVect) -
        aoCtrlId->sinAngleWithM2 * (*(pGuidesVect+1)) );

      *(pFgVect + 1) = tiltScale *
      ( aoCtrlId->sinAngleWithM2 * (*pGuidesVect) +
        aoCtrlId->cosAngleWithM2 * (*(pGuidesVect +1)) );

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
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };

   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgErrorsVect, pTime) != OK )
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
 *                          pGuidesVect, pFgVect, pFgErrorsVect,
 *                          pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage           (float *)    Pointer to the image from which to
 *                                     compute the centroids
 *   (>) aoCcdId          (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *                                     structure
 *   (>) aoCtrlId         (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect (double *)   Pointer to the total counts vector
 *   (<) pGuidesVect      (double *)   Pointer to the centroids vector
 *   (<) pFgVect          (double *)   Pointer to the zernikes vector
 *   (<) pFgErrorsVect    (double *)   Pointer to the associated errors vector
 *   (<) pTime            (double *)   Pointer to the time associated to the
 *                                     vectors
 *   (<) pWfsStatus       (int *)      Pointer to the status flag when computing
 *                                     the centroids
 *
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
 *   aoP1Lib.h
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
   double       tipScale;
   double       tipScale2;
   double       tiltScale;
   double       tiltScale2;
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

   tipScale = aoCtrlId->fgScaleFactorVect[0];
   tipScale2 = tipScale * tipScale;
   tiltScale = aoCtrlId->fgScaleFactorVect[1];
   tiltScale2 = tiltScale * tiltScale;

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

      *(pGuidesVect) = xTemp - xCenter;
      *(pGuidesVect + 1) = yTemp - yCenter;

      *(pFgVect) = tipScale *
      ( aoCtrlId->cosAngleWithM2 * (*pGuidesVect) -
        aoCtrlId->sinAngleWithM2 * (*(pGuidesVect+1)) );

      *(pFgVect + 1) = tiltScale *
      ( aoCtrlId->sinAngleWithM2 * (*pGuidesVect) +
        aoCtrlId->cosAngleWithM2 * (*(pGuidesVect +1)) );

      *(pFgVect + 2) = 0.0;

      xSigma = (((xErr / total) - (xTemp * xTemp))/total);
      ySigma = (((yErr / total) - (yTemp * yTemp))/total);

      if ( xSigma < AO_MIN_DOUBLE )
         xSigma = 0.0;
      if ( ySigma < AO_MIN_DOUBLE )
         ySigma = 0.0;

      *(pFgErrorsVect) = sqrt(tipScale2 * (cos2*xSigma + sin2*ySigma));
      *(pFgErrorsVect + 1) = sqrt(tiltScale2 * (sin2*xSigma + cos2*ySigma));
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
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };

   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgErrorsVect, pTime) != OK )
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
 *   aoP1Lib.h
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
      }

      if  ( aoCtrlId->coaddCounter == imageNb )
      {
          for ( p = ps ; p < pMax; p ++ )
          {
               *p = (*(p) / imageNb);
          }
          aoCtrlId->coaddCounter = 0;
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoRmsNoiseDarkCompute
 *
 *   INVOCATION:
 *   aoRmsNoiseDarkCompute (pDark, aoCcdId, pRmsNoise)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDark          (float *)    Pointer to the dark from which to compute
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
 *   This routine computes for a dedicated dark image pDark the rms of the
 *   noise.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoRmsNoiseDarkCompute (
   float *      pDark,
   AO_CCD_ID    aoCcdId,
   double *     pRmsNoise
   )
{
   int          imageSize;
   float *      p;
   float *      pd;
   float *      pMax;
   double       value;
   double       meanPixel;
   double       variance;
   double       rmsrms;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = pDark;
   pMax = (float *)((int)pd + imageSize*sizeof(float));

   /* Compute mean and variance */

   meanPixel = 0.0;
   variance = 0.0;

   for ( p = pd ; p < pMax ; p ++ )
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
 *   a 6x6 spots image pImage according to the following criteria: ratePixel%
 *   of the brightest pixels.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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

   index = (int) ceil ((double)(aoCcdId->pixelsNb) * ratePixel);
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
 *   aoP1Lib.h
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
   double       totalSubap;
   double       total;
   double       xSubap;
   double       ySubap;
   double       pixelVal;
   double       xSubapCenter;
   double       ySubapCenter;

   m = 0;
   subapOffNb = 0;
   total = 0;
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
                      pixelVal = (double)(*pi) - aoCtrlId->threshold;
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
 *   aoModeCompute (pImage, aoCcdId, aoCtrlId, imageNb, aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage    (float *)    Pointer to the float buffer which contains the
 *                              image to be coadded
 *   (>) aoCcdId   (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *   (>) imageNb   (int)        Number of images to average
 *   (!) aoCbCtrlId (AO_CB_CTRL_ID) Pointer to the control circular buffer
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
 *   saved into the aoCbCtrlId circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoModeCompute (
   float *       pImage,
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId,
   int           imageNb,
   AO_CB_CTRL_ID aoCbCtrlId
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
   double *     pAoErrorsVect; 
   double *     pAo;
   double *     pErrorAo;
   double *     pCent;
   double *     pMaxAo;
   double *     pMaxCent;
   double *     pMat;
   double *     pScale;
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

         indexCtrl = aoCbCtrlId->position;
         pTotalCountsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].totalCountsVect;
         pCentroidsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].centroidsVect;
         pErrorCentroidsVect = 
         aoCbCtrlId->cbCtrlRecord[indexCtrl].errorCentroidsVect;
         pAoVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].aoVect;
         pAoErrorsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].aoErrorsVect; 
         pWfsStatus = &(aoCbCtrlId->cbCtrlRecord[indexCtrl].wfsStatus); 
         pTime = &(aoCbCtrlId->cbCtrlRecord[indexCtrl].time); 

         pErrorAo = pAoErrorsVect;
         pMaxAo = pAoVect + aoCtrlId->aoModeNb;
         pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;
         pMat = aoCtrlId->contMat;
         pScale = aoCtrlId->aoScaleFactorVect;

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

            for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
            {
                *pAo *= (*(pScale ++));
                *(pErrorAo ++) = 0.0;
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
            ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
            return (ERROR);
         };

         if ( writeWfsToTcs(aoCtrlId, pAoVect, pAoErrorsVect, pTime) != OK )
         {
            ERROR_SET ( 0, "Failed to write data to the TCS", ERROR_LOG_SAVE);
            return (ERROR);
         };

         /* Update the aoCbCtrlId circular buffer */

         if ( ++ aoCbCtrlId->position == CB_CTRL_RECORD_NB )
         {
            aoCbCtrlId->position = 0;
            aoCbCtrlId->counter ++;
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
 *   aoP1Lib.h
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
                   "./D%04d%02d%02dT%02d%02d%02dP1.cbi",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5]);
      }
      else
      {
         sprintf ( aoHeaderCbIm.cbImFileName, 
                   "%s/D%04d%02d%02dT%02d%02d%02dP1.cbi",
                   pCbImFilePath, timeArray[0], timeArray[1], timeArray[2], 
                   timeArray[3], timeArray[4], timeArray[5]);
      }
   }
   else
   {
      if ( ( strcmp (pCbImFilePath, "") == 0 ) ||
           ( strcmp (pCbImFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbIm.cbImFileName, "./defaultP1.cbi" );
      }
      else
      {
       
         sprintf ( aoHeaderCbIm.cbImFileName, "%s/defaultP1.cbi",
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
 *   aoP1Lib.h
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
 *   aoCbCtrlZero
 *
 *   INVOCATION:
 *   aoCbCtrlZero (aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCbCtrlId (AO_CB_CTRL_ID)  Pointer to the control circular buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine set to zero all the records of the control circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbCtrlZero
   (
   AO_CB_CTRL_ID   aoCbCtrlId     /* Pointer to the control circular buffer */
   )
{
   int             index;
   int             i;

   for ( index = 0 ; index < CB_CTRL_RECORD_NB ; index ++ )
   {
       aoCbCtrlId->cbCtrlRecord[index].time = 0.0;
       aoCbCtrlId->cbCtrlRecord[index].wfsStatus = 0;
       for ( i = 0 ; i < 2*SUBAP_NB ; i ++ )
       {
           aoCbCtrlId->cbCtrlRecord[index].totalCountsVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].centroidsVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].errorCentroidsVect[i] = 0.0;
       }
       for ( i = 0 ; i < MODE_NB ; i ++ )
       {
           aoCbCtrlId->cbCtrlRecord[index].aoVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].aoErrorsVect[i] = 0.0;
       }
   }

   aoCbCtrlId->exposureTime = 0.0;
   aoCbCtrlId->processingMode = 0;
   aoCbCtrlId->averageImageNb = 0;
   aoCbCtrlId->position = 0;
   aoCbCtrlId->offset = 0;
   aoCbCtrlId->counter = 0;

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
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbFgCtrlZero
   (
   AO_CB_FG_CTRL_ID   aoCbFgCtrlId  /* Pointer to the control circular buffer */
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
 *   aoCbCtrlSave
 *
 *   INVOCATION:
 *   aoCbCtrlSave (pCbCtrlFilePath, aoCcdId, aoCtrlId, aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbCtrlFilePath (char *)        Directory where to save the circular 
 *                                       buffer control
 *   (>) aoCcdId         (AO_CCD_ID)     Pointer to the CCD geometry structure
 *   (>) aoCtrlId        (AO_CTRL_ID)    Pointer to the control context 
 *                                       structure
 *   (>) aoCbCtrlId      (AO_CB_CTRL_ID) Pointer to the control circular buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Save the control circular buffer.
 *
 *   DESCRIPTION:
 *   This routine save the contents of the control circular buffer into a file
 *   for further purposes. The contents of the circular buffer is sorted in
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
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbCtrlSave
   (
   char *          pCbCtrlFilePath, /* Control circular buffer directory      */
   AO_CCD_ID       aoCcdId,         /* Pointer to the CCD geometry structure  */
   AO_CTRL_ID      aoCtrlId,        /* Pointer to the control structure       */
   AO_CB_CTRL_ID   aoCbCtrlId       /* Pointer to the control circular buffer */
   )
{
   int                         i;
   int                         index;
   int                         defNameFlag;
   int                         itemNb;
   int                         timeArray[7];
   double                      timeSave;

   AO_HEADER_CB_CTRL_ID_STRUCT aoHeaderCbCtrl;

   FILE *                      pFile;

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
      if ( ( strcmp (pCbCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbCtrlFilePath, "NONE") == 0 ) )
      {
         sprintf ( aoHeaderCbCtrl.cbCtrlFileName,
                   "./D%04d%02d%02dT%02d%02d%02dP1.cbc",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5]);
      }
      else
      {
         sprintf ( aoHeaderCbCtrl.cbCtrlFileName,
                   "%s/D%04d%02d%02dT%02d%02d%02dP1.cbc",
                   pCbCtrlFilePath, timeArray[0], timeArray[1], timeArray[2], 
                   timeArray[3], timeArray[4], timeArray[5]);
      }
   }
   else
   {
      if ( ( strcmp (pCbCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbCtrlFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbCtrl.cbCtrlFileName, "./defaultP1.cbc" );
      }
      else
      {
         sprintf ( aoHeaderCbCtrl.cbCtrlFileName, "%s/defaultP1.cbc",
                   pCbCtrlFilePath );
      }
   }

#ifdef DEBUG
   printf ( "File name: %s\n" , aoHeaderCbCtrl.cbCtrlFileName );
#endif

   /*
    * Init the header of the file
    */

   index = aoCbCtrlId->position;

   if ( aoCbCtrlId->counter == 0 )
   {
      aoHeaderCbCtrl.recordNb = index;
   }
   else
   {
      aoHeaderCbCtrl.recordNb = CB_CTRL_RECORD_NB;
   }

   aoHeaderCbCtrl.processingMode = aoCbCtrlId->processingMode;
   aoHeaderCbCtrl.centroidsNb = aoCcdId->centroidsNb;
   aoHeaderCbCtrl.aoModeNb = aoCtrlId->aoModeNb;
   aoHeaderCbCtrl.averageImageNb = aoCbCtrlId->averageImageNb;
   aoHeaderCbCtrl.exposureTime = aoCbCtrlId->exposureTime;
   for ( i = 0 ; i < aoCcdId->centroidsNb ; i ++ )
       aoHeaderCbCtrl.refWfsVect[i] = aoCtrlId->refWfsVect[i];
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       aoHeaderCbCtrl.aoScaleFactorVect[i] = aoCtrlId->aoScaleFactorVect[i];
   aoHeaderCbCtrl.threshold = aoCtrlId->threshold;
   aoHeaderCbCtrl.totalThreshold = aoCtrlId->totalThreshold;
   aoHeaderCbCtrl.angleWithM1 = aoCtrlId->angleWithM1;

   /*
    * Open the image circular buffer
    */

   pFile = fopen ( aoHeaderCbCtrl.cbCtrlFileName, "w" );

   if ( pFile == (FILE *)(NULL) )
   {
      ERROR_SET1 (0, "Failed to open in write mode file %s",
                 ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
   }
   else
   {
      /*
       * First save the header of the file
       */

      itemNb = fwrite ( (char *)&aoHeaderCbCtrl,
                        sizeof (char),
                        sizeof (AO_HEADER_CB_CTRL_ID_STRUCT),
                        pFile);

      if ( itemNb == NULL )
      {
         ERROR_SET1 (0, "Failed to write header in file %s",
                     ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
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
      printf ( "index: %d, counter: %d\n", index, aoCbCtrlId->counter);
#endif
      if ( aoCbCtrlId->counter == 0 )
      {
#ifdef DEBUG
         printf ( "Counter= 0 -> save records from 0 to %d\n", index - 1);
#endif
         for ( i = 0 ; i < index ; i ++ )
         {
             itemNb = fwrite ( (char *)&(aoCbCtrlId->cbCtrlRecord[i]),
                               sizeof (char),
                               sizeof (CB_CTRL_RECORD_STRUCT),
                               pFile);

             if ( itemNb == NULL )
             {
                ERROR_SET1 (0, "Failed to write record in file %s",
                            ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
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
                     index , CB_CTRL_RECORD_NB - 1 );
#endif
            for ( i = index ; i < CB_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbCtrlId->cbCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }

#ifdef DEBUG
            printf ( "then -> save records from 0 to %d\n", index - 1 );
#endif
            for ( i = 0 ; i < index ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbCtrlId->cbCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
                   (void)fclose (pFile);
                   return (ERROR);
                }
            }
         }
         else
         {
#ifdef DEBUG
            printf ( "Counter# 0, index = 0 -> save records from 0 to %d\n",
                     CB_CTRL_RECORD_NB - 1 );
#endif
            for ( i = 0 ; i < CB_CTRL_RECORD_NB ; i ++ )
            {
                itemNb = fwrite ( (char *)&(aoCbCtrlId->cbCtrlRecord[i]),
                                  sizeof (char),
                                  sizeof (CB_CTRL_RECORD_STRUCT),
                                  pFile);

                if ( itemNb == NULL )
                {
                   ERROR_SET1 (0, "Failed to write record in file %s",
                               ERROR_LOG_NOW, aoHeaderCbCtrl.cbCtrlFileName);
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
 *   aoCbFgCtrlSave
 *
 *   INVOCATION:
 *   aoCbFgCtrlSave (pCbFgCtrlFilePath, aoCcdId, aoCtrlId, aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbFgCtrlFilePath (char *)     Directory where to save the circular 
 *                                      buffer FG control
 *   (>) aoCcdId      (AO_CCD_ID)       Pointer to the CCD geometry structure
 *   (>) aoCtrlId     (AO_CTRL_ID)      Pointer to the control context structure
 *   (>) aoCbFgCtrlId (AO_CB_FGCTRL_ID) Pointer to the FG control circular 
 *                                      buffer
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
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS aoCbFgCtrlSave
   (
   char *           pCbFgCtrlFilePath, /* Control circular buffer directory   */
   AO_CCD_ID        aoCcdId,      /* Pointer to the CCD geometry structure    */
   AO_CTRL_ID       aoCtrlId,     /* Pointer to the control structure         */
   AO_CB_FG_CTRL_ID aoCbFgCtrlId  /* Pointer to the FG control circular buffer*/
   )
{
   int                         i;
   int                         index;
   int                         defNameFlag;
   int                         itemNb;
   int                         timeArray[7];
   double                      timeSave;

   AO_HEADER_CB_FG_CTRL_ID_STRUCT aoHeaderCbFgCtrl;

   FILE *                      pFile;

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
      if ( ( strcmp (pCbFgCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbFgCtrlFilePath, "NONE") == 0 ) )
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName,
                   "./D%04d%02d%02dT%02d%02d%02dP1.cbfgc",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5]);
      }
      else
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName,
                   "%s/D%04d%02d%02dT%02d%02d%02dP1.cbfgc",
                   pCbFgCtrlFilePath, timeArray[0], timeArray[1], timeArray[2], 
                   timeArray[3], timeArray[4], timeArray[5]);
      }
   }
   else
   {
      if ( ( strcmp (pCbFgCtrlFilePath, "") == 0 ) ||
           ( strcmp (pCbFgCtrlFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbFgCtrl.cbFgCtrlFileName, "./defaultP1.cbfgc" );
      }
      else
      {
         sprintf ( aoHeaderCbFgCtrl.cbFgCtrlFileName, "%s/defaultP1.cbfgc",
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
                               ERROR_LOG_NOW, aoHeaderCbFgCtrl.cbFgCtrlFileName);
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
 *                    pCentroidsVect, *pErrorCentroidsVect,
 *                    pFgVect, pFgErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage              (float *)    Pointer to the float buffer which 
 *                                        contains the image to be coadded
 *   (>) aoCcdId             (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId            (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect    (double *)   Pointer to the vector of total counts
 *   (<) pCentroidsVect      (double *)   Pointer to the guides vector
 *   (<) pErrorCentroidsVect (double *)   Pointer to the guides vector
 *   (<) pFgVect             (double *)   Pointer to the zernikes vector to 
 *                                        send to M2
 *   (<) pFgErrorsVect       (double *)   Pointer to the associated errors 
 *                                        vector
 *   (<) pTime               (double *)   Pointer to the time associated to the
 *                                        vectors
 *   (<) pWfsStatus          (int *)      Pointer to the status flag when 
 *                                        computing the centroids
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute tip, tilt and focus FG modes - Associated errors are not 
 *   computed and set to 0.
 *
 *   DESCRIPTION:
 *   This routine compute the centroids for each subapertures, basically a tip
 *   and tilt information. The centroids information are then used to compute
 *   the average tip, tilt and focus modes to send to the secondary mirror.
 *   This routine is the standard routine for FG on PWFS1 and should be used 
 *   after centering all the spots (aoGlobalGuide()).
 *   Note also that a temporal filter is used for the focus mode. This filter
 *   consists to a sliding average.

 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoGuideAndFocus (
   float *       pImage,
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId,
   double        *pTotalCountsVect,
   double        *pCentroidsVect,
   double        *pErrorCentroidsVect,
   double        *pFgVect,
   double        *pFgErrorsVect,
   double        *pTime,
   int           *pWfsStatus
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
   double       tipScale;
   double       tiltScale;
   double       focusScale;
   double       averageFocus;
   FG_VECT      fg;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));

   pMaxFg = fg + aoCtrlId->fgModeNb;
   pErrorFg = pFgErrorsVect;
   pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;
   pMat = aoCtrlId->fgContMat;

   tipScale = aoCtrlId->fgScaleFactorVect[0];
   tiltScale = aoCtrlId->fgScaleFactorVect[1];
   focusScale = aoCtrlId->fgScaleFactorVect[2];

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

      *(pFgVect) = tipScale * 
                   ( aoCtrlId->cosAngleWithM2 * (*fg) -
                   aoCtrlId->sinAngleWithM2 * (*(fg + 1)) );
      *(pFgVect + 1) = tiltScale * 
                       ( aoCtrlId->sinAngleWithM2 * (*fg) +
                         aoCtrlId->cosAngleWithM2 * (*(fg + 1)) );

      if ( aoCtrlId->focusCounter == 0 )
      {
         aoCtrlId->previousFocus = *(fg + 2);
         aoCtrlId->focusCounter ++;
      }
     
      averageFocus = (aoCtrlId->slidingFocusGain * (*(fg+2))) +
         (aoCtrlId->one_slidingFocusGain * aoCtrlId->previousFocus);

      *(pFgVect + 2) = focusScale * averageFocus;

      aoCtrlId->previousFocus = averageFocus; /* Bug fixed 11 June 2000 - cb */

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

   /* Get a timestamp to record at which time FG data are sent to M2 */

   if ( timeNow (pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };

   if ( writeWfsToSynchro(aoCtrlId, pFgVect, pFgErrorsVect, pTime) != OK )
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
 *   aoModeAnalyze (pImage, aoCcdId, aoCtrlId, aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage    (float *)    Pointer to the float buffer which contains the
 *                              image coadded
 *   (>) aoCcdId   (AO_CCD_ID)  Pointer to the AO CCD geometry context
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *   (!) aoCbCtrlId (AO_CB_CTRL_ID) Pointer to the control circular buffer
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
 *   the aO modes and centroids and modes are saved into the aoCbCtrlId 
 *   circular buffer.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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
   AO_CB_CTRL_ID aoCbCtrlId
   )
{
   int          indexCtrl;
   int *        pWfsStatus;
   double *     pTotalCountsVect;
   double *     pCentroidsVect;
   double *     pErrorCentroidsVect;
   double *     pAoVect;
   double *     pAoErrorsVect; 
   double *     pAo;
   double *     pErrorAo;
   double *     pCent;
   double *     pMaxAo;
   double *     pMaxCent;
   double *     pMat;
   double *     pScale;
   double *     pTime;

   /* Some initialisations */

   indexCtrl = aoCbCtrlId->position;
   pTotalCountsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].totalCountsVect;
   pCentroidsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].centroidsVect;
   pErrorCentroidsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].errorCentroidsVect;
   pAoVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].aoVect;
   pAoErrorsVect = aoCbCtrlId->cbCtrlRecord[indexCtrl].aoErrorsVect; 
   pWfsStatus = &(aoCbCtrlId->cbCtrlRecord[indexCtrl].wfsStatus); 
   pTime = &(aoCbCtrlId->cbCtrlRecord[indexCtrl].time); 

   pErrorAo = pAoErrorsVect;
   pMaxAo = pAoVect + aoCtrlId->aoModeNb;
   pMaxCent = pCentroidsVect + aoCcdId->centroidsNb;
   pMat = aoCtrlId->contMat;

   pScale = aoCtrlId->aoScaleFactorVect;

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

      for ( pAo = pAoVect ; pAo < pMaxAo ; pAo ++ )
      {
          *pAo *= (*(pScale ++));
          *(pErrorAo ++) = 0.0;
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
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };

   if ( writeWfsToTcs(aoCtrlId, pAoVect, pAoErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the TCS", ERROR_LOG_SAVE);
      return (ERROR);
   };

   /* Update the aoCbCtrlId circular buffer */

   if ( ++ aoCbCtrlId->position == CB_CTRL_RECORD_NB )
   {
      aoCbCtrlId->position = 0;
      aoCbCtrlId->counter ++;
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
 *   aoP1Lib.h
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
 *   aoColImStructZero
 *
 *   INVOCATION:
 *   aoColImStructZero (aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) aoCtrlId  (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Set to zero the interaction matrix structure
 *
 *   DESCRIPTION:
 *   This function sets to zero the interaction matrix structure of the AO 
 *   control structure given by aoCtrlId. 
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoColImStructZero (
   AO_CTRL_ID     aoCtrlId
   )
{
   int      i, j; /* Index */

   /* Set to the zero the interaction matrix structure */

   for ( i = 0 ; i < AO_MODE_NB ; i ++ )
   {
       aoCtrlId->intMatStruct[i].posAmplitude = 0.0;
       aoCtrlId->intMatStruct[i].negAmplitude = 0.0;

       for ( j = 0 ; j < 2*SUBAP_NB ; j ++ )
       {
           aoCtrlId->intMatStruct[i].posCentroidsVect[j] = 0.0;
           aoCtrlId->intMatStruct[i].negCentroidsVect[j] = 0.0;
       }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoColImStructShow
 *
 *   INVOCATION:
 *   aoColImStructShow (aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) aoCcdId  (AO_CCD_ID)  Pointer to the AO CCD geometry structure
 *   (>) aoCtrlId (AO_CTRL_ID) Pointer to the AO control structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the interaction matrix structure
 *
 *   DESCRIPTION:
 *   This function displays the contents of the interaction matrix structure 
 *   of the AO control structure given by aoCtrlId. 
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS aoColImStructShow (
   AO_CCD_ID     aoCcdId,
   AO_CTRL_ID    aoCtrlId
   )
{
   int      i, j; /* Index */

   /* Display the interaction matrix structure */

   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       printf ( "Mode : %d\n" , i + 1 );
       printf ( "Amplitude positive in microns: %f\n" , 
                (float)(aoCtrlId->intMatStruct[i].posAmplitude) );

       for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
           printf ( "%f " , aoCtrlId->intMatStruct[i].posCentroidsVect[j]);

       printf ( "\n" );

       printf ( "Amplitude negative in microns: %f\n" , 
                (float)(aoCtrlId->intMatStruct[i].negAmplitude) );

       for ( j = 0 ; j < aoCcdId->centroidsNb ; j ++ )
           printf ( "%f " , aoCtrlId->intMatStruct[i].negCentroidsVect[j] );

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
 *   Set to zero the interaction matrix and the control matrix of the aoCtrlId
 *   context structure
 *
 *   DESCRIPTION:
 *   This function sets to zero the interaction matrix and the control matrix 
 *   of the AO control structure given by aoCtrlId. 
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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

   /* Set to the zero the interaction matrix */

   aoCtrlId->intMatInitFlag = FALSE;
   aoCtrlId->contMatInitFlag = FALSE;

   strcpy ( aoCtrlId->intMatFileName, "");
   strcpy ( aoCtrlId->contMatFileName, "");

   for ( i = 0 ; i < 2*AO_MODE_NB*SUBAP_NB ; i ++ )
   {
       aoCtrlId->intMat[i] = 0.0;
       aoCtrlId->contMat[i] = 0.0;
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
 *   Compute the interaction and the control matrixes 
 *
 *   DESCRIPTION:
 *   This function computes the interaction matrix from the interaction matrix 
 *   structure:
 *   For all the modes:
 *   column interaction matrix [mode] = 
 *   (posCentroidsVect - negCentroidsVect)/(posAmplitude - negAmplitude)
 *   Then, the matrix is reduced to the non null columns, tip, tilt modes
 *   are filtered and the this result matrix is inverted by using the svd 
 *   routine. Then the inverse matrix is extended with null line corresponding 
 *   to the null column of the interaction matrix and multiply by -1 to give 
 *   the final and so well known control matrix. 
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   aoP1Lib.h
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
   int      row, col, k;  /* Index                                           */
   int      redColNb;     /* Column number of the reduced interaction matrix */

   double   tip, tilt;    /* Tip and tilt values to filter from the          */
                          /* interaction matrix                              */

   MATRIX   redIntMat;    /* Reduced interaction matrix                      */
   MATRIX   invRedIntMat; /* Inverse of the reduced interaction matrix       */

   /* Compute each column of the interaction matrix */

   k = 0;
   for ( col = 0 ; col < aoCtrlId->aoModeNb ; col ++ )
   {
       if ( (aoCtrlId->intMatStruct[col].posAmplitude != 0.0) &&
            (aoCtrlId->intMatStruct[col].negAmplitude != 0.0) )
       {
          for ( row = 0 ; row < aoCcdId->centroidsNb ; row ++ )
          {
            aoCtrlId->intMat[row*aoCtrlId->aoModeNb + col] =
            (aoCtrlId->intMatStruct[col].posCentroidsVect[row] -
             aoCtrlId->intMatStruct[col].negCentroidsVect[row])/
            (aoCtrlId->intMatStruct[col].posAmplitude - 
             aoCtrlId->intMatStruct[col].negAmplitude);
            
            *(redIntMat + row*aoCtrlId->aoModeNb + k) = 
            aoCtrlId->intMat[row*aoCtrlId->aoModeNb + col];
          }

          k ++;
       }
       else
       {
          for ( row = 0 ; row < aoCcdId->centroidsNb ; row ++ )
              aoCtrlId->intMat[row*aoCtrlId->aoModeNb + col] = 0.0;
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

   /* Determine the control matrix */
   
   k = 0;
   for ( row = 0 ; row < aoCtrlId->aoModeNb ; row ++ )
   {
       if ( (aoCtrlId->intMatStruct[row].posAmplitude != 0.0) &&
            (aoCtrlId->intMatStruct[row].negAmplitude != 0.0) )
       {
          for ( col = 0 ; col < aoCcdId->centroidsNb ; col ++)
          {
              aoCtrlId->contMat[row*aoCcdId->centroidsNb + col] =
              (-1.0) * (*(invRedIntMat + k*aoCcdId->centroidsNb + col));
          }
 
          k ++;
       }
       else
       {
          for ( col = 0 ; col < aoCcdId->centroidsNb ; col ++)
              aoCtrlId->contMat[row*aoCcdId->centroidsNb + col] = 0.0;
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
 *   aoP1Lib.h
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
 *                   pRefFileName, pRefX, pRefY, pImFileName, pCmFileName, 
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
 *   (<) pImFileName   (char *)   Pointer to the interaction matrix file name
 *   (<) pCmFileName   (char *)   Pointer to the control matrix file name
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
 *   aoP1Lib.h
 *
 *   DEFICIENCIES:
 *   Can't use ERROR_SET1, replace by printf for now - CB 11 july 2000
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
   char *   pImFileName,
   char *   pCmFileName,
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

   /* Read the name of the dark fits file and init the dark vector */

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

   /* Read the name of the flat fits file and init the flat vector */

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

   /* Skip the next lines of comments */

   for ( i = 0 ; i < 6 ; i ++ )
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

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

   /* Read the name of the interaction matrix file */

   if ( fgets (pImFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the name of the IM from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pImFileName[strlen(pImFileName) - 1] == '\n' )
   {
      pImFileName[strlen(pImFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n", 
               pImFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): IM file name: %s\n", pImFileName );
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

   /* Read the name of the control matrix file */

   if ( fgets (pCmFileName, STRING_SIZE, pFile) == (char *)NULL )
   {
      printf (  
      "Failed to read the name of the CM from the AO init file %s\n",
      pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pCmFileName[strlen(pCmFileName) - 1] == '\n' )
   {
      pCmFileName[strlen(pCmFileName) - 1] = '\0';
#ifdef DEBUG
      printf ( "aoCtrlFileRead(): last character of %s was return\n", 
               pCmFileName );
#endif
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): CM file name: %s\n", pCmFileName );
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

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): %s\n", comment );
#endif

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

   /* Read angle between M2 and P1 */

   if ( (fscanf (pFile, "%lf\n", pAngleM2)) == EOF )
   {
      printf ( "Failed to read angleM2 from the AO init file %s\n",
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

   /* Read angle between M1 and P1 */

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
