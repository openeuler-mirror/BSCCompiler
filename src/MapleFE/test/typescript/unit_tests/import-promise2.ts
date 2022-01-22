const m : string = "./M";
import(`${m}`).then(() => {
  console.log("Completed")
}).catch(() => {
  console.log("Failed")
});
