require('jquery')
require('popper.js')
require("bootstrap")
var usb = require('usb')
const libftdi = require('./icenode')
var events = require('events');
var fpga = new events.EventEmitter();


const VENDOR_ID = 0x403;
const PRODUCT_ID = 0x6010;

//-- FPGA board constructor
function fpga_dev(product, manufacturer)
{
  this.product = product;
  this.manufacturer = manufacturer;
}

/* Get the description of the Board: Product and manufacturer */
function get_description(device, callback) {
  device.open();
  dd = device.deviceDescriptor;
  device.getStringDescriptor(dd.iManufacturer, (err, manufacturer) => {
    device.getStringDescriptor(dd.iProduct, (err, product) => {
        device.close();
        let desc = new fpga_dev(product, manufacturer);
        console.log("Prueba: " + desc)
        callback(desc)
    });
  });
}

function main()
{
  console.log("Main loop!!")

  let display = document.getElementById('display')
  let manufacturer = document.getElementById('manufacturer')
  let product = document.getElementById('product')
  let description = document.getElementById('description')
  let connection = document.getElementById('connection')
  let button_test1 = document.getElementById('button_test1')

  //-- Check the current devices connected
  let device = usb.findByIds(VENDOR_ID, PRODUCT_ID);

  //-- If connected, emit the attach event
  if (device) {
    get_description(device, (desc)=>{
      console.log(desc);
      fpga.emit('attach', desc);
    });
  }

  //-- Evento: Conexión de un USB
  //-- Comprobar si es una FPGA
  usb.on('attach', (device)=> {
    dd = device.deviceDescriptor;
    vid = dd.idVendor
    pid = dd.idProduct

    if (vid == VENDOR_ID && pid == PRODUCT_ID) {
      get_description(device, (desc)=> {
        console.log(desc);
        fpga.emit('attach', desc);
      });
    }
  });

  //-- Evento: Desconexión de un USB
  //-- Comprobar si es una FPGA
  usb.on('detach', (device) => {
    dd = device.deviceDescriptor;
    vid = dd.idVendor
    pid = dd.idProduct

    if (vid == VENDOR_ID && pid == PRODUCT_ID) {
      fpga.emit('detach');
    }
  });

  //-- Evento: FPGA conectada
  fpga.on('attach', (board)=>{
    console.log("FPGA!!!!!!")
    display.innerHTML = "OK!";
    manufacturer.innerHTML = board.manufacturer;
    product.innerHTML = board.product;
    description.style.visibility = "visible";
    connection.src = "icelectron-A2-on-300px.png"
  });

  //-- Evento: FPGA desconectada
  fpga.on('detach', () => {
    console.log("DETACH!")
    display.innerHTML = "Connect your <mark>FPGA</mark> board"
    manufacturer.innerHTML = "";
    product.innerHTML = "";
    description.style.visibility = "hidden";
    connection.src = "icelectron-A2-off-300px.png"
  });

  //-- Test1 button pressed
  button_test1.onclick = ()=> {
    console.log("Test1!!")
  }

}
