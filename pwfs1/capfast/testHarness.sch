[schematic2]
uniq 35
[tools]
[detail]
w 1176 843 100 0 n#31 hwin.hwin#30.in 1184 832 1216 832 esirs.sourceAWavelength.INP
s 1488 80 500 512 testHarness.sch
s -32 2048 500 0 Test Harness
s -592 2192 500 0 Gemini A&G Wavefront Sensing System
[cell use]
use esirs 1216 1831 100 0 name
xform 0 1424 1984
p 1280 1792 100 0 1 DESC:Name of TCS
p 1472 1760 100 0 0 EGU:units
p 1280 1760 100 0 1 FTVL:STRING
p 1280 1728 100 0 1 PV:tcs:sad:
p 1456 1760 100 0 1 VAL:Simulated TCS
use hwin 992 791 100 0 hwin#30
xform 0 1088 832
p 995 824 100 0 -1 val(in):5500.0
use esirs 1216 1415 100 0 sourceAInputFrame
xform 0 1424 1568
p 1280 1376 100 0 1 DESC:TCS Source A Input Frame
p 1472 1344 100 0 0 EGU:units
p 1280 1344 100 0 1 FTVL:STRING
p 1280 1312 100 0 1 PV:tcs:sad:
p 1456 1344 100 0 1 VAL:FK5
use esirs 1216 999 100 0 sourceAEquinox
xform 0 1424 1152
p 1280 960 100 0 1 DESC:TCS Source A Equinox
p 1472 928 100 0 0 EGU:units
p 1280 928 100 0 1 FTVL:STRING
p 1280 896 100 0 1 PV:tcs:sad:
p 1456 928 100 0 1 VAL:J2000
use esirs 1216 583 100 0 sourceAWavelength
xform 0 1424 736
p 1280 544 100 0 1 DESC:TCS Source A Wavelength
p 1472 512 100 0 1 EGU:Angstroms
p 1280 512 100 0 1 FTVL:DOUBLE
p 1280 480 100 0 1 PV:tcs:sad:
use egenSub 576 887 100 0 astCtx
xform 0 720 1312
p 656 1648 100 0 1 FTA:DOUBLE
p 656 1616 100 0 1 FTVA:DOUBLE
p 656 848 100 0 1 INAM:
p 656 1584 100 0 1 NOA:39
p 656 1552 100 0 1 NOVA:39
p 656 784 100 0 1 PV:tcs:ak:
p 656 752 100 0 1 SCAN:10 second
p 656 816 100 0 1 SNAM:testSimulateAstCtx
use testWfs -192 695 100 0 testWfs#18
xform 0 -96 816
p -192 688 100 0 1 set1:wfs oi:
use testWfs -192 1095 100 0 testWfs#17
xform 0 -96 1216
p -192 1088 100 0 1 set1:wfs p2:
use testWfs -192 1479 100 0 testWfs#16
xform 0 -96 1600
p -192 1472 100 0 1 set1:wfs p1:
use notes 1696 231 100 0 notes#15
xform 0 1952 416
p 2224 382 100 0 0 AUTHOR:S.M.Beard
p 1724 542 100 0 -1 COMMENT1:This is the under top level schematic for
p 1724 510 100 0 -1 COMMENT2:the Gemini A&G Wavefront Processing
p 1724 480 100 0 -1 COMMENT3:System test harness.
p 1724 416 100 0 -1 COMMENT5:It is not needed by the operational
p 1724 384 100 0 -1 COMMENT6:system, but may be used for testing
p 1724 352 100 0 -1 COMMENT7:in the absence of other systems.
use bc200tr -1024 -120 -100 0 frame
xform 0 656 1184
p 1552 48 100 0 -1 author:$Author: cboyer $
p 1776 32 100 0 -1 border:C
p 1552 16 100 0 1 checked:B.Goodrich
p 1776 0 100 0 -1 date:$Date: 1999-05-18 22:01:48 $
p 1552 2352 100 0 -1 id:$Id: testHarness.sch,v 1.1.1.1 1999-05-18 22:01:48 cboyer Exp $
p 1792 160 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 128 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 96 100 0 -1 title:Test Harness Database (for testing only)
[comments]
