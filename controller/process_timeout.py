import base64
import json
from datetime import datetime
import matplotlib.pyplot as plt

def extract_lorawan_packets(filepath,devAddr):
    packets = []
    start_processing = False

    with open(filepath, 'r') as file:
        lines = file.readlines()

    # Reverse the list to find the last occurrence of "MAC scheme set to"
    for line in reversed(lines):
        if "MAC scheme set to" in line:
            start_processing = True
            start_lineindex = lines.index(line) + 1
            break

    if start_processing:
        i = start_lineindex

        while i < len(lines):  # Start processing from the identified line
            line = lines[i]

            # print(i)

            # Check for the start of a packet transmission
            if "[LoRaWAN] Sending uplink packet ||" in line:
                packet_info = {
                    "message": line.split("||")[1].strip(),
                    "Tx_timestamp": line.split("]")[0].replace("SERIAL [", "").strip(),
                    "Rx_timestamp": None,
                    "ACK_timestamp": None
                }

                # Look for Rx and ACK timestamps until the next packet starts
                for j in range(i+1, len(lines)):
                    # print(lines[j])

                    if "[LoRaWAN] Sending uplink packet ||" in lines[j] or j == len(lines) - 1:
                        i = j
                        break

                    if "application/46f95f5e-fb72-4178-9652-e5976b974ea2/device/70b3d57ed005e1a1/event/up" in lines[j]:
                        packet_info["Rx_timestamp"] = lines[j].split("]")[0].replace("MQTT [", "").strip()

                    if "command/down || MESSAGE:" in lines[j] and packet_info["Rx_timestamp"]:
                        message_part = lines[j].split("|| MESSAGE:")[1].strip()  # Directly get the part after "MESSAGE:"
                        try:
                            json_data = json.loads(message_part)  # Assuming message_part is a valid JSON string
                            base64_value = json_data["items"][0]["phyPayload"]
                            decoded_hex = base64.b64decode(base64_value).hex()
                            devAddr_extracted = decoded_hex[2:10]
                            if devAddr_extracted == devAddr:
                                packet_info["ACK_timestamp"] = lines[j].split("]")[0].replace("MQTT [", "").strip()
                        except Exception as e:
                            print(f"Error processing JSON data: {e}")

                packets.append({packet_info["message"]: [packet_info["Tx_timestamp"], packet_info["Rx_timestamp"] or "null", packet_info["ACK_timestamp"] or "null"]})

            else:
                i += 1

        return packets

# Define a function to calculate the difference in seconds between two timestamps
def calculate_delay(start, end):
    fmt = '%Y-%m-%d %H:%M:%S.%f'
    start_time = datetime.strptime(start, fmt)
    end_time = datetime.strptime(end, fmt)
    return (end_time - start_time).total_seconds()


# Call the function with the updated filepath
filename = './latency_test/20240131_NodeC2/ALOHA/MAX_RT_0/DN_250_tr1.txt'
devADDr = 'c2674523'

lorawan_packets = extract_lorawan_packets(filename,devADDr)
# json.dump(lorawan_packets, open('timeout_json.json', 'w'), indent=4)

# Define a list to store all the ACK delays
ack_delays = []

# Iterate through each dictionary in the data list
for item in lorawan_packets:
    for key, value in item.items():
        # Check if the last two values are not "null"
        if value[1] != 'null' and value[2] != 'null':
            # Calculate the ACK delay
            delay = calculate_delay(value[0], value[2])
            ack_delays.append(delay)

# Plotting the histogram of ACK delays with a maximum x-tick of 2.0
plt.figure(figsize=(10, 6))
plt.hist(ack_delays, bins=30, color='skyblue', edgecolor='black')
plt.title('Histogram of ACK Delays')
plt.xlabel('Delay (seconds)')
plt.ylabel('Frequency')
plt.grid(axis='y', alpha=0.75)
plt.xlim(0, 3.0)  # Setting the maximum x-tick to 2.0
plt.xticks([i * 0.1 for i in range(31)])  # Setting x-ticks to show every 0.1 interval up to 2.0
plt.show()

