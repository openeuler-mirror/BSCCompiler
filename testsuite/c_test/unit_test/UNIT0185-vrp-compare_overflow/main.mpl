flavor 1
srclang 1
id 65535
numfuncs 108
entryfunc &main
fileinfo {
  @INFO_filename "./main.ast"}
srcfileinfo {
  2 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/bits/libio.h",
  3 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/stdio.h",
  4 "/home/yangyi/maple/Maple/OpenArkCompiler/main.c",
  5 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/bits/types.h",
  6 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/bits/types/__mbstate_t.h",
  7 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/bits/_G_config.h",
  8 "/usr/lib/gcc-cross/aarch64-linux-gnu/5.5.0/../../../../aarch64-linux-gnu/include/bits/sys_errlist.h"}
type $_IO_FILE <struct {
  @_flags i32,
  @_IO_read_ptr <* u8>,
  @_IO_read_end <* u8>,
  @_IO_read_base <* u8>,
  @_IO_write_base <* u8>,
  @_IO_write_ptr <* u8>,
  @_IO_write_end <* u8>,
  @_IO_buf_base <* u8>,
  @_IO_buf_end <* u8>,
  @_IO_save_base <* u8>,
  @_IO_backup_base <* u8>,
  @_IO_save_end <* u8>,
  @_markers <* <$_IO_marker>>,
  @_chain <* <$_IO_FILE>>,
  @_fileno i32,
  @_flags2 i32,
  @_old_offset i64,
  @_cur_column u16,
  @_vtable_offset i8,
  @_shortbuf <[1] u8>,
  @_lock <* void>,
  @_offset i64,
  @__pad1 <* void>,
  @__pad2 <* void>,
  @__pad3 <* void>,
  @__pad4 <* void>,
  @__pad5 u64,
  @_mode i32,
  @_unused2 <[20] u8>}>
type $__va_list <struct {
  @__stack <* void>,
  @__gr_top <* void>,
  @__vr_top <* void>,
  @__gr_offs i32,
  @__vr_offs i32}>
type $_G_fpos_t <struct {
  @__pos i64,
  @__state <$__mbstate_t>}>
type $__fsid_t <struct {
  @__val <[2] i32>}>
type $_IO_marker <struct {
  @_next <* <$_IO_marker>>,
  @_sbuf <* <$_IO_FILE>>,
  @_pos i32}>
type $__mbstate_t <struct {
  @__count i32,
  @__value <$unnamed.74>}>
type $unnamed.74 <union {
  @__wch u32,
  @__wchb <[4] u8>}>
type $_G_fpos64_t <struct {
  @__pos i64,
  @__state <$__mbstate_t>}>
type $_IO_FILE_plus <structincomplete {}>
LOC 2 389 12
func &func public noinline (var %a i32 used, var %b u1 used, var %c i32 used, var %d i32 used, var %f u1 used) i32
LOC 4 18 5
func &main public () i32
LOC 2 319 29
var $_IO_2_1_stdin_ extern <$_IO_FILE_plus>
LOC 2 320 29
var $_IO_2_1_stdout_ extern <$_IO_FILE_plus>
LOC 2 321 29
var $_IO_2_1_stderr_ extern <$_IO_FILE_plus>
LOC 3 135 25
var $stdin extern <* <$_IO_FILE>>
LOC 3 136 25
var $stdout extern <* <$_IO_FILE>>
LOC 3 137 25
var $stderr extern <* <$_IO_FILE>>
LOC 8 26 12
var $sys_nerr extern i32
LOC 8 27 26
var $sys_errlist extern <[1] <* u8> incomplete_array> const
func &rand implicit public used () i32
LOC 4 4 5
func &func public noinline (var %a i32 used, var %b u1 used, var %c i32 used, var %d i32 used, var %f u1 used) i32 {
  funcid 107
  funcinfo {
    @INFO_fullname "func"}

LOC 4 5 9
  var %e_5_9 u1 used
  var %retVar_72 i32

LOC 4 6 7
  callassigned &rand () { dassign %retVar_72 0 }
LOC 4 6 3
  if (ne u1 i32 (dread i32 %retVar_72, constval i32 0)) {
LOC 4 7 7
    dassign %a 0 (dread i32 %c)
LOC 4 8 7
    dassign %e_5_9 0 (dread u32 %b)
  }
  else {
LOC 4 10 7
    dassign %a 0 (dread i32 %d)
LOC 4 11 7
    dassign %e_5_9 0 (dread u32 %f)
  }
LOC 4 13 3
  # CHECK:cmp
  if (lt u1 u1 (
    dread i32 %a,
    dread u1 %e_5_9)) {
LOC 4 14 5
    return (constval i32 0)
  }
LOC 4 16 3
  return (constval i32 1)
}
LOC 4 18 5
func &main public () i32 {
  funcid 108
  funcinfo {
    @INFO_fullname "main"}


LOC 4 19 3
  return (constval i32 0)
}
