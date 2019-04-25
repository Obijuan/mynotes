var libftdi = require('./build/Release/ftdimin01')
var msg = libftdi.hello();

console.log("Llamando a libftdi.hello()");
console.log(msg)

//var ctx = libftdi.create_context();
