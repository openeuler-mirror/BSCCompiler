#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_0 = (signed char)-1;
unsigned short var_1 = (unsigned short)29282;
signed char var_2 = (signed char)13;
short var_3 = (short)8033;
unsigned char var_4 = (unsigned char)116;
signed char var_5 = (signed char)-25;
unsigned long long int var_6 = 3060008527955846234ULL;
unsigned long long int var_7 = 17108457571267521559ULL;
int var_8 = -607052051;
_Bool var_9 = (_Bool)0;
signed char var_10 = (signed char)103;
unsigned char var_11 = (unsigned char)191;
short var_12 = (short)-25646;
signed char var_13 = (signed char)-41;
int var_14 = 1244534841;
unsigned int var_15 = 1542520077U;
unsigned short var_16 = (unsigned short)25835;
unsigned short var_17 = (unsigned short)50387;
unsigned int var_18 = 3870444117U;
int var_19 = -1182623431;
_Bool var_20 = (_Bool)1;
short var_21 = (short)11194;
__thread unsigned short var_22 = (unsigned short)54584;
unsigned char var_23 = (unsigned char)154;
signed char var_24 = (signed char)-50;
int var_25 = -12834007;
__thread long long int var_26 = -4167964093285423711LL;
unsigned short var_27 = (unsigned short)43647;
unsigned int var_28 = 3339695607U;
unsigned long long int var_29 = 16710086995624239903ULL;
_Bool var_30 = (_Bool)0;
unsigned short var_31 = (unsigned short)26030;
long long int var_32 = -228001644254834936LL;
int var_33 = 1649508108;
unsigned long long int var_34 = 12490950785326187617ULL;
__thread _Bool var_35 = (_Bool)1;
_Bool var_36 = (_Bool)0;
_Bool var_37 = (_Bool)1;
unsigned char var_38 = (unsigned char)89;
unsigned char var_39 = (unsigned char)193;
int var_40 = -1599217040;
short var_41 = (short)25532;
_Bool var_42 = (_Bool)0;
unsigned long long int var_43 = 11330365801005871741ULL;
unsigned int var_44 = 4260499067U;
unsigned char var_45 = (unsigned char)214;
unsigned char var_46 = (unsigned char)62;
unsigned char var_47 = (unsigned char)123;
long long int var_48 = -4986808148012098056LL;
unsigned int var_49 = 1502053039U;
unsigned long long int var_50 = 7995749801150870491ULL;
unsigned char var_51 = (unsigned char)107;
unsigned long long int var_52 = 6537723742214157810ULL;
unsigned char var_53 = (unsigned char)137;
short var_54 = (short)27024;
unsigned char var_55 = (unsigned char)180;
unsigned char var_56 = (unsigned char)88;
signed char var_57 = (signed char)68;
unsigned int var_58 = 3171861888U;
__thread signed char var_59 = (signed char)72;
unsigned long long int var_60 = 7259328727577554028ULL;
_Bool arr_0 [14] ;
_Bool arr_5 [14] [14] ;
unsigned char arr_7 [14] [14] [19] ;
long long int arr_8 [14] [14] [19] ;
int arr_9 [14] [14] [19] [24] ;
unsigned long long int arr_10 [14] [14] [19] [24] ;
short arr_11 [14] [14] [19] [17] ;
long long int arr_12 [14] [14] [19] [17] ;
signed char arr_17 [14] [14] [19] [17] [11] ;
unsigned int arr_23 [14] [14] [19] [17] [19] ;
int arr_37 [14] [14] [19] [10] ;
_Bool arr_39 [19] ;
long long int arr_40 [19] ;
signed char arr_43 [19] ;
signed char arr_44 [19] ;
long long int arr_45 [19] [15] ;
signed char arr_46 [19] [15] ;
long long int arr_48 [19] [15] [24] ;
signed char arr_49 [19] [15] [24] ;
int arr_51 [19] [15] [24] [23] ;
unsigned char arr_52 [19] [15] [24] [23] ;
unsigned long long int arr_53 [19] [15] [24] [23] ;
unsigned long long int arr_55 [19] [15] [24] [18] ;
unsigned char arr_56 [19] [15] [24] [18] ;
short arr_57 [19] [15] [24] [18] ;
unsigned short arr_59 [19] [15] [24] [18] ;
int arr_60 [19] [15] [24] [18] ;
signed char arr_61 [19] [15] [24] [18] [25] ;
long long int arr_62 [19] [15] [24] [18] [25] ;
unsigned int arr_64 [19] [15] [24] [18] [12] ;
unsigned short arr_67 [19] [15] [24] [18] [17] ;
unsigned long long int arr_68 [19] [15] [24] [18] [17] ;
short arr_75 [19] [15] [24] [18] [17] ;
short arr_76 [19] [15] [24] [18] [17] ;
unsigned char arr_77 [19] [15] [24] [18] [17] ;
unsigned char arr_78 [19] [15] [24] [18] [17] ;
int arr_80 [19] [15] [10] ;
_Bool arr_88 [19] [12] ;
unsigned long long int arr_89 [19] [12] ;
int arr_91 [19] [12] [20] ;
int arr_92 [19] [12] [20] ;
long long int arr_95 [19] [12] [20] [21] ;
signed char arr_98 [19] [12] [16] ;
unsigned int arr_100 [19] [12] [16] ;
int arr_105 [19] [12] [16] [15] [25] ;
short arr_106 [19] [12] [16] [15] [25] ;
unsigned char arr_112 [19] [12] [16] [15] [25] ;
unsigned char arr_113 [19] [12] [16] [15] [25] ;
long long int arr_118 [19] [12] [16] [15] [16] ;
unsigned int arr_132 [19] [12] [25] ;
unsigned short arr_133 [19] [12] [25] [15] ;
int arr_134 [19] [12] [25] [15] ;
unsigned char arr_136 [19] [12] [25] [15] [11] ;
int arr_137 [19] [12] [25] [15] [11] ;
unsigned short arr_141 [19] [12] [21] ;
unsigned int arr_142 [19] [12] [21] ;
unsigned char arr_148 [10] ;
long long int arr_149 [10] ;
signed char arr_150 [10] ;
_Bool arr_13 [14] [14] [19] [17] ;
unsigned int arr_14 [14] [14] [19] [17] ;
int arr_15 [14] [14] [19] [17] ;
unsigned int arr_16 [14] [14] [19] [17] ;
unsigned long long int arr_21 [14] [14] [19] [17] [11] ;
unsigned long long int arr_22 [14] [14] [19] [17] [11] ;
unsigned int arr_27 [14] [14] [19] [17] [19] ;
_Bool arr_28 [14] [14] [19] [17] [19] ;
int arr_32 [14] [14] [19] [17] [12] ;
signed char arr_33 [14] [14] [19] [17] [12] ;
unsigned short arr_34 [14] [14] [19] [17] [12] ;
unsigned long long int arr_38 [14] [14] [19] ;
unsigned int arr_41 [19] ;
signed char arr_42 [19] ;
signed char arr_50 [19] [15] [24] ;
unsigned char arr_54 [19] [15] [24] [23] ;
int arr_66 [19] [15] [24] [18] [12] ;
_Bool arr_69 [19] [15] [24] [18] [17] ;
short arr_70 [19] [15] [24] [18] ;
unsigned char arr_87 [19] [15] [10] [12] [22] ;
int arr_96 [19] [12] [20] [21] ;
long long int arr_97 [19] [12] [20] [21] ;
unsigned char arr_101 [19] [12] [16] ;
unsigned int arr_107 [19] [12] [16] [15] [25] ;
unsigned long long int arr_108 [19] [12] [16] [15] [25] ;
long long int arr_109 [19] [12] [16] [15] [25] ;
unsigned char arr_110 [19] [12] [16] [15] [25] ;
short arr_111 [19] [12] [16] [15] [25] ;
unsigned char arr_115 [19] [12] [16] [15] [25] ;
unsigned char arr_116 [19] [12] [16] [15] [25] ;
long long int arr_120 [19] [12] [16] [15] [16] ;
unsigned char arr_121 [19] [12] [16] [15] [16] ;
long long int arr_122 [19] [12] [16] [15] [16] ;
unsigned short arr_123 [19] [12] [16] [15] [16] ;
signed char arr_124 [19] [12] [16] [15] [16] ;
unsigned int arr_128 [19] [12] [16] ;
short arr_138 [19] [12] [25] [15] [11] ;
int arr_139 [19] [12] [25] [15] [11] ;
_Bool arr_140 [19] [12] [25] [15] [11] ;
int arr_145 [19] [12] [21] ;
short arr_146 [19] ;
unsigned char arr_151 [10] ;
int arr_152 [10] ;
unsigned short arr_153 [10] ;
void init() {
    for (size_t i_0 = 0; i_0 < 19; ++i_0) 
        for (size_t i_1 = 0; i_1 < 15; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 18; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 25; ++i_4) 
                        arr_62 [i_0] [i_1] [i_2] [i_3] [i_4] = 5388088388012331884LL;
    for (size_t i_0 = 0; i_0 < 19; ++i_0) 
        for (size_t i_1 = 0; i_1 < 15; ++i_1) 
            for (size_t i_2 = 0; i_2 < 24; ++i_2) 
                for (size_t i_3 = 0; i_3 < 18; ++i_3) 
                    for (size_t i_4 = 0; i_4 < 12; ++i_4) 
                        arr_64 [i_0] [i_1] [i_2] [i_3] [i_4] = 421101516U;
}

