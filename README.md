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

----------

### Installation

Special Delivery requires quite a bit of things to function properly.

 1. Get a Particle Electron with an Asset Tracker Shield [here](https://store.particle.io/#asset-tracker)
 2. Go through the full setup for the Electron.
 3. Clone the repository locally.
 5. You may need to recompile special-delivery.ino depending on the system firmware your Electron is running. The binary file needs to be compiled against a specific system firmware so flashing the included binary may not work for you.
 6. Flash either your recompiled firmware or the included firmware.bin to your Electron.
 7. Clone the server code repository `git clone https://github.com/bropane/special-delivery`.
 8. Setup your environment vars (TODO include .env file)
 9. Push code onto Heroku Instance
 10. Add your Electron using to the database <br/>`heroku run bash`<br/> `python manage.py shell`<br/> `from device_manager import Device`<br/>`device=Device.objects.create(name='Your name', device_id='Your Particle Electron's ID', owner='A Django user')`
 11. Next, setup you have to setup the two webhooks so that the data being published from your Electron pushes into you're Heroku instance. I've included event_webhook.json and location_webhook.json to help you get started. The url field is for you're Heroku instance, the Authorization header a Django Rest Framework Token generated for the device's owner/Django user.
 12. At this point you should be setup and Particle Cloud should be talking to your instance using HTTP API calls.

----------

##Usage

The device is fairly simple to use. Place the device where you want it to sit then call <br/>`particle call 'device name' arm`.
This will arm the device using the default settings. The device will activate its GPS and transmit coordinates if it is moved with a high enough acceleration. It will also wake up every periodically to check battery levels and publish an event if it is running low. There are more commands exposed to play with the device if you interested. Check the special-delivery.ino file for details.

--------

##Packaging

I've included some models that I used to package my device up. These were 3D printed using ABS plastic and sealed together using Acetone. There are three files that are named enc_*.stl.
