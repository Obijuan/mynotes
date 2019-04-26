var libftdi = require('./build/Release/ftdimin07')

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

code = libftdi.ftdi_read_chipid(ctx)
console.log("Code: " + code.toString(16))

// Buffer
var buf = new Buffer.alloc(256);

// Read eeprom
var size = libftdi.ftdi_read_eeprom_getsize(ctx, buf)
console.log(libftdi.ftdi_get_error_string(ctx));
console.log('Byte count: ' + size);
console.log(buf.length)
console.log(buf.toString('hex'));
