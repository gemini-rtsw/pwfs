[schematic2]
uniq 27
[tools]
[detail]
s 1488 80 500 512 pwfs1Sad.sch
s -784 2144 500 0 Gemini A&G Peripheral Wavefront Sensing 1 System
[cell use]
use wfsSad 272 1367 100 0 wfsSad#20
xform 0 368 1488
p 272 1360 100 0 1 set1:wfs dc:
p 272 1328 100 0 1 set2:hindex $(sadtop)combHlt.C
p 272 1296 100 0 1 set3:mindex $(sadtop)combHlt.D
use systemSad 272 1783 100 0 systemSad#19
xform 0 368 1904
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the under top level schematic for
p 1564 558 100 0 -1 COMMENT2:the Gemini A&G PWFS1
p 1564 528 100 0 -1 COMMENT3:status alarm database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 0 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-05-18 22:01:51 $
p 1552 2368 100 0 -1 id:
p 1552 32 100 0 1 modified:C. Boyer
p 1792 176 100 0 -1 project:Gemini PWFS1
p 1552 144 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 112 100 0 -1 title:Under Top Level Status Alarm Database
[comments]
