[schematic2]
uniq 142
[tools]
[detail]
w 3378 1227 100 0 n#134 egenSubC.aoE.FLNK 3328 1216 3488 1216 3488 1248 3584 1248 egenSubC.aoZ.SLNK
w 2786 1227 100 0 n#133 egenSub.ao.FLNK 2688 1216 2944 1216 2944 1248 3040 1248 egenSubC.aoE.SLNK
w 3154 2059 100 0 n#131 egenSub.ao.OUTA 2688 1920 2880 1920 2880 2048 3488 2048 3488 1952 3584 1952 egenSubC.aoZ.A
w 2786 1867 100 0 n#130 egenSub.ao.OUTB 2688 1856 2944 1856 2944 1952 3040 1952 egenSubC.aoE.A
w -168 187 100 0 n#63 esirs.observing.FLNK -256 240 -192 240 -192 176 -96 176 elongouts.obsPut.SLNK
w -200 219 100 0 n#62 esirs.observing.VAL -256 208 -96 208 elongouts.obsPut.DOL
w 152 155 100 0 n#61 elongouts.obsPut.OUT 160 144 192 144 hwout.hwout#60.outp
w 168 571 100 0 n#32 estringouts.pushVal.OUT 160 560 224 560 hwout.hwout#37.outp
w 168 795 100 0 n#31 estringouts.pushOmss.OUT 160 784 224 784 hwout.hwout#36.outp
w -238 739 100 0 n#30 esirs.health.OMSS -256 736 -160 736 -160 832 -96 832 estringouts.pushOmss.DOL
w -206 811 100 0 n#29 esirs.health.FLNK -256 800 -96 800 estringouts.pushOmss.SLNK
w -220 683 100 0 n#28 esirs.health.VAL -256 768 -224 768 -224 608 -96 608 estringouts.pushVal.DOL
w -14 907 100 0 n#27 estringouts.pushOmss.FLNK 160 816 224 816 224 896 -192 896 -192 576 -96 576 estringouts.pushVal.SLNK
s 128 2176 500 0 PWFS2 - WFS Status Records
s 2464 -704 500 512 wfsSad.sch
s 224 384 100 0 added after $(hindex).
s 224 416 100 0 properly, so "PP MS" has to be
s 224 448 100 0 translate the PP property of OUT
s 224 480 100 0 BUG WORK AROUND: e2sr does not
s 576 752 100 0 These "stringout" records update the "genSub" record combHlt,
s 576 720 100 0 which determines the overall health of the instrument.
s 576 688 100 0 They are necessary to convert the value fields of the SIR record
s 576 656 100 0 into link fields, which can use a channel access put.
s -208 960 100 0 Variables "mindex" and "hindex" direct the output to the
s -208 928 100 0 approriate "genSub" record fields for this component.
s 288 -32 100 0 included in the string.
s 288 0 100 0 properly, so "PP MS" has to be
s 288 32 100 0 translate the PP property of OUT
s 288 64 100 0 BUG WORK AROUND: e2sr does not
[cell use]
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
use egenSubC 3584 1159 100 0 aoZ
xform 0 3728 1584
p 3648 1104 100 0 1 DESC:Display AO Zernike values
p 3648 1072 100 0 1 INAM:
p 3648 944 100 0 1 NOA:19
p 3296 1710 100 0 0 PREC:4
p 3648 1008 100 0 1 PV:$(top)$(wfs)
p 3648 976 100 0 1 SCAN:Passive
p 3648 1040 100 0 1 SNAM:gensubFanDoubles
use egenSubC 3040 1159 100 0 aoE
xform 0 3184 1584
p 3104 1104 100 0 1 DESC:Display AO Error values
p 3104 1072 100 0 1 INAM:
p 3104 944 100 0 1 NOA:19
p 2752 1710 100 0 0 PREC:4
p 3104 1008 100 0 1 PV:$(top)$(wfs)
p 3104 976 100 0 1 SCAN:Passive
p 3104 1040 100 0 1 SNAM:gensubFanDoubles
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
use egenSub 1824 1159 100 0 ttf
xform 0 1968 1584
p 1888 1104 100 0 1 DESC:Time averaged T-T-F data
p 1904 1424 100 0 1 FTJ:DOUBLE
p 1904 1392 100 0 1 FTVJ:DOUBLE
p 1888 1072 100 0 1 INAM:gensubToTcsInit
p 1904 1360 100 0 1 NOJ:8
p 1904 1328 100 0 1 NOVJ:8
p 1888 944 100 0 1 PREC:9
p 1888 1008 100 0 1 PV:$(sadtop)$(wfs)
p 1888 976 100 0 1 SCAN:.1 second
p 1888 1040 100 0 1 SNAM:gensubToTcsTtf
p 1776 1930 75 0 -1 pproc(INPA):NPP
p 2112 1354 75 0 -1 pproc(OUTJ):NPP
use egenSub 2400 1159 100 0 ao
xform 0 2544 1584
p 2464 1104 100 0 1 DESC:Active optics data
p 2480 1424 100 0 1 FTJ:DOUBLE
p 2480 1936 100 0 1 FTVA:DOUBLE
p 2480 1856 100 0 1 FTVB:DOUBLE
p 2480 1392 100 0 1 FTVJ:DOUBLE
p 2464 1072 100 0 1 INAM:gensubToTcsInit
p 2480 1360 100 0 1 NOJ:40
p 2480 1904 100 0 1 NOVA:19
p 2480 1824 100 0 1 NOVB:19
p 2480 1328 100 0 1 NOVJ:40
p 2112 1710 100 0 0 PREC:2
p 2464 1008 100 0 1 PV:$(sadtop)$(wfs)
p 2464 976 100 0 1 SCAN:.1 second
p 2464 1040 100 0 1 SNAM:gensubToTcsAo
p 2688 1354 75 0 -1 pproc(OUTJ):NPP
use wfsDataSad 2960 423 100 0 wfsDataSad#102
xform 0 3056 544
use wfsDetSad 2512 423 100 0 wfsDetSad#101
xform 0 2608 544
use wfsTimSad 3408 423 100 0 wfsTimSad#100
xform 0 3504 544
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
p 3120 -784 200 0 -1 date:$Date: 2000-07-10 21:47:06 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2608 -496 200 0 -1 revision:$Revision: 1.3 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
