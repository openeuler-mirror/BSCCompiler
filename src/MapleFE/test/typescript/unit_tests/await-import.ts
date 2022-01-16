(async () => {
  const { default: myX, getx, setx } = await import("./M");  
  console.log(myX, getx());
  setx(3);
  console.log(myX, getx());
})();
