
func $fact (
 var %n i32, var %p1 a64, var %p2 a64) i32 {
  var %r1 i32
  var  %r2 i32
  var  %r3 i32
 if (ne i32 i32 (dread i32 %n, constval i32 1)) {
    # Not correct, but for parse and dump test.
    intrinsiccallassigned JSOP_ADD (
      dread a64 %p1, dread a64 %p2, dread i32 %n) {
      dassign %r1 1
      dassign %r2 2
      dassign %r3 3
    }
    return (constval i32 0)
  }
}
