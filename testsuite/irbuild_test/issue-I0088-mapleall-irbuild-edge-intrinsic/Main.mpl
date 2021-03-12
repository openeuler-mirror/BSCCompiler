func $foo ( var %i i32, var %j i32, var %k i32) i32 { 
   return (
     intrinsicop i32 JSOP_STRICTEQ (
      dread i32 %i, 
      intrinsicop i32 JS_NUMBER (dread i32 %k)))
}

