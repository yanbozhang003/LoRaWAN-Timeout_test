import serial
import time
import paho.mqtt.client as mqtt
import threading
import datetime
import sys
import random

# send command
def send_initial_command(serial_port, baud_rate, DN):
    # send the first command
    command = f"DN {DN}\n"
    ser.write(command.encode('utf-8'))  # Send command
 
# Function to log serial data
def log_serial_data():
    global keep_running
#    ser = serial.Serial(serial_port, baud_rate)

    try:
        with open(log_file, 'a') as log:
            while keep_running:
                line = ser.readline().decode('utf-8')
                timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime()) + f".{int((time.time() % 1) * 1000):03d}"
                log_entry = f'SERIAL [{timestamp}] {line}'
                log.write(log_entry)
                log.flush()
    finally:
        ser.close()

# Function to log MQTT messages
def log_mqtt_messages():
    global keep_running

    def on_message(client, userdata, message):
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:23]  # Include the first three digits of the millisecond
        log_entry = f'MQTT [{timestamp}] TOPIC: {message.topic} || MESSAGE: {message.payload.decode("utf-8", "ignore")}\n'
        with open(log_file, "a") as log:
            log.write(log_entry)

    client = mqtt.Client()
    client.on_message = on_message
    client.connect(broker_ip, broker_port, keepalive=60)

    for topic in topics:
#        client.subscribe(topic)
        client.subscribe("#")

    while keep_running:
        client.loop()


# Function to shutdown the threads after a specific duration
def shutdown():
    global keep_running
    keep_running = False

if __name__ == "__main__":

    # Define the serial port and baud rate
    serial_port = '/dev/ttyACM0'  # Replace with your actual serial port
    baud_rate = 9600

    # connect to serial port
    ser = serial.Serial(serial_port, baud_rate)

    # Define the MQTT broker information
    broker_ip = "10.100.186.44"
    broker_port = 1883
    topics = [
        "application/46f95f5e-fb72-4178-9652-e5976b974ea2/device/70b3d57ed005e1a2/event/up"
    ]

    if len(sys.argv) != 3:
        print("Usage: python script.py DN duration_secs")
        sys.exit(1)

    # # Parse arguments from the command line
    DN = sys.argv[1]
    duration_secs = int(sys.argv[2])  # New parameter for script execution duration

    # Define the log file name
    log_file = f'/home/pi/ACK_reliability_test/20240203_A2_DL_cause/DN_{DN}_XCSMA2.txt'

    # Global variable to control thread execution
    keep_running = True

    # starting recording earlier than configuration only for debugging purpose
#    serial_thread = threading.Thread(target=log_serial_data)
#    mqtt_thread = threading.Thread(target=log_mqtt_messages)

    # wait for configuration to complete
    random_number = round(random.uniform(1, 6), 3)
    time.sleep(random_number)

    # Send initial command to serial port
    send_initial_command(serial_port, baud_rate, DN)

    # # wait for configuration to complete
    # time.sleep(90)

    #print("Node A1 starts recording ...")
    # Create two threads to run the tasks concurrently
    # starting recording earlier than configuration only for debugging purpose
    serial_thread = threading.Thread(target=log_serial_data)
    mqtt_thread = threading.Thread(target=log_mqtt_messages)   
 
    # Start the threads
    serial_thread.start()
    mqtt_thread.start()

    # Schedule the shutdown function
    shutdown_timer = threading.Timer(duration_secs, shutdown)
    shutdown_timer.start()

    # Wait for the threads to complete
    serial_thread.join()
    mqtt_thread.join()

    print("Node A2 completed recording")
