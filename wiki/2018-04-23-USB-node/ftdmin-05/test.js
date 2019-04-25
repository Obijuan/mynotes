var libftdi = require('./build/Release/ftdimin05')
var msg = libftdi.hello();

console.log("Llamando a libftdi.hello()");
console.log(msg)

console.log("Otras pruebas")

const obj = libftdi.FtdiContextWrapper(10);
console.log(obj.plusOne());
// Prints: 11
console.log(obj.plusOne());
// Prints: 12
console.log(obj.plusOne());
// Prints: 13


// FTDI USB identifiers
var usbVendor = 0x0403;
var usbProduct = 0x6010;

// Read buffer size
var rxBufSize = 2048;

// Read baud rate
var rxBaudRate = 9600;

// Serial number of device
var serial = 'FT123456';

console.log("Creando contexto")
var ctx = libftdi.create_context();

console.log("Abriendo dispositivo!")
libftdi.ftdi_usb_open_desc(ctx, usbVendor, usbProduct, null, null);

console.log(ctx)
//console.log(libftdi.ftdi_get_error_string(ctx));
