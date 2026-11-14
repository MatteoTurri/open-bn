/*
 * Option ids of the scene objects and text widgets (SetOption/GetOption): FourCC
 * codes, named after the four characters as they are in memory.
 */

#ifndef OPTID_H
#define OPTID_H

#define OPT_ACTV     0x56544341 /* "ACTV" */
#define OPT_ALP0     0x30504c41 /* "ALP0" */
#define OPT_ALP1     0x31504c41 /* "ALP1" */
#define OPT_ALP2     0x32504c41 /* "ALP2" */
#define OPT_ALP3     0x33504c41 /* "ALP3" */
#define OPT_ALPH     0x48504c41 /* "ALPH" */
#define OPT_ALTG     0x47544c41 /* "ALTG" */
#define OPT_AMB      0x00424d41 /* "AMB" */
#define OPT_AMBS     0x53424d41 /* "AMBS" */
#define OPT_ANTI     0x49544e41 /* "ANTI" */
#define OPT_AP64     0x34365041 /* "AP64" */
#define OPT_BACK     0x4b434142 /* "BACK" */
#define OPT_BBOX     0x584f4242 /* "BBOX" */
#define OPT_BGC4     0x34434742 /* "BGC4" */
#define OPT_BSIZ     0x5a495342 /* "BSIZ" */
#define OPT_CDW      0x20574443 /* "CDW" */
#define OPT_CLID     0x44494c43 /* "CLID" */
#define OPT_CLIP     0x50494c43 /* "CLIP" */
#define OPT_CLUD     0x44554c43 /* "CLUD" */
#define OPT_COL4     0x344c4f43 /* "COL4" */
#define OPT_COLR     0x524c4f43 /* "COLR" */
#define OPT_CONE     0x454e4f43 /* "CONE" */
#define OPT_CPDL     0x4c445043 /* "CPDL" */
#define OPT_CPDM     0x4d445043 /* "CPDM" */
#define OPT_CPDY     0x59445043 /* "CPDY" */
#define OPT_CPOS     0x534f5043 /* "CPOS" */
#define OPT_CPSM     0x4d535043 /* "CPSM" */
#define OPT_CPSX     0x58535043 /* "CPSX" */
#define OPT_CPSY     0x59535043 /* "CPSY" */
#define OPT_CROP     0x504f5243 /* "CROP" */
#define OPT_CRP0     0x30505243 /* "CRP0" */
#define OPT_CRP1     0x31505243 /* "CRP1" */
#define OPT_CRP2     0x32505243 /* "CRP2" */
#define OPT_CRP3     0x33505243 /* "CRP3" */
#define OPT_CRSW     0x57535243 /* "CRSW" */
#define OPT_CSC4     0x34435343 /* "CSC4" */
#define OPT_CSPC     0x43505343 /* "CSPC" */
#define OPT_CX64     0x34365843 /* "CX64" */
#define OPT_CY64     0x34365943 /* "CY64" */
#define OPT_DBG      0x00474244 /* "DBG" */
#define OPT_DCID     0x44494344 /* "DCID" */
#define OPT_DCLD     0x444c4344 /* "DCLD" */
#define OPT_DEBG     0x47424544 /* "DEBG" */
#define OPT_DIFF     0x46464944 /* "DIFF" */
#define OPT_DISP     0x50534944 /* "DISP" */
#define OPT_DIVL     0x4c564944 /* "DIVL" */
#define OPT_DIVU     0x55564944 /* "DIVU" */
#define OPT_DIVV     0x56564944 /* "DIVV" */
#define OPT_DOFS     0x53464f44 /* "DOFS" */
#define OPT_DS64     0x34365344 /* "DS64" */
#define OPT_EDTE     0x45544445 /* "EDTE" */
#define OPT_EMC      0x00434d45 /* "EMC" */
#define OPT_EMC0     0x30434d45 /* "EMC0" */
#define OPT_EMC1     0x31434d45 /* "EMC1" */
#define OPT_EMIS     0x53494d45 /* "EMIS" */
#define OPT_EMT      0x00544d45 /* "EMT" */
#define OPT_FACE     0x45434146 /* "FACE" */
#define OPT_FACT     0x54434146 /* "FACT" */
#define OPT_FADE     0x45444146 /* "FADE" */
#define OPT_FBUF     0x46554246 /* "FBUF" */
#define OPT_FDEG     0x47454446 /* "FDEG" */
#define OPT_FILE     0x454c4946 /* "FILE" */
#define OPT_FLIP     0x50494c46 /* "FLIP" */
#define OPT_FOG      0x00474f46 /* "FOG" */
#define OPT_FOGC     0x43474f46 /* "FOGC" */
#define OPT_FOGR     0x52474f46 /* "FOGR" */
#define OPT_FONT     0x544e4f46 /* "FONT" */
#define OPT_FRAM     0x4d415246 /* "FRAM" */
#define OPT_FSIZ     0x5a495346 /* "FSIZ" */
#define OPT_GMC      0x00434d47 /* "GMC" */
#define OPT_GMT      0x00544d47 /* "GMT" */
#define OPT_GNLN     0x4e4c4e47 /* "GNLN" */
#define OPT_GSSY     0x59535347 /* "GSSY" */
#define OPT_H64      0x20343648 /* "H64" */
#define OPT_HALN     0x4e4c4148 /* "HALN" */
#define OPT_HDIV     0x56494448 /* "HDIV" */
#define OPT_INST     0x54534e49 /* "INST" */
#define OPT_KP64     0x3436504b /* "KP64" */
#define OPT_LAY      0x0059414c /* "LAY" */
#define OPT_LAY0     0x3059414c /* "LAY0" */
#define OPT_LAY1     0x3159414c /* "LAY1" */
#define OPT_LAY2     0x3259414c /* "LAY2" */
#define OPT_LAY3     0x3359414c /* "LAY3" */
#define OPT_LEN      0x204e454c /* "LEN" */
#define OPT_LEVL     0x4c56454c /* "LEVL" */
#define OPT_LIGT     0x5447494c /* "LIGT" */
#define OPT_LIT      0x0054494c /* "LIT" */
#define OPT_LITG     0x4754494c /* "LITG" */
#define OPT_LITS     0x5354494c /* "LITS" */
#define OPT_LM64     0x34364d4c /* "LM64" */
#define OPT_LOOP     0x504f4f4c /* "LOOP" */
#define OPT_LS64     0x3436534c /* "LS64" */
#define OPT_LSPC     0x4350534c /* "LSPC" */
#define OPT_LSPL     0x4c50534c /* "LSPL" */
#define OPT_LSPU     0x5550534c /* "LSPU" */
#define OPT_MDFY     0x5946444d /* "MDFY" */
#define OPT_MGN      0x004e474d /* "MGN" */
#define OPT_MGNT     0x544e474d /* "MGNT" */
#define OPT_MMC      0x00434d4d /* "MMC" */
#define OPT_MMOD     0x444f4d4d /* "MMOD" */
#define OPT_MMT      0x00544d4d /* "MMT" */
#define OPT_MNUM     0x4d554e4d /* "MNUM" */
#define OPT_MOT      0x00544f4d /* "MOT" */
#define OPT_MOTF     0x46544f4d /* "MOTF" */
#define OPT_MOTI     0x49544f4d /* "MOTI" */
#define OPT_MOTL     0x4c544f4d /* "MOTL" */
#define OPT_MOTS     0x53544f4d /* "MOTS" */
#define OPT_MOTT     0x54544f4d /* "MOTT" */
#define OPT_MTIM     0x4d49544d /* "MTIM" */
#define OPT_MTRG     0x4752544d /* "MTRG" */
#define OPT_MWGT     0x5447574d /* "MWGT" */
#define OPT_NDSP     0x5053444e /* "NDSP" */
#define OPT_NEAR     0x5241454e /* "NEAR" */
#define OPT_NSPR     0x5250534e /* "NSPR" */
#define OPT_NUM      0x004d554e /* "NUM" */
#define OPT_OFS      0x2053464f /* "OFS" */
#define OPT_OX64     0x3436584f /* "OX64" */
#define OPT_PAXS     0x53584150 /* "PAXS" */
#define OPT_PCOL     0x4c4f4350 /* "PCOL" */
#define OPT_PERS     0x53524550 /* "PERS" */
#define OPT_PIDX     0x58444950 /* "PIDX" */
#define OPT_PIXT     0x54584950 /* "PIXT" */
#define OPT_PIXW     0x57584950 /* "PIXW" */
#define OPT_PMOD     0x444f4d50 /* "PMOD" */
#define OPT_PNOR     0x524f4e50 /* "PNOR" */
#define OPT_PNUM     0x4d554e50 /* "PNUM" */
#define OPT_POS      0x00534f50 /* "POS" */
#define OPT_POS1     0x31534f50 /* "POS1" */
#define OPT_PPOS     0x534f5050 /* "PPOS" */
#define OPT_PSIZ     0x5a495350 /* "PSIZ" */
#define OPT_PTEX     0x58455450 /* "PTEX" */
#define OPT_PWMD     0x444d5750 /* "PWMD" */
#define OPT_RANG     0x474e4152 /* "RANG" */
#define OPT_RAY      0x20594152 /* "RAY" */
#define OPT_RAYH     0x48594152 /* "RAYH" */
#define OPT_RAYM     0x4d594152 /* "RAYM" */
#define OPT_RAYW     0x57594152 /* "RAYW" */
#define OPT_RAYX     0x58594152 /* "RAYX" */
#define OPT_RAYY     0x59594152 /* "RAYY" */
#define OPT_RECT     0x54434552 /* "RECT" */
#define OPT_REFA     0x41464552 /* "REFA" */
#define OPT_REFS     0x53464552 /* "REFS" */
#define OPT_RFC      0x00434652 /* "RFC" */
#define OPT_RFC0     0x30434652 /* "RFC0" */
#define OPT_RFC1     0x31434652 /* "RFC1" */
#define OPT_RFF      0x00464652 /* "RFF" */
#define OPT_RFF0     0x30464652 /* "RFF0" */
#define OPT_RFF1     0x31464652 /* "RFF1" */
#define OPT_RM64     0x34364d52 /* "RM64" */
#define OPT_ROT      0x00544f52 /* "ROT" */
#define OPT_ROT1     0x31544f52 /* "ROT1" */
#define OPT_ROTL     0x4c544f52 /* "ROTL" */
#define OPT_RSCL     0x4c435352 /* "RSCL" */
#define OPT_RXO      0x204f5852 /* "RXO" */
#define OPT_RYO      0x204f5952 /* "RYO" */
#define OPT_SALP     0x504c4153 /* "SALP" */
#define OPT_SBF      0x20464253 /* "SBF" */
#define OPT_SCAL     0x4c414353 /* "SCAL" */
#define OPT_SCL4     0x344c4353 /* "SCL4" */
#define OPT_SCLY     0x594c4353 /* "SCLY" */
#define OPT_SDU      0x00554453 /* "SDU" */
#define OPT_SDV      0x00564453 /* "SDV" */
#define OPT_SDWE     0x45574453 /* "SDWE" */
#define OPT_SIZE     0x455a4953 /* "SIZE" */
#define OPT_SNOR     0x524f4e53 /* "SNOR" */
#define OPT_SPD0     0x30445053 /* "SPD0" */
#define OPT_SPEC     0x43455053 /* "SPEC" */
#define OPT_SPED     0x44455053 /* "SPED" */
#define OPT_SPHR     0x52485053 /* "SPHR" */
#define OPT_SPHV     0x56485053 /* "SPHV" */
#define OPT_SPOS     0x534f5053 /* "SPOS" */
#define OPT_SRES     0x53455253 /* "SRES" */
#define OPT_SUV      0x00565553 /* "SUV" */
#define OPT_SX64     0x34365853 /* "SX64" */
#define OPT_SY64     0x34365953 /* "SY64" */
#define OPT_SYNC     0x434e5953 /* "SYNC" */
#define OPT_TEXC     0x43584554 /* "TEXC" */
#define OPT_TEXF     0x46584554 /* "TEXF" */
#define OPT_TEXM     0x4d584554 /* "TEXM" */
#define OPT_TH64     0x34364854 /* "TH64" */
#define OPT_TW64     0x34365754 /* "TW64" */
#define OPT_TXC0     0x30435854 /* "TXC0" */
#define OPT_TXC1     0x31435854 /* "TXC1" */
#define OPT_TXC2     0x32435854 /* "TXC2" */
#define OPT_TXC3     0x33435854 /* "TXC3" */
#define OPT_TXF0     0x30465854 /* "TXF0" */
#define OPT_TXF1     0x31465854 /* "TXF1" */
#define OPT_TXF2     0x32465854 /* "TXF2" */
#define OPT_TXF3     0x33465854 /* "TXF3" */
#define OPT_TXM0     0x304d5854 /* "TXM0" */
#define OPT_TXM1     0x314d5854 /* "TXM1" */
#define OPT_TXM2     0x324d5854 /* "TXM2" */
#define OPT_TXM3     0x334d5854 /* "TXM3" */
#define OPT_TYPE     0x45505954 /* "TYPE" */
#define OPT_UTF8     0x38465455 /* "UTF8" */
#define OPT_VALN     0x4e4c4156 /* "VALN" */
#define OPT_VDIV     0x56494456 /* "VDIV" */
#define OPT_W64      0x20343657 /* "W64" */
#define OPT_XOFS     0x53464f58 /* "XOFS" */
#define OPT_YOFS     0x53464f59 /* "YOFS" */
#define OPT_ZBUF     0x4655425a /* "ZBUF" */
#define OPT_ZOFS     0x53464f5a /* "ZOFS" */
#define OPT_ZSRT     0x5452535a /* "ZSRT" */
#define OPT_h64      0x20343668 /* "h64" */
#define OPT_th64     0x34366874 /* "th64" */
#define OPT_tw64     0x34367774 /* "tw64" */
#define OPT_w64      0x20343677 /* "w64" */

