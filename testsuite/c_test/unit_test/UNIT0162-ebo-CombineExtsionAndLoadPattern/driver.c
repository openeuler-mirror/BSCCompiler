#include <stdio.h>

#define hash(seed, v) { \
    printf("%s = %llu\n", #v, v);   \
    hash_1(seed, v);    \
}

unsigned long long int seed = 0;
void hash_1(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

long long int var_0 = -4753688147037785570LL;
signed char var_1 = (signed char)12;
int var_2 = 582759849;
int var_3 = 112743781;
int var_4 = -21454109;
signed char var_5 = (signed char)-41;
long long int var_6 = -578566840357553841LL;
signed char var_7 = (signed char)-73;
unsigned char var_8 = (unsigned char)203;
int var_9 = -2130141792;
int var_10 = 1894190780;
short var_11 = (short)-35;
unsigned char var_12 = (unsigned char)44;
long long int var_13 = 3730067208602679636LL;
_Bool var_14 = (_Bool)1;
int var_15 = -841985453;
_Bool var_16 = (_Bool)0;
unsigned long long int var_17 = 7786212152187387573ULL;
unsigned int var_18 = 2533806422U;
unsigned int var_19 = 915729290U;
int var_20 = 2033526117;
unsigned int var_21 = 2273717440U;
unsigned char var_22 = (unsigned char)170;
signed char var_23 = (signed char)8;
int var_24 = 1955346450;
short var_25 = (short)-30839;
unsigned int var_26 = 1606515912U;
int var_27 = -883034743;
unsigned char var_28 = (unsigned char)186;
_Bool var_29 = (_Bool)1;
int var_30 = 1510442985;
__thread signed char var_31 = (signed char)112;
signed char var_32 = (signed char)86;
long long int var_33 = -8566733383752989456LL;
long long int var_34 = 1952303428297929249LL;
__thread short var_35 = (short)-11476;
long long int var_36 = 7939819935467557543LL;
unsigned char var_37 = (unsigned char)206;
long long int var_38 = -6153924745108393649LL;
int var_39 = -1937906963;
signed char var_40 = (signed char)-65;
long long int var_41 = -6672264869491139150LL;
unsigned long long int var_42 = 7471322697117516607ULL;
unsigned long long int var_43 = 11686845892602254690ULL;
__thread _Bool var_44 = (_Bool)1;
unsigned int var_45 = 1591991068U;
_Bool var_46 = (_Bool)0;
_Bool var_47 = (_Bool)1;
signed char var_48 = (signed char)122;
unsigned char arr_3 [20] [19] ;
signed char arr_4 [20] [19] ;
short arr_5 [20] [19] ;
unsigned long long int arr_6 [20] [19] ;
int arr_8 [20] [19] [25] ;
long long int arr_13 [20] [19] [25] [21] [17] ;
unsigned int arr_22 [18] [16] ;
unsigned long long int arr_23 [18] [16] ;
unsigned int arr_24 [18] [16] [24] ;
unsigned char arr_26 [18] [16] [24] ;
long long int arr_28 [18] [16] [24] [22] ;
unsigned int arr_31 [18] [16] [24] [22] [19] ;
unsigned long long int arr_32 [18] [16] [24] [22] [19] ;
signed char arr_33 [18] [16] [24] [22] [19] ;
unsigned long long int arr_45 [18] [12] ;
unsigned short arr_46 [18] [12] ;
unsigned long long int arr_47 [18] [12] ;
_Bool arr_50 [23] ;
_Bool arr_51 [23] ;
unsigned int arr_54 [23] [19] ;
long long int arr_55 [23] [19] [21] ;
long long int arr_57 [23] [19] [21] ;
unsigned int arr_58 [23] [19] [21] [23] ;
unsigned int arr_59 [23] [19] [21] [23] ;
signed char arr_60 [23] [19] [21] [23] ;
short arr_61 [23] [19] [21] [23] ;
int arr_65 [15] ;
_Bool arr_71 [15] [12] [10] ;
int arr_76 [15] [12] [10] [19] [16] ;
int arr_81 [23] ;
long long int arr_82 [23] [14] ;
signed char arr_88 [23] [14] [19] [21] ;
_Bool arr_89 [23] [14] [19] [21] ;
unsigned char arr_7 [20] [19] ;
unsigned char arr_14 [20] [19] [25] [21] [17] ;
unsigned short arr_15 [20] [19] [25] [21] [17] ;
unsigned int arr_16 [20] [19] ;
int arr_17 [20] [19] ;
unsigned int arr_35 [18] [16] [24] [22] [19] ;
unsigned int arr_36 [18] [16] [24] [22] [19] ;
signed char arr_37 [18] [16] ;
unsigned char arr_38 [18] [16] ;
short arr_39 [18] [16] ;
long long int arr_40 [18] [16] ;
short arr_48 [18] [12] ;
_Bool arr_49 [18] [12] ;
long long int arr_62 [23] [19] [21] [23] ;
short arr_63 [23] [19] [21] [23] ;
unsigned int arr_64 [23] [19] ;
short arr_72 [15] [12] [10] ;
long long int arr_77 [15] [12] [10] [19] [16] ;
signed char arr_78 [15] [12] [10] [19] [16] ;
int arr_79 [15] [12] [10] [19] [16] ;
unsigned short arr_90 [23] [14] [19] [21] ;
unsigned int arr_91 [23] [14] [19] [21] ;
int arr_92 [23] [14] [19] [21] ;
_Bool arr_93 [23] [14] [19] [21] ;
unsigned char arr_94 [23] [14] ;
void init() {
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_3 [i_0] [i_1] = (unsigned char)216;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_4 [i_0] [i_1] = (signed char)-22;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_5 [i_0] [i_1] = (short)10972;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_6 [i_0] [i_1] = 7617672066863936399ULL;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 25; ++i_2) 
                arr_8 [i_0] [i_1] [i_2] = -367424557;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 25; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 17; ++i_4) 
                        arr_13 [i_0] [i_1] [i_2] [i_3] [i_4] = 2461092873145119815LL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_22 [i_0] [i_1] = 3771521687U;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_23 [i_0] [i_1] = 10896997290934869152ULL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                arr_24 [i_0] [i_1] [i_2] = 2689787010U;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                arr_26 [i_0] [i_1] [i_2] = (unsigned char)113;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    arr_28 [i_0] [i_1] [i_2] [i_3] = -136968988483491489LL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 19; ++i_4) 
                        arr_31 [i_0] [i_1] [i_2] [i_3] [i_4] = 1518678833U;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 19; ++i_4) 
                        arr_32 [i_0] [i_1] [i_2] [i_3] [i_4] = 6213611581793329887ULL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 19; ++i_4) 
                        arr_33 [i_0] [i_1] [i_2] [i_3] [i_4] = (signed char)-84;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            arr_45 [i_0] [i_1] = 4958626133280835718ULL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            arr_46 [i_0] [i_1] = (unsigned short)18691;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            arr_47 [i_0] [i_1] = 3544092557254811493ULL;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        arr_50 [i_0] = (_Bool)1;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        arr_51 [i_0] = (_Bool)0;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_54 [i_0] [i_1] = 2997391537U;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                arr_55 [i_0] [i_1] [i_2] = -8050429996972813400LL;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                arr_57 [i_0] [i_1] [i_2] = -2278772028358221440LL;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_58 [i_0] [i_1] [i_2] [i_3] = 1777737235U;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_59 [i_0] [i_1] [i_2] [i_3] = 3648468248U;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_60 [i_0] [i_1] [i_2] [i_3] = (signed char)62;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_61 [i_0] [i_1] [i_2] [i_3] = (short)-14497;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        arr_65 [i_0] = -264616889;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                arr_71 [i_0] [i_1] [i_2] = (_Bool)1;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                for (size_t i_3 = 0; i_3 < 19; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 16; ++i_4) 
                        arr_76 [i_0] [i_1] [i_2] [i_3] [i_4] = -1517806046;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        arr_81 [i_0] = -693943685;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            arr_82 [i_0] [i_1] = -4375922196765858869LL;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_88 [i_0] [i_1] [i_2] [i_3] = (signed char)-76;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_89 [i_0] [i_1] [i_2] [i_3] = (_Bool)1;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_7 [i_0] [i_1] = (unsigned char)158;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 25; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 17; ++i_4) 
                        arr_14 [i_0] [i_1] [i_2] [i_3] [i_4] = (unsigned char)180;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 25; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 17; ++i_4) 
                        arr_15 [i_0] [i_1] [i_2] [i_3] [i_4] = (unsigned short)45045;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_16 [i_0] [i_1] = 963744428U;
    for (size_t i_0 = 0; i_0 < 20; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_17 [i_0] [i_1] = 1057874994;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 19; ++i_4) 
                        arr_35 [i_0] [i_1] [i_2] [i_3] [i_4] = 2135228601U;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 22; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 19; ++i_4) 
                        arr_36 [i_0] [i_1] [i_2] [i_3] [i_4] = 860178157U;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_37 [i_0] [i_1] = (signed char)47;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_38 [i_0] [i_1] = (unsigned char)13;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_39 [i_0] [i_1] = (short)-19337;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 16; ++i_1) 
            arr_40 [i_0] [i_1] = -8406889858776225552LL;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            arr_48 [i_0] [i_1] = (short)15863;
    for (size_t i_0 = 0; i_0 < 18; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            arr_49 [i_0] [i_1] = (_Bool)0;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_62 [i_0] [i_1] [i_2] [i_3] = -689082257326864773LL;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            for (size_t i_2 = 0; i_2 < 21; ++i_2) 
                for (size_t i_3 = 0; i_3 < 23; ++i_3) 
                    arr_63 [i_0] [i_1] [i_2] [i_3] = (short)-30984;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 19; ++i_1) 
            arr_64 [i_0] [i_1] = 3921365277U;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                arr_72 [i_0] [i_1] [i_2] = (short)6828;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                for (size_t i_3 = 0; i_3 < 19; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 16; ++i_4) 
                        arr_77 [i_0] [i_1] [i_2] [i_3] [i_4] = -2280935045040780897LL;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                for (size_t i_3 = 0; i_3 < 19; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 16; ++i_4) 
                        arr_78 [i_0] [i_1] [i_2] [i_3] [i_4] = (signed char)24;
    for (size_t i_0 = 0; i_0 < 15; ++i_0) 
        for (size_t i_1 = 0; i_1 < 12; ++i_1) 
            for (size_t i_2 = 0; i_2 < 10; ++i_2) 
                for (size_t i_3 = 0; i_3 < 19; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 16; ++i_4) 
                        arr_79 [i_0] [i_1] [i_2] [i_3] [i_4] = 66841243;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_90 [i_0] [i_1] [i_2] [i_3] = (unsigned short)10048;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_91 [i_0] [i_1] [i_2] [i_3] = 2813322819U;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_92 [i_0] [i_1] [i_2] [i_3] = -1687817282;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                for (size_t i_3 = 0; i_3 < 21; ++i_3) 
                    arr_93 [i_0] [i_1] [i_2] [i_3] = (_Bool)0;
    for (size_t i_0 = 0; i_0 < 23; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            arr_94 [i_0] [i_1] = (unsigned char)83;
}

