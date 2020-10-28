/*+
 * MODULE NAME:
 * errorLog
 *
 * FILENAME:
 * errorLog.h
 *
 * PURPOSE:
 * Include file for errorLog application code
 *
 *-
 */

#ifndef   __INCerrorLogh
#define   __INCerrorLogh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <stdio.h>
#include "gemModNum.h"


/* defines */

/*
 * Define the task name, pipe name, message prefix and initial message size
 * for the error logging task.
 */

#define   LOGTASK_TASK_NAME         "errorLog"
#define   LOGTASK_PIPE_NAME         "errorLog"
#define   LOGTASK_INIT_MSG_PREFIX   "Initial_Error_Count: "
#define   LOGTASK_INIT_MSG_SIZE     21

   /*
    * Error number codes used by errorLog.
    * These are designed to be processed using the vxWorks "makeStatTbl" utility
    */

#define S_errorLog_FOPEN_FAIL   (M_errorLog | 1) /* Error while opening log   */
                                                 /* file                      */
#define S_errorLog_FCLOSE_FAIL  (M_errorLog | 2) /* Error while closing log   */
                                                 /* file                      */
#define S_errorLog_BAD_COMMAND  (M_errorLog | 3) /* Unrecognised command      */
#define S_errorLog_READ_FAILURE (M_errorLog | 4) /* Read from pipe failed     */
#define S_errorLog_EPICS_ERROR  (M_errorLog | 5) /* Error reported by EPICS   */


/* Define the commands recognised by the error logging task.
 * Only one command (set mode) is recognised.
 */

enum
   {
   LOGTASK_CMD_OPEN = 0,     /* Open log file.             */
   LOGTASK_CMD_CLOSE,        /* Close log file.            */
   LOGTASK_CMD_CLEAR         /* Clear error counters.      */
   };

#endif /* __INCerrorLogh */
