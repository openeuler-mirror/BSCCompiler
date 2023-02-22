flavor 1
srclang 1
id 65535
numfuncs 2
entryfunc &main
fileinfo {
  @INFO_filename "./Shl.ast"}
srcfileinfo {
  2 "/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/Shl.c"}
LOC 2 1 5
func &main public () i32
LOC 2 5 3
func &printf implicit public used varargs extern called_once (var %arg|4 <* u8>, ...) i32
func &printf implicit public used varargs extern called_once (var %arg|4 <* u8>, ...) i32
LOC 2 1 5
func &main public () i32 {
  funcid 1
  funcinfo {
    @INFO_fullname "main"}

LOC 2 2 7
  var %a_2_7 i32 used
LOC 2 3 7
  var %b_3_7 i32 used
LOC 2 4 7
  var %c_4_7 i32 used
  var %retVar_3 i32

LOC 2 2 7
  dassign %a_2_7 0 (constval i32 1)
LOC 2 3 7
  dassign %b_3_7 0 (shl i32 (dread i32 %a_2_7, constval i32 4))
LOC 2 4 7
  dassign %c_4_7 0 (shl i32 (dread i32 %a_2_7, dread i32 %b_3_7))
LOC 2 5 3
  callassigned &printf (conststr a64 "%d,%d\x0a", dread i32 %b_3_7, dread i32 %c_4_7) { dassign %retVar_3 0 }
LOC 2 6 3
  return (constval i32 0)
}
