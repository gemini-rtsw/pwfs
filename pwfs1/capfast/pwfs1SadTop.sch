[schematic2]
uniq 16
[tools]
[detail]
s 1488 80 500 512 pwfs1SadTop.sch
s -832 2144 500 0 Gemini A&G Peripheral Wavefront Sensing 1 System
[cell use]
use pwfs1Sad 64 1079 100 0 pwfs1Sad#15
xform 0 544 1280
p 64 1056 100 0 1 set1:top pwfs1:
p 64 1024 100 0 1 set2:sadtop pwfs1:
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the top level schematic for the
p 1564 558 100 0 -1 COMMENT2:Gemini A&G PWFS1
p 1564 528 100 0 -1 COMMENT3:status alarm database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 0 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 2001-02-07 02:51:10 $
p 1552 2368 100 0 -1 id:
p 1552 32 100 0 1 modified:C. Boyer
p 1792 176 100 0 -1 project:Gemini PWFS1
p 1552 144 100 0 -1 revision:$Revision: 1.5 $
p 1792 112 100 0 -1 title:Top Level Status Alarm Database
[comments]
