[schematic2]
uniq 163
[tools]
[detail]
s 2384 -704 500 512 wfsCadMore1.sch
[cell use]
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
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 0 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-07-12 00:51:42 $
p 1888 -432 200 0 -1 id:
p 2720 -768 100 0 1 modified:C. Boyer
p 3120 -432 200 0 -1 project:Gemini PWFS1
p 2592 -528 200 0 -1 revision:$Revision: 1.2 $
p 3120 -560 200 0 -1 title:Wavefront Sensor CAD Records
[comments]
