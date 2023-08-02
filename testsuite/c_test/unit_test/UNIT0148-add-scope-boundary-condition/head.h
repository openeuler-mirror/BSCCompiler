#define IF1 \
int j = 0;\
if (j< 10)\
{\
if (i > 3) \
 {\
   i--;\
 }\
 else if (i < 5)\
 {\
   j++;\
 }\
}
#define BEGIN for(i = 0; i < 10 ; i++) \
{ \
     IF1 \
     for ( ;j < 10;j++) { \
         j++;
     

#define END }}
