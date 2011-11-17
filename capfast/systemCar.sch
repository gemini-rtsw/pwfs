[schematic2]
uniq 177
[tools]
[detail]
w 1930 1483 100 0 n#111 carID.carID#21.FLNK 1504 2048 1696 2048 1696 1472 2224 1472 egenSub.combSystem.SLNK
w 1266 1483 100 0 n#111 carID.carID#63.FLNK 576 1760 896 1760 896 1472 1696 1472 junction
w 1090 971 100 0 n#111 carID.carID#159.FLNK 544 960 1696 960 1696 1472 junction
w 1090 171 100 0 n#111 carID.carID#164.FLNK 544 160 1696 160 1696 960 junction
w 1218 267 100 0 n#176 carID.carID#164.CLID 544 256 1952 256 1952 1696 2224 1696 egenSub.combSystem.INPH
w 258 299 100 0 n#166 elongouts.parkPut.OUT 224 288 352 288 carID.carID#164.IVAL
w 1090 363 100 0 n#166 junction 320 288 320 352 1920 352 1920 1760 2224 1760 egenSub.combSystem.INPG
w -494 555 100 0 n#175 esirs.parking.VAL -608 544 -320 544 -320 352 -32 352 elongouts.parkPut.DOL
w -222 331 100 0 n#174 esirs.parking.FLNK -608 576 -352 576 -352 320 -32 320 elongouts.parkPut.SLNK
w 1186 1067 100 0 n#173 carID.carID#159.CLID 544 1056 1888 1056 1888 1824 2224 1824 egenSub.combSystem.INPF
w 258 1099 100 0 n#157 elongouts.rebootPut.OUT 224 1088 352 1088 carID.carID#159.IVAL
w 1058 1195 100 0 n#157 junction 320 1088 320 1184 1856 1184 1856 1888 2224 1888 egenSub.combSystem.INPE
w -494 1355 100 0 n#172 esirs.rebooting.VAL -608 1344 -320 1344 -320 1152 -32 1152 elongouts.rebootPut.DOL
w -222 1131 100 0 n#171 esirs.rebooting.FLNK -608 1376 -352 1376 -352 1120 -32 1120 elongouts.rebootPut.SLNK
w 1346 1739 100 0 n#152 carID.carID#63.CLID 576 1856 928 1856 928 1728 1824 1728 1824 1952 2224 1952 egenSub.combSystem.INPD
w 290 1899 100 0 n#146 elongouts.measPut.OUT 256 1888 384 1888 carID.carID#63.IVAL
w 1330 1771 100 0 n#146 junction 352 1888 352 1952 960 1952 960 1760 1760 1760 1760 2016 2224 2016 egenSub.combSystem.INPC
w 1850 2091 100 0 n#115 carID.carID#21.CLID 1504 2144 1536 2144 1536 2080 2224 2080 egenSub.combSystem.INPB
w 1866 2155 100 0 n#114 carID.carID#21.IVAL 1312 2176 1280 2176 1280 2240 1568 2240 1568 2144 2224 2144 egenSub.combSystem.INPA
w -520 2155 100 0 n#75 esirs.measuring.VAL -576 2144 -416 2144 ecalcs.measCalc.INPA
w -508 1963 100 0 n#74 esirs.measuring.FLNK -576 2176 -512 2176 -512 1760 -416 1760 ecalcs.measCalc.SLNK
w -88 1963 100 0 n#73 ecalcs.measCalc.VAL -128 1952 0 1952 elongouts.measPut.DOL
w -72 1931 100 0 n#72 ecalcs.measCalc.FLNK -128 1984 -96 1984 -96 1920 0 1920 elongouts.measPut.SLNK
w 2878 2315 100 0 VAL egenSub.combSystem.VALA 2512 2176 2752 2176 2752 2304 3040 2304 outhier.VAL.p
w 2752 1451 100 0 FLNK egenSub.combSystem.FLNK 2512 1440 3040 1440 outhier.FLNK.p
w 2640 2123 100 0 CLID egenSub.combSystem.VALB 2512 2112 2816 2112 2816 1984 3040 1984 outhier.CLID.p
w 2752 2155 100 0 OVAL egenSub.combSystem.OUTA 2512 2144 3040 2144 outhier.OVAL.p
s -256 2560 500 0 PWFS2 - System CAR Records
s 2000 -352 500 512 systemCar.sch
[cell use]
use carID 384 1639 100 0 carID#63
xform 0 480 1792
p 384 1632 100 0 1 set1:car WFS measurement
p 384 1600 100 0 1 set2:pv $(top)measure
use carID 1312 1927 100 0 carID#21
xform 0 1408 2080
p 1312 1920 100 0 1 set1:car Control
p 1312 1888 100 0 1 set2:pv $(top)control
use carID 352 839 100 0 carID#159
xform 0 448 992
p 352 832 100 0 1 set1:car WFS booting
p 352 800 100 0 1 set2:pv $(top)reboot
use carID 352 39 100 0 carID#164
xform 0 448 192
p 352 32 100 0 1 set1:car WFS park
p 352 0 100 0 1 set2:pv $(top)park
use esirs -992 1927 100 0 measuring
xform 0 -784 2080
p -928 1888 100 0 1 DESC:Wavefront measurement status
p -928 1824 100 0 1 EGU:0/1
p -1056 1664 100 0 0 FDSC:Measurement status (0=IDLE;1=BUSY)
p -928 1856 100 0 1 FTVL:LONG
p -768 1792 100 0 1 HIGH:1
p -768 1760 100 0 1 HIHI:1
p -928 1760 100 0 1 LOLO:0
p -928 1792 100 0 1 LOW:0
p -928 1728 100 0 1 PV:$(top)
use esirs -1024 1127 100 0 rebooting
xform 0 -816 1280
p -960 1088 100 0 1 DESC:Rebooting status record
p -960 1024 100 0 1 EGU:0/1
p -1088 864 100 0 0 FDSC:Measurement status (0=IDLE;1=BUSY)
p -960 1056 100 0 1 FTVL:LONG
p -800 992 100 0 1 HIGH:2
p -800 960 100 0 1 HIHI:3
p -960 960 100 0 1 LOLO:0
p -960 992 100 0 1 LOW:0
p -960 928 100 0 1 PV:$(top)
use esirs -1024 327 100 0 parking
xform 0 -816 480
p -960 288 100 0 1 DESC:Park status record
p -1088 192 100 0 0 DISS:NO_ALARM
p -960 224 100 0 1 EGU:0/1
p -1088 64 100 0 0 FDSC:Park status record 
p -960 256 100 0 1 FTVL:LONG
p -800 192 100 0 1 HIGH:2
p -800 160 100 0 1 HIHI:3
p -960 160 100 0 1 LOLO:0
p -960 192 100 0 1 LOW:0
p -960 128 100 0 1 PV:$(top)
use elongouts 0 1831 100 0 measPut
xform 0 128 1920
p -160 2062 100 0 0 EGU:CAR state
p 64 1808 100 0 1 OMSL:closed_loop
p 64 1776 100 0 1 PV:$(top)
p 256 1888 75 768 -1 pproc(OUT):PP
use elongouts -32 1031 100 0 rebootPut
xform 0 96 1120
p -192 1262 100 0 0 EGU:CAR state
p 32 1008 100 0 1 OMSL:closed_loop
p 32 976 100 0 1 PV:$(top)
p 224 1088 75 768 -1 pproc(OUT):PP
use elongouts -32 231 100 0 parkPut
xform 0 96 320
p -192 462 100 0 0 EGU:CAR state
p 32 208 100 0 1 OMSL:closed_loop
p 32 176 100 0 1 PV:$(top)
p 224 288 75 768 -1 pproc(OUT):PP
use egenSub 2224 1383 100 0 combSystem
xform 0 2368 1808
p 2288 1344 100 0 1 DESC:Combine CAR values
p 2096 2176 100 0 1 FTA:LONG
p 2096 2112 100 0 1 FTB:LONG
p 2096 2048 100 0 1 FTC:LONG
p 2096 1984 100 0 1 FTD:LONG
p 2096 1920 100 0 1 FTE:LONG
p 2096 1856 100 0 1 FTF:LONG
p 2096 1792 100 0 1 FTG:LONG
p 2096 1728 100 0 1 FTH:LONG
p 2096 1664 100 0 1 FTI:LONG
p 2096 1600 100 0 1 FTJ:LONG
p 2544 2176 100 0 1 FTVA:LONG
p 2544 2112 100 0 1 FTVB:LONG
p 2544 2048 100 0 1 FTVC:LONG
p 2288 1280 100 0 1 PV:$(top)
p 2288 1312 100 0 1 SNAM:cicsCarValCombine
p 2001 1157 100 0 0 UFC:
use bd200tr -1504 -568 -100 0 frame
xform 0 1136 1136
p 2128 -336 200 0 -1 author:S.M.Beard
p 2640 -368 100 0 0 border:D
p 2128 -416 200 0 1 checked:B.Goodrich
p 2624 -432 200 0 -1 date:$Date: 2000-07-10 21:47:05 $
p 2112 2672 200 0 -1 id:$Id$
p 2640 -80 200 0 -1 project:Gemini PWFS2
p 2128 -160 200 0 -1 revision:$Revision: 1.3 $
p 2640 -208 200 0 -1 title:System CAR Records
use ecalcs -416 1671 100 0 measCalc
xform 0 -272 1936
p -352 1632 100 0 1 CALC:A*2
p -704 1822 100 0 0 EGU:CAR state
p -352 1600 100 0 1 PV:$(top)
use outhier 3008 2263 100 0 VAL
xform 0 3024 2304
use outhier 3008 1399 100 0 FLNK
xform 0 3024 1440
use outhier 3008 1591 100 0 OERR
xform 0 3024 1632
use outhier 3008 1783 100 0 OMSS
xform 0 3024 1824
use outhier 3008 1943 100 0 CLID
xform 0 3024 1984
use outhier 3008 2103 100 0 OVAL
xform 0 3024 2144
[comments]
