import serial
import time
import statistics
import threading
import speech_recognition as sr
import pyautogui

# Optional: enable plotting
ENABLE_PLOTTING = False
if ENABLE_PLOTTING:
    import matplotlib.pyplot as plt

# ---------------- CONFIG ---------------- #

SERIAL_PORT = "COM5"      # Change to your Arduino COM port
BAUD_RATE = 115200
CALIBRATION_SECONDS = 10

CLICK_LOCKOUT = 0.75  # Prevents rapid double-clicking

# ---------------------------------------- #

running = True
threshold = 0
current_value = 0
last_click_time = 0


# --------------- SERIAL READER --------------- #

def serial_reader():
    global current_value
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("[INFO] Serial connected. Listening for Mindflex EEG...")

    while running:
        line = ser.readline().decode(errors='ignore').strip()

        if line.startswith("ATT:"):
            try:
                val = int(line.split(":")[1])
                current_value = val
                print(f"EEG Attention: {val}")
            except:
                pass


# --------------- CALIBRATION ---------------- #

def calibrate():
    global threshold
    print("\n[CALIBRATION] Relax and clear your mind...")
    values = []

    start_time = time.time()
    while time.time() - start_time < CALIBRATION_SECONDS:
        values.append(current_value)
        time.sleep(0.1)

    baseline = statistics.mean(values)
    threshold = baseline + 20

    print(f"\n[CALIBRATION COMPLETE]")
    print(f"Baseline: {baseline:.2f}")
    print(f"⚡ New Click Threshold: {threshold}\n")


# --------------- VOICE COMMAND SYSTEM --------------- #

def voice_listener():
    global running

    recognizer = sr.Recognizer()
    mic = sr.Microphone()

    print("\n[VOICE COMMANDS READY]")
    print("Say: 'calibrate' | 'stop'")

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
                print("[SYSTEM] Stopping all threads...")
                break

        except sr.UnknownValueError:
            pass
        except sr.RequestError:
            print("[ERROR] Voice recognition connection issue.")


# --------------- EEG CLICK LOGIC (WITH PYAUTOGUI) --------------- #

def eeg_click_monitor():
    global current_value, threshold, last_click_time

    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)

    print("[LOGIC] Monitoring EEG for attention spikes...")

    while running:
        now = time.time()

        if current_value > threshold and (now - last_click_time) > CLICK_LOCKOUT:
            print("⚡ [EVENT] Attention spike detected → CLICK")

            # PyAutoGUI click
            pyautogui.click()

            # Also notify Arduino (optional)
            ser.write(b"CLICK\n")

            last_click_time = now

        time.sleep(0.05)


# --------------- OPTIONAL LIVE PLOTTING --------------- #

def eeg_plotter():
    if not ENABLE_PLOTTING:
        return

    plt.ion()
    xs = []
    ys = []

    while running:
        xs.append(time.time())
        ys.append(current_value)

        if len(xs) > 100:
            xs.pop(0)
            ys.pop(0)

        plt.clf()
        plt.plot(xs, ys, label="EEG Attention")
        plt.axhline(threshold, color='r', linestyle='--', label="Threshold")
        plt.legend()
        plt.pause(0.01)


# ---------------- MAIN ---------------- #

if __name__ == "__main__":
    print("======================================")
    print("   MIND FLEX EEG → PYTHON LEFT CLICK  ")
    print("======================================")
    time.sleep(1)

    # Start serial reader
    threading.Thread(target=serial_reader, daemon=True).start()

    time.sleep(2)
    calibrate()

    # Start EEG click thread
    threading.Thread(target=eeg_click_monitor, daemon=True).start()

    # Start plotter thread
    if ENABLE_PLOTTING:
        threading.Thread(target=eeg_plotter, daemon=True).start()

    # Start voice listener in foreground
    voice_listener()

    print("[SYSTEM] Program exited.")
