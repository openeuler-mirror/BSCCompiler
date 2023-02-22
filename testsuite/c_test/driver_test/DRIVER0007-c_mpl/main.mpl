flavor 1
srclang 1
id 65535
numfuncs 92
entryfunc &main
fileinfo {
  @INFO_filename "./main.ast"}
srcfileinfo {
  2 "/usr/include/stdio.h",
  3 "/home/guowanlong/Maple/OpenArkCompiler/testsuite/c_test/x64_be_enable_test/ENABLE0004-Shl/main.c",
  4 "/usr/include/bits/types.h",
  5 "/usr/include/bits/types/__mbstate_t.h",
  6 "/usr/include/bits/types/__fpos_t.h",
  7 "/usr/include/bits/types/__fpos64_t.h",
  8 "/usr/include/bits/types/struct_FILE.h",
  9 "/usr/include/bits/sys_errlist.h"}
type $_IO_FILE <struct {
  @_flags i32 align(4),
  @_IO_read_ptr <* u8> align(8),
  @_IO_read_end <* u8> align(8),
  @_IO_read_base <* u8> align(8),
  @_IO_write_base <* u8> align(8),
  @_IO_write_ptr <* u8> align(8),
  @_IO_write_end <* u8> align(8),
  @_IO_buf_base <* u8> align(8),
  @_IO_buf_end <* u8> align(8),
  @_IO_save_base <* u8> align(8),
  @_IO_backup_base <* u8> align(8),
  @_IO_save_end <* u8> align(8),
  @_markers <* <$_IO_marker>> align(8),
  @_chain <* <$_IO_FILE>> align(8),
  @_fileno i32 align(4),
  @_flags2 i32 align(4),
  @_old_offset i64 align(8),
  @_cur_column u16 align(2),
  @_vtable_offset i8,
  @_shortbuf <[1] u8>,
  @_lock <* void> align(8),
  @_offset i64 align(8),
  @_codecvt <* <$_IO_codecvt>> align(8),
  @_wide_data <* <$_IO_wide_data>> align(8),
  @_freeres_list <* <$_IO_FILE>> align(8),
  @_freeres_buf <* void> align(8),
  @__pad5 u64 align(8),
  @_mode i32 align(4),
  @_unused2 <[20] u8>}>
type $__va_list <struct {
  @__stack <* void> align(8),
  @__gr_top <* void> align(8),
  @__vr_top <* void> align(8),
  @__gr_offs i32 align(4),
  @__vr_offs i32 align(4)}>
type $_G_fpos_t <struct {
  @__pos i64 align(8),
  @__state <$__mbstate_t> align(4)}>
type $__fsid_t <struct {
  @__val <[2] i32> align(4)}>
type $__mbstate_t <struct {
  @__count i32 align(4),
  @__value <$unnamed.49> align(4)}>
type $unnamed.49 <union {
  @__wch u32 align(4),
  @__wchb <[4] u8>}>
type $_G_fpos64_t <struct {
  @__pos i64 align(8),
  @__state <$__mbstate_t> align(4)}>
