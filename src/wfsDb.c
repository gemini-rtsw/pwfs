static struct {void *v; char *c;} rcsid = {&rcsid,
 "$Id: wfsDb.c,v 1.11 2000-12-16 03:25:36 cboyer Exp $"};

/*+
 * MODULE NAME:
 * wfsDb
 *
 * FILENAME:
 * wfsDb.c
 *
 * PURPOSE:
 * PWFS2 database definition
 *
 * DESCRIPTION:
 * This module initialises the data structures which describe the records
 * contained in the EPICS database and the properties of the commands
 * associated with those records. The data structure arrays are declared in
 * "wfsDb.h" and their contents declared in "dbTypes.h". A record cannot
 * by accessed by the epToVxLib library unless it is declared here.
 *
 * The database is initialised in wfsDb.c rather than wfsDb.h because of
 * the programming convention that header files only declare objects and do
 * not allocate memory space
 *
 * FUNCTION NAME(S):
 * None
 *
 * DEFICIENCIES:
 * This module assumes that all EPICS records have the same prefix.
 *
 * The default values of attributes for the detGeometry command should be set to
 * values derived from the detector properties rather than fixed values. 
 *
 * NOTE:
 * This file only defines the records that the WFS epToVxLib library needs to
 * know about. There may be other EPICS records - see the Capfast schematics.
 *
 * I am concerned that all the EPICS database definitions need to be
 * duplicated here and in the Capfast schematics, as there is a risk the
 * two definitions will diverge. Can the database information be extracted
 * from the files generated from the Capfast schematics, or at least
 * downloaded from a file or defined in function calls at boot time?
 * SMB - 26 Nov 97.
 *
 * Note that the initialisers in this module do not necessarily fill
 * all of a CAD record structure. For example, if a CAD record does not
 * have any attributes, then its attribute structure is not initialised.
 * This feature may generate warnings with some compilers. It is assumed
 * that uninitialised parts of the data structures will be filled with
 * zero or NULL values.
 *
 * IMPORTANT:
 * The record names and fields declared in this file should exactly
 * match the names and fields for those same records as defined in
 * the Capfast schematics
 *
 * ORIGINAL AUTHOR:
 * Nick Dillon
 *
 * MODIFIED BY:
 * Steven Beard
 *
 * HISTORY MODIFICATIONS
 * 11 December 2000 - cb - add detSigReset
 * 30 October 2000 - cb - add detSigInitBW
 * 12 April 2000 - cb - add detType, detID, dataLabel, intTime, nexpRQ,
 *                      nexp, nframes, bunit, exposedRQ, exposed, utstart, 
 *                      utend, elapsed sir records
 * 11 April 2000 - cb - replace detSigMode by a set of CAD
 * 7 April 2000 - cb - add parameters to detSigMode 
 * 30 March 2000 - cb - add detFrameSize 
 * 22 March 2000 - cb - remove detSetup
 * 8 March 2000 - cb - tidy up, and replace detInit and detTest with init 
 *                and test after deleting the original init and test
 *
 *-
 */


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif /* vxWorks */


#include "dbTypes.h"
#include "wfsDb.h"
#include "wfsLib.h"
#include "epToVxLib.h"
#include "detControl.h"   /* This is where DET_CONTROL_ parameters come from. */
#include "wfsControl.h"   /* This is where WFS_CONTROL_ parameters come from. */
#include "errorLog.h"     /* This is where LOGTASK_ parameters comes from.    */


/* The pWfsDbCadList data structure array contains information on the CAD 
 * records recognised by the system, and the commands associated with them. Each
 * CAD record is described by the following information:
 * - Record name (excluding the system prefix).
 * - Name of task to receive commands from that record.
 * - Command number associated with that record.
 * - Flag indicating whether the stop directive is supported.
 * - Flag indicating whether the command can run in simulation mode.
 * - Command timeout in seconds.
 * - List of command attributes, together with the data type, default value
 *   and allowed range for each attribute. There can be zero or more attributes.
 */

