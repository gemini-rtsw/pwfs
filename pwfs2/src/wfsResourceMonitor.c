static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsResourceMonitor.c,v 1.1.1.1 1999-05-19 00:04:53 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   wfsResourceMonitor
 *
 *   FILENAME:
 *   wfsResourceMonitor.c
 *
 *   PURPOSE:
 *   Wavefront sensor task application code
 *
 *   DESCRIPTION:
 *   This file contains the function "wfsResourceMonitor", which is executed 
 *   by the resource monitoring task; together with any private functions used 
 *   by that task alone
 *
 *   INCLUDE FILES:
 *   wfsLib.h
 *   wfsResourceMonitor.h
 *
 *   DEFICIENCIES:
 *   None
 *
 *   ORIGINAL AUTHOR:
 *   Nick Dillon
 *
 *   MODIFIED BY:
 *   Steven Beard
 *
 * INDENT-OFF*
 *
 *INDENT-ON*
 *-
 */


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <taskLib.h>
#include <stdio.h>
#include <pipeDrv.h>
#include <ioLib.h>
#include <memLib.h>
#include <math.h>
#include <tickLib.h>
#include <ppc.h>
#include "car.h"
#include "gemTypes.h"
#include "timeoutLib.h"
#include "errorLib.h"

#include "epToVxLib.h"
#include "wfsLib.h"
#include "wfsResourceMonitor.h"

/* defines */

/* Private function */

LOCAL int wfsResource_getExecTime (int nLoop, int priority);

/* private variables */

BOOL wfsResourceMonitorStop = FALSE;         /* Stop resource monitoring task */
LOCAL double wfsResourceXDummy;              /* Dummy variables used          */
LOCAL double wfsResourceYDummy = 1.23456789; /* by wfsResource_getExecTime    */


/* -------------------------------------------------------------------------- */

