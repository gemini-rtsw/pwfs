[schematic2]
uniq 145
[tools]
[detail]
w 3378 1227 100 0 n#134 egenSubC.aoE.FLNK 3328 1216 3488 1216 3488 1248 3584 1248 egenSubC.aoZ.SLNK
w 2786 1227 100 0 n#133 egenSub.ao.FLNK 2688 1216 2944 1216 2944 1248 3040 1248 egenSubC.aoE.SLNK
w 3154 2059 100 0 n#131 egenSub.ao.OUTA 2688 1920 2880 1920 2880 2048 3488 2048 3488 1952 3584 1952 egenSubC.aoZ.A
w 2786 1867 100 0 n#130 egenSub.ao.OUTB 2688 1856 2944 1856 2944 1952 3040 1952 egenSubC.aoE.A
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
use esirs 672 -441 100 0 aoSaveCbFgCtrl
xform 0 880 -288
p 736 -480 100 0 1 DESC:Save CB FG Control Flag
p 608 -704 100 0 0 FDSC:Save CB FG Control Flag
p 736 -512 100 0 1 FTVL:STRING
p 736 -544 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 -441 100 0 aoSaveCbCtrl
xform 0 272 -288
p 128 -480 100 0 1 DESC:Save CB Control Flag
p 0 -704 100 0 0 FDSC:Save CB Control Flag
p 128 -512 100 0 1 FTVL:STRING
p 128 -544 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 -441 100 0 aoSaveCbIm
xform 0 -336 -288
p -480 -480 100 0 1 DESC:Save CB Image Flag
p -608 -704 100 0 0 FDSC:Save CB Image Flag
p -480 -512 100 0 1 FTVL:STRING
p -480 -544 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 679 100 0 aoFgContMatInit
xform 0 1488 832
p 1344 640 100 0 1 DESC:FG Control matrix Init
p 1216 416 100 0 0 FDSC:Control matrix init
p 1344 608 100 0 1 FTVL:STRING
p 1344 576 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 1703 100 0 aoThresh
xform 0 1488 1856
p 1344 1664 100 0 1 DESC:Threshold for centroids computation
p 1216 1504 100 0 0 EGU:units
p 1216 1440 100 0 0 FDSC:Threshold for centroids computation
p 1344 1632 100 0 1 FTVL:DOUBLE
p 1344 1600 100 0 1 PV:$(sadtop)$(wfs)
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
use esirs 64 1703 100 0 aoDarkInit
xform 0 272 1856
p 128 1664 100 0 1 DESC:Dark Init
p 0 1440 100 0 0 FDSC:Dark init
p 128 1632 100 0 1 FTVL:STRING
p 128 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 1703 100 0 aoFlatInit
xform 0 880 1856
p 736 1664 100 0 1 DESC:Flat init
p 608 1440 100 0 0 FDSC:Flat init
p 736 1632 100 0 1 FTVL:STRING
p 736 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 1703 100 0 aoCtrlInit
xform 0 -336 1856
p -480 1664 100 0 1 DESC:Signal processing init
p -608 1440 100 0 0 FDSC:Signal processing init
p -480 1632 100 0 1 FTVL:STRING
p -480 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 1191 100 0 aoProcessMode
xform 0 -336 1344
p -480 1152 100 0 1 DESC:Processing mode
p -608 992 100 0 0 EGU:units
p -608 928 100 0 0 FDSC:Processing mode
p -480 1120 100 0 1 FTVL:STRING
p -480 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs 64 1191 100 0 aoTotal
xform 0 272 1344
p 128 1152 100 0 1 DESC:Threshold for total counts
p 0 992 100 0 0 EGU:units
p 0 928 100 0 0 FDSC:Threshold for total counts
p 128 1120 100 0 1 FTVL:DOUBLE
p 128 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs 672 1191 100 0 aoContMatInit
xform 0 880 1344
p 736 1152 100 0 1 DESC:Control matrix Init
p 608 928 100 0 0 FDSC:Control matrix init
p 736 1120 100 0 1 FTVL:STRING
p 736 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 1191 100 0 aoIntMatInit
xform 0 1488 1344
p 1344 1152 100 0 1 DESC:Interaction matrix Init
p 1216 928 100 0 0 FDSC:Interaction matrix init
p 1344 1120 100 0 1 FTVL:STRING
p 1344 1088 100 0 1 PV:$(sadtop)$(wfs)
use egenSubC 3040 1159 100 0 aoE
xform 0 3184 1584
p 3104 1104 100 0 1 DESC:Display AO Error values
p 3104 1072 100 0 1 INAM:
p 3104 944 100 0 1 NOA:19
p 2752 1710 100 0 0 PREC:4
p 3104 1008 100 0 1 PV:$(top)$(wfs)
p 3104 976 100 0 1 SCAN:Passive
p 3104 1040 100 0 1 SNAM:gensubFanDoubles
use egenSubC 3584 1159 100 0 aoZ
xform 0 3728 1584
p 3648 1104 100 0 1 DESC:Display AO Zernike values
p 3648 1072 100 0 1 INAM:
p 3648 944 100 0 1 NOA:19
p 3296 1710 100 0 0 PREC:4
p 3648 1008 100 0 1 PV:$(top)$(wfs)
p 3648 976 100 0 1 SCAN:Passive
p 3648 1040 100 0 1 SNAM:gensubFanDoubles
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
p 2464 976 100 0 1 SCAN:.5 second
p 2464 1040 100 0 1 SNAM:gensubToTcsAo
p 2688 1354 75 0 -1 pproc(OUTJ):NPP
use egenSub 1824 1159 100 0 ttf
xform 0 1968 1584
p 1888 1104 100 0 1 DESC:Time averaged T-T-F data
p 1904 1424 100 0 1 FTJ:DOUBLE
p 1904 1392 100 0 1 FTVJ:DOUBLE
p 1888 1072 100 0 1 INAM:gensubToTcsInit
p 1904 1360 100 0 1 NOJ:8
p 1904 1328 100 0 1 NOVJ:8
p 1536 1710 100 0 0 PREC:4
p 1888 1008 100 0 1 PV:$(sadtop)$(wfs)
p 1888 976 100 0 1 SCAN:.1 second
p 1888 1040 100 0 1 SNAM:gensubToTcsTtf
p 1776 1930 75 0 -1 pproc(INPA):NPP
p 2112 1354 75 0 -1 pproc(OUTJ):NPP
use wfsDataSad 2960 423 100 0 wfsDataSad#102
xform 0 3056 544
use wfsDetSad 2512 423 100 0 wfsDetSad#101
xform 0 2608 544
use wfsTimSad 3408 423 100 0 wfsTimSad#100
xform 0 3504 544
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
p 3120 -784 200 0 -1 date:$Date: 2000-12-16 03:32:55 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2608 -496 200 0 -1 revision:$Revision: 1.5 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
