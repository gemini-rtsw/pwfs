[schematic2]
uniq 149
[tools]
[detail]
w -40 331 100 0 n#63 esirs.observing.FLNK -128 384 -64 384 -64 320 32 320 elongouts.obsPut.SLNK
w -72 363 100 0 n#62 esirs.observing.VAL -128 352 32 352 elongouts.obsPut.DOL
w 280 299 100 0 n#61 elongouts.obsPut.OUT 288 288 320 288 hwout.hwout#60.outp
w 296 715 100 0 n#32 estringouts.pushVal.OUT 288 704 352 704 hwout.hwout#37.outp
w 296 939 100 0 n#31 estringouts.pushOmss.OUT 288 928 352 928 hwout.hwout#36.outp
w -110 883 100 0 n#30 esirs.health.OMSS -128 880 -32 880 -32 976 32 976 estringouts.pushOmss.DOL
w -78 955 100 0 n#29 esirs.health.FLNK -128 944 32 944 estringouts.pushOmss.SLNK
w -92 827 100 0 n#28 esirs.health.VAL -128 912 -96 912 -96 752 32 752 estringouts.pushVal.DOL
w 114 1051 100 0 n#27 estringouts.pushOmss.FLNK 288 960 352 960 352 1040 -64 1040 -64 720 32 720 estringouts.pushVal.SLNK
s 416 208 100 0 BUG WORK AROUND: e2sr does not
s 416 176 100 0 translate the PP property of OUT
s 416 144 100 0 properly, so "PP MS" has to be
s 416 112 100 0 included in the string.
s 352 624 100 0 BUG WORK AROUND: e2sr does not
s 352 592 100 0 translate the PP property of OUT
s 352 560 100 0 properly, so "PP MS" has to be
s 352 528 100 0 added after $(hindex).
s 2464 -704 500 512 wfsSad.sch
s 128 2176 500 0 PWFS1 - WFS Status Records
[cell use]
use wfsSigSad -256 1607 100 0 wfsSigSad#148
xform 0 -32 1792
use wfsGensubSad -672 1623 100 0 wfsGensubSad#147
xform 0 -480 1792
use esirs -544 135 100 0 observing
xform 0 -336 288
p -480 96 100 0 1 DESC:WFS observation status
p -480 32 100 0 1 EGU:CAR state
p -608 -128 100 0 0 FDSC:WFS obs status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -480 64 100 0 1 FTVL:LONG
p -320 0 100 0 1 HIGH:2
p -320 -32 100 0 1 HIHI:3
p -480 -32 100 0 1 LOLO:0
p -480 0 100 0 1 LOW:0
p -480 -64 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 695 100 0 health
xform 0 -336 848
p -480 656 100 0 1 DESC:WFS $(wfs) health
p -608 432 100 0 0 FDSC:WFS $(wfs) health
p -480 624 100 0 1 FTVL:STRING
p -480 592 100 0 1 PV:$(sadtop)$(wfs)
use hwout 352 663 100 0 hwout#37
xform 0 448 704
p 448 695 100 0 -1 val(outp):$(hindex) PP MS
use hwout 352 887 100 0 hwout#36
xform 0 448 928
p 448 919 100 0 -1 val(outp):$(mindex)
use hwout 320 247 100 0 hwout#60
xform 0 416 288
p 416 279 100 0 -1 val(outp):$(top)$(wfs)observeC.IVAL PP NMS
use estringouts 32 647 100 0 pushVal
xform 0 160 720
p 96 624 100 0 1 OMSL:closed_loop
p 96 592 100 0 1 PV:$(sadtop)$(wfs)
p 32 752 75 1280 -1 palrm(DOL):MS
p 320 704 75 768 -1 palrm(OUT):MS
p 288 704 75 768 -1 pproc(OUT):PP
use estringouts 32 871 100 0 pushOmss
xform 0 160 944
p 96 848 100 0 1 OMSL:closed_loop
p 96 816 100 0 1 PV:$(sadtop)$(wfs)
p 32 976 75 1280 -1 palrm(DOL):MS
p 320 928 75 768 -1 palrm(OUT):MS
p 288 928 75 768 -1 pproc(OUT):NPP
use wfsDataSad 768 1671 100 0 wfsDataSad#102
xform 0 864 1792
use wfsDetSad 320 1671 100 0 wfsDetSad#101
xform 0 416 1792
use wfsTimSad 1216 1671 100 0 wfsTimSad#100
xform 0 1312 1792
use elongouts 32 231 100 0 obsPut
xform 0 160 320
p -128 462 100 0 0 EGU:CAR state
p 96 208 100 0 1 OMSL:closed_loop
p 96 176 100 0 1 PV:$(sadtop)$(wfs)
p 288 288 75 768 -1 pproc(OUT):PP
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2002-06-05 04:11:35 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2608 -496 200 0 -1 revision:$Revision: 1.7 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
