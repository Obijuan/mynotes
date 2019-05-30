{
  "version": "1.2",
  "package": {
    "name": "",
    "version": "",
    "description": "",
    "author": "",
    "image": ""
  },
  "design": {
    "board": "alhambra-ii",
    "graph": {
      "blocks": [
        {
          "id": "74202e1e-0af3-4471-b80e-e4d487a2aae8",
          "type": "basic.output",
          "data": {
            "name": "LED",
            "pins": [
              {
                "index": "0",
                "name": "D0",
                "value": "2"
              }
            ],
            "virtual": false
          },
          "position": {
            "x": 944,
            "y": 208
          }
        },
        {
          "id": "759c432c-d831-405d-a57e-430f9c0374f6",
          "type": "basic.input",
          "data": {
            "name": "Boton",
            "pins": [
              {
                "index": "0",
                "name": "D13",
                "value": "64"
              }
            ],
            "virtual": false,
            "clock": false
          },
          "position": {
            "x": 744,
            "y": 208
          }
        }
      ],
      "wires": [
        {
          "source": {
            "block": "759c432c-d831-405d-a57e-430f9c0374f6",
            "port": "out"
          },
          "target": {
            "block": "74202e1e-0af3-4471-b80e-e4d487a2aae8",
            "port": "in"
          }
        }
      ]
    }
  },
  "dependencies": {}
}