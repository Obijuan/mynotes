var fs = require('fs');
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

// ---------------------------------------------------------
// FLASH definitions
// ---------------------------------------------------------

// Flash command definitions
// This command list is based on the Winbond W25Q128JV Datasheet

const FC_WE = 0x06;  // Write Enable
const FC_RPD = 0xAB; // Release Power-Down, returns Device ID
const FC_JEDECID = 0x9F; // Read JEDEC ID
const FC_PP = 0x02; // Page Program
const FC_RD = 0x03; // Read Data
const FC_PD = 0xB9; // Power-down
const FC_RSR1 = 0x05; // Read Status Register 1
const FC_BE64 = 0xD8; // Block Erase 64kb


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

function mpsse_xfer_spi(data, n)
{
	if (n < 1)
		return;

	/* Input and output, update data on negative edge read on positive. */
	mpsse_send_byte(MC_DATA_IN | MC_DATA_OUT | MC_DATA_OCN);
	mpsse_send_byte(n - 1);
	mpsse_send_byte((n - 1) >> 8);

	let rc = libftdi.ftdi_write_data(ctx, data, n);
	if (rc != n) {
    mpsse_error(rc, "Write error (chunk, rc=" + rc + ", expected  " + n + ")");
	}

	for (i = 0; i < n; i++)
		data[i] = mpsse_recv_byte();
}

function mpsse_send_spi(data, n)
{
	if (n < 1)
		return;

	// Output only, update data on negative clock edge.
	mpsse_send_byte(MC_DATA_OUT | MC_DATA_OCN);
	mpsse_send_byte(n - 1);
	mpsse_send_byte((n - 1) >> 8);

	let rc = libftdi.ftdi_write_data(ctx, data, n);
	if (rc != n) {
		mpsse_error(rc, "Write error (chunk, rc=" + rc + ", expected " + n + ")", rc, n);
	}
}

