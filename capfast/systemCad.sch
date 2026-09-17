[schematic2]
uniq 87
[tools]
[detail]
s -448 2112 500 0 PWFS2 - System CAD Records
s 1808 32 500 512 systemCad.sch
[cell use]
use ecad2 -320 1319 100 0 reboot
xform 0 -160 1632
p -256 1248 100 0 1 DESC:Reboot system
p -224 1696 100 0 0 FTVA:STRING
p -224 1632 100 0 0 FTVB:STRING
p -256 1216 100 0 1 INAM:epToVxCadInit
p -256 1152 100 0 1 PV:$(top)
p -256 1184 100 0 1 SNAM:epToVxCadExecute
use ecad2 320 1319 100 0 simulate
xform 0 480 1632
p 384 1248 100 0 1 DESC:Set simulation mode
p 416 1696 100 0 1 FTVA:STRING
p 416 1632 100 0 1 FTVB:STRING
p 384 1216 100 0 1 INAM:epToVxCadInit
p 384 1152 100 0 1 PV:$(top)
p 384 1184 100 0 1 SNAM:epToVxCadExecute
use ecad2 896 1319 100 0 debug
xform 0 1056 1632
p 960 1248 100 0 1 DESC:Set debugging mode
p 992 1696 100 0 1 FTVA:STRING
p 992 1632 100 0 1 FTVB:STRING
p 960 1216 100 0 1 INAM:epToVxCadInit
p 960 1152 100 0 1 PV:$(top)
p 960 1184 100 0 1 SNAM:epToVxCadExecute
use ecad2 1536 1319 100 0 park
xform 0 1696 1632
p 1600 1248 100 0 1 DESC:Park the WFS
p 1632 1696 100 0 0 FTVA:STRING
p 1632 1632 100 0 0 FTVB:STRING
p 1600 1216 100 0 1 INAM:epToVxCadInit
p 1600 1152 100 0 1 PV:$(top)
p 1600 1184 100 0 1 SNAM:epToVxCadExecute
use bc200tr -704 -152 -100 0 frame
xform 0 976 1152
p 1872 16 100 0 -1 author:S.M.Beard
p 2096 0 100 0 -1 border:C
p 1872 -16 100 0 1 checked:A.Foster
p 2096 -32 100 0 -1 date:$Date: 2000-07-10 21:47:05 $
p 1872 2320 100 0 -1 id:$Id: systemCad.sch 39190 2011-11-17 02:21:40Z aebbers $
p 2112 128 100 0 -1 project:Gemini PWFS2
p 1872 96 100 0 -1 revision:$Revision: 1.3 $
p 2112 64 100 0 -1 title:System CAD Records
[comments]