type $_IO_marker <structincomplete {}>
type $_IO_codecvt <structincomplete {}>
type $_IO_wide_data <structincomplete {}>
LOC 2 146 12
func &remove public extern (var %__filename <* u8>) i32
LOC 2 148 12
func &rename public extern (var %__old <* u8>, var %__new <* u8>) i32
LOC 2 152 12
func &renameat public extern (var %__oldfd i32, var %__old <* u8>, var %__newfd i32, var %__new <* u8>) i32
LOC 2 173 14
func &tmpfile public extern () <* <$_IO_FILE>>
LOC 2 187 14
func &tmpnam public extern (var %__s <* u8>) <* u8>
LOC 2 192 14
func &tmpnam_r public extern (var %__s <* u8>) <* u8>
LOC 2 204 14
func &tempnam public extern (var %__dir <* u8>, var %__pfx <* u8>) <* u8>
LOC 2 213 12
func &fclose public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 218 12
func &fflush public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 227 12
func &fflush_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 246 14
func &fopen public extern (var %__filename <* u8> restrict, var %__modes <* u8> restrict) <* <$_IO_FILE>>
LOC 2 252 14
func &freopen public extern (var %__filename <* u8> restrict, var %__modes <* u8> restrict, var %__stream <* <$_IO_FILE>> restrict) <* <$_IO_FILE>>
LOC 2 279 14
func &fdopen public extern (var %__fd i32, var %__modes <* u8>) <* <$_IO_FILE>>
LOC 2 292 14
func &fmemopen public extern (var %__s <* void>, var %__len u64, var %__modes <* u8>) <* <$_IO_FILE>>
LOC 2 298 14
func &open_memstream public extern (var %__bufloc <* <* u8>>, var %__sizeloc <* u64>) <* <$_IO_FILE>>
LOC 2 304 13
func &setbuf public extern (var %__stream <* <$_IO_FILE>> restrict, var %__buf <* u8> restrict) void
LOC 2 308 12
func &setvbuf public extern (var %__stream <* <$_IO_FILE>> restrict, var %__buf <* u8> restrict, var %__modes i32, var %__n u64) i32
LOC 2 314 13
func &setbuffer public extern (var %__stream <* <$_IO_FILE>> restrict, var %__buf <* u8> restrict, var %__size u64) void
LOC 2 318 13
func &setlinebuf public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 326 12
func &fprintf public varargs extern (var %__stream <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 332 12
func &printf public used varargs extern (var %__format <* u8> restrict, ...) i32
LOC 2 334 12
func &sprintf public varargs extern (var %__s <* u8> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 341 12
func &vfprintf public extern (var %__s <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 347 12
func &vprintf public extern (var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 349 12
func &vsprintf public extern (var %__s <* u8> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 354 12
func &snprintf public varargs extern (var %__s <* u8> restrict, var %__maxlen u64, var %__format <* u8> restrict, ...) i32
LOC 2 358 12
func &vsnprintf public extern (var %__s <* u8> restrict, var %__maxlen u64, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 379 12
func &vdprintf public extern (var %__fd i32, var %__fmt <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 382 12
func &dprintf public varargs extern (var %__fd i32, var %__fmt <* u8> restrict, ...) i32
LOC 2 391 12
func &fscanf public varargs extern (var %__stream <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 397 12
func &scanf public varargs extern (var %__format <* u8> restrict, ...) i32
LOC 2 399 12
func &sscanf public varargs extern (var %__s <* u8> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 407 12
func &__isoc99_fscanf public varargs extern (var %__stream <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 410 12
func &__isoc99_scanf public varargs extern (var %__format <* u8> restrict, ...) i32
LOC 2 412 12
func &__isoc99_sscanf public varargs extern (var %__s <* u8> restrict, var %__format <* u8> restrict, ...) i32
LOC 2 432 12
func &vfscanf public extern (var %__s <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 440 12
func &vscanf public extern (var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 444 12
func &vsscanf public extern (var %__s <* u8> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 451 12
func &__isoc99_vfscanf public extern (var %__s <* <$_IO_FILE>> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 456 12
func &__isoc99_vscanf public extern (var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 459 12
func &__isoc99_vsscanf public extern (var %__s <* u8> restrict, var %__format <* u8> restrict, var %__arg <$__va_list>) i32
LOC 2 485 12
func &fgetc public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 486 12
func &getc public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 492 12
func &getchar public extern () i32
LOC 2 499 12
func &getc_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 500 12
func &getchar_unlocked public extern () i32
LOC 2 510 12
func &fgetc_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 521 12
func &fputc public extern (var %__c i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 522 12
func &putc public extern (var %__c i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 528 12
func &putchar public extern (var %__c i32) i32
LOC 2 537 12
func &fputc_unlocked public extern (var %__c i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 545 12
func &putc_unlocked public extern (var %__c i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 546 12
func &putchar_unlocked public extern (var %__c i32) i32
LOC 2 553 12
func &getw public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 556 12
func &putw public extern (var %__w i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 564 14
func &fgets public extern (var %__s <* u8> restrict, var %__n i32, var %__stream <* <$_IO_FILE>> restrict) <* u8>
LOC 2 603 18
func &__getdelim public extern (var %__lineptr <* <* u8>> restrict, var %__n <* u64> restrict, var %__delimiter i32, var %__stream <* <$_IO_FILE>> restrict) i32
LOC 2 606 18
func &getdelim public extern (var %__lineptr <* <* u8>> restrict, var %__n <* u64> restrict, var %__delimiter i32, var %__stream <* <$_IO_FILE>> restrict) i32
LOC 2 616 18
func &getline public extern (var %__lineptr <* <* u8>> restrict, var %__n <* u64> restrict, var %__stream <* <$_IO_FILE>> restrict) i32
LOC 2 626 12
func &fputs public extern (var %__s <* u8> restrict, var %__stream <* <$_IO_FILE>> restrict) i32
LOC 2 632 12
func &puts public extern (var %__s <* u8>) i32
LOC 2 639 12
func &ungetc public extern (var %__c i32, var %__stream <* <$_IO_FILE>>) i32
LOC 2 646 15
func &fread public extern (var %__ptr <* void> restrict, var %__size u64, var %__n u64, var %__stream <* <$_IO_FILE>> restrict) u64
LOC 2 652 15
func &fwrite public extern (var %__ptr <* void> restrict, var %__size u64, var %__n u64, var %__s <* <$_IO_FILE>> restrict) u64
LOC 2 673 15
func &fread_unlocked public extern (var %__ptr <* void> restrict, var %__size u64, var %__n u64, var %__stream <* <$_IO_FILE>> restrict) u64
LOC 2 675 15
func &fwrite_unlocked public extern (var %__ptr <* void> restrict, var %__size u64, var %__n u64, var %__stream <* <$_IO_FILE>> restrict) u64
LOC 2 684 12
func &fseek public extern (var %__stream <* <$_IO_FILE>>, var %__off i64, var %__whence i32) i32
LOC 2 689 17
func &ftell public extern (var %__stream <* <$_IO_FILE>>) i64
LOC 2 694 13
func &rewind public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 707 12
func &fseeko public extern (var %__stream <* <$_IO_FILE>>, var %__off i64, var %__whence i32) i32
LOC 2 712 16
func &ftello public extern (var %__stream <* <$_IO_FILE>>) i64
LOC 2 731 12
func &fgetpos public extern (var %__stream <* <$_IO_FILE>> restrict, var %__pos <* <$_G_fpos_t>> restrict) i32
LOC 2 736 12
func &fsetpos public extern (var %__stream <* <$_IO_FILE>>, var %__pos <* <$_G_fpos_t>>) i32
LOC 2 757 13
func &clearerr public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 759 12
func &feof public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 761 12
func &ferror public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 765 13
func &clearerr_unlocked public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 766 12
func &feof_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 767 12
func &ferror_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 775 13
func &perror public extern (var %__s <* u8>) void
LOC 2 786 12
func &fileno public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 791 12
func &fileno_unlocked public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 800 14
func &popen public extern (var %__command <* u8>, var %__modes <* u8>) <* <$_IO_FILE>>
LOC 2 806 12
func &pclose public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 812 14
func &ctermid public extern (var %__s <* u8>) <* u8>
LOC 2 840 13
func &flockfile public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 844 12
func &ftrylockfile public extern (var %__stream <* <$_IO_FILE>>) i32
LOC 2 847 13
func &funlockfile public extern (var %__stream <* <$_IO_FILE>>) void
LOC 2 858 12
func &__uflow public extern (var %arg|44 <* <$_IO_FILE>>) i32
LOC 2 859 12
func &__overflow public extern (var %arg|45 <* <$_IO_FILE>>, var %arg|46 i32) i32
LOC 3 3 5
func &max public used (var %a i32, var %b i32) i32
LOC 3 5 5
func &main public (var %argc i32, var %argv <* <* u8>>) i32
LOC 2 137 14
var $stdin extern <* <$_IO_FILE>>
LOC 2 138 14
var $stdout extern <* <$_IO_FILE>>
LOC 2 139 14
var $stderr extern <* <$_IO_FILE>>
LOC 9 26 12
var $sys_nerr extern i32
LOC 9 27 26
var $sys_errlist extern <[1] <* u8> incomplete_array> const
LOC 3 5 5
func &main public (var %argc i32, var %argv <* <* u8>>) i32 {
  funcid 92
  funcinfo {
    @INFO_fullname "main"}

LOC 3 7 9
  var %a_7_9 i32 used
LOC 3 9 9
  var %b_9_9 i32 used
  var %retVar_48 i32
  var %retVar_47 i32

LOC 3 7 9
  dassign %a_7_9 0 (constval i32 5)
LOC 3 9 9
  dassign %b_9_9 0 (constval i32 6)
LOC 3 11 19
  callassigned &max (dread i32 %a_7_9, dread i32 %b_9_9) { dassign %retVar_48 0 }
LOC 3 11 5
  callassigned &printf (conststr ptr "%d\x0a", dread i32 %retVar_48) { dassign %retVar_47 0 }
LOC 3 13
  return (constval i32 0)
}
