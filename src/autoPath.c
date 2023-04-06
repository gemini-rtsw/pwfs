static struct {void *v; char *c;} rcsid = {&rcsid, "$Id: autoPath.c 39188 2011-11-17 02:20:32Z aebbers $"};

/*+
 *   MODULE NAME:
 *   autoPath
 *
 *   FILENAME:
 *   autoPath.c
 *
 *   PURPOSE:
 *   Wavefront sensor task application code
 *
 *   DESCRIPTION:
 *   This file contains the function "autoPath", which is started from
 *   the command line (i.e., startup script) to watch for UTC midnight and
 *   create a default directory and default paths for writing fits files
 *   and circular buffers.
 *
 *   INCLUDE FILES:
 *   None
 *
 *   DEFICIENCIES:
 *   Never
 *
 *   ORIGINAL AUTHOR:
 *   TCC
 *
 *   MODIFIED BY:
 *   TCC
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

#include <stdio.h>
#include <taskLib.h>
#include <pipeDrv.h>
#include <ioLib.h>
#include <memLib.h>
#include <math.h>
#include <tickLib.h>
#include <sys/stat.h>
#include "gemTypes.h"
#include "errorLib.h"
#include <time.h>
#include "detControl.h"

extern int snprintf(char *str, size_t count, const char *fmt, ...);

/* Note that EPICS_MAX_BYTES_STRING_ATTRIB == 40, and the current path is at 32 chars */
extern char ioc_path[EPICS_MAX_BYTES_STRING_ATTRIB];		/* Path to where to write images, cb's	*/
extern char data_filename[EPICS_MAX_BYTES_STRING_ATTRIB];

extern char epToVxTopName[];					/* Name of pwfs				*/

#define SECSNADAY	(24 * 60 * 60)
#define SECSNADIR	(14 * 60 * 60)

STATUS autoPath () {
   struct tm *tmnow;
   time_t now;
   int secstomidnight;
   int offset;
   struct stat sb;
   char *site = getenv("SITE");

   if (site == NULL)                 offset = 10;     /* Default HST */
   else if (strcmp("MK", site) == 0) offset = 10;     /* -10 hours UTC */
   else if (strcmp("CP", site) == 0) offset =  3;     /* -3  hours UTC, ignore CLST */
   else                              offset = 10;     /* Default HST */

   if (errorInit () == ERROR) {
      printErr("autoPath: Failed to initialise error context structure.\n");
      return (ERROR);
   }

   while (1) {
      time( &now );
      tmnow = localtime( &now );
      /*tmnow = gmtime(&now); */
      /*printf("Current local time and date: %s", asctime(tmnow));*/
      tmnow->tm_hour=tmnow->tm_hour-offset;
      /*printf("tmnow->tm_hour = %d \n",tmnow->tm_hour);
      printf("tmnow->tm_mday = %d \n",tmnow->tm_mday);*/
      if ( tmnow != NULL &&
                 ( tmnow->tm_hour >= 14 ) )
      {
          /*
            It is after 14:00, so the day in the prefix is tomorrow's date.
         */

          now = now + ( 64800 ) ;
          tmnow = localtime( &now );
      }

      /*
        Compose the time part of the prefix into YYYYddMM.
      */

    /*  strftime( ioc_path, sizeof(ioc_path), "%Y%m%d", tmnow );  */
      sprintf(ioc_path, "%s/pwfs2/%04d%02d%02d", DET_CONTROL_DATA_FILE_PATH,
         1900 + tmnow->tm_year, tmnow->tm_mon + 1, tmnow->tm_mday);

      /*printf("autoPath: Setting IOC path: \"%s\".\n", ioc_path);*/

      now = time(NULL);
      /* secstomidnight = (SECSNADAY - (now % SECSNADAY))+SECSNADIR; */
      secstomidnight = 3;	
      /*printf("autoPath: waiting %d seconds to 14 hours local time\n", secstomidnight);*/
      taskDelay(secstomidnight * sysClkRateGet());	/* Wait until midnight */
   }

   MESSAGE_LOG(MSG_WARNING, "autoPath task exited.");

   return (OK);
}
