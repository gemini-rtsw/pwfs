[schematic2]
uniq 150
[tools]
[detail]
w 1394 1227 100 0 n#149 egenSubC.aoZ.FLNK 1344 1216 1504 1216 1504 1248 1600 1248 egenSubC.aoTemp.SLNK
w 850 1227 100 0 n#134 egenSubC.aoE.FLNK 800 1216 960 1216 960 1248 1056 1248 egenSubC.aoZ.SLNK
w 258 1227 100 0 n#133 egenSub.ao.FLNK 160 1216 416 1216 416 1248 512 1248 egenSubC.aoE.SLNK
w 626 2059 100 0 n#131 egenSub.ao.OUTA 160 1920 352 1920 352 2048 960 2048 960 1952 1056 1952 egenSubC.aoZ.A
w 258 1867 100 0 n#130 egenSub.ao.OUTB 160 1856 416 1856 416 1952 512 1952 egenSubC.aoE.A
s 2464 -704 500 512 wfsGensubSad.sch
s 128 2176 500 0 PWFS1 - WFS Gensub Status Records
[cell use]
use egenSubC 1600 1159 100 0 aoTemp
xform 0 1744 1584
p 1664 1120 100 0 1 DESC:Display Intermediate values for astig/coma/trefoil
p 1664 1088 100 0 1 INAM:
p 1664 928 100 0 1 PREC:4
p 1312 1662 100 0 0 PRIO:LOW
p 1664 1024 100 0 1 PV:$(sadtop)$(wfs)
p 1664 992 100 0 1 SCAN:Passive
p 1664 1056 100 0 1 SNAM:gensubTempAo
use egenSubC 512 1159 100 0 aoE
xform 0 656 1584
p 576 1104 100 0 1 DESC:Display AO Error values
p 576 1072 100 0 1 INAM:
p 576 944 100 0 1 NOA:19
p 576 912 100 0 1 PREC:4
p 576 1008 100 0 1 PV:$(top)$(wfs)
p 576 976 100 0 1 SCAN:Passive
p 576 1040 100 0 1 SNAM:gensubFanDoubles
use egenSubC 1056 1159 100 0 aoZ
xform 0 1200 1584
p 1120 1104 100 0 1 DESC:Display AO Zernike values
p 1120 1072 100 0 1 INAM:
p 1120 944 100 0 1 NOA:19
p 1120 912 100 0 1 PREC:4
p 1120 1008 100 0 1 PV:$(top)$(wfs)
p 1120 976 100 0 1 SCAN:Passive
p 1120 1040 100 0 1 SNAM:gensubFanDoubles
use egenSub -128 1159 100 0 ao
xform 0 16 1584
p -64 1104 100 0 1 DESC:Active optics data
p -48 1424 100 0 1 FTJ:DOUBLE
p -48 1936 100 0 1 FTVA:DOUBLE
p -48 1856 100 0 1 FTVB:DOUBLE
p -48 1392 100 0 1 FTVJ:DOUBLE
p -64 1072 100 0 1 INAM:gensubToTcsInit
p -48 1360 100 0 1 NOJ:40
p -48 1904 100 0 1 NOVA:19
p -48 1824 100 0 1 NOVB:19
p -48 1328 100 0 1 NOVJ:40
p -64 944 100 0 1 PREC:4
p -64 1008 100 0 1 PV:$(sadtop)$(wfs)
p -64 976 100 0 1 SCAN:.5 second
p -64 1040 100 0 1 SNAM:gensubToTcsAo
p 160 1354 75 0 -1 pproc(OUTJ):NPP
use egenSub -704 1159 100 0 ttf
xform 0 -560 1584
p -640 1104 100 0 1 DESC:Time averaged T-T-F data
p -624 1424 100 0 1 FTJ:DOUBLE
p -624 1392 100 0 1 FTVJ:DOUBLE
p -640 1072 100 0 1 INAM:gensubToTcsInit
p -624 1360 100 0 1 NOJ:8
p -624 1328 100 0 1 NOVJ:8
p -640 944 100 0 1 PREC:4
p -640 1008 100 0 1 PV:$(sadtop)$(wfs)
p -640 976 100 0 1 SCAN:.1 second
p -640 1040 100 0 1 SNAM:gensubToTcsTtf
p -752 1930 75 0 -1 pproc(INPA):NPP
p -416 1354 75 0 -1 pproc(OUTJ):NPP
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2006-04-21 21:26:04 $
p 2576 2320 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2608 -496 200 0 -1 revision:$Revision: 1.2 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Gensub Status Records
[comments]
