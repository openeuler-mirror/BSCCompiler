type RemoveFirstType<T extends any[]> = 
  T['length'] extends 0 ? undefined :
  (((...b: T) => void) extends (a, ...b: infer I) => void ? I : [])