void checksum() {
    hash(&seed, var_23);
}
void test(long long int var_0, signed char var_1, int var_2, int var_3, int var_4, signed char var_5, long long int var_6, signed char var_7, unsigned char var_8, int var_9, int var_10, short var_11, unsigned char var_12, long long int var_13, _Bool var_14, int var_15, _Bool var_16, unsigned long long int var_17, unsigned int var_18, unsigned char arr_3 [20] [19] , signed char arr_4 [20] [19] , short arr_5 [20] [19] , unsigned long long int arr_6 [20] [19] , int arr_8 [20] [19] [25] , long long int arr_13 [20] [19] [25] [21] [17] , unsigned int arr_22 [18] [16] , unsigned long long int arr_23 [18] [16] , unsigned int arr_24 [18] [16] [24] , unsigned char arr_26 [18] [16] [24] , long long int arr_28 [18] [16] [24] [22] , unsigned int arr_31 [18] [16] [24] [22] [19] , unsigned long long int arr_32 [18] [16] [24] [22] [19] , signed char arr_33 [18] [16] [24] [22] [19] , unsigned long long int arr_45 [18] [12] , unsigned short arr_46 [18] [12] , unsigned long long int arr_47 [18] [12] , _Bool arr_50 [23] , _Bool arr_51 [23] , unsigned int arr_54 [23] [19] , long long int arr_55 [23] [19] [21] , long long int arr_57 [23] [19] [21] , unsigned int arr_58 [23] [19] [21] [23] , unsigned int arr_59 [23] [19] [21] [23] , signed char arr_60 [23] [19] [21] [23] , short arr_61 [23] [19] [21] [23] , int arr_65 [15] , _Bool arr_71 [15] [12] [10] , int arr_76 [15] [12] [10] [19] [16] , int arr_81 [23] , long long int arr_82 [23] [14] , signed char arr_88 [23] [14] [19] [21] , _Bool arr_89 [23] [14] [19] [21] );

int main() {
    init();
    test(var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, arr_3 , arr_4 , arr_5 , arr_6 , arr_8 , arr_13 , arr_22 , arr_23 , arr_24 , arr_26 , arr_28 , arr_31 , arr_32 , arr_33 , arr_45 , arr_46 , arr_47 , arr_50 , arr_51 , arr_54 , arr_55 , arr_57 , arr_58 , arr_59 , arr_60 , arr_61 , arr_65 , arr_71 , arr_76 , arr_81 , arr_82 , arr_88 , arr_89 );
    checksum();
    printf("%llu\n", seed);
}