function mpsse_close()
{
	libftdi.ftdi_usb_close(ctx);
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

function flash_power_up()
{
  let data = new Buffer.alloc(1);
  data[0] = FC_RPD;
  flash_chip_select();
  mpsse_xfer_spi(data, 1);
  flash_chip_deselect();
}

function flash_power_down()
{
  let data = new Buffer.alloc(1);
  data[0] = FC_PD;
	flash_chip_select();
	mpsse_xfer_spi(data, 1);
	flash_chip_deselect();
}

function flash_read_id()
{
  /* JEDEC ID structure:
   * Byte No. | Data Type
   * ---------+----------
   *        0 | FC_JEDECID Request Command
   *        1 | MFG ID
   *        2 | Dev ID 1
   *        3 | Dev ID 2
   *        4 | Ext Dev Str Len
   */

   let data = new Buffer.alloc(5);
   data[0] = FC_JEDECID;
   let len = 5; // command + 4 response bytes

   console.log("read flash ID..");
   flash_chip_select();

   // Write command and read first 4 bytes
   mpsse_xfer_spi(data, len);

   if (data[4] == 0xFF)
     console.log("Extended Device String Length is 0xFF, " +
                 "this is likely a read error. Ignorig...");

   //-- code for reading the extended (not implemented yet)
   /*
   else {
     // Read extended JEDEC ID bytes
     if (data[4] != 0) {
       len += data[4];
       mpsse_xfer_spi(data + 5, len - 5);
     }
   }*/

   flash_chip_deselect();

   // TODO: Add full decode of the JEDEC ID.
   let flash_id_str = "flash ID: ";
   for (let i = 1; i < len; i++)
     flash_id_str += " 0x" + data[i].toString(16);

   console.log(flash_id_str);

}


function flash_read(addr, data, n, verbose)
{
	if (verbose)
    console.log("read 0x" + addr.toString(16) + " 0x" + n.toString(16));

  let command = new Buffer.alloc(4);
  command[0] = FC_RD;
  command[1] = (addr >> 16);
  command[2] = (addr >> 8);
  command[3] = addr;

	flash_chip_select();
	mpsse_send_spi(command, 4);
	//memset(data, 0, n);
	mpsse_xfer_spi(data, n);
	flash_chip_deselect();

  if (verbose) {
    let str = ""
		for (let i = 0; i < n; i++)
			str += data[i].toString(16) + (i == n - 1 || i % 32 == 31 ? '\n' : ' ');
    console.log(str);
  }
}

function flash_prog(addr, data, n, verbose)
{
	if (verbose)
		console.log("prog 0x" + addr.toString(16) + " 0x" + n.toString(16));

  let command = new Buffer.alloc(4);
  command[0] = FC_PP;
  command[1] = (addr >> 16);
  command[2] = (addr >> 8);
  command[3] = addr;

	flash_chip_select();
	mpsse_send_spi(command, 4);
	mpsse_send_spi(data, n);
	flash_chip_deselect();

	if (verbose) {
    let str = ""
		for (let i = 0; i < n; i++)
			str += data[i].toString(16) + (i == n - 1 || i % 32 == 31 ? '\n' : ' ');
    console.log(str);
  }
}






function flash_read_status()
{
  let data = new Buffer.alloc(2);
  data[0] = FC_RSR1;

  flash_chip_select();
  mpsse_xfer_spi(data, 2);
  flash_chip_deselect();

  sleep.usleep(1000);

	return data[1];
}

function flash_write_enable(verbose)
{
	if (verbose) {
		console.log("status before enable:");
		var status = flash_read_status();
    flash_print_status(status)
	}

	if (verbose)
		console.log("write enable..");

  let data = new Buffer.alloc(1);
  data[0] = FC_WE;

	flash_chip_select();
	mpsse_xfer_spi(data, 1);
	flash_chip_deselect();

	if (verbose) {
		console.log("status after enable:");
		status = flash_read_status();
    flash_print_status(status)
	}
}


function flash_64kB_sector_erase(addr)
{
	console.log("erase 64kB sector at 0x" + addr.toString(16) + "..");

  let command = new Buffer.alloc(4);
  command[0] = FC_BE64;
  command[1] = (addr >> 16);
  command[2] = (addr >> 8);
  command[3] = addr;

	flash_chip_select();
	mpsse_send_spi(command, 4);
	flash_chip_deselect();
}


function flash_print_status(status)
{
  console.log("SR1: 0x" + status.toString(16))
  console.log(" - SPRL: " + ((status & (1 << 7)) == 0 ? "unlocked" : "locked"));
  console.log(" -  SPM: " + (((status & (1 << 6)) == 0) ? "Byte/Page Prog Mode" : "Sequential Prog Mode"));
  console.log(" -  EPE: " + (((status & (1 << 5)) == 0) ? "Erase/Prog success" : "Erase/Prog error"));
  console.log("-  SPM: " +  (((status & (1 << 4)) == 0) ?  "~WP asserted" : "~WP deasserted"));

  var spm = "";
  switch((status >> 2) & 0x3) {
    case 0:
      spm = "All sectors unprotected";
      break;
    case 1:
      spm = "Some sectors protected";
      break;
    case 2:
      spm = "Reserved (xxxx 10xx)";
      break;
    case 3:
      spm = "All sectors protected";
      break;
  }

  console.log(" -  SWP: " + spm);
  console.log(" -  WEL: " + (((status & (1 << 1)) == 0) ? "Not write enabled" : "Write enabled"));
  console.log(" - ~RDY: " + (((status & 0x1) == 0) ? "Ready" : "Busy"));
}

function flash_wait(verbose)
{
  if (verbose)
    console.log("waiting..");
  var count = 0;
  while (1)
  {
    let data = new Buffer.alloc(2);
    data[0] = FC_RSR1;

    flash_chip_select();
    mpsse_xfer_spi(data, 2);
    flash_chip_deselect();

    if ((data[1] & 0x01) == 0) {
      if (count < 2) {
        count++;
        if (verbose) {
          console.log("r");
        }
      } else {
        if (verbose) {
          console.log("R");
        }
        break;
      }
    } else {
      if (verbose) {
        console.log(".");
      }
      count = 0;
    }

    sleep.usleep(1000);
  }
}

//-- Read the Flash ID, for testing purposes
function test_mode()
{
  console.log("---> TEST MODE")
  console.log("reset..")
  flash_chip_deselect();
  sleep.usleep(250000);

  cdone = get_cdone()
  console.log("cdone: " + (cdone ? "high" : "low"))

  flash_reset()

  flash_power_up()

  flash_read_id();

  flash_power_down();

  flash_release_reset();
  sleep.usleep(250000);
  console.log("cdone: " + (cdone ? "high" : "low"))
}

//------------------------- MAIN -------------------------------

//-- Inicializar USB
console.log("init..")
var ctx = libftdi.create_context();
mpsse_init(ctx);

//-- Test: Read FTDI chip id
code = libftdi.ftdi_read_chipid(ctx)
console.log("FTDI ID Code: " + code.toString(16))

let cdone = get_cdone()
console.log("Cdone: " + (cdone ? "high" : "low"))

flash_release_reset();
sleep.usleep(100000);

//-- Test Mode
//test_mode()

//-- Programing the FPGA

//-- Open the bitstream file
//const BITSTREAM_FILE = 'test.bin'
const BITSTREAM_FILE = 'test-LED7.bin'
let bitstream_data = fs.readFileSync(BITSTREAM_FILE)
console.log("Filename: " + BITSTREAM_FILE)
var file_size = bitstream_data.length
console.log("Length: " + file_size)

console.log("reset..");
flash_chip_deselect();
sleep.usleep(250000);

console.log("cdone: " + (cdone ? "high" : "low"))

flash_reset();
flash_power_up();
flash_read_id();

// ---------------------------------------------------------
// Program
// ---------------------------------------------------------
console.log("Length: " + file_size)

let rw_offset = 0;

//-- Flash erase
let verbose = false;

let begin_addr = rw_offset & ~0xffff;
let end_addr = (rw_offset + file_size + 0xffff) & ~0xffff;

for (let addr = begin_addr; addr < end_addr; addr += 0x10000) {
  flash_write_enable(verbose);
  flash_64kB_sector_erase(addr);
  if (verbose)
    console.log("Status after block erase:");
  let status = flash_read_status()
  if (verbose)
    flash_print_status(status)
  flash_wait(verbose);
}

console.log("programming..")

let addr = 0;
let total_blocks = Math.trunc(file_size / 256);
let remaining = Math.trunc(file_size % 256);

console.log("Total 256 bytes blocks: " + total_blocks)

//-- Write complete blocks
for (let b = 0; b < total_blocks; b++) {
  let buf = bitstream_data.slice(addr, addr + 256);
  //console.log("Bloque: " + b + ". Size: " + buf.length);

  flash_write_enable();
  flash_prog(rw_offset + addr, buf, 256, false);
  flash_wait();

  addr += 256;
}

//-- Write the remaining not full block
if (remaining > 0) {
  let buf = bitstream_data.slice(addr, addr + remaining);
  flash_write_enable();
  flash_prog(rw_offset + addr, buf, remaining, false);
  flash_wait();
}

//-----------------------------------------------------------
//   VERYFICATION
//-----------------------------------------------------------

console.log("reading.. for verification");
addr = 0;
let buf_flash = new Buffer.alloc(256);

//-- Verify complete blocks
for (let b = 0; b < total_blocks; b++) {
  let buf_file = bitstream_data.slice(addr, addr + 256);
  flash_read(rw_offset + addr, buf_flash, 256, false);
  if (!buf_flash.equals(buf_file))
    mpsse_error(3, "Found difference between flash and file!")
  addr += 256;
}
//-- Verify the remaining block
if (remaining > 0) {
  let buf_file = bitstream_data.slice(addr, addr + remaining);
  let buf_flash = new Buffer.alloc(remaining);

  flash_read(rw_offset + addr, buf_flash, remaining, false);
  if (!buf_flash.equals(buf_file))
    mpsse_error(3, "Found difference between flash and file!")
}


// ---------------------------------------------------------
// Reset
// ---------------------------------------------------------

flash_power_down();

set_cs_creset(1, 1);
sleep.usleep(250000);

console.log("cdone: " + (cdone ? "high" : "low"))

console.log("Bye.")
mpsse_close(ctx)
