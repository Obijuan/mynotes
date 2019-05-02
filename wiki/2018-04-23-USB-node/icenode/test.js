var libftdi = require('./build/Release/icenode')
var sleep = require('sleep');

// FTDI USB identifiers
const usbVendor = 0x0403;
const usbProduct = 0x6010;
const BITMODE_MPSSE  = 0x02;
const INTERFACE_A   = 1;

/* Mode commands */
const	MC_SETB_LOW = 0x80;    // Set Data bits LowByte
const MC_READB_LOW = 0x81;   // Read Data bits LowByte
const MC_TCK_D5 = 0x8B;      // Enable /5 div, backward compat to FT2232D
const MC_SET_CLK_DIV = 0x86; // Set clock divisor

const MC_DATA_IN  =  0x20 // When set read data (Data IN)
const MC_DATA_OUT =  0x10 // When set write data (Data OUT)
const MC_DATA_OCN = 0x01  // When set update data on negative clock edge
const MC_DATA_BITS = 0x02 // When set count bits not bytes


function mpsse_error(ret, msg) {
  console.log(msg);
  console.log("Error: " + libftdi.ftdi_get_error_string(ctx));
  console.log("Operation code: " + ret)
  console.log("Abort.")
  process.exit(1)
}

function mpsse_send_byte(data)
{
  let buf = new Buffer.alloc(1);
  buf[0] = data;
  var rc = libftdi.ftdi_write_data(ctx, buf, 1)
  if (rc != 1) {
    mpsse_error(rc, "Write error (single byte, rc=" + rc + "expected 1)");
  }
}

function mpsse_init(ctx) {

  libftdi.ftdi_set_interface(ctx, INTERFACE_A);

  //-- Abrir dispositivo
  ret = libftdi.ftdi_usb_open(ctx, usbVendor, usbProduct);
  if (ret) {
    mpsse_error(ret, "No encontrado dispositivo: (0x" + usbVendor.toString(16) + ', 0x' + usbProduct.toString(16) + ')' )
  }

  //-- Reset
  ret = libftdi.ftdi_usb_reset(ctx)
  if (ret) {
    mpsse_error(ret, "Failed to reset iCE FTDI USB device");
  }

  ret = libftdi.ftdi_usb_purge_buffers(ctx)
  if (ret) {
    mpsse_error(ret, "Failed to purge buffers on iCE FTDI USB device")
  }

  var buf_latency = new Buffer.alloc(1);
  ret = libftdi.ftdi_get_latency_timer(ctx, buf_latency)
  if (ret) {
    mpsse_error(ret, "Failed to get latency timer");
  }

  var latency = buf_latency[0];
  console.log("latency: "+latency)

  // 1 is the fastest polling, it means 1 kHz polling
  ret = libftdi.ftdi_set_latency_timer(ctx, 1)
  if (ret) {
    mpsse_error(ret, "Failed to set latency timer")
  }

  // Enter MPSSE (Multi-Protocol Synchronous Serial Engine) mode. Set all pins to output
  ret = libftdi.ftdi_set_bitmode(ctx, 0xFF, BITMODE_MPSSE);
  if (ret) {
    mpsse_error(ret, "Failed to set BITMODE_MPSSE on iCE FTDI USB device")
  }

	// enable clock divide by 5
	mpsse_send_byte(MC_TCK_D5);

  // set 6 MHz clock
	mpsse_send_byte(MC_SET_CLK_DIV);
	mpsse_send_byte(0x00);
	mpsse_send_byte(0x00);
}

function mpsse_recv_byte()
{
  var data = new Buffer.alloc(1);
  while (1) {
    let rc = libftdi.ftdi_read_data(ctx, data, 1);

    if (rc < 0) {
      mpsse_error(rc, "Read error")
    }

    if (rc == 1)
      break;

    sleep.uleep(100);
  }

  return data[0]
}

function mpsse_readb_low()
{
  mpsse_send_byte(MC_READB_LOW);
  let data = mpsse_recv_byte();
  return data;
}

function mpsse_set_gpio(gpio, direction)
{
	mpsse_send_byte(MC_SETB_LOW);
	mpsse_send_byte(gpio); // Value
	mpsse_send_byte(direction); // Direction
}

