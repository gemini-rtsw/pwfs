/*
 * This file is used for benchmarking WFS system.
 * Source provided by Sean Prior
 * CB - 17March 1999
 */

/* define masks for turning bits on and off */

#define BIT_ZERO_ON	0x1
#define BIT_ZERO_OFF	0xfe
#define XYCOM_BASE_ADDRESS (0xfbffd080)

/* XYCOM base address is actually 0xffffd000 but the registers dont start
 * until 0xffffd080
 */

typedef struct
{
  char intInReg;
  char statConReg;
  char intPendReg;
  char intMaskReg;
  char intClrReg;
  char intVecReg;
  char flagOutReg;
  char portDirReg;
  char port0;
  char port1;
  char port2;
  char port3;
  char port4;
  char port5;
  char port6;
  char port7;
} xycomCard;

