/*+
 * MODULE NAME:
 * wfsControl
 *
 * FILENAME:
 * wfsControl.h
 *
 * PURPOSE:
 * Include file for wavefront sensor control application code
 *
 * HISTORY MODIFICATION
 * 18 April 2000 - cb - tidy up not used commands
 *-
 */

#ifndef   __INCwfsControlh
#define   __INCwfsControlh


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

#define WFS_CONTROL_TASK_NAME      "wfsControl"    /* WFS control task name   */

#define WFS_CONTROL_STATE_SIR_NAME "state"         /* Name of SIR record to   */
                                                   /* contain WFS controller  */
                                                   /* state.                  */

#define WFS_CONTROL_HEALTH_NAME    "controlHealth" /* Name of SIR record to   */
                                                   /* contain WFS controller  */
                                                   /* health.                 */

#define WFS_CONTROL_REBOOT_SIR_NAME "rebooting"    /* Name of SIR record to   */
                                                   /* contain WFS controller  */
                                                   /* rebooting state.        */

#define WFS_CONTROL_PARK_SIR_NAME   "parking"      /* Name of SIR record to   */
                                                   /* contain WFS controller  */
                                                   /* parking state.          */

/*
 * Error number codes used by wfsControl.
 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
 */

#define S_wfsControl_BAD_COMMAND (M_wfsControl | 1)   /* Unrecognised command */

/*
 * Define the commands recognised by the wavefront sensor control task.
 */

enum
{
   WFS_CONTROL_CMD_PARK,         /* Park wavefront sensors.        */
   WFS_CONTROL_CMD_REBOOT,       /* Reboot wavefront sensor.       */
   WFS_CONTROL_CMD_SIMULATE,     /* Set simulation mode.           */
   WFS_CONTROL_CMD_DEBUG         /* Set debugging mode.            */
};


/* function declarations */

#ifndef   NO_EPICS
IMPORT STATUS   wfsControl (void);
#endif /* NO_EPICS */

#endif /* __INCwfsControlh */
