type $Ljava/lang/Object; <class  {
  @DEXINFO_isuserclass 0}>
type $Leh; <class <$Ljava/lang/Object;> {
  @DEXINFO_isuserclass 1,
  &foo public static (<* <$Leh;>>,<* <[0] <* <$Ljava/lang/String;>>>>) void}>
type $Ljava/lang/String; <class  {
  @DEXINFO_isuserclass 0}>
type $Ljava/io/PrintStream; <class  {
  @DEXINFO_isuserclass 0}>
type $Ljava/lang/System; <class  {
  @DEXINFO_isuserclass 0,
  @out <* <$Ljava/io/PrintStream;>>}>
type $Ljava/lang/Exception; <class  {
  @DEXINFO_isuserclass 0}>
  javaclass $javaclass|Leh; <$Leh;>

func &foo public static (var %_this <* <$Leh;>>, var %Reg2_R39 <* <[0] <* <$Ljava/lang/String;>>>>) void
var $const_array_0 <[3] i32> readonly = [1, 2, 3]
var $static|Ljava/lang/System;|out|Ljava/io/PrintStream;| <* <$Ljava/io/PrintStream;>>
func &Ljava/io/PrintStream;|print|_I_V| (var %_this <* <$Ljava/io/PrintStream;>>, var %Reg1_I i32) void
var $const_chars_1 <[11] u8> const readonly = [0, 0, 7, 0, 101, 114, 114, 111, 114, 49, 10]
func &Ljava/io/PrintStream;|print|_Ljava/lang/String;_V| (var %_this <* <$Ljava/io/PrintStream;>>, var %Reg1_R37 <* <$Ljava/lang/String;>>) void
var $const_chars_2 <[11] u8> const readonly = [0, 0, 7, 0, 101, 114, 114, 111, 114, 50, 10]

func &foo public static (var %_this <* <$Leh;>>, var %Reg2_R39 <* <[0] <* <$Ljava/lang/String;>>>>) void {
  var %Reg0_I i32
  var %Reg0_R45 <* <[3] i32>>
  var %Reg1_I i32
  var %Reg1_R47 <* <$Ljava/io/PrintStream;>>
  var %Reg0_R51 <* <$Ljava/lang/Exception;>>
  var %Reg0_R47 <* <$Ljava/io/PrintStream;>>
  var %Reg1_R53 <* u8>
  var %Reg0_R55 <* dynany>
  dassign %Reg0_I 0 (cvt i32 i8 (constval i8 3))
  javatry { @label1 }
  dassign %Reg0_R45 0 (malloc ptr (mul i32 (dread i32 %Reg0_I, sizeoftype u32 i32)))
  endtry
  intrinsiccallassigned DEX_ARRAY_SETELEM (dread ptr %Reg0_R45, constval i32 3, addrof ptr $const_array_0) {}
  dassign %Reg1_I 0 (cvt i32 i8 (constval i8 4))
  javatry { @label1 @label2 }
  dassign %Reg0_I 0 (iread i32 <* <[3] i32>> 0 (array ptr <* <[3] i32>> (dread ptr %Reg0_R45, dread i32 %Reg1_I)))
  dassign %Reg1_R47 0 (dread ptr $static|Ljava/lang/System;|out|Ljava/io/PrintStream;|)
  virtualcallassigned &Ljava/io/PrintStream;|print|_I_V| (dread ptr %Reg1_R47, dread i32 %Reg0_I) {}
  endtry
@label0   return ()
@label2   javacatch <* <$Ljava/lang/Exception;>>
  dassign %Reg0_R51 0 (regread ptr %%thrownval)
  javatry { @label1 }
  dassign %Reg0_R47 0 (dread ptr $static|Ljava/lang/System;|out|Ljava/io/PrintStream;|)
  dassign %Reg1_R53 0 (addrof ptr $const_chars_1)
  virtualcallassigned &Ljava/io/PrintStream;|print|_Ljava/lang/String;_V| (dread ptr %Reg0_R47, dread ptr %Reg1_R53) {}
  endtry
  goto @label0
@label1   javacatch <* <$Ljava/lang/Exception;>>
  dassign %Reg0_R51 0 (regread ptr %%thrownval)
  dassign %Reg0_R47 0 (dread ptr $static|Ljava/lang/System;|out|Ljava/io/PrintStream;|)
  dassign %Reg1_R53 0 (addrof ptr $const_chars_2)
  virtualcallassigned &Ljava/io/PrintStream;|print|_Ljava/lang/String;_V| (dread ptr %Reg0_R47, dread ptr %Reg1_R53) {}
  goto @label0
}

