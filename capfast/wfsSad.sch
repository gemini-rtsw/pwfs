[schematic2]
uniq 152
[tools]
[detail]
w -168 187 100 0 n#63 esirs.observing.FLNK -256 240 -192 240 -192 176 -96 176 elongouts.obsPut.SLNK
w -200 219 100 0 n#62 esirs.observing.VAL -256 208 -96 208 elongouts.obsPut.DOL
w 152 155 100 0 n#61 elongouts.obsPut.OUT 160 144 192 144 hwout.hwout#60.outp
w 168 571 100 0 n#32 estringouts.pushVal.OUT 160 560 224 560 hwout.hwout#37.outp
w 168 795 100 0 n#31 estringouts.pushOmss.OUT 160 784 224 784 hwout.hwout#36.outp
w -238 739 100 0 n#30 esirs.health.OMSS -256 736 -160 736 -160 832 -96 832 estringouts.pushOmss.DOL
w -206 811 100 0 n#29 esirs.health.FLNK -256 800 -96 800 estringouts.pushOmss.SLNK
w -220 683 100 0 n#28 esirs.health.VAL -256 768 -224 768 -224 608 -96 608 estringouts.pushVal.DOL
w -14 907 100 0 n#27 estringouts.pushOmss.FLNK 160 816 224 816 224 896 -192 896 -192 576 -96 576 estringouts.pushVal.SLNK
s 128 2176 500 0 PWFS - WFS Status Records
s 2464 -704 500 512 wfsSad.sch
s 224 384 100 0 added after $(hindex).
s 224 416 100 0 properly, so "PP MS" has to be
s 224 448 100 0 translate the PP property of OUT
s 224 480 100 0 BUG WORK AROUND: e2sr does not
s 288 -32 100 0 included in the string.
s 288 0 100 0 properly, so "PP MS" has to be
s 288 32 100 0 translate the PP property of OUT
s 288 64 100 0 BUG WORK AROUND: e2sr does not
[cell use]
use wfsGensubSad -720 1591 100 0 wfsGensubSad#151
xform 0 -512 1760
use wfsSigSad -144 1559 100 0 wfsSigSad#150
xform 0 64 1760
use esirs -672 551 100 0 health
xform 0 -464 704
p -608 512 100 0 1 DESC:WFS $(wfs) health
p -736 288 100 0 0 FDSC:WFS $(wfs) health
p -608 480 100 0 1 FTVL:STRING
p -608 448 100 0 1 PV:$(sadtop)$(wfs)
use esirs -672 -9 100 0 observing
xform 0 -464 144
p -608 -48 100 0 1 DESC:WFS observation status
p -608 -112 100 0 1 EGU:CAR state
p -736 -272 100 0 0 FDSC:WFS obs status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -608 -80 100 0 1 FTVL:LONG
p -448 -144 100 0 1 HIGH:2
p -448 -176 100 0 1 HIHI:3
p -608 -176 100 0 1 LOLO:0
p -608 -144 100 0 1 LOW:0
p -608 -208 100 0 1 PV:$(sadtop)$(wfs)
use hwout 192 103 100 0 hwout#60
xform 0 288 144
p 288 135 100 0 -1 val(outp):$(top)$(wfs)observeC.IVAL PP NMS
use hwout 224 743 100 0 hwout#36
xform 0 320 784
p 320 775 100 0 -1 val(outp):$(mindex)
use hwout 224 519 100 0 hwout#37
xform 0 320 560
p 320 551 100 0 -1 val(outp):$(hindex) PP MS
use estringouts -96 727 100 0 pushOmss
xform 0 32 800
p -32 704 100 0 1 OMSL:closed_loop
p -32 672 100 0 1 PV:$(sadtop)$(wfs)
p -96 832 75 1280 -1 palrm(DOL):MS
p 192 784 75 768 -1 palrm(OUT):MS
p 160 784 75 768 -1 pproc(OUT):NPP
use estringouts -96 503 100 0 pushVal
xform 0 32 576
p -32 480 100 0 1 OMSL:closed_loop
p -32 448 100 0 1 PV:$(sadtop)$(wfs)
p -96 608 75 1280 -1 palrm(DOL):MS
p 192 560 75 768 -1 palrm(OUT):MS
p 160 560 75 768 -1 pproc(OUT):PP
use wfsDataSad 1120 1639 100 0 wfsDataSad#102
xform 0 1216 1760
use wfsDetSad 544 1639 100 0 wfsDetSad#101
xform 0 640 1760
use wfsTimSad 1696 1639 100 0 wfsTimSad#100
xform 0 1792 1760
use elongouts -96 87 100 0 obsPut
xform 0 32 176
p -256 318 100 0 0 EGU:CAR state
p -32 64 100 0 1 OMSL:closed_loop
p -32 32 100 0 1 PV:$(sadtop)$(wfs)
p 160 144 75 768 -1 pproc(OUT):PP
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2001/09/04 19:52:58 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS
p 2608 -496 200 0 -1 revision:$Revision: 1.6 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
