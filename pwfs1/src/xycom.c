/*
 * This file is used for benchmarking WFS system.
 * Source provided by Sean Prior
 * CB - 17March 1999
 */

#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <iv.h>
#include <logLib.h>
#include <recSup.h>
#include <sysLib.h>
#include <vxLib.h>

#include "xycom.h"

/* declare the card pointer as a global so you can get it anywhere */

int swapFlag = 0;
xycomCard *ptr = NULL;

/* function to initialise the card */

void    xycomInit (void)
{
	char    test = 0;

	/* check that XYCOM board is present */

	if (vxMemProbe ((void *) XYCOM_BASE_ADDRESS, READ, sizeof (char), &test) != OK)
	{
		printf ("xycom card not detected\n");
		ptr = NULL;
		return;
	}
	else
	{
	    ptr = (xycomCard *)XYCOM_BASE_ADDRESS;
	}

	/* Green LED on, red off */

/*	ptr->statConReg = 3;*/

	/* Ensure the flag output register is clear */

	/*ptr->flagOutReg = 0;*/

	/* set port directions 0 - 7 as output = in, setting a '1' in the
	 * port selects output; '0' for input. */

	/*ptr->portDirReg = 0xff;*/

	/* clear all output ports */

	/*ptr->port0 = 0x0;
	ptr->port1 = 0x0;
	ptr->port2 = 0x0;
	ptr->port3 = 0x0;
	ptr->port4 = 0x0;
	ptr->port5 = 0x0;
	ptr->port6 = 0x0;
	ptr->port7 = 0x0;*/

	return;
}


/*******************************************************************
 to set and clear the bits you use a mask in the usual way
 shown here setting and clearing bit zero of port 7
 you need to look at the xycom card manual to know which pins
 to connect your oscilloscope to 


 for example

*******************************************************************/ 

void changeBits(void)
{
  /* set bit zero of port 7 */

  if(ptr != NULL)
	ptr->port7 = ptr->port7 | BIT_ZERO_ON;



  /* clear bit zero of port 7 */

  if(ptr != NULL)
	ptr->port7 = ptr->port7 & BIT_ZERO_OFF;

}
