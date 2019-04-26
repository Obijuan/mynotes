var libftdi = require('./build/Release/ftdimin06')

// FTDI USB identifiers
var usbVendor = 0x0403;
var usbProduct = 0x6010;

console.log("Creando contexto")
var ctx = libftdi.create_context();

console.log("Estableciendo interfaz A");
libftdi.ftdi_set_interface(ctx, 1);

console.log("Abriendo dispositivo!")
ret = libftdi.ftdi_usb_open(ctx, usbVendor, usbProduct);
console.log(libftdi.ftdi_get_error_string(ctx));
console.log("Operation code: " + ret)
