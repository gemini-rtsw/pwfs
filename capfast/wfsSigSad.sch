[schematic2]
uniq 152
[tools]
[detail]
w 1634 1963 100 0 n#151 esirs.aoThresh.FLNK 1568 1952 1760 1952 1760 1248 1952 1248 egenSubC.threshDiagP2.SLNK
s -592 2192 500 0 PWFS2 - WFS Signal Processing Status Records
s 2464 -704 500 512 wfsSigSad.sch
[cell use]
use egenSubC 1952 1159 100 0 threshDiagP2
xform 0 2096 1584
p 2016 1120 100 0 1 DESC:Display threshold / sub-apertures for P2
p 2272 1952 100 0 1 FTVA:DOUBLE
p 2272 1920 100 0 1 FTVB:DOUBLE
p 2272 1888 100 0 1 FTVC:DOUBLE
p 2272 1856 100 0 1 FTVD:DOUBLE
p 2016 1024 100 0 1 PV:$(top)$(wfs)
p 2016 1088 100 0 1 SCAN:Passive
p 2016 1056 100 0 1 SNAM:showThreshDiagP2
use esirs -64 1223 100 0 aoTotal
xform 0 144 1376
p 16 1184 100 0 1 DESC:Threshold for total counts
p -128 1088 100 0 0 DISS:NO_ALARM
p 16 1152 100 0 1 FTVL:DOUBLE
p 16 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 1703 100 0 aoThresh
xform 0 1360 1856
p 1232 1664 100 0 1 DESC:Threshold for centroids computation
p 1088 1568 100 0 0 DISS:NO_ALARM
p 1232 1632 100 0 1 FTVL:DOUBLE
p 1232 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -672 1703 100 0 aoCtrlInit
xform 0 -464 1856
p -592 1664 100 0 1 DESC:Signal processing init
p -592 1632 100 0 1 FTVL:STRING
p -592 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -64 1703 100 0 aoDarkInit
xform 0 144 1856
p 16 1664 100 0 1 DESC:Dark init
p 16 1632 100 0 1 FTVL:STRING
p 16 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 544 1703 100 0 aoFlatInit
xform 0 752 1856
p 624 1664 100 0 1 DESC:Flat init
p 624 1632 100 0 1 FTVL:STRING
p 624 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -672 1223 100 0 aoProcessMode
xform 0 -464 1376
p -592 1184 100 0 1 DESC:Processing mode
p -592 1152 100 0 1 FTVL:STRING
p -592 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs -64 775 100 0 aoSaveCbIm
xform 0 144 928
p 16 736 100 0 1 DESC:Save CB Image flag
p -128 640 100 0 0 DISS:NO_ALARM
p 16 704 100 0 1 FTVL:STRING
p 16 672 100 0 1 PV:$(sadtop)$(wfs)
use esirs 544 1223 100 0 aoContMatInit
xform 0 752 1376
p 624 1184 100 0 1 DESC:aO control matrix init
p 624 1152 100 0 1 FTVL:STRING
p 624 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 1223 100 0 aoIntMatInit
xform 0 1360 1376
p 1232 1184 100 0 1 DESC:aO interaction matrix init
p 1232 1152 100 0 1 FTVL:STRING
p 1232 1120 100 0 1 PV:$(sadtop)$(wfs)
use esirs -672 775 100 0 fgContMatInit
xform 0 -464 928
p -592 736 100 0 1 DESC:FG control matrix init
p -592 704 100 0 1 FTVL:STRING
p -592 672 100 0 1 PV:$(sadtop)$(wfs)
use esirs 544 775 100 0 aoSaveCbAoCtrl
xform 0 752 928
p 624 736 100 0 1 DESC:Save CB aO control
p 480 640 100 0 0 DISS:NO_ALARM
p 624 704 100 0 1 FTVL:STRING
p 624 672 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1152 775 100 0 aoSaveCbFgCtrl
xform 0 1360 928
p 1232 736 100 0 1 DESC:Save CB FG control
p 1088 640 100 0 0 DISS:NO_ALARM
p 1232 704 100 0 1 FTVL:STRING
p 1232 672 100 0 1 PV:$(sadtop)$(wfs)
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2001-09-17 20:15:08 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2608 -496 200 0 -1 revision:$Revision: 1.2 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Signal Processing SIR Records
[comments]
