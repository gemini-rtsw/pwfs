[schematic2]
uniq 16
[tools]
[detail]
s 1488 80 500 512 testHarnessTop.sch
s 192 2016 500 0 Test Harness
s -368 2160 500 0 Gemini A&G Wavefront Sensing System
[cell use]
use testHarness 0 775 100 0 testHarness#15
xform 0 544 1264
p 384 784 100 0 1 set1:top ag:wfs:
p 384 752 100 0 1 set2:test th:
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard
p 1564 590 100 0 -1 COMMENT1:This is the top level schematic for the
p 1564 558 100 0 -1 COMMENT2:Gemini A&G Wavefront Processing System
p 1564 528 100 0 -1 COMMENT3:test harness.
p 1564 464 100 0 -1 COMMENT5:It is not needed by the operational
p 1564 432 100 0 -1 COMMENT6:system, but may be used for testing
p 1564 400 100 0 -1 COMMENT7:in the absence of other systems.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-05-18 22:01:48 $
p 1552 2368 100 0 -1 id:$Id: testHarnessTop.sch,v 1.1.1.1 1999-05-18 22:01:48 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 128 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 112 100 0 -1 title:Top Level Schematic for Test Harness
[comments]
