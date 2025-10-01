# ESP32 Digital Picture Frame

A full-stack IoT system that transforms an ESP32 microcontroller with a 4" TFT display (ST7796 / ILI9466) into a remotely managed digital photo frame inspired by the Aura Digital Frame. The ESP32 connects to a Raspberry Pi Flask server over WiFi, dynamically retrieving and rendering images while polling for user-sent messages.

---

## Project Overview

This project demonstrates real-time device-to-server communication and media streaming in embedded environments. It integrates embedded firmware, RESTful APIs, and a browser-based dashboard to deliver a remotely managed photo display system.

Designed for educational and portfolio purposes, it showcases:

- Embedded networking (ESP32, Portforwarding, REST)
- Backend development (Flask APIs, Image processing)
- Frontend UI design (JS, HTML + CSS)

---

## System Features

### ESP32 Client

- Connects to WiFi and communicates with Raspberry Pi server
- Downloads and displays 10 images per cycle on TFT screen
- Polls server for text messages during each cycle
- Lightweight loop: `fetch → display → check message`

### Raspberry Pi Server (`server.py`)

- Hosts images in `photos/` directory
- Converts HEIC/JPEG/PNG to JPEG (320×480) using Pillow
- Exposes RESTful endpoints for image management and messaging
- Serves dashboard UI and image assets

### Web Dashboard (`image.html`, `script.js`, `style.css`)

- View, upload, and delete images via browser
- Send text messages to ESP32
- Responsive grid layout with thumbnail overlays
- Minimalist controls for intuitive interaction

---

## Technology Stack

| Layer      | Tools & Libraries                                      |
|------------|--------------------------------------------------------|
| Frontend   | HTML5, CSS3, JavaScript                        |
| Backend    | Python 3.x, Flask, Pillow, pillow-heif, Werkzeug ProxyFix, Nginx |
| Embedded   | ESP32 (Arduino framework), ST7796 / ILI9466 TFT display |
| Hardware   | ESP32, Raspberry Pi (server + storage)                |

---

## Project Structure
```bash
ESP32-DigitalFrame/
├── server.py # Flask backend (Raspberry Pi)
├── sketch_aug12a.ino # ESP32 firmware (Arduino)
├── static/
│ ├── image.html # Web dashboard
│ ├── script.js # Frontend logic
│ ├── style.css # Dashboard styling
│ ├── trash-can.png 
│ └── cloud-upload.png 
├── photos/ # Stored and resized images
└── tools.txt # Libraries and plugins used in this project.
```
---

## API Endpoints

| Method | Endpoint                | Description                          |
|--------|-------------------------|--------------------------------------|
| GET    | `/api/images`           | Returns list of available images     |
| GET    | `/photos/<filename>`    | Serves individual image              |
| GET    | `/api/convertPhotos`    | Converts/resizes all images          |
| POST   | `/upload`               | Uploads new image(s)                 |
| POST   | `/api/deleteImage`      | Deletes selected image               |
| POST   | `/api/sendMessage`      | Stores message for ESP32             |
| GET    | `/api/sendMessage`      | Retrieves and clears stored message  |

---

## Setup Instructions

```bash
RaspberryPI (Host)
pip install flask pillow pillow-heif werkzeug
python3 server.py
Server accessible at: http://localhost:5000 or http://PublicIP
```

```bash
ESP32 (Client)
Flash sketch_aug12a.ino to ESP32 via Arduino IDE

Update WiFi credentials and server IP in code

On boot, ESP32 connects to server, downloads images, and begins display loop
```

**Development Notes**
 - Server-side image resizing ensures low memory footprint on ESP32
 
 - ESP32 loop is intentionally simple for reliability and clarity

 - Messages are just a fun part, not neccesarily a big part of the project nor something you will really notice unless you're staring at the screen when a message is received. 

**Future Enhancements**
 - Local image caching on ESP32 for smoother transitions

 - Overlay messages directly on displayed images

 - Folder/album support on Raspberry Pi server

 - Replace polling with MQTT or WebSocket for real-time updates

 - Touchscreen gesture support (if hardware permits)

---

# Author

**Tyler S.** <br />
September 24, 2025.
