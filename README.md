# Sakhi - Your AI Companion

Sakhi is a multi-faceted AI companion designed to assist users in various scenarios, including providing support for individuals with visual impairments and Alzheimer's, as well as acting as a personal assistant. The project integrates a web-based frontend, a Python backend, and ESP32-powered hardware for a complete interactive experience.

## Features

*   **Face Recognition**: Secure login and user identification using face recognition.
*   **Assistive Modes**:
    *   **Blind Mode**: Provides assistance for visually impaired users.
    *   **Alzheimer's Mode**: Offers support and reminders for users with Alzheimer's.
*   **Waiter Mode**: Acts as a personal assistant to help with daily tasks.
*   **IoT Integration**: Connects with an ESP32 microcontroller to interact with the physical world (e.g., controlling LEDs, motors).
*   **Real-time Communication**: Uses Adafruit IO for real-time messaging between the web interface and the ESP32.

## Tech Stack

*   **Frontend**: HTML, CSS, JavaScript, Face-API.js
*   **Backend**: Python (Flask/FastAPI - *assumed*), Supabase
*   **Hardware**: ESP32
*   **IoT Platform**: Adafruit IO

## Getting Started

Follow these instructions to get a local copy up and running.

### Prerequisites

*   [Git](https://git-scm.com/)
*   [Python 3.x](https://www.python.org/)
*   [Node.js](https://nodejs.org/) (for potential frontend dependencies)
*   [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/) for flashing the ESP32.

### Installation & Setup

1.  **Clone the repository:**
    ```sh
    git clone https://github.com/Eshwar-neitezen/Sakhi.git
    cd Sakhi
    ```

2.  **Frontend Setup:**
    *   The frontend is currently set up as a static site. Simply open the `frontend/index.html` file in your web browser to get started.
    *   You will need to create a `.env` file in the `frontend` directory and add your Supabase credentials.

3.  **Backend Setup:**
    *   Navigate to the backend directory:
        ```sh
        cd backend
        ```
    *   Create and activate a virtual environment:
        ```sh
        python -m venv venv
        source venv/bin/activate  # On Windows use `venv\Scripts\activate`
        ```
    *   Install the required Python packages:
        ```sh
        pip install -r requirements.txt
        ```
    *   Create a `.env` file in the `backend` directory with your necessary API keys and configurations.
    *   Run the backend server:
        ```sh
        python main.py
        ```

4.  **ESP32 Setup:**
    *   Open the `esp32/esp32ifttt/esp32ifttt.ino` file (or other relevant `.ino` files) in your Arduino IDE or PlatformIO.
    *   Update the file with your Wi-Fi and Adafruit IO credentials:
        ```cpp
        #define WIFI_SSID       "your_wifi_ssid"
        #define WIFI_PASS       "your_wifi_password"
        #define AIO_USERNAME    "your_adafruit_io_username"
        #define AIO_KEY         "your_adafruit_io_key"
        ```
    *   Flash the code to your ESP32 device.
