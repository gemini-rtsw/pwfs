static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsControl.c,v 1.6 2001-03-24 03:30:03 cboyer Exp $"};

/*+
 * MODULE NAME:
 * wfsControl
 *
 * FILENAME:
 * wfsControl.c
 *
 * PURPOSE:
 * Wavefront sensor control task application code
 *
 * DESCRIPTION:
 * This file contains the main CAD commands of the pwfs1 system.
 *
 * INCLUDE FILES:
 * wfsLib.h
 * wfsControl.h
 *
 * DEFICIENCIES:
 * None known
 *
 * AUTHORS:
 * Nick Dillon
 * Steven Beard
 *
 * HISTORY MODIFICATIONS
 * 20 February 2001 - cb - exit properly the dhs when reboot
 * 18 April 2000 - cb - remove all not used CAD commands. 
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

#include <rebootLib.h>

#include "dhs.h" 

#include "epToVxLib.h"
#include "detControl.h"
#include "wfsLib.h"
#include "wfsControl.h"

#include "wfsDb.h"


/***************************************************** External global data ***/

extern BOOL        detDhsConnected;                /* defined in detControl.c */

extern DHS_CONNECT detDhsConnection;               /* defined in detControl.c */


/**** Global variabnes. These are distinguished with a "wfsControl" prefix. ***/

BOOL wfsControlStop = FALSE;                /* Stop WFS control task.         */


/* -------------------------------------------------------------------------- */

