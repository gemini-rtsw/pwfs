/*+
 * MODULE NAME:
 * wfsSite
 *
 * FILENAME:
 * wfsSite.h
 *
 * PURPOSE:
 * Include file for wfsSite
 *
 *-
 */


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include "gemTypes.h"


/* defines */

   /*
    * The IPADDR_TO_HEX macro converts the four numbers which define an
    * IP address into a hexadecimal code.
    *
    * NOTE: Each of the four arguments to this macro must be values
    * that can fit into a single byte, otherwise an overflow will
    * occur.
    */

#define   IPADDR_TO_HEX(a,b,c,d)     (((a) & 0xff << 24) | \
                                     ((b) & 0xff << 16) | \
                                     ((c) & 0xff << 8)  | \
                                     ((d) & 0xff))


   /*
    * Declare the data structure to contain information about
    * the processor used by each wavefront sensor control task.
    */

IMPORT char               pWfsSiteName[];        /* Site name string.         */

IMPORT WFS_ARCH_PROCESSOR pWfsArchProcessor[];   /* Processor definition      */
                                                 /* data structures.          */

IMPORT int                pWfsNumProcessors;     /* Number of processors.     */
