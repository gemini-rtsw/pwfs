[schematic2]
uniq 87
[tools]
[detail]
s -448 2112 500 0 PWFS1 - System CAD Records
s 1808 32 500 512 systemCad.sch
[cell use]
use ecad2 1440 1255 100 0 park
xform 0 1600 1568
p 1504 1216 100 0 1 DESC:Park the WFS
p 1504 1184 100 0 1 INAM:epToVxCadInit
p 1504 1088 100 0 1 PV:$(top)
p 1504 1120 100 0 1 SCAN:Passive
p 1504 1152 100 0 1 SNAM:epToVxCadExecute
use ecad2 864 1255 100 0 debug
xform 0 1024 1568
p 928 1216 100 0 1 DESC:Set debugging mode
p 928 1184 100 0 1 INAM:epToVxCadInit
p 928 1088 100 0 1 PV:$(top)
p 928 1120 100 0 1 SCAN:Passive
p 928 1152 100 0 1 SNAM:epToVxCadExecute
use ecad2 288 1255 100 0 simulate
xform 0 448 1568
p 352 1216 100 0 1 DESC:Set simulation mode
p 352 1184 100 0 1 INAM:epToVxCadInit
p 352 1088 100 0 1 PV:$(top)
p 352 1120 100 0 1 SCAN:Passive
p 352 1152 100 0 1 SNAM:epToVxCadExecute
use ecad2 -288 1255 100 0 reboot
xform 0 -128 1568
p -224 1216 100 0 1 DESC:Reboot system
p -224 1184 100 0 1 INAM:epToVxCadInit
p -224 1088 100 0 1 PV:$(top)
p -224 1120 100 0 1 SCAN:Passive
p -224 1152 100 0 1 SNAM:epToVxCadExecute
use bc200tr -704 -152 -100 0 frame
xform 0 976 1152
p 1872 16 100 0 -1 author:S.M.Beard
p 2096 0 100 0 -1 border:C
p 1872 -16 100 0 1 checked:A.Foster
p 2096 -32 100 0 -1 date:$Date: 2000-06-21 01:27:40 $
p 1872 2320 100 0 -1 id:$Id: systemCad.sch,v 1.4 2000-06-21 01:27:40 cboyer Exp $
p 2112 128 100 0 -1 project:Gemini PWFS1
p 1872 96 100 0 -1 revision:$Revision: 1.4 $
p 2112 64 100 0 -1 title:System CAD Records
[comments]
