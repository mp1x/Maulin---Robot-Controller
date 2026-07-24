# Maulin---Robot-Controller
This project is a custom ESP32-based robotics controller designed for dual DC motors, quadrature encoders, an MPU6050 IMU, SSD1306 OLED display, emergency stop system, and battery monitoring. The project includes a full KiCad schematic, PCB layout, Wokwi simulation, and ESP32 firmware.

This project is not a robot itself, however, it is the electronics that power a robot, the complete control system that acts as the robot’s brain. Instead of building wheels or a chassis, this project focuses on designing a universal robotics controller built around the ESP32, capable of driving motors, reading quadrature encoders, processing IMU data, handling emergency stop safety, monitoring battery voltage, and displaying system information on an OLED dashboard. The controller integrates all the core functions real robots rely on internally: motion tracking, speed and distance measurement, drift correction, orientation sensing, and safety logic. This board can be mounted onto any future robot—rovers, line followers, balancing robots, robotic arms, or any system that needs precise motor control and feedback. In short, this project builds the brain, not the body, forming a professional foundation for future robotics development.

## Wokwi Simulation
This project includes a full Wokwi simulation of the ESP32, OLED, IMU, and encoders.

Simulation link: https://wokwi.com/projects/468589263635231745


## Project Files
- /schematic — KiCad schematic + PNG/PDF
- /pcb — PCB layout screenshots + 3D view
- /firmware — ESP32 code
- /wokwi — Simulation files
