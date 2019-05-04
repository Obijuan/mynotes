require('jquery')
require('popper.js')
require("bootstrap")
var usb = require('usb')
var events = require('events');
var fpga = new events.EventEmitter();

const VENDOR_ID = 0x403;
const PRODUCT_ID = 0x6010;

function main()
{
  console.log("Main loop!!")

  display = document.getElementById('display')

  usb.on('attach', (device)=> {
    bus = device.busNumber
    addr = device.deviceAddress
    deviceDescriptor = device.deviceDescriptor
    vid = device.deviceDescriptor.idVendor
    pid = device.deviceDescriptor.idProduct
    console.log("Bus " + bus + " Device " + addr + ": ID " + vid.toString(16) + ":" + pid.toString(16))

    device.open();
    device.getStringDescriptor(deviceDescriptor.iManufacturer, function (err, manufacturer) {
      device.getStringDescriptor(deviceDescriptor.iProduct, function (err, product) {
          console.log("Manufacturer: " + manufacturer);
          console.log("Product: " + product);
          device.close();
          if (vid == VENDOR_ID && pid == PRODUCT_ID) {
            console.log("Dispositivo conectado:" + product);
            fpga.emit('attach', product);
          }
      });
    });

  });

  usb.on('detach', (device) => {
    bus = device.busNumber
    addr = device.deviceAddress
    deviceDescriptor = device.deviceDescriptor
    vid = device.deviceDescriptor.idVendor
    pid = device.deviceDescriptor.idProduct
    console.log("Bus " + bus + " Device " + addr + ": ID " + vid.toString(16) + ":" + pid.toString(16))
    if (vid == VENDOR_ID && pid == PRODUCT_ID) {
      console.log("DESCONECTADO!")
      display.innerHTML = "Connect your FPGA board";
    }
  });

  fpga.on('attach', (product)=>{
    console.log("FPGA!!!!!!")
    //console.log(device);
    display.innerHTML = "OK!: " + product;
  });

}
