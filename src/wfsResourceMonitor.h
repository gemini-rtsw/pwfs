/*+
 * MODULE NAME:
 * wfsResourceMonitor
 *
 * FILENAME:
 * wfsResourceMonitor.h
 *
 * PURPOSE:
 * Include file for wavefront sensor application code
 *
 *-
 */

#ifndef   __INCwfsResourceMonitorh
#define   __INCwfsResourceMonitorh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include "wfsLib.h"
#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

/* defines */

#define WFS_RAM_USED_SIR_NAME       "ramUsed"
                                       /* Name of SIR record to   */
                                       /* contain amount of RAM   */
                                       /* used.                   */

#define WFS_RAM_LARGE_BLK_SIR_NAME  "ramFreeblk"
                                       /* Name of SIR record to   */
                                       /* contain size of largest */
                                       /* free RAM block.         */

#define WFS_CPU_USAGE_SIR_NAME      "cpuUsed"
                                       /* Name of SIR record to   */
                                       /* contain amount of CPU   */
                                       /* used.                   */

#define WFS_CPU_PRIORITY_MAX        0       /* Max task priority.      */
#define WFS_CPU_PRIORITY_MIN        255     /* Min task priority.      */

   /*
    * Error number codes used by wfsResourceMonitor.
    * These are designed to be processed using the vxWorks "makeStatTbl"
    * utility.
    */

   /* (No error codes) */


/* function declarations */

IMPORT STATUS wfsCpuResourceMonitor (int updateIntervalMicrosec,
                                     int cpuAveragingMicrosec);

#endif /* __INCwfsResourceMonitorh */

