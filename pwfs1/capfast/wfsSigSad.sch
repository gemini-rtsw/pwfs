[schematic2]
uniq 160
[tools]
[detail]
w 2418 1227 100 0 n#153 egenSubC.threshDiag1P1.FLNK 2304 1216 2592 1216 2592 1248 2816 1248 egenSubC.threshDiag2P1.SLNK
w 1746 1963 100 0 n#151 esirs.aoThresh.FLNK 1696 1952 1856 1952 1856 1248 2016 1248 egenSubC.threshDiag1P1.SLNK
s -832 2224 500 0 PWFS1 - WFS Signal Processing Status Records
s 2464 -704 500 512 wfsSigSad.sch
[cell use]
use esirs 672 -345 100 0 fgFocusGain100
xform 0 880 -192
p 736 -384 100 0 1 DESC:FG Focus Gain at 100Hz
p 608 -544 100 0 0 EGU:units
p 608 -608 100 0 0 FDSC:FG Focus Gain at 100Hz
p 736 -416 100 0 1 FTVL:DOUBLE
p 736 -448 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 -345 100 0 cfTipTiltBw
xform 0 272 -192
p 128 -384 100 0 1 DESC:Cutoff Freq. of the tip tilt BW filter
p 0 -544 100 0 0 EGU:units
p 0 -608 100 0 0 FDSC:Cutoff Freq. of the tip tilt BW filter
p 128 -416 100 0 1 FTVL:DOUBLE
p 128 -448 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 -345 100 0 cfFocusBw
xform 0 -336 -192
p -480 -384 100 0 1 DESC:Cutoff Freq. of the focus BW filter
p -608 -544 100 0 0 EGU:units
p -608 -608 100 0 0 FDSC:Cutoff Freq. of the focus BW filter
p -480 -416 100 0 1 FTVL:DOUBLE
p -480 -448 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 167 100 0 fgFocusGain
xform 0 1488 320
p 1344 128 100 0 1 DESC:FG Focus Gain
p 1216 -32 100 0 0 EGU:units
p 1216 -96 100 0 0 FDSC:FG Focus Gain
p 1344 96 100 0 1 FTVL:DOUBLE
p 1344 64 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 167 100 0 fgTiltGain
xform 0 880 320
p 736 128 100 0 1 DESC:FG Tilt Gain
p 608 -32 100 0 0 EGU:units
p 608 -96 100 0 0 FDSC:FG Tilt Gain
p 736 96 100 0 1 FTVL:DOUBLE
p 736 64 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 167 100 0 fgTipGain
xform 0 272 320
p 128 128 100 0 1 DESC:FG Tip Gain
p 0 -32 100 0 0 EGU:units
p 0 -96 100 0 0 FDSC:FG Tip Gain
p 128 96 100 0 1 FTVL:DOUBLE
p 128 64 100 0 1 PV:$(sadtop)$(wfs)
use egenSubC 2016 1159 100 0 threshDiag1P1
xform 0 2160 1584
p 2080 1120 100 0 1 DESC:Display threshold / sub-apertures for P1
p 2336 1792 100 0 0 FTF:DOUBLE
p 2336 1952 100 0 1 FTVA:DOUBLE
p 2336 1920 100 0 1 FTVB:DOUBLE
p 2336 1888 100 0 1 FTVC:DOUBLE
p 2336 1856 100 0 1 FTVD:DOUBLE
p 2336 1824 100 0 1 FTVE:DOUBLE
p 2336 1792 100 0 1 FTVF:DOUBLE
p 2336 1760 100 0 1 FTVG:DOUBLE
p 2336 1728 100 0 1 FTVH:DOUBLE
p 2336 1696 100 0 1 FTVI:DOUBLE
p 2336 1664 100 0 1 FTVJ:DOUBLE
p 2336 1632 100 0 1 FTVK:DOUBLE
p 2336 1600 100 0 1 FTVL:DOUBLE
p 2336 1568 100 0 1 FTVM:DOUBLE
p 2336 1536 100 0 1 FTVN:DOUBLE
p 2336 1504 100 0 1 FTVO:DOUBLE
p 2336 1472 100 0 1 FTVP:DOUBLE
p 2336 1440 100 0 1 FTVQ:DOUBLE
p 2336 1408 100 0 1 FTVR:DOUBLE
p 2336 1376 100 0 1 FTVS:DOUBLE
p 2336 1344 100 0 1 FTVT:DOUBLE
p 2336 1312 100 0 1 FTVU:DOUBLE
p 2080 1024 100 0 1 PV:$(top)$(wfs)
p 2080 1088 100 0 1 SCAN:Passive
p 2080 1056 100 0 1 SNAM:showThreshDiag1P1
use egenSubC 2816 1159 100 0 threshDiag2P1
xform 0 2960 1584
p 2880 1120 100 0 1 DESC:Display threshold / sub-apertures for P1
p 3136 1792 100 0 0 FTF:DOUBLE
p 3136 1952 100 0 1 FTVA:DOUBLE
p 3136 1920 100 0 1 FTVB:DOUBLE
p 3136 1888 100 0 1 FTVC:DOUBLE
p 3136 1856 100 0 1 FTVD:DOUBLE
p 3136 1824 100 0 1 FTVE:DOUBLE
p 3136 1792 100 0 1 FTVF:DOUBLE
p 3136 1760 100 0 1 FTVG:DOUBLE
p 3136 1728 100 0 1 FTVH:DOUBLE
p 3136 1696 100 0 1 FTVI:DOUBLE
p 3136 1664 100 0 1 FTVJ:DOUBLE
p 3136 1632 100 0 1 FTVK:DOUBLE
p 3136 1600 100 0 1 FTVL:DOUBLE
p 3136 1568 100 0 1 FTVM:DOUBLE
p 3136 1536 100 0 1 FTVN:DOUBLE
p 3136 1504 100 0 1 FTVO:DOUBLE
p 3136 1472 100 0 0 FTVP:DOUBLE
p 3136 1440 100 0 0 FTVQ:DOUBLE
p 3136 1408 100 0 0 FTVR:DOUBLE
p 3136 1376 100 0 0 FTVS:DOUBLE
p 3136 1344 100 0 0 FTVT:DOUBLE
p 3136 1312 100 0 0 FTVU:DOUBLE
p 2880 1024 100 0 1 PV:$(top)$(wfs)
p 2880 1088 100 0 1 SCAN:Passive
p 2880 1056 100 0 1 SNAM:showThreshDiag2P1
use esirs -544 167 100 0 aoRms
xform 0 -336 320
p -480 128 100 0 1 DESC:RMS
p -608 -32 100 0 0 EGU:units
p -608 -96 100 0 0 FDSC:Threshold for total counts
p -480 96 100 0 1 FTVL:DOUBLE
p -480 64 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 1191 100 0 aoIntMatInit
xform 0 1488 1344
p 1344 1152 100 0 1 DESC:aO interaction matrix Init
p 1216 928 100 0 0 FDSC:Interaction matrix init
p 1344 1120 100 0 1 FTVL:STRING
p 1344 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 1191 100 0 aoContMatInit
xform 0 880 1344
p 736 1152 100 0 1 DESC:aO control matrix Init
p 608 928 100 0 0 FDSC:Control matrix init
p 736 1120 100 0 1 FTVL:STRING
p 736 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 1191 100 0 aoTotal
xform 0 272 1344
p 128 1152 100 0 1 DESC:Threshold for total counts
p 0 992 100 0 0 EGU:units
p 0 928 100 0 0 FDSC:Threshold for total counts
p 128 1120 100 0 1 FTVL:DOUBLE
p 128 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 1191 100 0 aoProcessMode
xform 0 -336 1344
p -480 1152 100 0 1 DESC:Processing mode
p -608 992 100 0 0 EGU:units
p -608 928 100 0 0 FDSC:Processing mode
p -480 1120 100 0 1 FTVL:STRING
p -480 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 1703 100 0 aoCtrlInit
xform 0 -336 1856
p -480 1664 100 0 1 DESC:Signal processing init
p -608 1440 100 0 0 FDSC:Signal processing init
p -480 1632 100 0 1 FTVL:STRING
p -480 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 1703 100 0 aoFlatInit
xform 0 880 1856
p 736 1664 100 0 1 DESC:Flat init
p 608 1440 100 0 0 FDSC:Flat init
p 736 1632 100 0 1 FTVL:STRING
p 736 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 1703 100 0 aoDarkInit
xform 0 272 1856
p 128 1664 100 0 1 DESC:Dark Init
p 0 1440 100 0 0 FDSC:Dark init
p 128 1632 100 0 1 FTVL:STRING
p 128 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 1703 100 0 aoThresh
xform 0 1488 1856
p 1344 1664 100 0 1 DESC:Threshold for centroids computation
p 1216 1504 100 0 0 EGU:units
p 1216 1440 100 0 0 FDSC:Threshold for centroids computation
p 1344 1632 100 0 1 FTVL:DOUBLE
p 1344 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 679 100 0 fgContMatInit
xform 0 -336 832
p -480 640 100 0 1 DESC:FG Control matrix Init
p -608 416 100 0 0 FDSC:Control matrix init
p -480 608 100 0 1 FTVL:STRING
p -480 576 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 679 100 0 aoSaveCbIm
xform 0 272 832
p 128 640 100 0 1 DESC:Save CB Image Flag
p 0 416 100 0 0 FDSC:Save CB Image Flag
p 128 608 100 0 1 FTVL:STRING
p 128 576 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 679 100 0 aoSaveCbAoCtrl
xform 0 880 832
p 736 640 100 0 1 DESC:Save CB aO Control Flag
p 608 416 100 0 0 FDSC:Save CB Control Flag
p 736 608 100 0 1 FTVL:STRING
p 736 576 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 679 100 0 aoSaveCbFgCtrl
xform 0 1488 832
p 1344 640 100 0 1 DESC:Save CB FG Control Flag
p 1216 416 100 0 0 FDSC:Save CB FG Control Flag
p 1344 608 100 0 1 FTVL:STRING
p 1344 576 100 0 1 PV:$(sadtop)$(wfs)
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2002-06-05 04:11:35 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2608 -496 200 0 -1 revision:$Revision: 1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Signal processing SIR Records
[comments]
