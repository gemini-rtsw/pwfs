[schematic2]
uniq 90
[tools]
[detail]
w 1496 235 100 0 n#89 estringouts.clearError1.OUT 1440 224 1600 224 hwout.hwout#86.outp
w 1496 683 100 0 n#88 estringouts.clearError.OUT 1440 672 1600 672 hwout.hwout#85.outp
w 1240 811 100 0 n#84 estringouts.clearError.FLNK 1440 704 1536 704 1536 800 992 800 992 240 1184 240 estringouts.clearError1.SLNK
s 2512 -704 500 512 errorSad.sch
s -160 2208 500 0 PWFS2 - Error Log Status Records
[cell use]
use hwout 1600 183 100 0 hwout#86
xform 0 1696 224
p 1696 215 100 0 -1 val(outp):$(sadtop)errorLog1 PP NMS
use hwout 1600 631 100 0 hwout#85
xform 0 1696 672
p 1696 663 100 0 -1 val(outp):$(sadtop)errorLog PP NMS
use estringouts 1184 167 100 0 clearError1
xform 0 1312 240
p 1248 96 100 0 1 OMSL:closed_loop
p 1248 128 100 0 1 PV:$(sadtop)
p 1248 64 100 0 1 VAL:0
p 1440 224 75 768 -1 pproc(OUT):PP
use estringouts 1184 615 100 0 clearError
xform 0 1312 688
p 1248 544 100 0 1 OMSL:supervisory
p 1248 576 100 0 1 PV:$(sadtop)
p 1440 672 75 768 -1 pproc(OUT):PP
use esirs 1184 1735 100 0 historyLog
xform 0 1392 1888
p 1248 1696 100 0 1 DESC:Latest log message
p 1120 1472 100 0 0 FDSC:Latest message (history log)
p 1248 1664 100 0 1 FTVL:STRING
p 1248 1600 100 0 1 PV:$(sadtop)
p 1248 1632 100 0 1 SNAM:
use esirs 1184 1223 100 0 historyLog1
xform 0 1392 1376
p 1248 1184 100 0 1 DESC:Latest log message
p 1120 960 100 0 0 FDSC:Latest message (history log)
p 1248 1152 100 0 1 FTVL:STRING
p 1248 1088 100 0 1 PV:$(sadtop)
p 1248 1120 100 0 1 SNAM:
use esirs 1888 1735 100 0 errorLog
xform 0 2096 1888
p 1952 1696 100 0 1 DESC:Latest error message
p 1824 1472 100 0 0 FDSC:Latest error message
p 1952 1664 100 0 1 FTVL:STRING
p 1952 1600 100 0 1 PV:$(sadtop)
p 1952 1632 100 0 1 SNAM:
use esirs 1888 1223 100 0 errorLog1
xform 0 2096 1376
p 1952 1184 100 0 1 DESC:Latest error message
p 1824 960 100 0 0 FDSC:Latest error message
p 1952 1152 100 0 1 FTVL:STRING
p 1952 1088 100 0 1 PV:$(sadtop)
p 1952 1120 100 0 1 SNAM:
use notes 3520 -265 100 0 notes#13
xform 0 3776 -80
p 4048 -114 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3548 46 100 0 -1 COMMENT1:This schematic contains the error
p 3548 14 100 0 -1 COMMENT2:log status records.
p 3548 -48 100 0 -1 COMMENT4:NOTE: The error count records assume
p 3548 -80 100 0 -1 COMMENT5:there are between 1 and 3 processors.
p 3548 -112 100 0 -1 COMMENT6:It would be more flexible to put each
p 3548 -144 100 0 -1 COMMENT7:set of these records in a separate
p 3548 -176 100 0 -1 COMMENT8:"procSad.sch" schematic. SMB - 4/2/98.
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 0 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2001-02-09 20:17:12 $
p 2592 2336 200 0 -1 id:
p 2704 -752 100 0 1 modified:C. Boyer
p 3120 -416 200 0 -1 project:Gemini PWFS2
p 2592 -480 200 0 -1 revision:$Revision: 1.3 $
p 3120 -544 200 0 -1 title:Error Log Status Records
[comments]
