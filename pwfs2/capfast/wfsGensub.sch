[schematic2]
uniq 112
[tools]
[detail]
w 388 1851 100 2 n#109 hwin.hwin#108.in 384 1856 384 1856 egenSub.aoZero.INPC
w -348 1851 100 2 n#107 hwin.hwin#105.in -352 1856 -352 1856 egenSub.ttfZero.INPC
w 388 1979 100 2 n#103 hwin.hwin#98.in 384 1984 384 1984 egenSub.aoZero.INPA
w -348 1979 100 2 n#102 hwin.hwin#96.in -352 1984 -352 1984 egenSub.ttfZero.INPA
s -896 2080 100 0 port C reads the arm angle from the Zeiss system
s -896 2112 100 0 port B selects a fudge angle
s -896 2144 100 0 port A reads the table angle from the Zeiss system
s 2464 -704 500 512 wfsGensub.sch
s 112 2224 500 0 PWFS2 - WFS genSub records
[cell use]
use egenSubC -352 7 100 0 fgDiag
xform 0 -208 432
p -352 -48 100 0 1 DESC:Display ao Diagnostics
p -32 800 100 0 1 FTVA:DOUBLE
p -32 768 100 0 1 FTVB:DOUBLE
p -32 736 100 0 1 FTVC:DOUBLE
p -32 704 100 0 1 FTVD:DOUBLE
p -32 672 100 0 1 FTVE:DOUBLE
p -32 640 100 0 1 FTVF:DOUBLE
p -32 608 100 0 1 FTVG:DOUBLE
p -32 576 100 0 1 FTVH:DOUBLE
p -32 544 100 0 1 FTVI:DOUBLE
p -32 512 100 0 1 FTVJ:DOUBLE
p -32 480 100 0 1 FTVK:LONG
p -32 448 100 0 1 FTVL:LONG
p -32 416 100 0 1 FTVM:DOUBLE
p -32 384 100 0 1 FTVN:DOUBLE
p -32 352 100 0 1 FTVO:DOUBLE
p -32 320 100 0 1 FTVP:DOUBLE
p -32 288 100 0 1 FTVQ:DOUBLE
p -32 256 100 0 0 FTVR:DOUBLE
p -32 224 100 0 0 FTVS:DOUBLE
p -352 -96 100 0 1 INAM:
p -575 -859 100 0 0 NOJ:1
p -575 -859 100 0 0 NOVJ:1
p -640 558 100 0 0 PREC:4
p -352 -240 100 0 1 PV:$(top)$(wfs)
p -352 -208 100 0 1 SCAN:.5 second
p -352 -160 100 0 1 SNAM:showFgDiags
use egenSubC 384 7 100 0 cbDiag
xform 0 528 432
p 384 -48 100 0 1 DESC:Display CB Diagnostics
p 704 800 100 0 1 FTVA:LONG
p 704 768 100 0 1 FTVB:LONG
p 704 736 100 0 1 FTVC:LONG
p 704 704 100 0 1 FTVD:LONG
p 704 672 100 0 0 FTVE:DOUBLE
p 704 640 100 0 0 FTVF:DOUBLE
p 704 608 100 0 0 FTVG:DOUBLE
p 704 576 100 0 0 FTVH:DOUBLE
p 704 544 100 0 0 FTVI:DOUBLE
p 704 512 100 0 0 FTVJ:DOUBLE
p 704 480 100 0 0 FTVK:DOUBLE
p 704 448 100 0 0 FTVL:DOUBLE
p 161 -475 100 0 0 FTVM:DOUBLE
p 384 -96 100 0 1 INAM:
p 161 -859 100 0 0 NOJ:1
p 161 -859 100 0 0 NOVJ:1
p 96 558 100 0 0 PREC:4
p 384 -240 100 0 1 PV:$(top)$(wfs)
p 384 -208 100 0 1 SCAN:.5 second
p 384 -160 100 0 1 SNAM:showCbDiags
use hwin 192 1815 100 0 hwin#108
xform 0 288 1856
p 176 1888 100 0 -1 val(in):ag:p2:armAngle
use hwin -544 1815 100 0 hwin#105
xform 0 -448 1856
p -560 1888 100 0 -1 val(in):ag:p2:armAngle
use hwin 192 1943 100 0 hwin#98
xform 0 288 1984
p 176 2016 100 0 -1 val(in):ag:p2:tableAngle
use hwin -544 1943 100 0 hwin#96
xform 0 -448 1984
p -560 2016 100 0 -1 val(in):ag:p2:tableAngle
use egenSub 1056 1223 100 0 probeOffset
xform 0 1200 1648
p 1120 1184 100 0 1 DESC:Probe offsets
p 1136 1440 100 0 1 FTJ:DOUBLE
p 1376 1440 100 0 0 FTVJ:DOUBLE
p 1120 1152 100 0 1 INAM:epToVxGensubInit
p 1136 1408 100 0 1 NOJ:9
p 1136 1376 100 0 1 NOVJ:9
p 768 1774 100 0 0 PREC:4
p 1120 1088 100 0 1 PV:$(top)$(wfs)
p 1120 1120 100 0 1 SNAM:epToVxGensubInput
use egenSub -352 1223 100 0 ttfZero
xform 0 -208 1648
p -288 1184 100 0 1 DESC:T-T-F rotation angle and WFS zero point
p -288 1024 100 0 1 FTA:STRING
p -288 992 100 0 1 FTB:STRING
p -288 960 100 0 1 FTC:STRING
p -272 1440 100 0 1 FTJ:DOUBLE
p -32 1440 100 0 0 FTVJ:DOUBLE
p -288 1152 100 0 1 INAM:
p -272 1408 100 0 1 NOJ:8
p -272 1376 100 0 1 NOVJ:8
p -640 1774 100 0 0 PREC:4
p -288 1088 100 0 1 PV:$(top)$(wfs)
p -288 1056 100 0 1 SCAN:.1 second
p -288 1120 100 0 1 SNAM:ttfZero
use egenSub 384 1223 100 0 aoZero
xform 0 528 1648
p 448 1184 100 0 1 DESC:aO rotation angle and WFS zero point
p 448 1024 100 0 1 FTA:STRING
p 448 992 100 0 1 FTB:STRING
p 448 960 100 0 1 FTC:STRING
p 464 1440 100 0 1 FTJ:DOUBLE
p 704 1440 100 0 0 FTVJ:DOUBLE
p 448 1152 100 0 1 INAM:
p 464 1408 100 0 1 NOJ:24
p 464 1376 100 0 1 NOVJ:24
p 96 1774 100 0 0 PREC:4
p 448 1088 100 0 1 PV:$(top)$(wfs)
p 448 1056 100 0 1 SCAN:.1 second
p 448 1120 100 0 1 SNAM:aoZero
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-07-10 21:47:06 $
p 2592 2304 200 0 -1 id:$Id: wfsGensub.sch,v 1.4 2000-07-10 21:47:06 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2592 -528 200 0 -1 revision:$Revision: 1.4 $
p 3120 -560 200 0 -1 title:Wavefront Sensor genSub Records
use notes 3552 -281 100 0 notes#13
xform 0 3808 -96
p 4080 -130 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3580 30 100 0 -1 COMMENT1:This schematic contains the genSub records
p 3580 -2 100 0 -1 COMMENT2:for the commands connected with one
p 3580 -32 100 0 -1 COMMENT3:wavefront sensor. It may be duplicated
p 3580 -64 100 0 -1 COMMENT4:for each wavefront sensor, using the
p 3580 -96 100 0 -1 COMMENT5:wfs macro to distinguish each one.
p 3580 -160 100 0 -1 COMMENT7:See ICD 1.6.2/1.6.3 for a detailed
p 3580 -192 100 0 -1 COMMENT8:description of these commands.
[comments]
