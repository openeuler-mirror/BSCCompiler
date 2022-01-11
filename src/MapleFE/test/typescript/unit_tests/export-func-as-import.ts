function func(n: string, o?: string): Promise<any> {
  return new Promise<any>((resolve, reject) => { resolve("OK"); });
}
export { func as import }; 