CAD_RECORD pWfsDbCadList [] =
{
 {
  RECORD_NAME ("reboot"),
  WFS_CONTROL_TASK_NAME,
  WFS_CONTROL_CMD_REBOOT,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  NO_TIMEOUT
 },
 {
  RECORD_NAME ("park"),
  WFS_CONTROL_TASK_NAME,
  WFS_CONTROL_CMD_PARK,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  NO_TIMEOUT
 },
 {
  RECORD_NAME ("simulate"),
  WFS_CONTROL_TASK_NAME,
  WFS_CONTROL_CMD_SIMULATE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_UNSUPPORTED,
  10.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,  ATTRIB (EPTOVX_SIM_MODE_NONE),
                {ATTRIB (EPTOVX_SIM_MODE_VSM),
                ATTRIB (EPTOVX_SIM_MODE_NONE)}
 },
 {
  RECORD_NAME ("debug"),
  WFS_CONTROL_TASK_NAME,
  WFS_CONTROL_CMD_DEBUG,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_UNSUPPORTED,
  10.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,  ATTRIB (EPTOVX_DEBUG_MODE_NONE),
                {ATTRIB (EPTOVX_DEBUG_MODE_NONE),
                ATTRIB (EPTOVX_DEBUG_MODE_FULL)}
 },
 {
  RECORD_NAME ("init"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_INITIALISE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  180.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,ATTRIB (DET_CONTROL_PWFS2_SDSU_ADRS_VME), {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_VME_FILE, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, DET_CONTROL_GBD_OMF_TIM_FILE, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_UTL_FILE, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, ATTRIB (DET_CONTROL_PWFS2_MAX_FRAMES), {"0", "100"}
 },
 {
  RECORD_NAME ("test"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_TEST,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  120.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "8", {"0", "8"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "0", {"0", "1"}
 },
 {
  RECORD_NAME ("dc:detChop"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_CHOP,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "7"}
 },
 {
  RECORD_NAME ("dc:detFrameSize"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_FRAME_SIZE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "1"}
 },
 {
  RECORD_NAME ("dc:detDhsReconnect"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_DHS_RECONNECT,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "1"}
 },
 {
  RECORD_NAME ("dc:detDhsDisplay"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_DHS_DISPLAY,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "100", {"1", "500"}
 },
 {
  RECORD_NAME ("dc:detExposure"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_EXPOSURE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  30.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE, "0.01", {"0.005", "10000.0"}
 },
 {
  RECORD_NAME ("dc:detObstype"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_OBSTYPE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, "UNDEFINED", {"OBJECT", "DARK", "FLAT", "ZERO","UNDEFINED"},
 },
 {
  RECORD_NAME ("dc:detSetWcs"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SETWCS,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  120.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, "p2calib.wcs", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:observe"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_OBSERVE,
  STOP_DIRECTIVE_SUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  180.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE, "0.01", {"0.005", "10000.0"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "0", {"0", "2"},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, "2", {"0", "2"},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING, DET_CONTROL_DATA_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_STRING, "pwfs2.fits", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_H, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:stop"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_STOP,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  120.0
 },
 {
  RECORD_NAME ("dc:abort"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_ABORT,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  120.0
 },
 {
  RECORD_NAME ("dc:detReset"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_RESET,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  60.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "1", {"0", "1"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "1", {"0", "1"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_VME_FILE, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING, DET_CONTROL_GBD_OMF_TIM_FILE, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_UTL_FILE, {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSave"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SAVE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  120.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, "p2newparams.par", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "-1", {"-1", "3"}
 },
 {
  RECORD_NAME ("dc:detGeometry"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_GEOMETRY,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "1", {"1", "20"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "1", {"1", "20"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "40",{"1", ATTRIB (CCD_XSIZE)},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, "40",{"1", ATTRIB (CCD_YSIZE)},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, "1", {"1", ATTRIB (CCD_XSIZE)},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, "1", {"1", ATTRIB (CCD_YSIZE)},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG, "0", {"0", ATTRIB (CCD_XSIZE)},
  CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG, "0", {"0", ATTRIB (CCD_YSIZE)},
  CAD_ATTRIB_I, EPICS_DATA_TYPE_LONG, "0", {"0", ATTRIB (CCD_XSIZE)},
  CAD_ATTRIB_J, EPICS_DATA_TYPE_LONG, "0", {"0", ATTRIB (CCD_YSIZE)}
 },
 {
  RECORD_NAME ("dc:detPrim"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_PRIMITIVE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  60.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, "TDL", {NO_ATTRIBUTE_LIMITS},
                   /* Allow any command. */
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "1", {"1", "3"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "0x55aaff", {"0", NO_HI_LIMIT},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, "0", {"0", NO_HI_LIMIT},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, "0", {"0", NO_HI_LIMIT},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, "0", {"0", NO_HI_LIMIT},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG, "0", {"0", NO_HI_LIMIT},
  CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG, "0", {"0", NO_HI_LIMIT}
 },
 {
  RECORD_NAME ("dc:detMode"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_MODE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "-1", {"-1", "0xFFF"},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT}
 },
 {
  RECORD_NAME ("dc:detOffset"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_OFFSET,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, "-1", {"-1", NO_HI_LIMIT}
 },
 {
  RECORD_NAME ("dc:detTemp"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_TEMP,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE, "-20", {"-63", "25"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "0x80", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigReset"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGRESET,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0
 },
 {
  RECORD_NAME ("dc:detSigInit"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGINIT,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  180.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, "defFullP2Dark.fits", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, "defFullP2Flat.fits", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, "3.14159", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_DOUBLE, "40.5", {"0.5", "80.5"},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_DOUBLE, "40.5", {"0.5", "80.5"},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_STRING, "defFullRefP2.dat", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigInitGain"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGINITGAIN,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  180.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE, "-0.1", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE, "-0.1", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_DOUBLE, "-0.001", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, "0.001", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigInitBW"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGINITBW,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  180.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE, "6.0", {"0.0","100"}
 },
 {
  RECORD_NAME ("dc:detSigModeNone"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_NONE,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0
 },
 {
  RECORD_NAME ("dc:detSigModeDark"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_DARK,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0
 },
 {
  RECORD_NAME ("dc:detSigModeGg"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_GG,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0
 },
 {
  RECORD_NAME ("dc:detSigModeFgFocus"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_FG_FOCUS,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "4"}
 },
 {
  RECORD_NAME ("dc:detSigModeCoadd"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_COADD,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, "coadd.fits", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigModeThresh"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_THRESH,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "2"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_DOUBLE, "15.0", {"0.0", "100.0"},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, "5", {"0.0", NO_HI_LIMIT},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_DOUBLE, "50", {"0.0", "65536.0"}
 },
 {
  RECORD_NAME ("dc:detSigModeGgCoadd"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_GG_COADD,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, "coadd.fits", {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigModeSeq"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_SEQ,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE, "1.0", {"0.0", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "1", {"0", "1"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, "15.0", {"0.0", "100.0"},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, "1", {"0", "1"},
  CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_DOUBLE, "10.0", {"0.0", "100.0"},
  CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG, "0", {"0", "1"},
  CAD_ATTRIB_I, EPICS_DATA_TYPE_DOUBLE, "0.1", {"0.1", NO_HI_LIMIT},
  CAD_ATTRIB_J, EPICS_DATA_TYPE_STRING, DET_CONTROL_DATA_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_K, EPICS_DATA_TYPE_LONG, "0", {"0", "4"}
 },
 {
  RECORD_NAME ("dc:detSigModeTotal"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_TOTAL,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "1"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE, "0.0", {"0.0", NO_HI_LIMIT},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, "10.0", {"0.0", "100.0"}
 },
 {
  RECORD_NAME ("dc:detSigInitCB"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGINIT_CB,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "1"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "0", {"0", "1"},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, DET_CONTROL_DATA_FILE_PATH, {NO_ATTRIBUTE_LIMITS}
 },
 {
  RECORD_NAME ("dc:detSigModeSeqDark"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_SEQ_DARK,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, "coadd.fits", {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_E, EPICS_DATA_TYPE_DOUBLE, "5", {"0.0", NO_HI_LIMIT}
 },
 {
  RECORD_NAME ("dc:detSigModeFgCoadd"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  DET_CONTROL_CMD_SIGMODE_FG_FOCUS_COADD,
  STOP_DIRECTIVE_UNSUPPORTED,
  SIMULATION_MODE_SUPPORTED,
  40.0,
  CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "4"},
  CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, "100", {"1", NO_HI_LIMIT},
  CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
  CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, "coadd.fits", {NO_ATTRIBUTE_LIMITS}
 }
};

/* The pWfsDbGsubList data structure array contains information on the genSub 
 * records recognised by the system, and the commands associated with them. Each
 * genSub record is described by the following information:
 * - Record name (excluding the system prefix).
 * - Name of task to receive commands from that record.
 * - Command number associated with that record.
 * - Command timeout in seconds.
 */

GSUB_RECORD pWfsDbGsubList [] =
{
 {
  RECORD_NAME ("dc:ttfZero"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  "p2",
  GSUB_INPUT,
  DET_CONTROL_CMD_TTFZERO,
  1.0,
  8
 },
 {
  RECORD_NAME ("dc:aoZero"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  "p2",
  GSUB_INPUT,
  DET_CONTROL_CMD_AOZERO,
  1.0,
  24
 },
 {
  RECORD_NAME ("dc:probeOffset"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  "p2",
  GSUB_INPUT,
  DET_CONTROL_CMD_PROBEOFFSET,
  1.0,
  9
 },
 {
  RECORD_NAME ("dc:ttf"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  "p2",
  GSUB_OUTPUT,
  0,
  NO_TIMEOUT,
  8
 },
 {
  RECORD_NAME ("dc:ao"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME),
  "p2",
  GSUB_OUTPUT,
  0,
  NO_TIMEOUT,
  40
 }
};


/* The pWfsDbCarList data structure array contains information on the CAR records
 * recognised by the system. Each CAR record is described by the following
 * information:
 * - Record name (excluding the system prefix).
 * - Name of task responsible for that record.
 */

CAR_RECORD pWfsDbCarList [] =
{
 {
  RECORD_NAME ("controlC"),
  WFS_CONTROL_TASK_NAME
 },
 {
  RECORD_NAME ("dc:detC"),
  TASK_NAME ("p2", DET_CONTROL_TASK_NAME)
 }
};


/* The pWfsDbSirList data structure array contains information on the SIR records
 * recognised by the system. Each SIR record is described by the following
 * information:
 * - Record name (excluding the system prefix).
 * - Record data type.
 * - Hysteresis value (optional).
 */

SIR_RECORD pWfsDbSirList [] =
{
 {
  RECORD_NAME ("name"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("state"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("health"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("controlHealth"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("version"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("debugMode"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("simMode"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("measuring"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("rebooting"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("parking"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("trackId"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("arrayS"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("seeing"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("historyLog"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("historyLog1"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("errorLog"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("errorLog1"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("cpuUsed00"),
  EPICS_DATA_TYPE_LONG,
  5.0
 },
 {
  RECORD_NAME ("ramUsed00"),
  EPICS_DATA_TYPE_LONG,
  2.0
 },
 {
  RECORD_NAME ("ramFreeblk00"),
  EPICS_DATA_TYPE_LONG,
  256.0
 },
 {
  RECORD_NAME ("testResults"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:health"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:detPrimReply"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:detInitStatus"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoCtrlInit"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoDarkInit"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoFlatInit"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoThresh"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:aoTotal"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:aoSaveCbIm"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoSaveCbCtrl"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:aoProcessMode"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:initialising"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:testing"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:observing"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:outputs"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:detXsize"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:detYsize"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:xsubap"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:ysubap"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:xstart"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:ystart"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:xras"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:yras"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:xspace"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:yspace"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:xbin"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:ybin"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:detType"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:detID"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:dataLabel"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:intTime"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:nexpRQ"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:nexp"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:nframes"),
  EPICS_DATA_TYPE_LONG
 },
 {
  RECORD_NAME ("dc:bunit"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:exposed"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:elapsed"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:exposedRQ"),
  EPICS_DATA_TYPE_DOUBLE
 },
 {
  RECORD_NAME ("dc:utstart"),
  EPICS_DATA_TYPE_STRING
 },
 {
  RECORD_NAME ("dc:utend"),
  EPICS_DATA_TYPE_STRING
 }
};

/* Determine the number of CAD, CAR and SIR records defined above. */

int  wfsDbNCadRecord  = NELEMENTS (pWfsDbCadList);
int  wfsDbNGsubRecord = NELEMENTS (pWfsDbGsubList);
int  wfsDbNCarRecord  = NELEMENTS (pWfsDbCarList);
int  wfsDbNSirRecord  = NELEMENTS (pWfsDbSirList);

char pWfsDbRecNamePrefix [] = TOP;

/*
 * Record field names and types must be given in the order of the enums
 * nnn_RECORD_TYPE where nnn = CAD, CAR, SIR etc (see dbTypes.h). If field types
 * are listed, then the Value field must be the last one in the list, since the
 * fields are accessed in the order given here and, when writing to a record,
 * processing is triggered when the Value field is written. All record types are
 * initially un-initialised (pWfsDbRecInitialised[type] = FALSE.
 */

BOOL pWfsDbRecInitialised [N_RECORD_TYPES] = {FALSE, FALSE, FALSE, FALSE};
char pppWfsDbRecFieldName [N_RECORD_TYPES][EPICS_MAX_NFIELD_PER_RECORD][EPICS_MAX_BYTES_FIELD_NAME + 2] =
 {
  {""},            /* CAD record field names  */
  {".J", ".VALJ"},         /* genSub record field names */
  {"ID", ".IERR", ".IMSS", ".IVAL"},     /* CAR record field names  */
  {".VAL"}           /* SIR record field names  */
 };

/*
 * Initialise the flag indicating whether the EPICS database is contained on the
 * local processor. This flag is reset to TRUE by the wavefront sensor control
 * task running on the root processor.
 */

BOOL wfsDbEpicsDbIsLocal = FALSE;
