import serial
import json
import random
import datetime
import time
import re
import asyncio
from azure.iot.device.aio import IoTHubDeviceClient
from azure.iot.device import Message

# constants
NUMBER_OF_INPUTS = 3
CONNECTION_STRING = "HostName=irrigation-iothub.azure-devices.net;DeviceId=raspberry_pi_arduino;SharedAccessKey=bF2t8nqg6EaoWNX0nD56ief6CyVEImokev3ZtI/U6XI="

# plant specification
MOISTURE_REQUIREMENT = '20' # moisture (%) minimum before watering
WATERING_DURATION = '1' # in seconds

# sets the plant specifications in the Arduino's code
def setPlantSpecs(arduino):
    print("Preparing message.");
    plant_spec_values = [str(MOISTURE_REQUIREMENT), str(WATERING_DURATION)]
    spec_string = ','.join(plant_spec_values)
    spec_string += '\n'

    print("Sending specifications.")
    arduino.write(spec_string.encode('utf-8'))

    time.sleep(2)
    print(arduino.readline().decode('utf-8'))
    print(arduino.readline().decode('utf-8'))


async def main(arduino):
    device_client = IoTHubDeviceClient.create_from_connection_string(CONNECTION_STRING) # create device client
    input_values = [] # create array for the sensor values
    
    while True:
        await device_client.connect()
        arduino.write(b"1") # send a byte to the arduino to begin communication
        time.sleep(1)
        while arduino.inWaiting() == 0: pass
        if arduino.inWaiting() > 0:
            sensor_str = arduino.readline().rstrip().decode("utf-8") # decode bytes to UTF-8
            re_decimal = re.compile(r"[^\d.]+") # regex to strip anything but numbers
            raw_value = re_decimal.sub("", sensor_str)

            if raw_value:
                input_values.append(float(raw_value) if '.' in raw_value else int(raw_value)) # append the actual value of the sensor to array

            if len(input_values) == NUMBER_OF_INPUTS: # when array reaches the max number of sensors then continue
                print("Sending message...")

                now = datetime.datetime.now()
                date_time = now.strftime("%d/%m/%Y, %H:%M:%S")
                
                # the arduino will send the water flag. True if plant has been watered (in this instance), false otherwise. Note these are strings too
                water = arduino.readline().rstrip().decode("utf-8")
                re_findBooleanString = re.compile(r"\btrue|false\b", re.I) # regex to strip everything but true or false
                water = re_findBooleanString.findall(water)[0] # get first element of the findall function (it's only ever a one element array)
                
                # format data to be jsonified
                data = {"humidity": input_values[0], "temperature": input_values[1], "moisture": input_values[2], "water": water, "date": date_time}
                message = json.dumps(data, indent = 4, sort_keys = True, default = str)


                print(data)
                await device_client.send_message(message) # sends JSON message to Azure IoT Hub

                print("Message sucessfully sent.")
                
                input_values = [] # clear array for new data

if __name__ == '__main__':
    arduino=serial.Serial(port = "/dev/ttyACM0", baudrate = 115200, timeout = 1) # set the Arduino serial object
    time.sleep(1)
    arduino.reset_input_buffer() # clear buffer in case of left over serial data 
    time.sleep(1)

    setPlantSpecs(arduino)
    
    asyncio.run(main(arduino))
                
    
