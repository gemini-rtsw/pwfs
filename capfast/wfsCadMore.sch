[schematic2]
uniq 140
[tools]
[detail]
s 2464 -704 500 512 wfsCadMore.sch
[cell use]
use ecad2 -576 -473 100 0 detSigReset
xform 0 -416 -160
p -496 -528 100 0 1 DESC:Reset signal processing
p -480 -128 100 0 0 FTVA:STRING
p -496 -560 100 0 1 INAM:epToVxCadInit
p -496 -624 100 0 1 PV:$(top)$(wfs)
p -496 -592 100 0 1 SNAM:epToVxCadExecute
use ecad2 1536 1703 100 0 detSigModeFgFocus
xform 0 1696 2016
p 1616 1648 100 0 1 DESC:Set fast guide and focus mode
p 1632 2048 100 0 1 FTVA:LONG
p 1616 1616 100 0 1 INAM:epToVxCadInit
p 1616 1552 100 0 1 PV:$(top)$(wfs)
p 1616 1584 100 0 1 SNAM:epToVxCadExecute
use ecad2 832 1703 100 0 detSigModeGg
xform 0 992 2016
p 912 1648 100 0 1 DESC:Set global guide mode
p 912 1616 100 0 1 INAM:epToVxCadInit
p 912 1552 100 0 1 PV:$(top)$(wfs)
p 912 1584 100 0 1 SNAM:epToVxCadExecute
use ecad2 128 1703 100 0 detSigModeDark
xform 0 288 2016
p 208 1648 100 0 1 DESC:Set dark subtraction mode
p 208 1616 100 0 1 INAM:epToVxCadInit
p 208 1552 100 0 1 PV:$(top)$(wfs)
p 208 1584 100 0 1 SNAM:epToVxCadExecute
use ecad2 -576 1703 100 0 detSigModeNone
xform 0 -416 2016
p -496 1648 100 0 1 DESC:Set no processing mode
p -496 1616 100 0 1 INAM:epToVxCadInit
p -496 1552 100 0 1 PV:$(top)$(wfs)
p -496 1584 100 0 1 SNAM:epToVxCadExecute
use ecad2 2944 1319 100 0 detSigInitBW
xform 0 3104 1632
p 3024 1264 100 0 1 DESC:Set butterworth cutoff frequency
p 3040 1664 100 0 1 FTVA:DOUBLE
p 3024 1232 100 0 1 INAM:epToVxCadInit
p 3024 1168 100 0 1 PV:$(top)$(wfs)
p 3024 1200 100 0 1 SNAM:epToVxCadExecute
use ecad20 3632 -9 100 0 detSigModeSeq
xform 0 3792 880
p 3680 -112 100 0 1 DESC:Set sequence closed loop mode
p 3728 1456 100 0 1 FTVA:DOUBLE
p 3728 1424 100 0 1 FTVB:LONG
p 3728 1392 100 0 1 FTVC:LONG
p 3728 1360 100 0 1 FTVD:DOUBLE
p 3728 1328 100 0 1 FTVE:LONG
p 3728 1296 100 0 1 FTVF:LONG
p 3728 1264 100 0 1 FTVG:DOUBLE
p 3728 1232 100 0 1 FTVH:LONG
p 3728 1200 100 0 1 FTVI:DOUBLE
p 3728 1168 100 0 1 FTVJ:STRING
p 3728 1136 100 0 1 FTVK:LONG
p 3680 -144 100 0 1 INAM:epToVxCadInit
p 3680 -80 100 0 1 PREC:1
p 3680 -208 100 0 1 PV:$(top)$(wfs)
p 3680 -176 100 0 1 SNAM:epToVxCadExecute
use ecad8 1536 423 100 0 detSigModeSeqDark
xform 0 1696 928
p 1600 352 100 0 1 DESC:Set sequence dark mode
p 1632 1120 100 0 1 FTVA:LONG
p 1632 1088 100 0 1 FTVB:STRING
p 1632 1056 100 0 1 FTVC:STRING
p 1632 1024 100 0 1 FTVD:LONG
p 1632 992 100 0 1 FTVE:DOUBLE
p 1632 960 100 0 0 FTVF:STRING
p 1600 304 100 0 1 INAM:epToVxCadInit
p 1600 384 100 0 1 PREC:1
p 1600 208 100 0 1 PV:$(top)$(wfs)
p 1600 256 100 0 1 SNAM:epToVxCadExecute
use ecad8 2944 -25 100 0 detSigModeTotal
xform 0 3104 480
p 3008 -96 100 0 1 DESC:Set average flux computation mode
p 3040 672 100 0 1 FTVA:LONG
p 3040 640 100 0 1 FTVB:DOUBLE
p 3040 608 100 0 1 FTVC:LONG
p 3040 576 100 0 1 FTVD:DOUBLE
p 3040 544 100 0 0 FTVE:STRING
p 3040 512 100 0 0 FTVF:STRING
p 3008 -144 100 0 1 INAM:epToVxCadInit
p 3008 -64 100 0 1 PREC:1
p 3008 -240 100 0 1 PV:$(top)$(wfs)
p 3008 -192 100 0 1 SNAM:epToVxCadExecute
use ecad8 2240 -25 100 0 detSigInitCB
xform 0 2400 480
p 2304 -96 100 0 1 DESC:Init CB parameters
p 2336 672 100 0 1 FTVA:LONG
p 2336 640 100 0 1 FTVB:LONG
p 2336 608 100 0 1 FTVC:STRING
p 2304 -144 100 0 1 INAM:epToVxCadInit
p 2304 -64 100 0 1 PREC:0
p 2304 -240 100 0 1 PV:$(top)$(wfs)
p 2304 -192 100 0 1 SNAM:epToVxCadExecute
use ecad8 832 423 100 0 detSigModeGgCoadd
xform 0 992 928
p 896 352 100 0 1 DESC:Set global guide and coadd mode
p 928 1120 100 0 1 FTVA:LONG
p 928 1088 100 0 1 FTVB:STRING
p 928 1056 100 0 1 FTVC:STRING
p 896 304 100 0 1 INAM:epToVxCadInit
p 896 208 100 0 1 PV:$(top)$(wfs)
p 896 256 100 0 1 SNAM:epToVxCadExecute
use ecad8 128 423 100 0 detSigModeThresh
xform 0 288 928
p 192 352 100 0 1 DESC:Set compute threshold mode
p 224 1120 100 0 1 FTVA:LONG
p 224 1088 100 0 1 FTVB:LONG
p 224 1056 100 0 1 FTVC:DOUBLE
p 224 1024 100 0 1 FTVD:DOUBLE
p 224 992 100 0 1 FTVE:DOUBLE
p 192 304 100 0 1 INAM:epToVxCadInit
p 192 384 100 0 1 PREC:1
p 192 208 100 0 1 PV:$(top)$(wfs)
p 192 256 100 0 1 SNAM:epToVxCadExecute
use ecad8 -576 423 100 0 detSigModeCoadd
xform 0 -416 928
p -512 352 100 0 1 DESC:Set coadd mode
p -480 1120 100 0 1 FTVA:LONG
p -480 1088 100 0 1 FTVB:STRING
p -480 1056 100 0 1 FTVC:STRING
p -512 304 100 0 1 INAM:epToVxCadInit
p -512 208 100 0 1 PV:$(top)$(wfs)
p -512 256 100 0 1 SNAM:epToVxCadExecute
use ecad8 2240 1319 100 0 detSigModeFgCoadd
xform 0 2400 1824
p 2304 1248 100 0 1 DESC:Set FG and Focus and coadd dark mode
p 2336 2016 100 0 1 FTVA:LONG
p 2336 1984 100 0 1 FTVB:LONG
p 2336 1952 100 0 1 FTVC:STRING
p 2336 1920 100 0 1 FTVD:STRING
p 2336 1888 100 0 0 FTVE:STRING
p 2336 1856 100 0 0 FTVF:STRING
p 2304 1200 100 0 1 INAM:epToVxCadInit
p 2304 1280 100 0 1 PREC:0
p 2304 1104 100 0 1 PV:$(top)$(wfs)
p 2304 1152 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-12-16 03:25:36 $
p 2592 2304 200 0 -1 id:$Id: wfsCadMore.sch,v 1.4 2000-12-16 03:25:36 cboyer Exp $
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2592 -528 200 0 -1 revision:$Revision: 1.4 $
p 3120 -560 200 0 -1 title:Wavefront Sensor CAD Records
[comments]
