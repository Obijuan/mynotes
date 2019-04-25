// test.js
const libftdi = require('./build/Release/ftdimin04');

const obj = libftdi.FtdiContextWrapper(10);
console.log(obj.plusOne());
// Prints: 11
console.log(obj.plusOne());
// Prints: 12
console.log(obj.plusOne());
// Prints: 13

var obj2 = new libftdi.FtdiContextWrapper(100);
console.log(obj2.plusOne());
// Prints: 11
console.log(obj2.plusOne());
// Prints: 12
console.log(obj2.plusOne());