STATUS   wfsResourceMonitor
   (
   const int      updateIntervalMicrosec,  /* Update SIRs at this interval.   */
   const int      cpuAveragingMicrosec     /* Average CPU time usage over this*/
                                           /* interval.                       */
   )
{

   /* Variables associated with VxWorks environment. */

   int            procNumber;          /* Processor number.                   */
   int            taskOptions;         /* VxWorks task options.               */

   /* Variables associated with SIR records. */

   DATREC_CONTEXT pRamUsedContext;     /* Context for RAM used SIR record.    */
   DATREC_CONTEXT pRamBlkContext;      /* Context for largest RAM block SIR   */
                                       /* record                              */
   DATREC_CONTEXT pCpuUsedContext;     /* Context for CPU used SIR record.    */

   /* Other general variables. */

   char         pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                   /* Name of EPICS record.                   */
   int          execTimeReference; /* Reference execution time, measured at   */
                                   /* highest priority.                       */
   int          execTime;          /* Actual execution time.                  */
   int          nLoop = 1;         /* Recommended no. of times to repeat loop.*/
   int          minTickCountReqd;  /* Min. no. ticks per measurement interval.*/
   int          nTickInterval;     /* No. ticks to wait during each loop cycle*/
   long         ramFractionUsed;   /* Fraction of RAM used, as a percentage.  */
   long         ramLargestFreeBlk; /* Largest free block of RAM in Kbytes.    */
   long         cpuUsage;          /* CPU usage expressed as a percentage.    */
   MEM_PART_STATS   partStats;     /* Memory partition statistics structure.  */

   /* Turn off floating point excetion errors */

   setFPE() ;

   /* Create and initialise an error context structure for this task */

   if (errorInit () == ERROR)
   {
      printErr( 
      "wfsResourceMonitor: Failed to initialise error context structure.\n" );
      return (ERROR);
   }

   /* Check the task executes with floating point co-processor support. */

   if (taskOptionsGet (taskIdSelf (), & taskOptions) == ERROR)
   {
      ERROR_SET (0, "Could not get VxWorks task options", ERROR_LOG_NOW);
      return (ERROR);
   }

   if ((taskOptions & VX_FP_TASK) == 0)
   {
      ERROR_SET (0, "Task must be run with VX_FP_TASK option", ERROR_LOG_NOW);
      return (ERROR);
   }


   /*
    * Obtain the CPU processor number for the task.
    * A value of -1 indicate the CPU processor number is undefined.
    */

   procNumber = sysProcNumGet ();

   if ( procNumber == -1)
   {
      ERROR_SET (0, "CPU processor number not set", ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Calibrate CPU processor time usage estimate. This is done by finding
    * the value of the loop count, "nloop",  which makes 
    * "wfsResource_getExecTime()" take at least "cpuAveragingMicrosec" 
    * microseconds of time to execute at the highest priority. The actual 
    * execution time is remembered for comparison later.
    */

   minTickCountReqd = cpuAveragingMicrosec * sysClkRateGet () / 1000000;
   if (minTickCountReqd <= 0) minTickCountReqd = 1;
   if (minTickCountReqd < 10)
   {
      MESSAGE_LOG2 (MSG_WARNING,
"Warning: Processor %d CPU estimate will be accurate to only about %f percent",
                  procNumber, (100.0 / (float) minTickCountReqd));
   }

   while ((execTimeReference = 
           wfsResource_getExecTime (nLoop, WFS_CPU_PRIORITY_MAX))
          < minTickCountReqd)
      nLoop *= 2;               /* Double nloop */

   /*
    * Determine the number of ticks to wait during each loop cycle,
    * based on the update interval given in microseconds.
    * A negative update interval implies an infinite wait.
    */

   if (updateIntervalMicrosec < 0)
   {
      nTickInterval = WAIT_FOREVER;
   }
   else
   {
      nTickInterval = 
      (updateIntervalMicrosec * sysClkRateGet () / 1000000) - execTimeReference;
      if (nTickInterval <= 0) nTickInterval = 1;
   }

   /*
    * Obtain the context to the three SIR records updated by this task The names
    * of the records are derived from the processor number.
    */

   sprintf (pRecordName, WFS_RAM_USED_SIR_NAME "%.2d", procNumber);
   if (epToVxRecContextGet (pRecordName, & pRamUsedContext, NULL) == ERROR)
   {
      ERROR_LOG ("Can't get WFS_RAM_USED SIR context");
      return (ERROR);
   }

   sprintf (pRecordName, WFS_RAM_LARGE_BLK_SIR_NAME "%.2d", procNumber);
   if (epToVxRecContextGet (pRecordName, & pRamBlkContext, NULL) == ERROR)
   {
      ERROR_LOG ("Can't get WFS_RAM_LARGE_BLK SIR context");
      return (ERROR);
   }

   sprintf (pRecordName, WFS_CPU_USAGE_SIR_NAME "%.2d", procNumber);
   if (epToVxRecContextGet (pRecordName, & pCpuUsedContext, NULL) == ERROR)
   {
      ERROR_LOG ("Can't get WFS_CPU_USAGE SIR context");
      return (ERROR);
   }

   /*
    * The task has been successfully initialised, so it can now go into
    * a loop monitoring the resource usage.
    * The resource usage task can be terminated by setting the
    * "wfsResourceMonitorStop" variable from the console.
    */

   while (! wfsResourceMonitorStop)
   {

      /* Wait the required number of ticks each loop cycle. */

      taskDelay (nTickInterval);

      /* Inquire the current RAM usage from VxWorks.
       * This assumes memory blocks are 1024 bytes in size.
       */

      memPartInfoGet (memSysPartId, & partStats);
      ramFractionUsed = (long) (100 * partStats.numBytesAlloc /
         (partStats.numBytesAlloc + partStats.numBytesFree));
      ramLargestFreeBlk = (long) (partStats.maxBlockSizeFree / 1024);

      /*
       * Determine the amount of free CPU by running a test function
       * "wfsResource_getExecTime()" at the lowest possible priority. The 
       * ratio of the current execution time with the reference execution time
       * (recorded earlier at the highest priority) indicates the amount
       * of free CPU available. The CPU usage is not allowed to be negative.
       */

      execTime = wfsResource_getExecTime (nLoop, WFS_CPU_PRIORITY_MIN);

      if (execTime == 0)
         cpuUsage = 0;      /* Avoid divide by zero. */
      else
         cpuUsage = 100 - execTimeReference * 100 / execTime;

      if (cpuUsage < 0) cpuUsage = 0;

      /* Write the RAM and CPU statistics to the SIR records. */

      if ((epToVxPipeWrite (NULL, (char *) & ramFractionUsed, pRamUsedContext) 
          == ERROR) ||
         (epToVxPipeWrite (NULL, (char *) & ramLargestFreeBlk, pRamBlkContext) 
          == ERROR) ||
         (epToVxPipeWrite (NULL, (char *) & cpuUsage, pCpuUsedContext) 
          == ERROR))
      {
         ERROR_LOG ("Failed to update resource usage SIR records");
         return (ERROR);
      }
   }

   MESSAGE_LOG1 (MSG_WARNING, 
                 "Resource monitor task on processor %d stopped.", procNumber);

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   wfsResource_getExecTime
 *
 *   INVOCATION:
 *   wfsResource_getExecTime (nloop, priority)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   nloop      (int)   Number of times to execute loop
 *   (>)   priority   (int)   Number of times to execute loop
 *
 *   FUNCTION VALUE:
 *   (int)   Number of ticks taken to execute the code.
 *
 *   PURPOSE:
 *   Time how long it takes to execute some code at a given priority
 *
 *   DESCRIPTION:
 *   This is a private function used by "wfsResourceMonitor" to compare the
 *   time taken to execute the same piece of CPU-intensive code at high and
 *   low priority. It executes the "square root" function nloop times at the
 *   given priority and reports the.
 *
 *   EXTERNAL VARIABLES:
 *   (<) wfsResourceXDummy (double) Dummy global variable used in calculation
 *   (>) wfsResourceYDummy (double) Dummy global variable used in calculation
 *
 *   PRIOR REQUIREMENTS:
 *   wfsResourceYDummy must have been initialised to a positive, non-zero value.
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *
 *   NOTES:
 *   I suspect the wfsResourceXDummy and wfsResourceYDummy variables are 
 *   present to prevent the compiler from optimising the code. SMB - 26 Nov 97.
 *-
 */

int   wfsResource_getExecTime
   (
   int   nLoop,
   int   priority
   )
{
   int      i;
   uint32   tickCount1;
   uint32   tickCount2;
   int      oldPriority;

   taskPriorityGet (taskIdSelf (), & oldPriority);
   taskPrioritySet (taskIdSelf (), priority);

   tickCount1 = tickGet ();

   if ( wfsResourceYDummy > 0.0 )
   {
      for (i = 0; i < nLoop; i++)
         wfsResourceXDummy = sqrt (1.0 / wfsResourceYDummy);
   }

   tickCount2 = tickGet ();

   taskPrioritySet (taskIdSelf (), oldPriority);

   return ((int) (tickCount2 - tickCount1));
}
