import serial
import time
import statistics
import threading
import speech_recognition as sr

# --------------- USER CONFIG ---------------- #

SERIAL_PORT = "COM5"      # Change to your Arduino COM port
BAUD_RATE = 115200
CALIBRATION_SECONDS = 10  # How long to measure baseline EEG

# -------------------------------------------- #

running = True
threshold = 0
current_value = 0

# ---------------- SERIAL READER ---------------- #

def serial_reader():
    global current_value
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("[INFO] Serial connected. Listening for EEG...")

    while running:
        line = ser.readline().decode(errors='ignore').strip()

        if line.startswith("ATT:"):
            try:
                val = int(line.split(":")[1])
                current_value = val
                print(f"EEG Attention: {val}")
            except:
                pass

# ---------------- CALIBRATION ---------------- #

def calibrate():
    global threshold
    print("\n[CALIBRATION] Relax and clear your mind...")
    values = []

    start_time = time.time()
    while time.time() - start_time < CALIBRATION_SECONDS:
        values.append(current_value)
        time.sleep(0.1)

    baseline = statistics.mean(values)
    threshold = baseline + 20     # Adjustable buffer above resting state

    print(f"[CALIBRATION COMPLETE]")
    print(f"Baseline: {baseline}")
    print(f"Threshold set to: {threshold}\n")

# ---------------- VOICE COMMANDS ---------------- #

def voice_listener():
    global running

    recognizer = sr.Recognizer()
    mic = sr.Microphone()

    print("\n[VOICE ONLINE] Commands:")
    print(" - 'calibrate' → recalibrate EEG threshold")
    print(" - 'stop' → quit program\n")

    while running:
        try:
            with mic as source:
                recognizer.adjust_for_ambient_noise(source)
                audio = recognizer.listen(source)

            command = recognizer.recognize_google(audio).lower()
            print(f"[VOICE] You said: {command}")

            if "calibrate" in command:
                calibrate()

            elif "stop" in command:
                running = False
                print("[SYSTEM] Shutting down...")
                break

        except sr.UnknownValueError:
            pass
        except sr.RequestError:
            print("[ERROR] Could not contact speech API.")

# ---------------- MAIN LOOP ---------------- #

def eeg_logic():
    """
    This loop constantly checks EEG values and sends
    instructions to Arduino when the threshold is exceeded.
    """
    global current_value, threshold, running

    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("[LOGIC] Starting EEG left-click detector loop...")

    while running:
        if current_value > threshold:
            print("[EVENT] Attention spike! Triggering left click.")
            ser.write(b'CLICK\n')
            time.sleep(0.75)   # prevents double clicks

        time.sleep(0.05)

# ---------------- PROGRAM ENTRY ---------------- #

if __name__ == "__main__":
    print("======================================")
    print(" MIND FLEX EEG → LEFT CLICK SYSTEM ")
    print("======================================")
    print("Starting in 3 seconds...")
    time.sleep(3)

    # Start serial reader thread
    threading.Thread(target=serial_reader, daemon=True).start()

    # Initial calibration
    calibrate()

    # Start EEG click detection thread
    threading.Thread(target=eeg_logic, daemon=True).start()

    # Voice control thread
    voice_listener()

    print("[SYSTEM] Program exited.")
