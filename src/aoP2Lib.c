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
 *   aoCbCtrlContextCreate() - Create a AO control circular buffer context 
 *                             structure
 *   aoCcdContextShow() - Display a AO CCD geometry context structure
 *   aoRefRead() - Read the SH reference file 
 *   aoFitsImageFloatRead() - Read a float image from a FITS file 
 *   aoFitsImageFloatWrite() - Write a float image to a FITS file 
 *   aoCtrlContextInit() - Init the AO control context structure
 *   aoCtrlContextUpdate() - Update the AO control context structure
 *   aoDarkSubtract() - Subtract a dark from an image 
 *   aoGlobalGuide() - Compute tip and tilt modes only over the whole CCD
 *   aoGlobalGuideAndError() - Compute tip and tilt modes only over the whole 
 *                             CCD and the associated errors
 *   aoImageFloatAverage() - Average float images
 *   aoRmsNoiseImageCompute() - To compute the rms of the noise
 *   aoThresholdCompute() - Compute the threshold 
 *   aoCtrlContextShow() - Display a AO Control context structure
 *   aoGuideAndFocus() - Compute tip, tilt and focus modes
 *   aoGuideAndFocusAndError() - Compute tip, tilt and focus modes and 
 *                               the assiocated errors
 *   aoCbImSave() - Save the image circular buffer
 *   aoCbCtrlSave() - Save the control circular buffer
 *   aoCbImZero() - Set to zero the image circular buffer
 *   aoCbCtrlZero() - Set to zero the control circular buffer
 *   aoDarkUpdate() - Update the dark buffer of the control context structure
 *   aoCtrlFileRead () - Read parameters from the AO control file
 * 
 *INDENT-OFF*
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
#include "aoP2Lib.h"

/********************************************************External functions ***/

extern STATUS writeWfsToSynchro ();           /* defined into writeZernikes.c */

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
 *   aoCbCtrlContextCreate
 *
 *   INVOCATION:
 *   aoCbCtrlContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (AO_CB_CTRL_ID) Pointer to AO control circular buffer context structure, 
 *                   or NULL if unsuccessful.
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

AO_CB_CTRL_ID aoCbCtrlContextCreate (void)
{
   AO_CB_CTRL_ID   aoCbCtrlId;

   /* Allocate memory for the AO control circular buffer context structure, 
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
            "Memory allocation for AO control circular buffer context failed",
            ERROR_LOG_SAVE );
      return (NULL);
   }

   return (aoCbCtrlId);
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
 *   and stores the values into ctrlId. 
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
          aoCtrlId->refVect[2*j] = refVectRead[2*i];
          aoCtrlId->refVect[2*j+1] = refVectRead[2*i+1];
          j ++;
       }
   }

   aoCcdId->subapUsedNb = aoCcdId->subapNb - aoCcdId->subapNotUsedNb;
   aoCcdId->centroidsNb = 2 * (aoCcdId->subapUsedNb);
   
   aoCtrlId->refInitFlag = TRUE;

#ifdef DEBUG
   printf ( "aoRefRead(): centers\n" );
   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       printf ( "subaperture %d: %f, %f\n" , i+1, aoCtrlId->refVect[2*i],
                aoCtrlId->refVect[2*i+1] );
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

   /* Read value of zernikes mode to be corrected */

   if ( (fscanf (pFile, "%d\n", &mode)) == EOF )
   {
      ERROR_SET1 ( 0, 
            "Failed to read the mode number from the AO init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   if ( mode > MODE_NB )
   {
      ERROR_SET2 ( 0, 
                   "modeNb %d is greater than max mode %d",
                   ERROR_LOG_SAVE, mode, (int)(MODE_NB) );
      fclose (pFile);
      aoCtrlId->initFlag = FALSE;
      return (ERROR);
   }

   aoCtrlId->modeNb = mode;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): mode number=%d\n", aoCtrlId->modeNb );
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

   aoCtrlId->refVect[aoCcdId->centroidsNb] = value;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): x center for whole CCD=%f\n", 
            aoCtrlId->refVect[aoCcdId->centroidsNb] );
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

   aoCtrlId->refVect[aoCcdId->centroidsNb + 1] = value;

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): y center for whole CCD=%f\n", 
            aoCtrlId->refVect[aoCcdId->centroidsNb + 1] );
#endif   

   /* Read the scaleFactorVect from the init file */

   for ( i = 0 ; i < aoCtrlId->modeNb ; i ++ )
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

      aoCtrlId->scaleFactorVect[i] = value;

#ifdef DEBUG
      printf ( "aoCtrlContextInit(): scaleFactorVect[%d]=%f\n", i, 
               aoCtrlId->scaleFactorVect[i] );
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

   aoCtrlId->cosAngle = cos ( aoCtrlId->angleWithM2 );
   aoCtrlId->sinAngle = sin ( aoCtrlId->angleWithM2 );

#ifdef DEBUG
   printf ( "aoCtrlContextInit(): angleWithM2 = %f\n", aoCtrlId->angleWithM2 );
   printf ( "aoCtrlContextInit(): cos(angleWithM2) = %f\n", 
            aoCtrlId->cosAngle );
   printf ( "aoCtrlContextInit(): sin(angleWithM2) = %f\n", 
            aoCtrlId->sinAngle );
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
 *   aoCtrlContextUpdate (pDarkFileName, pFlatFileName, pRefFileName, xCenter,
 *                        yCenter, angle, aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDarkFileName (char *)     Pointer to the dark file name 
 *   (>) pFlatFileName (char *)     Pointer to the flat file name 
 *   (>) pRefFileName  (char *)     Pointer to the reference file name 
 *   (>) xCenter       (double)     New xCenter value for whole CCD
 *   (>) yCenter       (double)     New yCenter value for whole CCD
 *   (>) angle         (double)     New angle between M2 and P2 
 *   (>) aoCcdId       (AO_CCD_ID)  Pointer to the AO CCD geometry context 
 *                                  structure 
 *   (<) aoCtrlId      (AO_CTRL_ID) Pointer to the AO control context structure
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
   double     xCenter,
   double     yCenter, 
   double     angle,
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

   aoCtrlId->refVect[aoCcdId->centroidsNb] = xCenter;

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): x center for whole CCD=%f\n", 
            aoCtrlId->refVect[aoCcdId->centroidsNb] );
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

   aoCtrlId->refVect[aoCcdId->centroidsNb + 1] = yCenter;

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): y center for whole CCD=%f\n", 
            aoCtrlId->refVect[aoCcdId->centroidsNb + 1] );
