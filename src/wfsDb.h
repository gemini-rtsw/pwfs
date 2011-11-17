/*+
 * MODULE NAME:
 * wfsDb
 *
 * FILENAME:
 * wfsDb.h
 *
 * PURPOSE:
 * Include file for wfsDb.
 *
 * DEFICIENCIES:
 * Assuming that all records have the same top level prefix is incorrect,
 * since the HRWFS records have a different prefix.
 *
 * HISTORY MODIFICATIONS
 *-
 */

#ifndef   __INCwfsDbh
#define   __INCwfsDbh

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include "dbTypes.h"


/* defines */

   /*
    * Declare the data structures to contain EPICS record information.
    * The CAD_RECORD, CAR_RECORD and SIR_RECORD data types are declared
    * in "dbTypes.h". The structures are initialised in wfsDb.c.
    */

IMPORT char pppWfsDbRecFieldName [N_RECORD_TYPES][EPICS_MAX_NFIELD_PER_RECORD][EPICS_MAX_BYTES_FIELD_NAME + 2];
                                      /* Recognised field names for each      */
                                      /* type of record. (The "+2" in the     */
                                      /* string length allows for the "."     */
                                      /* and the null terminator).            */

IMPORT CAD_RECORD pWfsDbCadList [];   /* Array of CAD record structures.      */
IMPORT GSUB_RECORD pWfsDbGsubList []; /* Array of genSub record structures.   */
IMPORT CAR_RECORD pWfsDbCarList [];   /* Array of CAR record structures.      */
IMPORT SIR_RECORD pWfsDbSirList [];   /* Array of SIR record structures.      */

IMPORT int wfsDbNCadRecord;           /* Total number of CAD records.         */
IMPORT int wfsDbNGsubRecord;          /* Total number of genSub records.      */
IMPORT int wfsDbNCarRecord;           /* Total number of CAR records.         */
IMPORT int wfsDbNSirRecord;           /* Total number of SIR records.         */

IMPORT BOOL pWfsDbRecInitialised [];  /* Array of flags indicating when       */
                                      /* the set of records of each type      */
                                      /* have been initialised.               */
IMPORT BOOL wfsDbEpicsDbIsLocal;      /* Flag indicating whether the          */
                                      /* EPICS database resides on the        */
                                      /* local processor.                     */

#endif   /* ifndef __INCwfsDbh */
