[schematic2]
uniq 99
[tools]
[detail]
s -160 2208 500 0 PWFS2 - System Status Records
s 2512 -704 500 512 systemSad.sch
[cell use]
use esirs 1856 1767 100 0 testResults
xform 0 2064 1920
p 1920 1728 100 0 1 DESC:Test results
p 1792 1504 100 0 0 FDSC:System version number
p 1920 1696 100 0 1 FTVL:STRING
p 1920 1632 100 0 1 PV:$(sadtop)
p 1920 1664 100 0 1 SNAM:
use esirs -672 1767 100 0 name
xform 0 -464 1920
p -608 1728 100 0 1 DESC:System name
p -736 1504 100 0 0 FDSC:System name
p -608 1696 100 0 1 FTVL:STRING
p -608 1632 100 0 1 PV:$(sadtop)
p -608 1664 100 0 1 SNAM:
p -608 1600 100 0 1 VAL:Peripheral WFS
use esirs 0 1767 100 0 state
xform 0 208 1920
p 64 1728 100 0 1 DESC:System state
p -64 1504 100 0 0 FDSC:System state [BOOTING|INITIALISING|RUNNING]
p 64 1696 100 0 1 FTVL:STRING
p 64 1632 100 0 1 PV:$(sadtop)
p 64 1664 100 0 1 SNAM:
p 64 1600 100 0 1 VAL:BOOTING
use esirs 640 1767 100 0 version
xform 0 848 1920
p 704 1728 100 0 1 DESC:System version number
p 576 1504 100 0 0 FDSC:System version number
p 704 1696 100 0 1 FTVL:STRING
p 704 1632 100 0 1 PV:$(sadtop)
p 704 1664 100 0 1 SNAM:
use esirs -672 1159 100 0 simMode
xform 0 -464 1312
p -608 1120 100 0 1 DESC:Simulation mode
p -736 928 100 0 0 EVNT:0
p -736 896 100 0 0 FDSC:Simulation mode [VSM|FAST|FULL|NONE]
p -608 1088 100 0 1 FTVL:STRING
p -608 1024 100 0 1 PV:$(sadtop)
p -608 1056 100 0 1 SNAM:
use esirs 0 1159 100 0 debugMode
xform 0 208 1312
p 64 1120 100 0 1 DESC:Debugging mode
p -64 928 100 0 0 EVNT:0
p -64 896 100 0 0 FDSC:Debugging mode [NONE|MIN|FULL]
p 64 1088 100 0 1 FTVL:STRING
p 64 1024 100 0 1 PV:$(sadtop)
p 64 1056 100 0 1 SNAM:
use esirs 640 1159 100 0 trackId
xform 0 848 1312
p 704 1120 100 0 1 DESC:Data stream ID
p 704 1056 100 0 1 EGU:0/1
p 576 896 100 0 0 FDSC:Data stream ID
p 704 1088 100 0 1 FTVL:LONG
p 864 1024 100 0 1 HIGH:100000000
p 864 992 100 0 1 HIHI:100000000
p 704 992 100 0 1 LOLO:0
p 704 1024 100 0 1 LOW:0
p 704 960 100 0 1 PV:$(sadtop)
use esirs 1248 1159 100 0 arrayS
xform 0 1456 1312
p 1312 1120 100 0 1 DESC:Status of continuous data
p 1184 896 100 0 0 FDSC:Array status [GOOD|LATE|INVALID]
p 1312 1088 100 0 1 FTVL:STRING
p 1312 1024 100 0 1 PV:$(sadtop)
p 1312 1056 100 0 1 SNAM:
p 1312 992 100 0 1 VAL:BOOTING
use esirs 1248 1767 100 0 inPosition
xform 0 1456 1920
p 1312 1728 100 0 1 DESC:In position SIR record
p 1312 1664 100 0 1 EGU:0/1
p 1392 1840 100 0 0 FDSC:In position SIR record
p 1312 1696 100 0 1 FTVL:LONG
p 1472 1632 100 0 1 HIGH:100000000
p 1472 1600 100 0 1 HIHI:100000000
p 1312 1600 100 0 1 LOLO:0
p 1312 1632 100 0 1 LOW:0
p 1312 1568 100 0 1 PV:$(sadtop)
use errorLogSad 1920 -377 100 0 errorLogSad#79
xform 0 2016 -256
use resourceSad 1600 -377 100 0 resourceSad#78
xform 0 1696 -256
use systemHealthSad 1280 -377 100 0 systemHealthSad#48
xform 0 1376 -256
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 0 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2002/07/04 03:43:05 $
p 2592 2336 200 0 -1 id:
p 2704 -752 100 0 1 modified:C. Boyer
p 3120 -416 200 0 -1 project:Gemini PWFS2
p 2592 -480 200 0 -1 revision:$Revision: 1.5 $
p 3120 -544 200 0 -1 title:System Status Records
[comments]
