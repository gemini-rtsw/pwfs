[schematic2]
uniq 121
[tools]
[detail]
s 2464 -704 500 512 wfsGensub1.sch
s 112 2224 500 0 PWFS2 - WFS more genSub records
[cell use]
use egenSubB -480 1095 100 0 initSigModeSeq
xform 0 -336 1520
p -416 1056 100 0 1 DESC:Init the detSigModeSeq
p -400 1888 100 0 1 FTA:DOUBLE
p -400 1856 100 0 1 FTB:LONG
p -400 1824 100 0 1 FTC:LONG
p -400 1792 100 0 1 FTD:DOUBLE
p -400 1760 100 0 1 FTE:LONG
p -400 1728 100 0 1 FTF:LONG
p -400 1696 100 0 1 FTG:DOUBLE
p -400 1664 100 0 1 FTH:LONG
p -400 1632 100 0 1 FTI:DOUBLE
p -400 1600 100 0 1 FTJ:LONG
p -400 1568 100 0 1 FTK:DOUBLE
p -400 1536 100 0 1 FTL:LONG
p -400 1504 100 0 1 FTM:DOUBLE
p -400 1472 100 0 1 FTN:STRING
p -400 1440 100 0 1 FTO:LONG
p -400 1408 100 0 1 FTP:LONG
p -400 1376 100 0 1 FTQ:LONG
p -400 1344 100 0 1 FTR:DOUBLE
p -400 1312 100 0 1 FTS:DOUBLE
p -400 1280 100 0 1 FTT:LONG
p -128 1888 100 0 1 FTVA:DOUBLE
p -128 1856 100 0 1 FTVB:LONG
p -128 1824 100 0 1 FTVC:LONG
p -128 1792 100 0 1 FTVD:DOUBLE
p -128 1760 100 0 1 FTVE:LONG
p -128 1728 100 0 1 FTVF:LONG
p -128 1696 100 0 1 FTVG:DOUBLE
p -128 1664 100 0 1 FTVH:LONG
p -128 1632 100 0 1 FTVI:DOUBLE
p -128 1600 100 0 1 FTVJ:LONG
p -128 1568 100 0 1 FTVK:DOUBLE
p -128 1536 100 0 1 FTVL:LONG
p -128 1504 100 0 1 FTVM:DOUBLE
p -128 1472 100 0 1 FTVN:STRING
p -128 1440 100 0 1 FTVO:LONG
p -128 1408 100 0 1 FTVP:LONG
p -128 1376 100 0 1 FTVQ:LONG
p -128 1344 100 0 1 FTVR:DOUBLE
p -128 1312 100 0 1 FTVS:DOUBLE
p -128 1280 100 0 1 FTVT:LONG
p -768 1502 100 0 0 INAM:
p -416 896 100 0 1 PINI:NO
p -416 928 100 0 1 PREC:5
p -416 960 100 0 1 PV:$(top)$(wfs)
p -416 1024 100 0 1 SCAN:Passive
p -416 992 100 0 1 SNAM:detInitSigModeSeq
p 32 1888 100 0 1 def(OUTA):$(top)$(wfs)detSigModeSeq.A
p 32 1856 100 0 1 def(OUTB):$(top)$(wfs)detSigModeSeq.B
p 32 1824 100 0 1 def(OUTC):$(top)$(wfs)detSigModeSeq.C
p 32 1792 100 0 1 def(OUTD):$(top)$(wfs)detSigModeSeq.D
p 32 1760 100 0 1 def(OUTE):$(top)$(wfs)detSigModeSeq.E
p 32 1728 100 0 1 def(OUTF):$(top)$(wfs)detSigModeSeq.F
p 32 1696 100 0 1 def(OUTG):$(top)$(wfs)detSigModeSeq.G
p 32 1664 100 0 1 def(OUTH):$(top)$(wfs)detSigModeSeq.H
p 32 1632 100 0 1 def(OUTI):$(top)$(wfs)detSigModeSeq.I
p 32 1600 100 0 1 def(OUTJ):$(top)$(wfs)detSigModeSeq.J
p 32 1568 100 0 1 def(OUTK):$(top)$(wfs)detSigModeSeq.K
p 32 1536 100 0 1 def(OUTL):$(top)$(wfs)detSigModeSeq.L
p 32 1504 100 0 1 def(OUTM):$(top)$(wfs)detSigModeSeq.M
p 32 1472 100 0 1 def(OUTN):$(top)$(wfs)detSigModeSeq.N
p 32 1440 100 0 1 def(OUTO):$(top)$(wfs)detSigModeSeq.O
p 32 1408 100 0 1 def(OUTP):$(top)$(wfs)detSigModeSeq.P
p 32 1376 100 0 1 def(OUTQ):$(top)$(wfs)detSigModeSeq.Q
p 32 1344 100 0 1 def(OUTR):$(top)$(wfs)detSigModeSeq.R
p 32 1312 100 0 1 def(OUTS):$(top)$(wfs)detSigModeSeq.S
p 32 1280 100 0 1 def(OUTT):$(top)$(wfs)detSigModeSeq.T
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2004-01-08 23:44:05 $
p 2592 2304 200 0 -1 id:$Id: wfsGensub1.sch,v 1.1 2004-01-08 23:44:05 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini PWFS2
p 2592 -528 200 0 -1 revision:$Revision: 1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor more genSub Records
[comments]
