[schematic2]
uniq 117
[tools]
[detail]
w -348 1851 100 2 n#116 hwin.hwin#115.in -352 1856 -352 1856 egenSub.ttfZero.INPC
w 388 1851 100 2 n#114 hwin.hwin#113.in 384 1856 384 1856 egenSub.aoZero.INPC
w 2610 75 100 0 n#112 egenSubC.aoDiag4.FLNK 2560 64 2720 64 2720 96 2880 96 egenSubC.fgDiag.SLNK
w 1954 75 100 0 n#111 egenSubC.aoDiag3.FLNK 1888 64 2080 64 2080 96 2272 96 egenSubC.aoDiag4.SLNK
w 1330 75 100 0 n#109 egenSubC.aoDiag2.FLNK 1280 64 1440 64 1440 96 1600 96 egenSubC.aoDiag3.SLNK
w 466 -277 100 0 n#108 egenSubC.aoDiag1.FLNK -64 64 96 64 96 -288 896 -288 896 96 992 96 egenSubC.aoDiag2.SLNK
w 388 1979 100 2 n#103 hwin.hwin#98.in 384 1984 384 1984 egenSub.aoZero.INPA
w -348 1979 100 2 n#102 hwin.hwin#96.in -352 1984 -352 1984 egenSub.ttfZero.INPA
s 112 2224 500 0 PWFS1 - WFS genSub records
s 2464 -704 500 512 wfsGensub.sch
s -896 2144 100 0 port A reads the probe angle from the Zeiss system
s -896 2112 100 0 port B selects add or subtract the zeiss angle
s -896 2080 100 0 port C is a fundge rotation angle (degrees)
[cell use]
use hwin -544 1815 100 0 hwin#115
xform 0 -448 1856
p -560 1888 100 0 -1 val(in):ag:p1:armAngle
use hwin -544 1943 100 0 hwin#96
xform 0 -448 1984
p -560 2016 100 0 -1 val(in):ag:p1:RT34PosA.VALA
use hwin 192 1943 100 0 hwin#98
xform 0 288 1984
p 176 2016 100 0 -1 val(in):ag:p1:RT34PosA.VALA
use hwin 192 1815 100 0 hwin#113
xform 0 288 1856
p 176 1888 100 0 -1 val(in):ag:p1:armAngle
use egenSubC 2272 7 100 0 aoDiag4
xform 0 2416 432
p 2272 -48 100 0 1 DESC:Display ao Diagnostic 4
p 2272 -96 100 0 1 INAM:
p 1984 558 100 0 0 PREC:4
p 2272 -240 100 0 1 PV:$(top)$(wfs)
p 2272 -208 100 0 1 SCAN:Passive
p 2272 -160 100 0 1 SNAM:showAoDiag4
use egenSubC 1600 7 100 0 aoDiag3
xform 0 1744 432
p 1600 -48 100 0 1 DESC:Display ao Diagnostic 3
p 1600 -96 100 0 1 INAM:
p 1312 558 100 0 0 PREC:4
p 1600 -240 100 0 1 PV:$(top)$(wfs)
p 1600 -208 100 0 1 SCAN:Passive
p 1600 -160 100 0 1 SNAM:showAoDiag3
use egenSubC 992 7 100 0 aoDiag2
xform 0 1136 432
p 992 -48 100 0 1 DESC:Display ao Diagnostics 2
p 992 -96 100 0 1 INAM:
p 704 558 100 0 0 PREC:4
p 992 -240 100 0 1 PV:$(top)$(wfs)
p 992 -208 100 0 1 SCAN:Passive
p 992 -160 100 0 1 SNAM:showAoDiag2
use egenSubC 2880 7 100 0 fgDiag
xform 0 3024 432
p 2880 -48 100 0 1 DESC:Display FG Diagnostics
p 2880 -96 100 0 1 INAM:
p 2657 -859 100 0 0 NOJ:1
p 2657 -859 100 0 0 NOVJ:1
p 2592 558 100 0 0 PREC:4
p 2880 -240 100 0 1 PV:$(top)$(wfs)
p 2880 -208 100 0 1 SCAN:Passive
p 2880 -160 100 0 1 SNAM:showFgDiags
use egenSubC -352 7 100 0 aoDiag1
xform 0 -208 432
p -352 -48 100 0 1 DESC:Display ao Diagnostics 1
p -352 -96 100 0 1 INAM:
p -640 558 100 0 0 PREC:4
p -352 -240 100 0 1 PV:$(top)$(wfs)
p -352 -208 100 0 1 SCAN:.5 second
p -352 -160 100 0 1 SNAM:showAoDiag1
use egenSub 384 1223 100 0 aoZero
xform 0 528 1648
p 448 1184 100 0 1 DESC:aO rotation angle and WFS zero point
p 448 1024 100 0 1 FTA:STRING
p 448 992 100 0 1 FTB:STRING
p 448 960 100 0 1 FTC:STRING
p 480 928 100 0 1 FTD:STRING
p 464 1440 100 0 1 FTJ:DOUBLE
p 704 1440 100 0 0 FTVJ:DOUBLE
p 448 1152 100 0 1 INAM:
p 464 1408 100 0 1 NOJ:24
p 464 1376 100 0 1 NOVJ:24
p 96 1774 100 0 0 PREC:4
p 448 1088 100 0 1 PV:$(top)$(wfs)
p 448 1056 100 0 1 SCAN:.1 second
p 448 1120 100 0 1 SNAM:aoZero
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
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-04-19 01:36:46 $
p 2592 2304 200 0 -1 id:$Id: wfsGensub.sch,v 1.6 2000-04-19 01:36:46 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2592 -528 200 0 -1 revision:$Revision: 1.6 $
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
