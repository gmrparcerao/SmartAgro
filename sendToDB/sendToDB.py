""" *---------------Header----------------------------
    SENAI Technology College "Mariano Ferraz"
    Sao Paulo, 05/03/2022
    Postgraduate - Internet of Things

    Names of postgraduate students: Claudinei, Guilherme, Renan and Wellington
    Lecturer: André and Caio Vinícius

    Goals: 
    Hardware: ESP32, raspberry PI 3, rain sensor, humidity sensor, temperature and humidity sensor,
      atmospheric pressure sensor, solenoid valve

    Libraries:
    pymysql by Daniel Golding,
    bluepy by Ian Harvey,
    time

    Reviews: 
    R000 - begin

    http://arduino.esp8266.com/stable/package_esp8266com_index.json
	https://dl.espressif.com/dl/package_esp32_index.json
	------------------------------------------------* """

# Libraries

# MySQL
import pymysql.cursors

# BLE
import bluepy.btle as btle

# Sleep time
import time

# Connect to the database
connection = pymysql.connect(
    host='localhost',
    user='root',
    password='root',
    db='grupoB',
    charset='utf8mb4',
    cursorclass=pymysql.cursors.DictCursor
)

# Infinite loop
while True:
    # Sleep time to avoid overload the database
    time.sleep(1)
    print("Beginning connection...")
    try:
        # Connect BLE by device adress
        p = btle.Peripheral("34:94:54:25:A2:DE")
        print("Connected!")
        # Recieve data by UUID
        #s = p.getServiceByUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e") #TX UUID
        time.sleep(2)
        s = p.getServiceByUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")  #Service UUID
        services = p.getServices()
        c = s.getCharacteristics()[0]
        data = str(c.read(),'UTF-8')
        #print(data)
            
        # Split data string and stores into different variables
        Humidity,Temperature,Soil,ATM,Rain,Solenoid = data.split(";")
        print(Humidity)
        print(Temperature)
        print(Soil)
        print(ATM)
        print(Rain)
        print(Solenoid)

        # By using "with", I don't need to close the cursor when finish code execution
        with connection.cursor() as cursor:
            command = 'insert into sensores (Humidity, Temperature, ATM, Soil, Rain, Solenoid) values ("{}", "{}", "{}", "{}", "{}", "{}");'.format(Humidity,Temperature,ATM,Soil,Rain,Solenoid)
            try:
                cursor.execute(command)
                # Commit command is necessary to send data to the database
                connection.commit()
            except Exception as e:
                print("Error writing data into database! " + repr(e))

        p.disconnect()
        print("Disconnected")
        
    except Exception as f:
        print("Error connecting! " + repr(f))