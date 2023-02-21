flavor 1
srclang 1
id 65535
numfuncs 1
fileinfo {
  @INFO_filename "./max.ast"}
srcfileinfo {
  2 "/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/max.c"}
LOC 2 1 5
func &max public (var %a i32 used, var %b i32 used) i32
func &max public (var %a i32 used, var %b i32 used) i32 {
  funcid 1
  funcinfo {
    @INFO_fullname "max"}


LOC 2 5 9
  if (gt u1 i32 (dread i32 %a, dread i32 %b)) {
    return (dread i32 %a)
  }
  else {
    return (dread i32 %b)
  }
LOC 2 7 9
  return (constval i32 0)
}