function mpsse_xfer_spi_bits(data, n)
{
	if (n < 1)
		return 0;

	// Input and output, update data on negative edge read on positive, bits.
	mpsse_send_byte(MC_DATA_IN | MC_DATA_OUT | MC_DATA_OCN | MC_DATA_BITS);
	mpsse_send_byte(n - 1);
	mpsse_send_byte(data);

	return mpsse_recv_byte();
}






// ---------------------------------------------------------
// Hardware specific CS, CReset, CDone functions
// ---------------------------------------------------------

function get_cdone()
{
  return (mpsse_readb_low() & 0x40) != 0;
}

function set_cs_creset(cs_b, creset_b)
{
  let gpio = 0;
  const direction = 0x93;

  if (cs_b) {
    // ADBUS4 (GPIOL0)
    gpio |= 0x10;
  }

  if (creset_b) {
    // ADBUS7 (GPIOL3)
    gpio |= 0x80;
  }

  mpsse_set_gpio(gpio, direction);
}

// ---------------------------------------------------------
// FLASH function implementations
// ---------------------------------------------------------


// the FPGA reset is released so also FLASH chip select should be deasserted
function flash_release_reset()
{
  set_cs_creset(1, 1);
}

// FLASH chip select deassert
function flash_chip_deselect()
{
	set_cs_creset(1, 0);
}

// FLASH chip select assert
// should only happen while FPGA reset is asserted
function flash_chip_select()
{
	set_cs_creset(0, 0);
}

function flash_reset()
{
  flash_chip_select();
  mpsse_xfer_spi_bits(0xFF, 8);
  flash_chip_deselect();

  flash_chip_select();
  mpsse_xfer_spi_bits(0xFF, 2);
  flash_chip_deselect();
}

//------------------------- MAIN -------------------------------

//-- Inicializar USB
console.log("init..")
var ctx = libftdi.create_context();
mpsse_init(ctx);

let cdone = get_cdone()
console.log("Cdone: " + (cdone ? "high" : "low"))

flash_release_reset();
sleep.usleep(100000);

//-- Test Mode
console.log("reset..")
flash_chip_deselect();
sleep.usleep(250000);

cdone = get_cdone()
console.log("cdone: " + (cdone ? "high" : "low"))

flash_reset()

/*

if (test_mode)
	{
		flash_power_up();

		flash_read_id();

		flash_power_down();

		flash_release_reset();
		usleep(250000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");
	}
  */

/*
  static void flash_power_up()
  {
  	uint8_t data_rpd[1] = { FC_RPD };
  	flash_chip_select();
  	mpsse_xfer_spi(data_rpd, 1);
  	flash_chip_deselect();
  }*/

/*
  static void flash_read_id()
  {
  	* JEDEC ID structure:
  	 * Byte No. | Data Type
  	 * ---------+----------
  	 *        0 | FC_JEDECID Request Command
  	 *        1 | MFG ID
  	 *        2 | Dev ID 1
  	 *        3 | Dev ID 2
  	 *        4 | Ext Dev Str Len
  	 *

  	uint8_t data[260] = { FC_JEDECID };
  	int len = 5; // command + 4 response bytes

  	if (verbose)
  		fprintf(stderr, "read flash ID..\n");

  	flash_chip_select();

  	// Write command and read first 4 bytes
  	mpsse_xfer_spi(data, len);

  	if (data[4] == 0xFF)
  		fprintf(stderr, "Extended Device String Length is 0xFF, "
  				"this is likely a read error. Ignorig...\n");
  	else {
  		// Read extended JEDEC ID bytes
  		if (data[4] != 0) {
  			len += data[4];
  			mpsse_xfer_spi(data + 5, len - 5);
  		}
  	}


  	flash_chip_deselect();

  	// TODO: Add full decode of the JEDEC ID.
  	fprintf(stderr, "flash ID:");
  	for (int i = 1; i < len; i++)
  		fprintf(stderr, " 0x%02X", data[i]);
  	fprintf(stderr, "\n");
  }
  */


/*
static void flash_power_down()
{
	uint8_t data[1] = { FC_PD };
	flash_chip_select();
	mpsse_xfer_spi(data, 1);
	flash_chip_deselect();
}
*/

/*

*/


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