#endif   

   /* Init the new angle between M2 and P2 */

   aoCtrlId->angleWithM2 = angle;

   aoCtrlId->cosAngle = cos ( aoCtrlId->angleWithM2 );
   aoCtrlId->sinAngle = sin ( aoCtrlId->angleWithM2 );

#ifdef DEBUG
   printf ( "aoCtrlContextUpdate(): angleWithM2 = %f\n", aoCtrlId->angleWithM2 );
   printf ( "aoCtrlContextUpdate(): cos(angleWithM2) = %f\n", 
            aoCtrlId->cosAngle );
   printf ( "aoCtrlContextUpdate(): sin(angleWithM2) = %f\n", 
            aoCtrlId->sinAngle );
#endif

   /* End */

   aoCtrlId->initFlag = TRUE;

   return ( OK );
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
 *   aoGlobalGuideAndError
 *
 *   INVOCATION:
 *   aoGlobalGuideAndError (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, 
 *                          pCentroidsVect, pZernikesVect, 
 *                          pZernikesVectAfterRot, pErrorsVect, pTime, 
 *                          pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage                (float *)    Pointer to the image from which to 
 *                                          compute the centroids
 *   (>) aoCcdId               (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                          context structure
 *   (>) aoCtrlId              (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect      (double *)   Pointer to the total counts vector
 *   (<) pCentroidsVect        (double *)   Pointer to the centroids vector
 *   (<) pZernikesVect         (double *)   Pointer to the zernikes vector
 *   (<) pZernikesVectAfterRot (double *)   Pointer to the zernikes vector after
 *                                          rotation
 *   (<) pErrorsVect           (double *)   Pointer to the associated errors 
 *                                          vector
 *   (<) pTime                 (double *)   Pointer to the time associated to 
 *                                          the vectors
 *   (<) pWfsStatus            (int *)      Pointer to the status flag when 
 *                                          computing the centroids 
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
   double *     pCentroidsVect,
   double *     pZernikesVect,
   double *     pZernikesVectAfterRot,
   double *     pErrorsVect,
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
   double       *pGuidingVect;
   double       *pTotalVect;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refVect[aoCcdId->centroidsNb];
   yCenter = aoCtrlId->refVect[aoCcdId->centroidsNb + 1];

#ifdef DEBUG
   printf ( "aoGlobalGuideAndError(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   tipScale = aoCtrlId->scaleFactorVect[0];
   tipScale2 = tipScale * tipScale;
   tiltScale = aoCtrlId->scaleFactorVect[1];
   tiltScale2 = tiltScale * tiltScale;

   pGuidingVect = pCentroidsVect + aoCcdId->centroidsNb;
   pTotalVect = pTotalCountsVect + aoCcdId->subapUsedNb;

   cos2 = aoCtrlId->cosAngle * aoCtrlId->cosAngle;
   sin2 = aoCtrlId->sinAngle * aoCtrlId->sinAngle;

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

   *pTotalVect = total;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
#ifdef DEBUG
      printf ( "aoGlobalGuideAndError(): x=%f, y=%f, total=%f\n" , x , y , total );
#endif

      *pWfsStatus = OK;

      xTemp = (x / total);
      yTemp = (y / total);

      *(pGuidingVect) = xTemp - xCenter;
      *(pGuidingVect + 1) = yTemp - yCenter;

      *(pZernikesVect) = tipScale * 
      ( aoCtrlId->cosAngle * (*pGuidingVect) + 
        aoCtrlId->sinAngle * (*(pGuidingVect+1)) );

      *(pZernikesVect + 1) = tiltScale *
      ( aoCtrlId->cosAngle * (*(pGuidingVect +1)) -
        aoCtrlId->sinAngle * (*pGuidingVect) ); 

      *(pZernikesVect + 2) = 0.0;

      xSigma = (((xErr / total) - (xTemp * xTemp))/total);
      ySigma = (((yErr / total) - (yTemp * yTemp))/total);

      if ( xSigma < AO_MIN_DOUBLE )
         xSigma = 0.0;
      if ( ySigma < AO_MIN_DOUBLE )
         ySigma = 0.0;

      *(pErrorsVect) = sqrt(tipScale2 * (cos2*xSigma + sin2*ySigma));
      *(pErrorsVect + 1) = sqrt(tiltScale2 * (sin2*xSigma + cos2*ySigma));
      *(pErrorsVect + 2) = 0.0; 

#ifdef DEBUG
      printf ( "aoGlobalGuideAndError(): z[0]=%f, z[1]=%f\n" ,
               *pZernikesVect, *(pZernikesVect +1) );
#endif
   }
   else
   {
      *pWfsStatus = AO_SH_OFF;

      *(pGuidingVect) = 0.0;
      *(pGuidingVect + 1) = 0.0;

      *(pZernikesVect) = 0.0;
      *(pZernikesVect + 1) = 0.0;
      *(pZernikesVect + 2) = 0.0;

      *(pErrorsVect) = 0.0;
      *(pErrorsVect + 1) = 0.0;
      *(pErrorsVect + 2) = 0.0;
   }

   if ( timeNow (pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };
 
   if ( writeWfsToSynchro(aoCtrlId, pZernikesVect, pZernikesVectAfterRot, 
                          pErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoGlobalGuide
 *
 *   INVOCATION:
 *   aoGlobalGuide (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, pCentroidsVect,
 *                  pZernikesVect, pZernikesVectAfterRot, pErrorsVect, pTime, 
 *                  pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage                (float *)    Pointer to the image from which to 
 *                                          compute the centroids
 *   (>) aoCcdId               (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                          context structure
 *   (>) aoCtrlId              (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCountsVect      (double *)   Pointer to the total counts vector
 *   (<) pCentroidsVect        (double *)   Pointer to the centroids vector
 *   (<) pZernikesVect         (double *)   Pointer to the zernikes vector
 *   (<) pZernikesVectAfterRot (double *)   Pointer to the zernikes vector after
 *                                          rotation
 *   (<) pErrorsVect           (double *)   Pointer to the associated errors 
 *                                          vector
 *   (<) pTime                 (double *)   Pointer to the time associated 
 *                                          to the vectors
 *   (<) pWfsStatus            (int *)      Pointer to the status flag when 
 *                                          computing the centroids 
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
   double *     pCentroidsVect,
   double *     pZernikesVect,
   double *     pZernikesVectAfterRot,
   double *     pErrorsVect,
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
   double       *pGuidingVect;
   double       *pTotalVect;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refVect[aoCcdId->centroidsNb];
   yCenter = aoCtrlId->refVect[aoCcdId->centroidsNb + 1];

#ifdef DEBUG
   printf ( "aoGlobalGuide(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   tipScale = aoCtrlId->scaleFactorVect[0];
   tiltScale = aoCtrlId->scaleFactorVect[1];

   pGuidingVect = pCentroidsVect + aoCcdId->centroidsNb;
   pTotalVect = pTotalCountsVect + aoCcdId->subapUsedNb;

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

   *pTotalVect = total;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
#ifdef DEBUG
      printf ( "aoGlobalGuide(): x=%f, y=%f, total=%f\n" , x , y , total );
#endif

      *pWfsStatus = OK;

      *(pGuidingVect) = (x / total) - xCenter;
      *(pGuidingVect + 1) = (y / total) - yCenter;

      *(pZernikesVect) = tipScale * 
      ( aoCtrlId->cosAngle * (*pGuidingVect) + 
        aoCtrlId->sinAngle * (*(pGuidingVect+1)) );

      *(pZernikesVect + 1) = tiltScale *
      ( aoCtrlId->cosAngle * (*(pGuidingVect +1)) -
        aoCtrlId->sinAngle * (*pGuidingVect) ); 

      *(pZernikesVect + 2) = 0.0;

      *(pErrorsVect) = 0.0;
      *(pErrorsVect + 1) = 0.0;
      *(pErrorsVect + 2) = 0.0; 

#ifdef DEBUG
      printf ( "aoGlobalGuide(): z[0]=%f, z[1]=%f\n" ,
               *pZernikesVect, *(pZernikesVect +1) );
#endif
   }
   else
   {
      *pWfsStatus = AO_SH_OFF;

      *(pGuidingVect) = 0.0;
      *(pGuidingVect + 1) = 0.0;

      *(pZernikesVect) = 0.0;
      *(pZernikesVect + 1) = 0.0;
      *(pZernikesVect + 2) = 0.0;

      *(pErrorsVect) = 0.0;
      *(pErrorsVect + 1) = 0.0;
      *(pErrorsVect + 2) = 0.0;
   }

   if ( timeNow (pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };
 
   if ( writeWfsToSynchro(aoCtrlId, pZernikesVect, pZernikesVectAfterRot, 
                          pErrorsVect, pTime) != OK )
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
 *   aoCtrlContextShow
 *
 *   INVOCATION:
 *   aoCtrlContextShow (aoCcdId, aoCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) aoCcdId (AO_CCD_ID) Pointer to the AO CCD geometry context structure 
 *   (>) aoCtrlId (AO_CTRLID) Pointer to the AO control context structure 
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of the AO control context structure
 *
 *   DESCRIPTION:
 *   This function displays the content of the AO control context structure.
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
   AO_CTRL_ID aoCtrlId
   )
{
   int    i;

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
   printf ( "Dark file name: %s\n" , aoCtrlId->darkFileName );
   printf ( "Flat file name: %s\n" , aoCtrlId->flatFileName );
   printf ( "Reference file name: %s\n" , aoCtrlId->refVectFileName );

   printf ( "darkVect= %f %f %f %f %f\n" , 
            aoCtrlId->darkVect[0], aoCtrlId->darkVect[1],
            aoCtrlId->darkVect[2], aoCtrlId->darkVect[3],
            aoCtrlId->darkVect[4]); 
   printf ( "darkVect= %f %f %f %f %f ...\n" , 
            aoCtrlId->darkVect[5], aoCtrlId->darkVect[6], aoCtrlId->darkVect[7],
            aoCtrlId->darkVect[8], aoCtrlId->darkVect[9]);
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

   for ( i = 0 ; i < aoCcdId->subapUsedNb ; i ++ )
       printf ( "subaperture %d: %f, %f\n" , i+1, aoCtrlId->refVect[2*i],
                aoCtrlId->refVect[2*i+1] );

   printf ( "X center = %f\n" , aoCtrlId->refVect[aoCcdId->centroidsNb] );
   printf ( "Y center = %f\n" , aoCtrlId->refVect[aoCcdId->centroidsNb + 1] );

   printf ( "Mode number: %d\n" , aoCtrlId->modeNb );

   for ( i = 0 ; i < aoCtrlId->modeNb ; i ++ )
       printf ( "scalefactorVect[%d]= %f\n", i, aoCtrlId->scaleFactorVect[i] );


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
   printf ( "Cos Angle with M2: %f\n" , aoCtrlId->cosAngle );
   printf ( "Sin Angle with M2: %f\n" , aoCtrlId->sinAngle );
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
 *   aoGuideAndFocus
 *
 *   INVOCATION:
 *   aoGuideAndFocus (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, 
 *                    pCentroidsVect, pErrorCentroidsVect, pZernikesVect, 
 *                    pZernikesVectAfterRot, pErrorsVect, pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage                (float *)    Pointer to the image from which to 
 *                                          compute the centroids
 *   (>) aoCcdId               (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                          context structure
 *   (>) aoCtrlId              (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCounts          (double *)   Pointer to the total counts vector
 *   (<) pCentroidsVect        (double *)   Pointer to the centroids vector
 *   (<) pErrorCentroidsVect   (double *)   Pointer to the errors centroids 
 *                                          vector
 *   (<) pZernikesVect         (double *)   Pointer to the zernikes vector
 *   (<) pZernikesVectAfterRot (double *)   Pointer to the zernikes vector after
 *                                          rotation
 *   (<) pErrorsVect           (double *)   Pointer to the associated errors 
 *                                          vector
 *   (<) pTime                 (double *)   Pointer to the time associated to 
 *                                          the vectors
 *   (<) pWfsStatus            (int *)      Pointer to the status flag when 
 *                                          computing the centroids 
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
 *   This routine is the standard routine for PWFS2 and should be used after 
 *   centering all the spots.
 *   Note also that a temporal filter is used for the focus mode. This filter 
 *   consists to a sliding average.
 *   31 oct 2000 - cb remove the current sliding average. Replaced by a 
 *   butterworth filter in writeZernikes.c
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
   double *     pZernikesVect,
   double *     pZernikesVectAfterRot,
   double *     pErrorsVect,
   double *     pTime,
   int *        pWfsStatus
   )
{
   int          imageSize;
   int          i, j;
   int          k, l;
   int          m;
   int          subapNb;
   int          subapOffNb;
   int          totalOff;     /* TRUE or FALSE */
   float *      pi;
   float *      pd;
   float *      pMin;
   float *      pMax;
   double       total;
   double       x;
   double       y;
   double       totalSubap;
   double       xSubap;
   double       ySubap;
   double       pixelVal;
   double       xCenter;
   double       yCenter;
   double       xSubapCenter;
   double       ySubapCenter;
   double       tipScale;
   double       tiltScale;
   double       focusScale;
   double       tip, tilt, focus;
   double       averageFocus;
   double       *pGuidingVect;
   double       *pTotalVect;
   double       *pFocusMat;

   double       focusMatrix[2*SUBAP_NB] = {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};
   double       binFocusMatrix[2*SUBAP_NB] = {-2.0, -2.0, 2.0, -2.0, -2.0, 2.0, 2.0, 2.0};

   /* Some initialisations */

   if ( aoCcdId->binningFlag == TRUE )
      pFocusMat = binFocusMatrix;
   else
      pFocusMat = focusMatrix;

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refVect[aoCcdId->centroidsNb];
   yCenter = aoCtrlId->refVect[aoCcdId->centroidsNb + 1];

#ifdef DEBUG
   printf ( "aoGuideAndFocus(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   tipScale = aoCtrlId->scaleFactorVect[0];
   tiltScale = aoCtrlId->scaleFactorVect[1];
   focusScale = aoCtrlId->scaleFactorVect[2];

#ifdef DEBUG
   printf ( "tipScale = %f, tiltScale = %f, focusScale = %f\n" ,
            tipScale, tiltScale, focusScale );
#endif
 
   pTotalVect = pTotalCountsVect + aoCcdId->subapUsedNb;
   totalOff = FALSE;

   /* Dark subtraction */

   for ( pi = pImage ; pi < pMax ; pi ++ )
       *pi = ( *pi - *(pd ++) );

   /* Thresholding and centroiding */

   total = (double)(0.0);
   x = (double)(0.0);
   y = (double)(0.0);
   *pWfsStatus = OK;

   m = 0;
   subapOffNb = 0;
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

              xSubapCenter = aoCtrlId->refVect[2*m] - aoCcdId->xRaster*l;
              ySubapCenter = aoCtrlId->refVect[2*m+1] - 
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

              x += (xSubap + l*totalSubap*aoCcdId->xRaster); 
              y += (ySubap + k*totalSubap*aoCcdId->yRaster); 
              total += totalSubap;
#ifdef DEBUG
              printf ( "TOTAL x=%f, y=%f, total=%f\n", x, y, total);
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

   /* Compute the guiding values */

   *pTotalVect = total;

   pGuidingVect = pCentroidsVect + aoCcdId->centroidsNb;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
      *(pGuidingVect) = (x/total) - xCenter;
      *(pGuidingVect + 1) = (y/total) - yCenter;
   }
   else
   {
      *(pGuidingVect) = 0.0;
      *(pGuidingVect + 1) = 0.0;
        
      totalOff = TRUE;
   }

#ifdef DEBUG
   printf ( "totalOff = %d, GuidX=%f, guidY=%f\n", 
            totalOff, *(pGuidingVect), *(pGuidingVect + 1));
#endif

   /* Check the number of subapertures with no light and compute the Z modes */

   if ( totalOff == FALSE )
   {
      if ( *pWfsStatus == AO_SUBAP_OFF)
      {
         if ( subapOffNb > aoCtrlId->allowedSubapOff ) 
         {
            *(pZernikesVect) = 0.0;
            *(pZernikesVect + 1) = 0.0;
            *(pZernikesVect + 2) = 0.0;

            *(pErrorsVect) = 0.0;
            *(pErrorsVect + 1) = 0.0;
            *(pErrorsVect + 2) = 0.0;

            *pWfsStatus = AO_SH_OFF;
#ifdef DEBUG
            printf ( "subapOffNb %d -> all z to zero\n" , subapOffNb);
#endif
         }
         else
         {
            /* Computation of the tip/tilt modes only */

            tip = (double)(0.0);
            tilt = (double)(0.0);
            for ( m = 0 ; m < aoCcdId->subapUsedNb ; m ++ )
            {
                tip += *(pCentroidsVect + 2*m); 
                tilt += *(pCentroidsVect + 2*m + 1); 
            }

            tip /= (double)(aoCcdId->subapUsedNb - 1);
            tilt /= (double)(aoCcdId->subapUsedNb - 1);

            *(pZernikesVect + 0) = tipScale * 
             ( (aoCtrlId->cosAngle * tip) + (aoCtrlId->sinAngle * tilt) );

            *(pZernikesVect + 1) = tiltScale *
             ( (aoCtrlId->cosAngle * tilt) - (aoCtrlId->sinAngle * tip) ); 

            *(pZernikesVect + 2) = 0.0;

            *(pErrorsVect) = 0.0;
            *(pErrorsVect + 1) = 0.0;
            *(pErrorsVect + 2) = 0.0;
#ifdef DEBUG
            printf ( "subapOffNb %d -> compute only TT\n" , subapOffNb);
            printf ( "Tip=%f, tilt=%f\n", tip, tilt);
            printf ( "Z[0]=%f, z[1]=%f\n", *(pZernikesVect + 0), 
                     *(pZernikesVect + 1));
#endif
         }
      }
      else
      {
         /* Computation of the tip/tilt modes */

         tip = (double)(0.0);
         tilt = (double)(0.0);
         for ( m = 0 ; m < aoCcdId->subapUsedNb ; m ++ )
         {
             tip += *(pCentroidsVect + 2*m); 
             tilt += *(pCentroidsVect + 2*m + 1); 
         }

         tip /= (double)(aoCcdId->subapUsedNb);
         tilt /= (double)(aoCcdId->subapUsedNb);

         *(pZernikesVect + 0) = tipScale * 
          ( (aoCtrlId->cosAngle * tip) + (aoCtrlId->sinAngle * tilt) );

         *(pZernikesVect + 1) = tiltScale *
          ( (aoCtrlId->cosAngle * tilt) - (aoCtrlId->sinAngle * tip) ); 

         *(pErrorsVect) = 0.0;
         *(pErrorsVect + 1) = 0.0;
#ifdef DEBUG
         printf ( "subapOffNb %d -> compute everything\n" , subapOffNb);
         printf ( "Tip=%f, tilt=%f\n", tip, tilt);
         printf ( "Z[0]=%f, z[1]=%f\n", *(pZernikesVect + 0), 
                  *(pZernikesVect + 1));
#endif

         /* Computation of the focus only if all subap used */

         if ( aoCcdId->subapUsedNb == SUBAP_NB )
         {
            focus = 0.0;
            for ( m = 0 ; m < aoCcdId->centroidsNb ; m ++ )
            {
                focus += (*(pCentroidsVect + m) * (*(pFocusMat + m)));
            }

            focus /= (double)(aoCcdId->centroidsNb);
         }
         else
         {
            focus = 0.0;
         }

         if ( aoCtrlId->focusCounter == 0 )
         {
            aoCtrlId->previousFocus = focus;
            aoCtrlId->focusCounter ++;
         }

         averageFocus = (aoCtrlId->slidingFocusGain * focus) + 
         (aoCtrlId->one_slidingFocusGain * aoCtrlId->previousFocus);

         *(pZernikesVect + 2) = focusScale * averageFocus;

         aoCtrlId->previousFocus = averageFocus; 

#ifdef DEBUG
         printf ( "focus=%f, averageFocus=%f\n", focus, averageFocus);
#endif

/*
         *(pZernikesVect + 2) = focusScale * focus;
*/
      
         *(pErrorsVect + 2) = 0.0;

#ifdef DEBUG
         printf ( "focus=%f\n", focus);
         printf ( "Z[2]=%f\n", *(pZernikesVect + 2));
#endif
      }
   }
   else
   {
      *(pZernikesVect) = 0.0;
      *(pZernikesVect + 1) = 0.0;
      *(pZernikesVect + 2) = 0.0;

      *(pErrorsVect) = 0.0;
      *(pErrorsVect + 1) = 0.0;
      *(pErrorsVect + 2) = 0.0;

      *pWfsStatus = AO_SH_OFF;
   }

   if ( timeNow (pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };
 
   if ( writeWfsToSynchro(aoCtrlId, pZernikesVect, pZernikesVectAfterRot, 
                          pErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   aoGuideAndFocusAndError
 *
 *   INVOCATION:
 *   aoGuideAndFocusAndError (pImage, aoCcdId, aoCtrlId, pTotalCountsVect, 
 *                            pCentroidsVect, pErrorCentroidsVect, 
 *                            pZernikesVect, pZernikesVectAfterRot, pErrorsVect,
 *                            pTime, pWfsStatus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pImage                (float *)    Pointer to the image from which to 
 *                                          compute the centroids
 *   (>) aoCcdId               (AO_CCD_ID)  Pointer to the AO CCD geometry 
 *                                          context structure
 *   (>) aoCtrlId              (AO_CTRL_ID) Pointer to the AO control structure
 *   (<) pTotalCounts          (double *)   Pointer to the total counts vector
 *   (<) pCentroidsVect        (double *)   Pointer to the centroids vector
 *   (<) pErrorCentroidsVect   (double *)   Pointer to the errors centroids 
 *                                          vector
 *   (<) pZernikesVect         (double *)   Pointer to the zernikes vector
 *   (<) pZernikesVectAfterRot (double *)   Pointer to the zernikes vector after
 *                                          rotation
 *   (<) pErrorsVect           (double *)   Pointer to the associated errors 
 *                                          vector
 *   (<) pTime                 (double *)   Pointer to the time associated to 
 *                                          the vectors
 *   (<) pWfsStatus            (int *)      Pointer to the status flag when 
 *                                          computing the centroids 
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   To compute tip, tilt and focus modes - Associated errors are computed
 *
 *   DESCRIPTION:
 *   This routine computes the centroids for each subapertures, basically a tip 
 *   and tilt information. The centroids information are then used to compute 
 *   the average tip, tilt and focus modes to send to the secondary mirror. 
 *   This routine is the standard routine for PWFS2 and should be used after 
 *   centering all the spots.
 *   Note also that a temporal filter is used for the focus mode. This filter 
 *   consists to a sliding average.
 *   Associated errors are computed based on the centroiding error computation
 *   31 oct 2000 - cb remove the current sliding average. Replaced by a 
 *   butterworth filter in writeZernikes.c
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

STATUS aoGuideAndFocusAndError (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   double *     pTotalCountsVect,
   double *     pCentroidsVect,
   double *     pErrorCentroidsVect,
   double *     pZernikesVect,
   double *     pZernikesVectAfterRot,
   double *     pErrorsVect,
   double *     pTime,
   int *        pWfsStatus
   )
{
   int          imageSize;
   int          i, j;
   int          k, l;
   int          m;
   int          subapNb;
   int          subapOffNb;
   int          totalOff;     /* TRUE or FALSE */
   float *      pi;
   float *      pd;
   float *      pMin;
   float *      pMax;
   double       total;
   double       x;
   double       y;
   double       totalSubap;
   double       xSubap;
   double       ySubap;
   double       xTemp;
   double       yTemp;
   double       xErr;
   double       yErr;
   double       cos2;
   double       sin2;
   double       pixelVal;
   double       xCenter;
   double       yCenter;
   double       xSubapCenter;
   double       ySubapCenter;
   double       tipScale;
   double       tiltScale;
   double       focusScale;
   double       tipScale2;
   double       tiltScale2;
   double       focusScale2;
   double       tip, tilt, focus;
   double       tipErr, tiltErr, focusErr;
   double       averageFocus;
   double       *pGuidingVect;
   double       *pTotalVect;
   double       *pFocusMat;

   double       focusMatrix[2*SUBAP_NB] = {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};
   double       binFocusMatrix[2*SUBAP_NB] = {-2.0, -2.0, 2.0, -2.0, -2.0, 2.0, 2.0, 2.0};

   /* Some initialisations */

   if ( aoCcdId->binningFlag == TRUE )
      pFocusMat = binFocusMatrix;
   else
      pFocusMat = focusMatrix;

   imageSize = aoCcdId->pixelsNb;
   pd = aoCtrlId->darkVect;
   pMax = (float *)((int)pImage + imageSize*sizeof(float));
   
   xCenter = aoCtrlId->refVect[aoCcdId->centroidsNb];
   yCenter = aoCtrlId->refVect[aoCcdId->centroidsNb + 1];

#ifdef DEBUG
   printf ( "aoGuideAndFocusAndError(): xCenter = %f, yCenter= %f\n" ,
            xCenter, yCenter );
#endif

   tipScale = aoCtrlId->scaleFactorVect[0];
   tiltScale = aoCtrlId->scaleFactorVect[1];
   focusScale = aoCtrlId->scaleFactorVect[2];
   tipScale2 = tipScale * tipScale;
   tiltScale2 = tiltScale * tiltScale;
   focusScale2 = focusScale * focusScale;
   cos2 = aoCtrlId->cosAngle * aoCtrlId->cosAngle;
   sin2 = aoCtrlId->sinAngle * aoCtrlId->sinAngle;

#ifdef DEBUG
   printf ( "tipScale = %f, tiltScale = %f, focusScale = %f\n" ,
            tipScale, tiltScale, focusScale );
#endif
 
   pTotalVect = pTotalCountsVect + aoCcdId->subapUsedNb;
   totalOff = FALSE;

   /* Dark subtraction */

   for ( pi = pImage ; pi < pMax ; pi ++ )
       *pi = ( *pi - *(pd ++) );

   /* Thresholding and centroiding */

   total = (double)(0.0);
   x = (double)(0.0);
   y = (double)(0.0);
   *pWfsStatus = OK;

   m = 0;
   subapOffNb = 0;
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
              xErr = (double)(0.0);
              yErr = (double)(0.0);
              totalSubap = (double)(0.0);

              xSubapCenter = aoCtrlId->refVect[2*m] - aoCcdId->xRaster*l;
              ySubapCenter = aoCtrlId->refVect[2*m+1] - 
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
                         xTemp = pixelVal*j;
                         yTemp = pixelVal*i;
                         xSubap += xTemp;
                         ySubap += yTemp;
                         xErr += xTemp*j;
                         yErr += yTemp*i;
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
                 xTemp = xSubap/totalSubap;
                 yTemp = ySubap/totalSubap;
                 *(pCentroidsVect + 2*m) = xTemp - xSubapCenter;
                 *(pCentroidsVect + 2*m + 1) = yTemp - ySubapCenter;

                 *(pErrorCentroidsVect + 2*m) = 
                 (((xErr / totalSubap) - (xTemp * xTemp))/totalSubap);
                 *(pErrorCentroidsVect + 2*m + 1) = 
                 (((yErr / totalSubap) - (yTemp * yTemp))/totalSubap);
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
#ifdef DEBUG
                 printf ( "SUBAP%d OFF centX=%f, centY=%f\n",
                          subapNb, *(pCentroidsVect + 2*m), 
                          *(pCentroidsVect + 2*m + 1));
#endif
              }

              x += (xSubap + l*totalSubap*aoCcdId->xRaster); 
              y += (ySubap + k*totalSubap*aoCcdId->yRaster); 
              total += totalSubap;
#ifdef DEBUG
              printf ( "TOTAL x=%f, y=%f, total=%f\n", x, y, total);
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

   /* Compute the guiding values */

   *pTotalVect = total;

   pGuidingVect = pCentroidsVect + aoCcdId->centroidsNb;

   if ( (total - aoCtrlId->totalThreshold) > (double)(AO_MIN_DOUBLE) )
   {
      *(pGuidingVect) = (x/total) - xCenter;
      *(pGuidingVect + 1) = (y/total) - yCenter;
   }
   else
   {
      *(pGuidingVect) = 0.0;
      *(pGuidingVect + 1) = 0.0;
        
      totalOff = TRUE;
   }

#ifdef DEBUG
   printf ( "totalOff = %d, GuidX=%f, guidY=%f\n", 
            totalOff, *(pGuidingVect), *(pGuidingVect + 1));
#endif

   /* Check the number of subapertures with no light and compute the Z modes */

   if ( totalOff == FALSE )
   {
      if ( *pWfsStatus == AO_SUBAP_OFF)
      {
         if ( subapOffNb > aoCtrlId->allowedSubapOff ) 
         {
            *(pZernikesVect) = 0.0;
            *(pZernikesVect + 1) = 0.0;
            *(pZernikesVect + 2) = 0.0;

            *(pErrorsVect) = 0.0;
            *(pErrorsVect + 1) = 0.0;
            *(pErrorsVect + 2) = 0.0;

            *pWfsStatus = AO_SH_OFF;
#ifdef DEBUG
            printf ( "subapOffNb %d -> all z to zero\n" , subapOffNb);
#endif
         }
         else
         {
            /* Computation of the tip/tilt modes only */

            tip = (double)(0.0);
            tilt = (double)(0.0);
            tipErr = (double)(0.0);
            tiltErr = (double)(0.0);
            for ( m = 0 ; m < aoCcdId->subapUsedNb ; m ++ )
            {
                tip += *(pCentroidsVect + 2*m); 
                tilt += *(pCentroidsVect + 2*m + 1); 
                tipErr += *(pErrorCentroidsVect + 2*m); 
                tiltErr += *(pErrorCentroidsVect + 2*m + 1); 
            }

            tip /= (double)(aoCcdId->subapUsedNb - 1);
            tilt /= (double)(aoCcdId->subapUsedNb - 1);

            *(pZernikesVect + 0) = tipScale * 
             ( (aoCtrlId->cosAngle * tip) + (aoCtrlId->sinAngle * tilt) );

            *(pZernikesVect + 1) = tiltScale *
             ( (aoCtrlId->cosAngle * tilt) - (aoCtrlId->sinAngle * tip) ); 

            *(pZernikesVect + 2) = 0.0;

            if ( tipErr < AO_MIN_DOUBLE )
               tipErr = 0.0;
            if ( tiltErr < AO_MIN_DOUBLE )
               tiltErr = 0.0;

            *(pErrorsVect) = sqrt(tipScale2 * (cos2*tipErr + sin2*tiltErr));
            *(pErrorsVect + 1) = 
            sqrt(tiltScale2 * (sin2*tipErr + cos2*tiltErr));
            *(pErrorsVect + 2) = 0.0;
#ifdef DEBUG
            printf ( "subapOffNb %d -> compute only TT\n" , subapOffNb);
            printf ( "Tip=%f, tilt=%f\n", tip, tilt);
            printf ( "Z[0]=%f, z[1]=%f\n", *(pZernikesVect + 0), 
                     *(pZernikesVect + 1));
#endif
         }
      }
      else
      {
         /* Computation of the tip/tilt modes */

         tip = (double)(0.0);
         tilt = (double)(0.0);
         tipErr = (double)(0.0);
         tiltErr = (double)(0.0);
         for ( m = 0 ; m < aoCcdId->subapUsedNb ; m ++ )
         {
             tip += *(pCentroidsVect + 2*m); 
             tilt += *(pCentroidsVect + 2*m + 1); 
             tipErr += *(pErrorCentroidsVect + 2*m); 
             tiltErr += *(pErrorCentroidsVect + 2*m + 1); 
         }

         tip /= (double)(aoCcdId->subapUsedNb);
         tilt /= (double)(aoCcdId->subapUsedNb);

         *(pZernikesVect + 0) = tipScale * 
          ( (aoCtrlId->cosAngle * tip) + (aoCtrlId->sinAngle * tilt) );

         *(pZernikesVect + 1) = tiltScale *
          ( (aoCtrlId->cosAngle * tilt) - (aoCtrlId->sinAngle * tip) ); 

         *(pErrorsVect) = sqrt(tipScale2 * (cos2*tipErr + sin2*tiltErr));
         *(pErrorsVect + 1) = sqrt(tiltScale2 * (sin2*tipErr + cos2*tiltErr));
#ifdef DEBUG
         printf ( "subapOffNb %d -> compute everything\n" , subapOffNb);
         printf ( "Tip=%f, tilt=%f\n", tip, tilt);
         printf ( "Z[0]=%f, z[1]=%f\n", *(pZernikesVect + 0), 
                  *(pZernikesVect + 1));
#endif

         /* Computation of the focus only if all subap used */

         if ( aoCcdId->subapUsedNb == SUBAP_NB )
         {
            focus = 0.0;
            focusErr = 0.0;
            for ( m = 0 ; m < aoCcdId->centroidsNb ; m ++ )
            {
                focus += (*(pCentroidsVect + m) * (*(pFocusMat + m)));
                focusErr += *(pErrorCentroidsVect + m);
            }

            focus /= (double)(aoCcdId->centroidsNb);
         }
         else
         {
            focus = 0.0;
            focusErr = 0.0;
         }

         if ( aoCtrlId->focusCounter == 0 )
         {
            aoCtrlId->previousFocus = focus;
            aoCtrlId->focusCounter ++;
         }

         averageFocus = (aoCtrlId->slidingFocusGain * focus) + 
         (aoCtrlId->one_slidingFocusGain * aoCtrlId->previousFocus);

         *(pZernikesVect + 2) = focusScale * averageFocus;

         aoCtrlId->previousFocus = averageFocus; 
#ifdef DEBUG
         printf ( "focus=%f, averageFocus=%f\n", focus, averageFocus);
#endif

/*
         *(pZernikesVect + 2) = focusScale * focus;
*/

         if ( focusErr < AO_MIN_DOUBLE )
            focusErr = 0.0;

         *(pErrorsVect + 2) = sqrt (focusScale2*focusErr) ;

#ifdef DEBUG
         printf ( "focus=%f\n", focus);
         printf ( "Z[2]=%f\n", *(pZernikesVect + 2));
#endif
      }
   }
   else
   {
      *(pZernikesVect) = 0.0;
      *(pZernikesVect + 1) = 0.0;
      *(pZernikesVect + 2) = 0.0;

      *(pErrorsVect) = 0.0;
      *(pErrorsVect + 1) = 0.0;
      *(pErrorsVect + 2) = 0.0;

      *pWfsStatus = AO_SH_OFF;
   }

   if ( timeNow (pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to take the time" , ERROR_LOG_SAVE );
      return (ERROR);
   };
 
   if ( writeWfsToSynchro(aoCtrlId, pZernikesVect, pZernikesVectAfterRot, 
                          pErrorsVect, pTime) != OK )
   {
      ERROR_SET ( 0, "Failed to write data to the synchro bus", ERROR_LOG_SAVE);
      return (ERROR);
   };

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
 *   aoCbCtrlSave
 *
 *   INVOCATION:
 *   aoCbCtrlSave (pCbCtrlFilePath, aoCcdId, aoCtrlId, aoCbCtrlId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCbCtrlFilePath (char *)    Directory where to save the circular buffer
 *                                   control
 *   (>) aoCcdId    (AO_CCD_ID)      Pointer to the CCD geometry structure
 *   (>) aoCtrlId   (AO_CTRL_ID)     Pointer to the control context structure
 *   (>) aoCbCtrlId (AO_CB_CTRL_ID)  Pointer to the control circular buffer
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
 *   aoP2Lib.h
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
                   "./D%04d%02d%02dT%02d%02d%02dP2.cbc",
                   timeArray[0], timeArray[1], timeArray[2], timeArray[3],
                   timeArray[4], timeArray[5] );
      }
      else
      {
         sprintf ( aoHeaderCbCtrl.cbCtrlFileName, 
                   "%s/D%04d%02d%02dT%02d%02d%02dP2.cbc",
                   pCbCtrlFilePath, timeArray[0], timeArray[1], timeArray[2], 
                   timeArray[3], timeArray[4], timeArray[5]) ;
      }
   }
   else
   {
      if ( ( strcmp (pCbCtrlFilePath, "") == 0 ) || 
           ( strcmp (pCbCtrlFilePath, "NONE") == 0 ) )
      {
         strcpy ( aoHeaderCbCtrl.cbCtrlFileName, "./defaultP2.cbc" );
      }
      else
      {
         sprintf ( aoHeaderCbCtrl.cbCtrlFileName, "%s/defaultP2.cbc" ,
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
   aoHeaderCbCtrl.centroidsNb = aoCcdId->centroidsNb + 2; 
   aoHeaderCbCtrl.modeNb = aoCtrlId->modeNb;
   aoHeaderCbCtrl.exposureTime = aoCbCtrlId->exposureTime;
   for ( i = 0 ; i < aoCcdId->centroidsNb + 2 ; i ++ )
       aoHeaderCbCtrl.refVect[i] = aoCtrlId->refVect[i];
   for ( i = 0 ; i < aoCtrlId->modeNb ; i ++ )
       aoHeaderCbCtrl.scaleFactorVect[i] = aoCtrlId->scaleFactorVect[i];
   aoHeaderCbCtrl.threshold = aoCtrlId->threshold;
   aoHeaderCbCtrl.totalThreshold = aoCtrlId->totalThreshold;
   aoHeaderCbCtrl.angleWithM2 = aoCtrlId->angleWithM2;
   aoHeaderCbCtrl.slidingFocusGain = aoCtrlId->slidingFocusGain;

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
 *   aoP2Lib.h
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
       for ( i = 0 ; i < (2*SUBAP_NB + 2) ; i ++ )
       {
           aoCbCtrlId->cbCtrlRecord[index].totalCountsVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].centroidsVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].errorCentroidsVect[i] = 0.0;
       }
       for ( i = 0 ; i < MODE_NB ; i ++ )
       {
           aoCbCtrlId->cbCtrlRecord[index].zernikesVect[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].zernikesVectAfterRot[i] = 0.0;
           aoCbCtrlId->cbCtrlRecord[index].errorsVect[i] = 0.0;
       }
   }
  
   aoCbCtrlId->exposureTime = 0.0;
   aoCbCtrlId->processingMode = 0;
   aoCbCtrlId->position = 0;
   aoCbCtrlId->offset = 0;
   aoCbCtrlId->counter = 0;

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
 *   aoCtrlFileRead (pInitFileName, pPath, pDarkFileName, pFlatFileName, pAngle,
 *                   pRefX, pRefY, pRefFileName, pThresh, pTotalThresh)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName (char *)   Pointer to the AO init file name 
 *   (<) pPath         (char *)   Pointer to the path
 *   (<) pDarkFileName (char *)   Pointer to the dark file name
 *   (<) pFlatFileName (char *)   Pointer to the dark file name
 *   (<) pAngle        (double *) Pointer to the angle with M2
 *   (<) pRefX         (double *) Pointer to the X center for the whole CCD
 *   (<) pRefY         (double *) Pointer to the X center for the whole CCD
 *   (<) pRefFileName  (char *)   Pointer to the SH reference file name
 *   (<) pThresh       (double *) Pointer to the threshold
 *   (<) pTotalThresh  (double *) Pointer to the total flux threshold
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
   double * pAngle,
   double * pRefX,
   double * pRefY,
   char *   pRefFileName,
   double * pThresh,
   double * pTotalThresh
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

   /* Skip the next line - number of modes */

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

   for ( i = 0 ; i < 8 ; i ++ )
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

   if ( (fscanf (pFile, "%lf\n", pAngle)) == EOF )
   {
      printf ( "Failed to read angle from the AO init file %s\n",
               pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoCtrlFileRead(): angleWithM2 = %f\n", *pAngle );
#endif

   /* End - close and return */

   fclose (pFile);

   return ( OK );
}
