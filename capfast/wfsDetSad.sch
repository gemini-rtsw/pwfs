[schematic2]
uniq 118
[tools]
[detail]
w 2938 43 100 0 n#115 esirs.ybin.FLNK 2784 512 2848 512 2848 32 3088 32 egenSub.initSigInit.SLNK
w 2970 651 100 0 n#111 esirs.ybin.VAL 2784 480 2912 480 2912 640 3088 640 egenSub.initSigInit.INPB
w 2970 715 100 0 n#110 esirs.xbin.VAL 2784 960 2912 960 2912 704 3088 704 egenSub.initSigInit.INPA
s 128 2176 500 0 PWFS2 - WFS Detector Status Records
s 2464 -704 500 512 wfsDetSad.sch
[cell use]
use egenSub 3088 -57 100 0 initSigInit
xform 0 3232 368
p 3152 -112 100 0 1 DESC:Init the detSigInit record
p 3168 704 100 0 1 FTA:LONG
p 3168 640 100 0 1 FTB:LONG
p 3504 720 100 0 1 FTVA:STRING
p 3504 656 100 0 1 FTVB:STRING
p 3504 592 100 0 1 FTVC:STRING
p 3504 528 100 0 1 FTVD:DOUBLE
p 3504 464 100 0 1 FTVE:DOUBLE
p 3504 400 100 0 1 FTVF:DOUBLE
p 3504 336 100 0 1 FTVG:STRING
p 3152 -224 100 0 1 PV:$(top)$(wfs)
p 3152 -144 100 0 1 SCAN:Passive
p 3152 -176 100 0 1 SNAM:detInitSigInit
p 3680 720 100 0 1 def(OUTA):$(top)$(wfs)detSigInit.A
p 3680 656 100 0 1 def(OUTB):$(top)$(wfs)detSigInit.B
p 3680 592 100 0 1 def(OUTC):$(top)$(wfs)detSigInit.C
p 3680 528 100 0 1 def(OUTD):$(top)$(wfs)detSigInit.D
p 3680 464 100 0 1 def(OUTE):$(top)$(wfs)detSigInit.E
p 3680 400 100 0 1 def(OUTF):$(top)$(wfs)detSigInit.F
p 3680 336 100 0 1 def(OUTG):$(top)$(wfs)detSigInit.G
use esirs -64 -217 100 0 outputs
xform 0 144 -64
p 0 -256 100 0 1 DESC:Number of outputs
p 0 -352 100 0 1 EGU:
p -128 -480 100 0 0 FDSC:SDSU parameter YRAS
p 0 -288 100 0 1 FTVL:LONG
p 0 -320 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 743 100 0 xsubap
xform 0 1360 896
p 1216 704 100 0 1 DESC:Number of X subapertures
p 1088 480 100 0 0 FDSC:SDSU parameter XSUBAP
p 1216 672 100 0 1 FTVL:LONG
p 1216 640 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1760 1703 100 0 xstart
xform 0 1968 1856
p 1824 1664 100 0 1 DESC:Start of left edge of digitised area
p 1824 1568 100 0 1 EGU:pixels
p 1696 1440 100 0 0 FDSC:SDSU parameter XSTART
p 1824 1632 100 0 1 FTVL:LONG
p 1824 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 263 100 0 ysubap
xform 0 1360 416
p 1216 224 100 0 1 DESC:Number of Y subapertures
p 1088 0 100 0 0 FDSC:SDSU parameter YSUBAP
p 1216 192 100 0 1 FTVL:LONG
p 1216 160 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 1223 100 0 detYsize
xform 0 1360 1376
p 1216 1184 100 0 1 DESC:Total Y size of detector
p 1216 1088 100 0 1 EGU:pixels
p 1088 960 100 0 0 FDSC:Total Y size of detector
p 1216 1152 100 0 1 FTVL:LONG
p 1216 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 1703 100 0 detXsize
xform 0 1360 1856
p 1216 1664 100 0 1 DESC:Total X size of detector
p 1216 1568 100 0 1 EGU:pixels
p 1088 1440 100 0 0 FDSC:Total X size of detector
p 1216 1632 100 0 1 FTVL:LONG
p 1216 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 544 263 100 0 headTemp
xform 0 752 416
p 608 224 100 0 1 DESC:Detector head temperature
p 608 128 100 0 1 EGU:Celsius
p 480 0 100 0 0 FDSC:Detector head temperature
p 608 192 100 0 1 FTVL:DOUBLE
p 736 192 100 0 0 PREC:4
p 608 160 100 0 1 PV:$(sadtop)$(wfs)
p 608 96 100 0 1 SCAN:5 second
p 608 64 100 0 1 SNAM:detHeadTempGet
use esirs 544 743 100 0 nresets
xform 0 752 896
p 608 704 100 0 1 DESC:Number of resets between exposures
p 480 480 100 0 0 FDSC:NRESETS (always 0 for SDSU controller)
p 608 672 100 0 1 FTVL:LONG
p 608 640 100 0 1 PV:$(sadtop)$(wfs)
p 608 608 100 0 1 VAL:0
use esirs 544 1223 100 0 nreads
xform 0 752 1376
p 608 1184 100 0 1 DESC:Number of reads per exposure
p 480 960 100 0 0 FDSC:NREADS (always 1 for SDSU controller)
p 608 1152 100 0 1 FTVL:LONG
p 608 1120 100 0 1 PV:$(sadtop)$(wfs)
p 608 1088 100 0 1 VAL:1
use esirs 544 1703 100 0 intTime
xform 0 752 1856
p 608 1664 100 0 1 DESC:On-chip integration time
p 608 1568 100 0 1 EGU:seconds
p 480 1440 100 0 0 FDSC:SDSU parameter T_EXP
p 608 1632 100 0 1 FTVL:DOUBLE
p 768 1568 100 0 1 PREC:4
p 608 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -64 1223 100 0 detID
xform 0 144 1376
p 0 1184 100 0 1 DESC:Detector ID
p -128 960 100 0 0 FDSC:Detector ID (CCD39. CCD47, etc.)
p 0 1152 100 0 1 FTVL:STRING
p 0 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs -64 1703 100 0 detType
xform 0 144 1856
p 0 1664 100 0 1 DESC:Detector type
p -128 1440 100 0 0 FDSC:Detector type
p 0 1632 100 0 1 FTVL:STRING
p 0 1600 100 0 1 PV:$(sadtop)$(wfs)
p 0 1568 100 0 1 VAL:CCD+SDSU
use esirs -64 263 100 0 detInitStatus
xform 0 144 416
p 0 224 100 0 1 DESC:Detector initialisation status
p -128 0 100 0 0 FDSC:Detector initialisation status
p 0 192 100 0 1 FTVL:STRING
p 0 160 100 0 1 PV:$(sadtop)$(wfs)
use esirs -64 743 100 0 detPrimReply
xform 0 144 896
p 0 704 100 0 1 DESC:Reply from SDSU primitive command
p -128 480 100 0 0 FDSC:Reply from SDSU primitive command
p 0 672 100 0 1 FTVL:STRING
p 0 640 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1760 1223 100 0 ystart
xform 0 1968 1376
p 1824 1184 100 0 1 DESC:Start of bottom edge of digitised area
p 1824 1088 100 0 1 EGU:pixels
p 1696 960 100 0 0 FDSC:SDSU parameter YSTART
p 1824 1152 100 0 1 FTVL:LONG
p 1824 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1760 743 100 0 xras
xform 0 1968 896
p 1824 704 100 0 1 DESC:Number of X super-pixels per subaperture
p 1824 608 100 0 1 EGU:super-pixels
p 1696 480 100 0 0 FDSC:SDSU parameter XRAS
p 1824 672 100 0 1 FTVL:LONG
p 1824 640 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1760 263 100 0 yras
xform 0 1968 416
p 1824 224 100 0 1 DESC:Number of Y super-pixels per subaperture
p 1824 128 100 0 1 EGU:super-pixels
p 1696 0 100 0 0 FDSC:SDSU parameter YRAS
p 1824 192 100 0 1 FTVL:LONG
p 1824 160 100 0 1 PV:$(sadtop)$(wfs)
use esirs 2368 1703 100 0 xspace
xform 0 2576 1856
p 2432 1664 100 0 1 DESC:X pixels skipped between subapertures
p 2432 1568 100 0 1 EGU:pixels
p 2304 1440 100 0 0 FDSC:SDSU parameter XSPACE
p 2432 1632 100 0 1 FTVL:LONG
p 2432 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 2368 1223 100 0 yspace
xform 0 2576 1376
p 2432 1184 100 0 1 DESC:Y pixels skipped between subapertures
p 2432 1088 100 0 1 EGU:pixels
p 2304 960 100 0 0 FDSC:SDSU parameter YSPACE
p 2432 1152 100 0 1 FTVL:LONG
p 2432 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs 2368 743 100 0 xbin
xform 0 2576 896
p 2432 704 100 0 1 DESC:Binning factor in X direction
p 2304 480 100 0 0 FDSC:SDSU parameter XBIN
p 2432 672 100 0 1 FTVL:LONG
p 2432 640 100 0 1 PV:$(sadtop)$(wfs)
use esirs 2368 263 100 0 ybin
xform 0 2576 416
p 2432 224 100 0 1 DESC:Binning factor in Y direction
p 2304 0 100 0 0 FDSC:SDSU parameter YBIN
p 2432 192 100 0 1 FTVL:LONG
p 2432 160 100 0 1 PV:$(sadtop)$(wfs)
use notes 3568 -313 100 0 notes#13
xform 0 3824 -128
p 4096 -162 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3596 -2 100 0 -1 COMMENT1:This schematic contains the Status records
p 3596 -34 100 0 -1 COMMENT2:describing the detector controller of
p 3596 -64 100 0 -1 COMMENT3:one wavefront sensor.
p 3596 -96 100 0 -1 COMMENT4:It may be duplicated for each wavefront
p 3596 -128 100 0 -1 COMMENT5:sensor, using the wfs macro to distinguish
p 3596 -160 100 0 -1 COMMENT6:each one.
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-11-11 01:11:51 $
p 2576 2320 200 0 -1 id:
p 2704 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2608 -496 200 0 -1 revision:$Revision: 1.4 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Detector Status Records
[comments]
