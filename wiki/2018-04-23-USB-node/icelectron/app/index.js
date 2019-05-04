require('jquery')
require('popper.js')
require("bootstrap")
var usb = require('usb')
console.log("Estoy en index.js")

function main()
{
  console.log("Main loop!!")

  usb.on('attach', (device)=> {
    console.log("Dispositivo conectado!");
  });

  usb.on('detach', (devide) => {
    console.log("DESCONECTADO!")
  })

}
