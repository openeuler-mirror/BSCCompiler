srclang 1
func &printf public used varargs extern (var %format <* i8>, ...) i32

func &print_hello public () void {
  var %ret i32
  callassigned &printf (conststr ptr "Hello, world!\x0a") { dassign %ret 0 }
  return ()
}
