[schematic2]
uniq 15
[tools]
[detail]
s 1488 80 500 512 pwfs2Top.sch
s -368 2160 500 0 Gemini A&G PWFS2
[cell use]
use pwfs2 64 1063 100 0 pwfs2#14
xform 0 512 1360
p 288 1040 100 0 1 set1:top pwfs2:
p 288 1008 100 0 1 set2:sadtop pwfs2:
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 0 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 2001-09-04 19:52:57 $
p 1552 2368 100 0 -1 id:
p 1552 32 100 0 1 modified:C. Boyer
p 1792 176 100 0 -1 project:Gemini PWFS2
p 1552 128 100 0 -1 revision:$Revision: 1.3 $
p 1792 112 100 0 -1 title:Top Level Schematic for Main Database
[comments]