void checksum() {
    for (size_t i_0 = 0; i_0 < 14; ++i_0) 
        for (size_t i_1 = 0; i_1 < 14; ++i_1) 
            for (size_t i_2 = 0; i_2 < 19; ++i_2) 
                hash(&seed, arr_38 [i_0] [i_1] [i_2] );
}
void test(signed char var_0, unsigned short var_1, signed char var_2, short var_3, unsigned char var_4, signed char var_5, unsigned long long int var_6, unsigned long long int var_7, int var_8, _Bool var_9, _Bool arr_0 [14] , _Bool arr_5 [14] [14] , unsigned char arr_7 [14] [14] [19] , long long int arr_8 [14] [14] [19] , int arr_9 [14] [14] [19] [24] , unsigned long long int arr_10 [14] [14] [19] [24] , short arr_11 [14] [14] [19] [17] , long long int arr_12 [14] [14] [19] [17] , signed char arr_17 [14] [14] [19] [17] [11] , unsigned int arr_23 [14] [14] [19] [17] [19] , int arr_37 [14] [14] [19] [10] , _Bool arr_39 [19] , long long int arr_40 [19] , signed char arr_43 [19] , signed char arr_44 [19] , long long int arr_45 [19] [15] , signed char arr_46 [19] [15] , long long int arr_48 [19] [15] [24] , signed char arr_49 [19] [15] [24] , int arr_51 [19] [15] [24] [23] , unsigned char arr_52 [19] [15] [24] [23] , unsigned long long int arr_53 [19] [15] [24] [23] , unsigned long long int arr_55 [19] [15] [24] [18] , unsigned char arr_56 [19] [15] [24] [18] , short arr_57 [19] [15] [24] [18] , unsigned short arr_59 [19] [15] [24] [18] , int arr_60 [19] [15] [24] [18] , signed char arr_61 [19] [15] [24] [18] [25] , long long int arr_62 [19] [15] [24] [18] [25] , unsigned int arr_64 [19] [15] [24] [18] [12] , unsigned short arr_67 [19] [15] [24] [18] [17] , unsigned long long int arr_68 [19] [15] [24] [18] [17] , short arr_75 [19] [15] [24] [18] [17] , short arr_76 [19] [15] [24] [18] [17] , unsigned char arr_77 [19] [15] [24] [18] [17] , unsigned char arr_78 [19] [15] [24] [18] [17] , int arr_80 [19] [15] [10] , _Bool arr_88 [19] [12] , unsigned long long int arr_89 [19] [12] , int arr_91 [19] [12] [20] , int arr_92 [19] [12] [20] , long long int arr_95 [19] [12] [20] [21] , signed char arr_98 [19] [12] [16] , unsigned int arr_100 [19] [12] [16] , int arr_105 [19] [12] [16] [15] [25] , short arr_106 [19] [12] [16] [15] [25] , unsigned char arr_112 [19] [12] [16] [15] [25] , unsigned char arr_113 [19] [12] [16] [15] [25] , long long int arr_118 [19] [12] [16] [15] [16] , unsigned int arr_132 [19] [12] [25] , unsigned short arr_133 [19] [12] [25] [15] , int arr_134 [19] [12] [25] [15] , unsigned char arr_136 [19] [12] [25] [15] [11] , int arr_137 [19] [12] [25] [15] [11] , unsigned short arr_141 [19] [12] [21] , unsigned int arr_142 [19] [12] [21] , unsigned char arr_148 [10] , long long int arr_149 [10] , signed char arr_150 [10] );

int main() {
    init();
    test(var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, arr_0 , arr_5 , arr_7 , arr_8 , arr_9 , arr_10 , arr_11 , arr_12 , arr_17 , arr_23 , arr_37 , arr_39 , arr_40 , arr_43 , arr_44 , arr_45 , arr_46 , arr_48 , arr_49 , arr_51 , arr_52 , arr_53 , arr_55 , arr_56 , arr_57 , arr_59 , arr_60 , arr_61 , arr_62 , arr_64 , arr_67 , arr_68 , arr_75 , arr_76 , arr_77 , arr_78 , arr_80 , arr_88 , arr_89 , arr_91 , arr_92 , arr_95 , arr_98 , arr_100 , arr_105 , arr_106 , arr_112 , arr_113 , arr_118 , arr_132 , arr_133 , arr_134 , arr_136 , arr_137 , arr_141 , arr_142 , arr_148 , arr_149 , arr_150 );
    checksum();
    printf("%llu\n", seed);
}
