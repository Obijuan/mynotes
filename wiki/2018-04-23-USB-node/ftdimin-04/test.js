var libftdi = require('./build/Release/ftdimin04')
var msg = libftdi.hello();

console.log("Llamando a libftdi.hello()");
console.log(msg)

console.log("Creando contexto")
//var ctx = libftdi.create_context();

console.log("Otras pruebas")

const obj = libftdi.FtdiContextWrapper(10);
console.log(obj.plusOne());
// Prints: 11
console.log(obj.plusOne());
// Prints: 12
console.log(obj.plusOne());
// Prints: 13
