PCBNEW-LibModule-V1  нд, 05-лют-2012 17:55:27 +0200
# encoding utf-8
$INDEX
R_10
R_P_4_9
$EndINDEX
$INDEX
R_0805
$EndINDEX
$INDEX
RES0805
$EndINDEX
$MODULE R_0805
Po 0 0 0 15 42806E04 00000000 ~~
Li R_0805
Sc 00000000
AR R0805
Op 0 0 0
At SMD
T0 0 0 250 250 0 50 N V 21 N "R_0805"
T1 0 0 250 250 0 50 N I 21 N "Val*"
DC -650 300 -650 250 50 21
DS -200 300 -600 300 50 21
DS -600 300 -600 -300 50 21
DS -600 -300 -200 -300 50 21
DS 200 -300 600 -300 50 21
DS 600 -300 600 300 50 21
DS 600 300 200 300 50 21
$PAD
Sh "1" R 350 550 0 0 0
Dr 0 0 0
At SMD N 00888000
Ne 0 ""
Po -375 0
$EndPAD
$PAD
Sh "2" R 350 550 0 0 0
Dr 0 0 0
At SMD N 00888000
Ne 0 ""
Po 375 0
$EndPAD
$SHAPE3D
Na "smd/chip_cms.wrl"
Sc 0.100000 0.100000 0.100000
Of 0.000000 0.000000 0.000000
Ro 0.000000 0.000000 0.000000
$EndSHAPE3D
$EndMODULE  R_0805
$MODULE R_10
Po 0 0 0 15 4F2D92EC 00000000 ~~
Li R_10
Sc 00000000
AR
Op 0 0 0
T0 -250 500 600 400 0 100 N V 21 N "R_10"
T1 -250 -500 600 400 0 100 N V 21 N "R***"
DS 2000 0 1250 0 150 21
DS -2000 0 -1250 0 150 21
DS -1250 -750 1250 -750 150 21
DS 1250 -750 1250 750 150 21
DS 1250 750 -1250 750 150 21
DS -1250 750 -1250 -750 150 21
$PAD
Sh "2" C 1000 1000 0 0 0
Dr 400 0 0
At STD N 00E0FFFF
Ne 0 ""
Po 2000 0
$EndPAD
$PAD
Sh "1" C 1000 1000 0 0 0
Dr 400 0 0
At STD N 00E0FFFF
Ne 0 ""
Po -2000 0
$EndPAD
$EndMODULE  R_10
$MODULE R_P_4_9
Po 0 0 0 15 4F2EA619 00000000 ~~
Li R_P_4_9
Sc 00000000
AR 
Op 0 0 0
T0 0 -1250 600 600 0 120 N V 21 N "R_P_4_9"
T1 0 1250 600 600 0 120 N V 21 N "VAL**"
DS -1750 -750 -1750 750 150 21
DS -1750 750 1750 750 150 21
DS 1750 750 1750 -750 150 21
DS 1750 -750 -1750 -750 150 21
$PAD
Sh "1" C 800 800 0 0 0
Dr 200 0 0
At STD N 00E0FFFF
Ne 0 ""
Po -1000 0
$EndPAD
$PAD
Sh "2" C 800 800 0 0 0
Dr 200 0 0
At STD N 00E0FFFF
Ne 0 ""
Po 1000 0
$EndPAD
$PAD
Sh "3" C 800 800 0 0 0
Dr 200 0 0
At STD N 00E0FFFF
Ne 0 ""
Po 0 0
$EndPAD
$EndMODULE  R_P_4_9
$EndLIBRARY
