[schematic2]
uniq 16
[tools]
[detail]
s 1488 80 500 512 pwfs1SadTop.sch
s -832 2144 500 0 Gemini A&G Peripheral Wavefront Sensing 1 System
[cell use]
use pwfsSad 64 1095 100 0 pwfsSad#15
xform 0 544 1296
p 64 1072 100 0 1 set1:top pwfs1:
p 64 1040 100 0 1 set2:sadtop pwfs1:
p 64 992 100 0 1 set3:wfsnum 1
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 0 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 2002/07/04 03:43:05 $
p 1552 2368 100 0 -1 id:
p 1552 32 100 0 1 modified:C. Boyer
p 1792 176 100 0 -1 project:Gemini PWFS2
p 1552 144 100 0 -1 revision:$Revision: 1.4 $
p 1792 112 100 0 -1 title:Top Level Status Alarm Database
[comments]