STATUS   wfsControl (void)
{
   /* Variables associated with VxWorks environment. */

   int               taskOptions;         /* VxWorks task options.            */
   STATUS            (* pipeCreate) ();   /* Pointer to pipeCreate function.  */

   /* Variables associated with CAD/CAR command protocol. */

   CAD_CMD_CONTEXT   cadCmdContext;       /* Command context.                 */
   int               commandNumber;       /* Command number.                  */
   uint32            errorNumber;         /* Error number.                    */

   /* Variables associated with SIR records. */

   DATREC_CONTEXT    pStateContext;       /* Context for state SIR record.    */

   DATREC_CONTEXT    pRebootContext;      /* Context for rebooting state SIR  */
                                          /* record.                          */
   DATREC_CONTEXT    pParkContext;        /* Context for parking state SIR    */
                                          /* record.                          */

   /* Data Handling System variables. */

   DHS_STATUS        dhsErrno;            /* DHS error number.                */

   /* Other general variables. */

   long              simMode;             /* Code for simulation mode.        */
   char              pSimMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                          /* Simulation mode string.          */
   long              debugMode;           /* Code for debug mode.             */
   char              pDebugMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                          /* Debug mode string.               */
   long              rebootState;         /* Rebooting state.                 */
   long              parkState;           /* parking state.                   */

   /* Create and initialise an error context structure for this task */

   if (errorInit () == ERROR)
   {
      printErr( "wfsControl: Failed to initialise error context structure.\n" );
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

   pipeCreate = pipeDevCreate;  

   /*
    * Get the CAD command context structure (using the appropriate pipe driver)
    * which is used subsequently as a handle for the CAD/CAR command-input and
    * response-generation routines.
    */

   if ((cadCmdContext = epToVxCmdInit (NULL, pipeCreate)) == NULL)
   {
      ERROR_LOG ("Error getting CAD command context");
      return (ERROR);
   }

   /* Initialise context structures for the SIR records used by this task. */

   if (epToVxRecContextGet (WFS_CONTROL_STATE_SIR_NAME, & pStateContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get WFS_CONTROL_STATE_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (WFS_CONTROL_REBOOT_SIR_NAME, & pRebootContext, 
       NULL) == ERROR)
   {
      ERROR_LOG ("Can't get WFS_CONTROL_REBOOT_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (WFS_CONTROL_PARK_SIR_NAME, & pParkContext, 
       NULL) == ERROR)
   {
      ERROR_LOG ("Can't get WFS_CONTROL_PARK_SIR_NAME SIR context");
      return (ERROR);
   }

   /* Initialise the system state to "INITIALIZING". */

   if (epToVxPipeWrite( NULL, "INITIALIZING", pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set INITIALIZING state");
      return (ERROR);
   }

   /* Set the health for this task to "GOOD" */

   if ( epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "GOOD" ) == ERROR )
   {
      ERROR_LOG ("Failed to initialise WFS controller health");
      return (ERROR);
   }

   /*
    * Set the default simulation mode and debug mode.
    */

   epToVxSetCadSimMode (EPTOVX_SIM_MODE_NONE);
   if (epToVxPipeWrite ("simMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ("Failed to write default simulation mode to SIR record");
      return (ERROR);
   }

   errorMessageFilterSet( EPTOVX_DEBUG_MODE_NONE+1 );
   if (epToVxPipeWrite ("debugMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ("Failed to write default debug mode to SIR record");
      return (ERROR);
   }

   /* Finally, set the system state to RUNNING. */

   if (epToVxPipeWrite( NULL, "RUNNING", pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set RUNNING state");
      return (ERROR);
   }

   /* The task has been successfully initialised, so it can now go into
    * a loop waiting for commands or error messages from other tasks.
    * The task can be terminated by setting the "wfsControlStop"
    * variable from the console.
    */

   MESSAGE_LOG1 (MSG_MINDEBUG, 
               "Entering loop waiting for commands... pCmdPacket=0x%x",
               (int) cadCmdContext->pCmdPacket);

   while (! wfsControlStop)
   {

      /*
       * Initialise the error number and then read the command number from 
       * the pipe communicating CAD commands. The epToVxCmdRead() call will 
       * block until a command becomes available. The wfsControl task is 
       * aborted if it fails to read a command.
       */

      errorNumber = 0;
      if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
      {
         ERROR_LOG ("Error reading CAD command - task aborted");
         epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "BAD" );
         return (ERROR);
      }
      MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command #%d received", commandNumber);

      /* Process the command.
       * In simulation mode simply report the command,
       * otherwise switch according to the command number received.
       */

      if (EPTOVX_IS_SIMULATION(cadCmdContext, EPTOVX_SIM_MODE_FULL) ||
          EPTOVX_IS_SIMULATION(cadCmdContext, EPTOVX_SIM_MODE_FAST))
      {
         /* In FULL simulation mode nothing needs to be done except to log
          * a message. The function epToVxCmdFinish() will simulate the
          * response from the command.
          */

         MESSAGE_LOG1 (MSG_LOG, 
            "Command %d received in simulation mode... no action taken",
            commandNumber);
      }

      if (commandNumber == WFS_CONTROL_CMD_PARK)
      {
         /*
          * Park command received. Set the park state to BUSY
          */

         parkState = CAR_BUSY;
         if (epToVxPipeWrite ( NULL, (char *) &parkState, pParkContext ) ==
             ERROR )
         {
            ERROR_LOG ("Failed to set park state to BUSY");
            return (ERROR);
         }

         MESSAGE_LOG (MSG_LOG, "Park command received");

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*   
          * Set the park state to IDLE.
          */

         parkState = CAR_IDLE;
         if (epToVxPipeWrite ( NULL, (char *) &parkState, pParkContext ) ==
             ERROR )
         {
            ERROR_LOG ("Failed to set park state to BUSY");
            return (ERROR);
         }
      }

      else if (commandNumber == WFS_CONTROL_CMD_REBOOT)
      {
         /* Set the reboot state to BUSY */

         rebootState = CAR_BUSY;
         if (epToVxPipeWrite ( NULL, (char *) &rebootState, pRebootContext ) ==
             ERROR )
         {
            ERROR_LOG ("Failed to set reboot state to BUSY");
            return (ERROR);
         }

         if (epToVxPipeWrite( NULL, "BOOTING", pStateContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set BOOTING state");
            return (ERROR);
         }

         /*
          * Reboot command received. Close any connection to the DHS and reset 
          * the VME bus.
          */

         if ( detDhsInitialised )
         {
            MESSAGE_LOG (MSG_LOG, "Closing down DHS connection.");

            if ( detDhsConnected == CONNECTED )
            {
               dhsErrno = 0;
               dhsDisconnect (detDhsConnection, &dhsErrno);
               if ( dhsErrno == DHS_S_SUCCESS )
               {
                  detDhsConnected = NOT_CONNECTED;
                  MESSAGE_LOG (MSG_LOG, "Disconnected to DHS");
               }
               else
               {
                  MESSAGE_LOG (MSG_LOG, "dhsDisconnect returns an error");
               }
            }

            dhsErrno = 0;
            dhsEventLoopEnd (&dhsErrno);
            dhsErrno = 0;
            dhsExit ( &dhsErrno );
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (5 * sysClkRateGet());

         /*
          * Now reboot
          */

         reboot (BOOT_QUICK_AUTOBOOT);
      }

      else if (commandNumber == WFS_CONTROL_CMD_SIMULATE)
      {

         /* Set simulation mode command received.
          * Set the simulation mode and write its current value to the
          * SIR record.
          */

         EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                                (char *) & simMode);
         epToVxSetCadSimMode (simMode);

         switch (simMode)
         {
            case (EPTOVX_SIM_MODE_VSM):

               strncpy (pSimMode, "VSM", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_FAST):

               strncpy (pSimMode, "FAST", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_FULL):

               strncpy (pSimMode, "FULL", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_NONE):

               strncpy (pSimMode, "NONE", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            default:
               strncpy (pSimMode, "INVALID", EPICS_MAX_BYTES_STRING_ATTRIB);
         }

         MESSAGE_LOG1 (MSG_LOG, "Simulation mode set to %s", pSimMode);

         if (epToVxPipeWrite ("simMode", pSimMode, NULL) == ERROR)
         {
            ERROR_LOG ("Failed to write simulation mode to SIR record");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == WFS_CONTROL_CMD_DEBUG)
      {
         /* Debug command received.
          * Set the debugging mode.
          */

         EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                                (char *) & debugMode);
         errorMessageFilterSet( debugMode+1 );

         switch (debugMode)
         {
            case (EPTOVX_DEBUG_MODE_NONE):

               strncpy (pDebugMode, "NONE", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_DEBUG_MODE_MIN):

               strncpy (pDebugMode, "MIN", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_DEBUG_MODE_FULL):

               strncpy (pDebugMode, "FULL", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            default:
               strncpy (pDebugMode, "INVALID", EPICS_MAX_BYTES_STRING_ATTRIB);
         }

         MESSAGE_LOG1 (MSG_LOG, "Debug mode set to %s", pDebugMode);

         if (epToVxPipeWrite ("debugMode", pDebugMode, NULL) == ERROR)
         {
            ERROR_LOG ("Failed to write debug mode to SIR record");
            errorNumber = (uint32) errnoGet();
         }
      }

      else
      {
         ERROR_SET1 (S_wfsControl_BAD_COMMAND, 
                    "Command %d not currently implemented",
                    ERROR_LOG_NOW, commandNumber);
         errorNumber = S_wfsControl_BAD_COMMAND;
      }

      /* Finish the command. */

      if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
      {
         ERROR_LOG ("Error finishing command");
      }

   }

   /*
    * The task has been stopped. Issue a warning message, set the health to 
    * "BAD" and free the resources allocated.
    */

   MESSAGE_LOG (MSG_WARNING, "Wavefront Sensing control task stopped");
   epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "BAD" );

   epToVxCmdFree (cadCmdContext);
   errorFlush();
   errorFree();

   return (OK);
}
