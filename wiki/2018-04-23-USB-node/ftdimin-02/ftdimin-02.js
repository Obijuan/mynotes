var libftdi = require('./build/Release/ftdimin02')
var msg = libftdi.hello();

console.log("Llamando a libftdi.hello()");
console.log(msg)

console.log("Creando contexto")
var ctx = libftdi.create_context();
