type $ClassA <class {@e1 i32, @e2 f32, @e3 f64}>
type $ClassB <class {@e1 i32}>

func $foo() void {
  var %Reg1 <* <$ClassB>>
  dassign %Reg1 0 (intrinsicopwithtype ptr <* <$ClassB>> DEX_CONST_CLASS ())
  intrinsiccallwithtype <* <$ClassA>> DEX_CHECK_CAST (dread ptr %Reg1 0)
}

