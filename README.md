# Special Delivery

[![N|Particle](https://cdn.particle.io/images/particle-horizontal-dark-093e6b1e.png)](https://www.particle.io/)

Special Delivery is a hardware-server solution for catching mail thieves using the Particle Electron with the Asset Tracker Shield.

  - Low Powered
  - Built in cellular and GPS
  - Motion Activated

The platform consists of:
  - Particle Electron with Asset Tracker Shield running the Special Delivery firmware
  - Heroku app running Special Delivery server code
  - Particle.io webhooks feeding events into Heroku web server

The device's intended use is to be armed and inserted into a mail box. The device will sit for a period of time until the thief moves it from it location as detected by the onboard accelerometer. The device also wakes up periodically to phone home to update battery status. If the battery is low, it sends an email to the device's owner.

### Installation

Special Delivery requires quite a bit of things to function properly.

First clone the repo(sp placeholder).

Second

Install the dependencies and devDependencies and start the server.
