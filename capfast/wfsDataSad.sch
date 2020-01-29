[schematic2]
uniq 102
[tools]
[detail]
s 2464 -704 500 512 wfsDataSad.sch
s -368 2176 500 0 PWFS2 - Data Header Status Records
[cell use]
use esirs 288 1671 100 0 dataLabel
xform 0 496 1824
p 352 1632 100 0 1 DESC:DHS data label
p 224 1408 100 0 0 FDSC:DHS data label
p 352 1600 100 0 1 FTVL:STRING
p 352 1568 100 0 1 PV:$(sadtop)$(wfs)
use esirs 288 1191 100 0 calLabel
xform 0 496 1344
p 352 1152 100 0 1 DESC:DHS data label for calibration
p 224 928 100 0 0 FDSC:DHS data label for calibration
p 352 1120 100 0 1 FTVL:STRING
p 352 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs -416 1671 100 0 nexpRQ
xform 0 -208 1824
p -352 1632 100 0 1 DESC:Requested exposures per data set
p -480 1408 100 0 0 FDSC:Requested exposures per data set
p -352 1600 100 0 1 FTVL:LONG
p -352 1568 100 0 1 PV:$(sadtop)$(wfs)
use esirs 288 711 100 0 bunit
xform 0 496 864
p 352 672 100 0 1 DESC:Data units
p 224 448 100 0 0 FDSC:Data units
p 352 640 100 0 1 FTVL:STRING
p 352 608 100 0 1 PV:$(sadtop)$(wfs)
p 352 576 100 0 1 VAL:SDSU units
use esirs -416 711 100 0 nframes
xform 0 -208 864
p -352 672 100 0 1 DESC:Frames per data set
p -480 448 100 0 0 FDSC:Frames per data set (-1 for continuous)
p -352 640 100 0 1 FTVL:LONG
p -352 608 100 0 1 PV:$(sadtop)$(wfs)
use esirs -416 1191 100 0 nexp
xform 0 -208 1344
p -352 1152 100 0 1 DESC:Actual exposures per data set
p -480 928 100 0 0 FDSC:Actual exposures per data set
p -352 1120 100 0 1 FTVL:LONG
p -352 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs -416 231 100 0 dhsCon
xform 0 -208 384
p -352 192 100 0 1 DESC:DHS connection status
p -480 -32 100 0 0 FDSC:DHS connection status
p -352 160 100 0 1 FTVL:STRING
p -352 128 100 0 1 PV:$(sadtop)$(wfs)
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2001-09-04 19:52:58 $
p 2576 2320 200 0 -1 id:$Id: wfsDataSad.sch,v 1.3 2001-09-04 19:52:58 cboyer Exp $
p 2704 -784 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2608 -496 200 0 -1 revision:$Revision: 1.3 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Data Header Records
[comments]