/* Values of 'HALN' (left, center, right) and 'VALN' (center, baseline, top, bottom) of the text widgets. */
#define ALIGN_LEFT     0
#define ALIGN_CENTER   1
#define ALIGN_RIGHT    2
#define ALIGN_BASELINE 3
#define ALIGN_TOP      4
#define ALIGN_BOTTOM   5

/* Values of 'FACE' (Prim, Mesh, Model): the faces drawn with culling on; 0 draws every face without culling. */
#define FACE_FRONT 0x1
#define FACE_BACK  0x2

/* 'FADE' (a Z-only pass before the colour pass) and 'ANTI' (an antialiased pass): FADE_ALWAYS/ANTI_ALWAYS runs it
   always, any other nonzero value only while the object is translucent (FADE) or opaque (ANTI). */
#define FADE_ALWAYS 1
#define ANTI_ALWAYS 1

/* Values of 'DISP' of the items of Mesh, Particle and Plane. */
#define DISP_VISIBLE  0x01
#define DISP_MIRROR_U 0x10  /* texture mirrored along u */
#define DISP_MIRROR_V 0x20  /* texture mirrored along v */

/* Values of 'MMOD' (Mesh and Particle items): the morph weight kernel in the low 4 bits, MORPH_CYCLIC makes the
   sequence of targets cyclic; 0 means the weights are set with 'MWGT'. */
#define MORPH_LINEAR        1  /* 2 targets */
#define MORPH_QUAD_BSPLINE  2  /* 3 targets, half-step offset */
#define MORPH_CUBIC_BSPLINE 3  /* 4 targets */
#define MORPH_QUAD_BEZIER   6  /* on pairs of targets */
#define MORPH_CUBIC_BEZIER  7  /* on triples of targets */
#define MORPH_KERNEL_MASK   0xf
#define MORPH_CYCLIC        0x1000

/* item_info::flags of Mesh, Particle and Plane: the item has its own transform ('POS', 'ROT', 'SCAL'). */
#define ITEM_POS       0x1
#define ITEM_ROT       0x2
#define ITEM_SCAL      0x4
#define ITEM_TRANSFORM (ITEM_POS | ITEM_ROT | ITEM_SCAL)

#endif
