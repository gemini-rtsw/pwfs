[schematic2]
uniq 163
[tools]
[detail]
w -206 651 100 0 n#162 ecad8.detSigModeFgCoadd.VALB -224 640 -128 640 -128 896 -32 896 elongouts.loFgCoadd.DOL
w 1986 171 100 0 n#161 ecad8.detSigModeGgCoadd.FLNK 1952 160 2080 160 2080 864 2144 864 elongouts.loGgCoadd.SLNK
w 1970 715 100 0 n#160 ecad8.detSigModeGgCoadd.VALA 1952 704 2048 704 2048 896 2144 896 elongouts.loGgCoadd.DOL
w 898 171 100 0 n#157 ecad8.detSigModeCoadd.FLNK 864 160 992 160 992 864 1056 864 elongouts.loCoadd.SLNK
w 882 715 100 0 n#156 ecad8.detSigModeCoadd.VALA 864 704 960 704 960 896 1056 896 elongouts.loCoadd.DOL
w -190 171 100 0 n#153 ecad8.detSigModeFgCoadd.FLNK -224 160 -96 160 -96 864 -32 864 elongouts.loFgCoadd.SLNK
w 530 1995 100 0 n#149 ecalcs.calSeqDark.FLNK 416 1984 704 1984 704 2112 736 2112 elongouts.loSeqDark.SLNK
w 610 2155 100 0 n#147 ecalcs.calSeqDark.VAL 416 1952 544 1952 544 2144 736 2144 elongouts.loSeqDark.DOL
w -126 1419 100 0 n#146 ecad8.detSigModeSeqDark.FLNK -224 1408 32 1408 32 1760 128 1760 ecalcs.calSeqDark.SLNK
w -158 1771 100 0 n#145 ecad8.detSigModeSeqDark.VALD -224 1760 -32 1760 -32 2112 128 2112 ecalcs.calSeqDark.INPB
w -6 2155 100 0 n#143 ecad8.detSigModeSeqDark.VALA -224 1952 -80 1952 -80 2144 128 2144 ecalcs.calSeqDark.INPA
s 2384 -704 500 512 wfsCadMore1.sch
[cell use]
use elongouts 736 2023 100 0 loSeqDark
xform 0 864 2112
p 800 1904 100 0 1 OMSL:closed_loop
p 800 1936 100 0 1 PV:$(top)$(wfs)
p 800 1968 100 0 1 def(OUT):$(top)$(wfs)observe.A
use elongouts -32 775 100 0 loFgCoadd
xform 0 96 864
p 32 656 100 0 1 OMSL:closed_loop
p 32 688 100 0 1 PV:$(top)$(wfs)
p 32 720 100 0 1 def(OUT):$(top)$(wfs)observe.A
use elongouts 1056 775 100 0 loCoadd
xform 0 1184 864
p 1120 656 100 0 1 OMSL:closed_loop
p 1120 688 100 0 1 PV:$(top)$(wfs)
p 1120 720 100 0 1 def(OUT):$(top)$(wfs)observe.A
use elongouts 2144 775 100 0 loGgCoadd
xform 0 2272 864
p 2208 656 100 0 1 OMSL:closed_loop
p 2208 688 100 0 1 PV:$(top)$(wfs)
p 2208 720 100 0 1 def(OUT):$(top)$(wfs)observe.A
use ecad8 -544 -57 100 0 detSigModeFgCoadd
xform 0 -384 448
p -448 -128 100 0 1 DESC:Set FG and Focus and Coadd mode
p -432 688 100 0 1 FTVA:LONG
p -432 640 100 0 1 FTVB:LONG
p -432 592 100 0 1 FTVC:STRING
p -432 544 100 0 1 FTVD:STRING
p -432 496 100 0 0 FTVE:STRING
p -448 -160 100 0 1 INAM:epToVxCadInit
p -448 -96 100 0 1 PREC:0
p -448 -224 100 0 1 PV:$(top)$(wfs)
p -448 -192 100 0 1 SNAM:epToVxCadExecute
use ecad8 -544 1191 100 0 detSigModeSeqDark
xform 0 -384 1696
p -448 1120 100 0 1 DESC:Set sequence dark mode
p -432 1936 100 0 1 FTVA:LONG
p -432 1888 100 0 1 FTVB:STRING
p -432 1840 100 0 1 FTVC:STRING
p -432 1792 100 0 1 FTVD:LONG
p -432 1744 100 0 1 FTVE:DOUBLE
p -448 1088 100 0 1 INAM:epToVxCadInit
p -448 1152 100 0 1 PREC:1
p -448 1024 100 0 1 PV:$(top)$(wfs)
p -448 1056 100 0 1 SNAM:epToVxCadExecute
use ecad8 544 -57 100 0 detSigModeCoadd
xform 0 704 448
p 640 -128 100 0 1 DESC:Set Coadd mode
p 656 688 100 0 1 FTVA:LONG
p 656 640 100 0 1 FTVB:STRING
p 656 592 100 0 1 FTVC:STRING
p 656 544 100 0 0 FTVD:STRING
p 656 496 100 0 0 FTVE:STRING
p 640 -160 100 0 1 INAM:epToVxCadInit
p 640 -96 100 0 1 PREC:0
p 640 -224 100 0 1 PV:$(top)$(wfs)
p 640 -192 100 0 1 SNAM:epToVxCadExecute
use ecad8 1632 -57 100 0 detSigModeGgCoadd
xform 0 1792 448
p 1728 -128 100 0 1 DESC:Set Global Guide and Coadd mode
p 1744 688 100 0 1 FTVA:LONG
p 1744 640 100 0 1 FTVB:STRING
p 1744 592 100 0 1 FTVC:STRING
p 1744 544 100 0 0 FTVD:STRING
p 1744 496 100 0 0 FTVE:STRING
p 1728 -160 100 0 1 INAM:epToVxCadInit
p 1728 -96 100 0 1 PREC:0
p 1728 -224 100 0 1 PV:$(top)$(wfs)
p 1728 -192 100 0 1 SNAM:epToVxCadExecute
use ecalcs 128 1671 100 0 calSeqDark
xform 0 272 1936
p 192 1632 100 0 1 CALC:A+B
p 192 1536 100 0 1 PV:$(top)$(wfs)
p 192 1584 100 0 1 SCAN:Passive
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-06-21 01:27:43 $
p 1888 -432 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2592 -528 200 0 -1 revision:$Revision: 1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor CAD Records
[comments]
