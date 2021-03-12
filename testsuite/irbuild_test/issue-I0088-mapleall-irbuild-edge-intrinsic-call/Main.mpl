#int fact (int n) {
#  if(n != 1)
#    return foo(n - 1);
#  else return 1;
#}

func $fact (
 var %n i32, var %p1 a64, var %p2 a64) i32 {
 if (ne i32 i32 (dread i32 %n, constval i32 1)) {
    intrinsiccall JSOP_ADD (
      dread a64 %p1, dread a64 %p2, dread i32 %n)
    return (regread i32 %%retval)}
   else {
     return (constval i32 1) } }
