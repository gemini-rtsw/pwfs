[schematic2]
uniq 168
[tools]
[detail]
w -28 1947 100 2 n#167 hwin.hwin#113.in -32 1952 -32 1952 egenSubE.aoZero.INPC
w -28 2011 100 2 n#164 hwin.hwin#163.in -32 2016 -32 2016 egenSubE.aoZero.INPA
w -636 1851 100 2 n#116 hwin.hwin#115.in -640 1856 -640 1856 egenSub.ttfZero.INPC
w -636 1979 100 2 n#102 hwin.hwin#96.in -640 1984 -640 1984 egenSub.ttfZero.INPA
s -896 2080 100 0 port C reads the arm angle from the Zeiss system
s -896 2112 100 0 port B selects a fudge angle
s -896 2144 100 0 port A reads the table angle from the Zeiss system
s 2464 -704 500 512 wfsGensub.sch
s 112 2224 500 0 PWFS1 - WFS genSub records
[cell use]
use hwin -224 1911 100 0 hwin#113
xform 0 -128 1952
p -240 1984 100 0 -1 val(in):ag:p1:armAngle
use hwin -832 1943 100 0 hwin#96
xform 0 -736 1984
p -848 2016 100 0 -1 val(in):ag:p1:RT34PosA.VALA
use hwin -832 1815 100 0 hwin#115
xform 0 -736 1856
p -848 1888 100 0 -1 val(in):ag:p1:armAngle
use hwin -224 1975 100 0 hwin#163
xform 0 -128 2016
p -240 2048 100 0 -1 val(in):ag:p1:RT34PosA.VALA
use egenSubE -32 1223 100 0 aoZero
xform 0 112 1648
p 32 1200 100 0 1 DESC:ao rotation angle and WFS zero point
p 64 2000 100 0 1 FTA:STRING
p 64 1968 100 0 1 FTB:STRING
p 64 1936 100 0 1 FTC:STRING
p 64 1904 100 0 1 FTD:STRING
p 64 1584 100 0 1 FTJ:DOUBLE
p 64 1552 100 0 1 NOJ:24
p 64 1520 100 0 1 NOVJ:24
p 32 1072 100 0 1 PREC:4
p 32 1168 100 0 1 PV:$(top)$(wfs)
p 32 1136 100 0 1 SCAN:.1 second
p 32 1104 100 0 1 SNAM:aoZero
use egenSubC 1184 1223 100 0 cbDiag
xform 0 1328 1648
p 1248 1184 100 0 1 DESC:Display CB Diagnostics
p 1504 2016 100 0 1 FTVA:LONG
p 1504 1984 100 0 1 FTVB:LONG
p 1504 1952 100 0 1 FTVC:LONG
p 1504 1920 100 0 1 FTVD:LONG
p 1504 1888 100 0 1 FTVE:LONG
p 1504 1856 100 0 1 FTVF:LONG
p 1248 1152 100 0 1 INAM:
p 961 357 100 0 0 NOJ:1
p 961 357 100 0 0 NOVJ:1
p 896 1774 100 0 0 PREC:4
p 1248 1056 100 0 1 PV:$(top)$(wfs)
p 1248 1088 100 0 1 SCAN:1 second
p 1248 1120 100 0 1 SNAM:showCbDiag
use egenSub 576 1223 100 0 probeOffset
xform 0 720 1648
p 640 1184 100 0 1 DESC:Probe offsets
p 656 1440 100 0 1 FTJ:DOUBLE
p 896 1440 100 0 0 FTVJ:DOUBLE
p 640 1152 100 0 1 INAM:epToVxGensubInit
p 656 1408 100 0 1 NOJ:9
p 656 1376 100 0 1 NOVJ:9
p 288 1774 100 0 0 PREC:4
p 640 1088 100 0 1 PV:$(top)$(wfs)
p 640 1120 100 0 1 SNAM:epToVxGensubInput
use egenSub -640 1223 100 0 ttfZero
xform 0 -496 1648
p -576 1184 100 0 1 DESC:T-T-F rotation angle and WFS zero point
p -576 1024 100 0 1 FTA:STRING
p -576 992 100 0 1 FTB:STRING
p -576 960 100 0 1 FTC:STRING
p -560 1440 100 0 1 FTJ:DOUBLE
p -320 1440 100 0 0 FTVJ:DOUBLE
p -576 1152 100 0 1 INAM:
p -560 1408 100 0 1 NOJ:8
p -560 1376 100 0 1 NOVJ:8
p -928 1774 100 0 0 PREC:4
p -576 1088 100 0 1 PV:$(top)$(wfs)
p -576 1056 100 0 1 SCAN:.1 second
p -576 1120 100 0 1 SNAM:ttfZero
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2002-06-05 04:11:34 $
p 2592 2304 200 0 -1 id:$Id: wfsGensub.sch,v 1.9 2002-06-05 04:11:34 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2592 -528 200 0 -1 revision:$Revision: 1.9 $
p 3120 -560 200 0 -1 title:Wavefront Sensor genSub Records
use notes 1040 -761 100 0 notes#13
xform 0 1296 -576
p 1568 -610 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1068 -450 100 0 -1 COMMENT1:This schematic contains the genSub records
p 1068 -482 100 0 -1 COMMENT2:for the commands connected with one
p 1068 -512 100 0 -1 COMMENT3:wavefront sensor. It may be duplicated
p 1068 -544 100 0 -1 COMMENT4:for each wavefront sensor, using the
p 1068 -576 100 0 -1 COMMENT5:wfs macro to distinguish each one.
p 1068 -640 100 0 -1 COMMENT7:See ICD 1.6.2/1.6.3 for a detailed
p 1068 -672 100 0 -1 COMMENT8:description of these commands.
[comments]
