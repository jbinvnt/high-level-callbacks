const updater = require('../../build/Release/vertexcentricjs.node')

let product = 1;

const updaterA = new updater.Updater(0, 1);
const updaterB = new updater.Updater(1, 2);

updaterA.setUpdate((a, b) => {
  next = a + b;
  product *= b;
  return next;
});
updaterA.run();
console.log("The 10th fibonacci number: " + updaterA.vertex());
console.log("Product of first 10 fibonacci numbers: " + product);
updaterA.run();
console.log("The 20th fibonacci number: " + updaterA.vertex());
console.log("Product of first 20 fibonacci numbers: " + product);

updaterB.setUpdate((a, b) => {
  return a * b; // computes 2^{F_10} = 2^{55}
});
updaterB.run();
console.log("");
console.log("Updater B result (this may lack precision, so compare to system value of 2^55): " + updaterB.vertex());
console.log("Value of 2^55: " + Math.pow(2, 55));