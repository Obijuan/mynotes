require('jquery')
require('popper.js')
require("bootstrap")
var usb = require('usb')
console.log("Estoy en index.js")

function main()
{
  console.log("Main loop!!")

  display = document.getElementById('display')

  usb.on('attach', (device)=> {
    console.log("Dispositivo conectado!");
    display.innerHTML = "OK!";
  });

  usb.on('detach', (devide) => {
    console.log("DESCONECTADO!")
    display.innerHTML = "DISCONNECTED!";
  })

}
