[schematic2]
uniq 148
[tools]
[detail]
w 1730 267 100 0 n#147 ecad8.detSigMeasIm.FLNK 1696 256 1824 256 1824 800 1920 800 elongouts.loMeasIm.SLNK
w 1826 843 100 0 n#146 ecad8.detSigMeasIm.VALD 1696 608 1792 608 1792 832 1920 832 elongouts.loMeasIm.DOL
w 642 267 100 0 n#144 ecad8.detSigModeTotal.FLNK 608 256 736 256 736 800 832 800 elongouts.loTotal.SLNK
w 738 843 100 0 n#143 ecad8.detSigModeTotal.VALC 608 672 704 672 704 832 832 832 elongouts.loTotal.DOL
w -350 203 100 0 n#141 ecad8.detSigModeThresh.VALB -480 96 -384 96 -384 192 -256 192 elongouts.loThresh.DOL
w -446 -373 100 0 n#140 ecad8.detSigModeThresh.FLNK -480 -384 -352 -384 -352 160 -256 160 elongouts.loThresh.SLNK
w 994 331 100 0 n#129 eaos.detSigTotalAo.OUT 1088 416 1184 416 1184 320 864 320 864 256 912 256 hwout.hwout#130.outp
w 1538 -453 100 0 n#128 eaos.detSigFluxAo.OUT 1632 -368 1728 -368 1728 -464 1408 -464 1408 -528 1456 -528 hwout.hwout#127.outp
w -94 -309 100 0 n#125 eaos.detSigThreshAo.OUT 0 -224 96 -224 96 -320 -224 -320 -224 -384 -176 -384 hwout.hwout#124.outp
s 3936 -304 500 512 wfsCadMore.sch
[cell use]
use elongouts -256 71 100 0 loThresh
xform 0 -128 160
p -192 32 100 0 1 OMSL:closed_loop
p -192 -32 100 0 1 PV:$(top)$(wfs)
p -192 0 100 0 1 def(OUT):$(top)$(wfs)observe.A
use elongouts 832 711 100 0 loTotal
xform 0 960 800
p 896 672 100 0 1 OMSL:closed_loop
p 896 608 100 0 1 PV:$(top)$(wfs)
p 896 640 100 0 1 def(OUT):$(top)$(wfs)observe.A
use elongouts 1920 711 100 0 loMeasIm
xform 0 2048 800
p 1984 672 100 0 1 OMSL:closed_loop
p 1984 608 100 0 1 PV:$(top)$(wfs)
p 1984 640 100 0 1 def(OUT):$(top)$(wfs)observe.A
use ecad20 -800 551 100 0 detSigInitGain
xform 0 -640 1440
p -736 512 100 0 1 DESC:Init Zernikes Gains
p -688 2080 100 0 1 FTVA:DOUBLE
p -688 2016 100 0 1 FTVB:DOUBLE
p -688 1952 100 0 1 FTVC:DOUBLE
p -688 1888 100 0 1 FTVD:DOUBLE
p -688 1824 100 0 1 FTVE:DOUBLE
p -688 1760 100 0 1 FTVF:DOUBLE
p -688 1696 100 0 1 FTVG:DOUBLE
p -688 1632 100 0 1 FTVH:DOUBLE
p -688 1568 100 0 1 FTVI:DOUBLE
p -688 1504 100 0 1 FTVJ:DOUBLE
p -688 1440 100 0 1 FTVK:DOUBLE
p -688 1376 100 0 1 FTVL:DOUBLE
p -688 1312 100 0 1 FTVM:DOUBLE
p -688 1248 100 0 1 FTVN:DOUBLE
p -688 1184 100 0 1 FTVO:DOUBLE
p -688 1120 100 0 1 FTVP:DOUBLE
p -688 1056 100 0 1 FTVQ:DOUBLE
p -688 992 100 0 1 FTVR:DOUBLE
p -688 928 100 0 1 FTVS:DOUBLE
p -688 864 100 0 0 FTVT:STRING
p -736 480 100 0 1 INAM:epToVxCadInit
p -736 416 100 0 1 PV:$(top)$(wfs)
p -736 448 100 0 1 SNAM:epToVxCadExecute
use ecad20 -256 551 100 0 detSigModeSeq
xform 0 -96 1440
p -192 480 100 0 1 DESC:Sequence closed loop mode
p -160 2016 100 0 1 FTVA:DOUBLE
p -160 1984 100 0 1 FTVB:LONG
p -160 1952 100 0 1 FTVC:LONG
p -160 1920 100 0 1 FTVD:DOUBLE
p -160 1888 100 0 1 FTVE:LONG
p -160 1856 100 0 1 FTVF:LONG
p -160 1824 100 0 1 FTVG:LONG
p -160 1792 100 0 1 FTVH:DOUBLE
p -160 1760 100 0 1 FTVI:LONG
p -160 1728 100 0 1 FTVJ:DOUBLE
p -192 448 100 0 1 INAM:epToVxCadInit
p -192 512 100 0 1 PREC:1
p -96 512 100 0 1 PV:$(top)$(wfs)
p -192 416 100 0 1 SNAM:epToVxCadExecute
use ecad8 1376 39 100 0 detSigMeasIm
xform 0 1536 544
p 1472 -32 100 0 1 DESC:Measure col int matrix
p 1488 768 100 0 1 FTVA:LONG
p 1488 736 100 0 1 FTVB:LONG
p 1488 688 100 0 1 FTVC:DOUBLE
p 1488 640 100 0 1 FTVD:LONG
p 1488 592 100 0 1 FTVE:STRING
p 1488 560 100 0 1 FTVF:STRING
p 1488 528 100 0 1 FTVG:STRING
p 1472 -64 100 0 1 INAM:epToVxCadInit
p 1472 0 100 0 1 PREC:1
p 1600 0 100 0 1 PV:$(top)$(wfs)
p 1472 -96 100 0 1 SNAM:epToVxCadExecute
use ecad8 2464 39 100 0 detSigModeFgFocus
xform 0 2624 544
p 2560 -32 100 0 1 DESC:Set FG and Focus mode
p 2576 800 100 0 1 FTVA:LONG
p 2576 736 100 0 0 FTVB:STRING
p 2576 688 100 0 0 FTVC:STRING
p 2576 640 100 0 0 FTVD:STRING
p 2560 -64 100 0 1 INAM:epToVxCadInit
p 2560 0 100 0 1 PREC:0
p 2560 -128 100 0 1 PV:$(top)$(wfs)
p 2560 -96 100 0 1 SNAM:epToVxCadExecute
use ecad8 1376 1319 100 0 detSigModeAo
xform 0 1536 1824
p 1472 1248 100 0 1 DESC:Set aO mode
p 1488 2080 100 0 1 FTVA:LONG
p 1488 2016 100 0 1 FTVB:LONG
p 1488 1968 100 0 0 FTVC:STRING
p 1488 1920 100 0 0 FTVD:STRING
p 1472 1216 100 0 1 INAM:epToVxCadInit
p 1472 1280 100 0 1 PREC:0
p 1472 1152 100 0 1 PV:$(top)$(wfs)
p 1472 1184 100 0 1 SNAM:epToVxCadExecute
use ecad8 832 1319 100 0 detSigInitCB
xform 0 992 1824
p 928 1248 100 0 1 DESC:Init CB parameters
p 944 2080 100 0 1 FTVA:LONG
p 944 2016 100 0 1 FTVB:LONG
p 944 1968 100 0 1 FTVC:LONG
p 944 1920 100 0 0 FTVD:STRING
p 928 1216 100 0 1 INAM:epToVxCadInit
p 928 1280 100 0 1 PREC:0
p 928 1152 100 0 1 PV:$(top)$(wfs)
p 928 1184 100 0 1 SNAM:epToVxCadExecute
use ecad8 288 39 100 0 detSigModeTotal
xform 0 448 544
p 384 -32 100 0 1 DESC:Set average flux computation mode
p 400 800 100 0 1 FTVA:LONG
p 400 736 100 0 1 FTVB:DOUBLE
p 400 688 100 0 1 FTVC:LONG
p 400 640 100 0 1 FTVD:DOUBLE
p 384 608 100 0 0 FTVE:STRING
p 384 -64 100 0 1 INAM:epToVxCadInit
p 384 0 100 0 1 PREC:1
p 384 -128 100 0 1 PV:$(top)$(wfs)
p 384 -96 100 0 1 SNAM:epToVxCadExecute
use ecad8 1952 1319 100 0 detSigInitFGGain
xform 0 2112 1824
p 2048 1248 100 0 1 DESC:Update FG gains
p 2064 2080 100 0 1 FTVA:DOUBLE
p 2064 2016 100 0 1 FTVB:DOUBLE
p 2064 1968 100 0 1 FTVC:DOUBLE
p 2064 1920 100 0 1 FTVD:DOUBLE
p 2048 1216 100 0 1 INAM:epToVxCadInit
p 2048 1152 100 0 1 PV:$(top)$(wfs)
p 2048 1184 100 0 1 SNAM:epToVxCadExecute
use ecad8 -800 -601 100 0 detSigModeThresh
xform 0 -640 -96
p -704 -672 100 0 1 DESC:Set compute threshold mode
p -688 160 100 0 1 FTVA:LONG
p -688 96 100 0 1 FTVB:LONG
p -688 48 100 0 1 FTVC:DOUBLE
p -688 0 100 0 1 FTVD:DOUBLE
p -704 -32 100 0 1 FTVE:DOUBLE
p -704 -704 100 0 1 INAM:epToVxCadInit
p -704 -640 100 0 1 PREC:4
p -704 -768 100 0 1 PV:$(top)$(wfs)
p -704 -736 100 0 1 SNAM:epToVxCadExecute
use ecad8 3552 39 100 0 detSigModeFgFocusAo
xform 0 3712 544
p 3648 -32 100 0 1 DESC:Set FG, Focus and aO mode
p 3664 800 100 0 1 FTVA:LONG
p 3664 736 100 0 1 FTVB:LONG
p 3664 688 100 0 0 FTVC:STRING
p 3664 640 100 0 0 FTVD:STRING
p 3648 -64 100 0 1 INAM:epToVxCadInit
p 3648 0 100 0 1 PREC:0
p 3648 -128 100 0 1 PV:$(top)$(wfs)
p 3648 -96 100 0 1 SNAM:epToVxCadExecute
use ecad8 3024 39 100 0 detSigModeGgAo
xform 0 3184 544
p 3120 -32 100 0 1 DESC:Set Global Guide and aO mode
p 3136 800 100 0 1 FTVA:LONG
p 3136 736 100 0 1 FTVB:LONG
p 3136 688 100 0 0 FTVC:STRING
p 3136 640 100 0 0 FTVD:STRING
p 3120 -64 100 0 1 INAM:epToVxCadInit
p 3120 0 100 0 1 PREC:0
p 3120 -128 100 0 1 PV:$(top)$(wfs)
p 3120 -96 100 0 1 SNAM:epToVxCadExecute
use ecad8 288 1319 100 0 detSigCompMat
xform 0 448 1824
p 384 1248 100 0 1 DESC:Compute and save matrixes
p 400 2080 100 0 1 FTVA:STRING
p 400 2016 100 0 1 FTVB:STRING
p 400 1968 100 0 1 FTVC:STRING
p 400 1920 100 0 0 FTVD:STRING
p 384 1216 100 0 1 INAM:epToVxCadInit
p 384 1152 100 0 1 PV:$(top)$(wfs)
p 384 1184 100 0 1 SNAM:epToVxCadExecute
use eaos 832 359 100 0 detSigTotalAo
xform 0 960 448
p 896 528 100 0 1 PV:$(top)$(wfs)
use eaos 1376 -425 100 0 detSigFluxAo
xform 0 1504 -336
p 1440 -256 100 0 1 PV:$(top)$(wfs)
use eaos -256 -281 100 0 detSigThreshAo
xform 0 -128 -192
p -192 -112 100 0 1 PV:$(top)$(wfs)
use hwout 912 215 100 0 hwout#130
xform 0 1008 256
p 1008 247 100 0 -1 val(outp):$(top)$(wfs)detSigModeTotal.D
use hwout 1456 -569 100 0 hwout#127
xform 0 1552 -528
p 1552 -537 100 0 -1 val(outp):$(top)$(wfs)detSigModeSeq.D
use hwout -176 -425 100 0 hwout#124
xform 0 -80 -384
p -80 -393 100 0 -1 val(outp):$(top)$(wfs)detSigModeThresh.C
use ecad2 3008 1319 100 0 detSigModeNone
xform 0 3168 1632
p 3088 1248 100 0 1 DESC:Set no processing mode
p 3088 1216 100 0 1 INAM:epToVxCadInit
p 3088 1152 100 0 1 PV:$(top)$(wfs)
p 3088 1184 100 0 1 SNAM:epToVxCadExecute
use ecad2 3552 1319 100 0 detSigModeDark
xform 0 3712 1632
p 3632 1248 100 0 1 DESC:Set dark subtraction mode
p 3632 1216 100 0 1 INAM:epToVxCadInit
p 3616 1152 100 0 1 PV:$(top)$(wfs)
p 3632 1184 100 0 1 SNAM:epToVxCadExecute
use ecad2 2464 1319 100 0 detSigModeGg
xform 0 2624 1632
p 2544 1248 100 0 1 DESC:Set Global Guide mode
p 2544 1216 100 0 1 INAM:epToVxCadInit
p 2544 1152 100 0 1 PV:$(top)$(wfs)
p 2544 1184 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-06-21 01:27:43 $
p 1888 -432 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2592 -528 200 0 -1 revision:$Revision: 1.5 $
p 3120 -560 200 0 -1 title:Wavefront Sensor CAD Records
[comments]
