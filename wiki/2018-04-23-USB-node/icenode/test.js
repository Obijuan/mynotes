var libftdi = require('./build/Release/icenode')

// FTDI USB identifiers
const usbVendor = 0x0403;
const usbProduct = 0x6010;

function mpsse_init(ctx) {

  libftdi.ftdi_set_interface(ctx, 1);

  //-- Abrir dispositivo
  ret = libftdi.ftdi_usb_open(ctx, usbVendor, usbProduct);
  if (ret) {
    console.log("ERROR!!!")
    console.log("No encontrado dispositivo: (0x" + usbVendor.toString(16) + ', 0x' + usbProduct.toString(16) + ')');
    console.log("Error: " + libftdi.ftdi_get_error_string(ctx));
    console.log("Operation code: " + ret)
    console.log("Abort.")
    process.exit(1)
  }

  //-- Reset
  ret = libftdi.ftdi_usb_reset(ctx)
  if (ret) {
    console.log("Failed to reset iCE FTDI USB device")
    console.log("Error: " + libftdi.ftdi_get_error_string(ctx));
    console.log("Operation code: " + ret)
    console.log("Abort.")
    process.exit(1)
  }



  /*
  if (ftdi_usb_purge_buffers(&mpsse_ftdic)) {
		fprintf(stderr, "Failed to purge buffers on iCE FTDI USB device.\n");
		mpsse_error(2);
	}

	if (ftdi_get_latency_timer(&mpsse_ftdic, &mpsse_ftdi_latency) < 0) {
		fprintf(stderr, "Failed to get latency timer (%s).\n", ftdi_get_error_string(&mpsse_ftdic));
		mpsse_error(2);
	}

	//* 1 is the fastest polling, it means 1 kHz polling *
	if (ftdi_set_latency_timer(&mpsse_ftdic, 1) < 0) {
		fprintf(stderr, "Failed to set latency timer (%s).\n", ftdi_get_error_string(&mpsse_ftdic));
		mpsse_error(2);
	}

	mpsse_ftdic_latency_set = true;

	/* Enter MPSSE (Multi-Protocol Synchronous Serial Engine) mode. Set all pins to output. /
	if (ftdi_set_bitmode(&mpsse_ftdic, 0xff, BITMODE_MPSSE) < 0) {
		fprintf(stderr, "Failed to set BITMODE_MPSSE on iCE FTDI USB device.\n");
		mpsse_error(2);
	}

	// enable clock divide by 5
	mpsse_send_byte(MC_TCK_D5);

	if (slow_clock) {
		// set 50 kHz clock
		mpsse_send_byte(MC_SET_CLK_DIV);
		mpsse_send_byte(119);
		mpsse_send_byte(0x00);
	} else {
		// set 6 MHz clock
		mpsse_send_byte(MC_SET_CLK_DIV);
		mpsse_send_byte(0x00);
		mpsse_send_byte(0x00);
	}

  */

}


//--- Inicializar USB
console.log("init..")
var ctx = libftdi.create_context();
mpsse_init(ctx);

console.log("Dispositivo Abierto...")

code = libftdi.ftdi_read_chipid(ctx)
console.log("Code: " + code.toString(16))


/*




// Buffer
var buf = new Buffer.alloc(256);

// Read eeprom
var size = libftdi.ftdi_read_eeprom_getsize(ctx, buf)
console.log(libftdi.ftdi_get_error_string(ctx));
console.log('Byte count: ' + size);
console.log(buf.length)
console.log(buf.toString('hex'));
*/
