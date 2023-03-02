/*
yarpgen version 2.0 (build  on 2021:05:18)
Seed: 1737115969
Invocation: /home/jenkins/workspace/MapleC_pipeline/MapleC_Yarpgen_pipeline/maplec-test/Public/tools/yarpgen/yarpgen -o /home/jenkins/workspace/MapleC_pipeline/MapleC_Yarpgen_pipeline/maplec-test/../Report/LangFuzz/report/1677097781_5979/src --std=c
*/
short var_0 = (short)27984;
int var_1 = -602765228;
short var_2 = (short)18531;
unsigned short var_3 = (unsigned short)53065;
unsigned char var_4 = (unsigned char)137;
unsigned int var_5 = 981406895U;
int var_6 = 642968968;
unsigned long long int var_7 = 6761959113873782143ULL;
short var_8 = (short)12146;
short var_9 = (short)14376;
unsigned short var_10 = (unsigned short)9521;
unsigned short var_11 = (unsigned short)28341;
unsigned int var_12 = 3424791700U;
unsigned int var_13 = 3326626420U;
int var_14 = 1580706874;
_Bool var_49 = (_Bool)1;
unsigned long long int var_67 = 4834657044061192081ULL;
int arr_147 [14] [10] [23] [16] [14] ;


__attribute__((noinline))
void test(short var_0, int var_1, short var_2, unsigned short var_3, unsigned char var_4, unsigned int var_5, int var_6, unsigned long long int var_7, short var_8, short var_9, unsigned short var_10, unsigned short var_11, unsigned int var_12, unsigned int var_13, int var_14, int arr_147 [14] [10] [23] [16] [14]) {
    var_49 = ((/* implicit */_Bool) ((((/* implicit */int) ((((/* implicit */unsigned int) ((/* implicit */int) var_3))) > (var_12)))) == (((((/* implicit */int) (unsigned short)65535)) ^ (-1530240743)))));

            for(short i_28 = 0; i_28 < 1; i_28 += 4) {
              for(short i_29 = 0; i_29 < 1; i_29 += 3) {
                for(short i_35 = 0; i_35 < 1; i_35 += 1) {
                  for(short i_36 = 0; i_36 < 1; i_36 += 4) {
                    // 
// The original type of var_12 is uint. In the source code, it will be converted to int. Since i_38 subsequently accesses the array arr147 as a subscript, the type conversion will be performed. The corresponding conversion statement is cvt (i64, i32) (u32 var_12). Because ivopt uses u32 instead of i32 when creating cvt, the wrong type conversion is performed.
                    for (int i_38 = ((((/* implicit */int) var_8)) - (12146))/*0*/; i_38 < 13/*13*/; i_38 += ((((/* implicit */int) var_12)) + (870175597))/*1*/) 
                    {
                        if (((/* implicit */_Bool) ((((/* implicit */_Bool) (((_Bool)1) ? (1383282605) : (-2147483630)))) ? (((/* implicit */long long int) ((((/* implicit */_Bool) -743261339)) ? (((/* implicit */int) (signed char)-2)) : (((/* implicit */int) (short)-20123))))) : (((((/* implicit */_Bool) 31)) ? (7184191891616795253LL) : (((/* implicit */long long int) ((/* implicit */int) (unsigned char)6))))))))
                        {   printf("%d\n", i_38); // When ivopt inserts the wrong cvt, the output of 38 is not in range [0,12].
                            var_67 = ((/* implicit */unsigned long long int) ((((((/* implicit */_Bool) 1563014865U)) ? (((/* implicit */int) (signed char)-101)) : (((/* implicit */int) (unsigned char)168)))) >= (arr_147 [i_28] [i_29] [i_35] [i_36] [i_38])));

                        }

                    }
                }
            }
        }
    }
}

int main() {
  test(var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11, var_12, var_13, var_14, arr_147);
  return 0;
}

// When ivopt inserts the wrong cvt, the output of 38 is not in range [0,12].
