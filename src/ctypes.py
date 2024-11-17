import ctypes
from ctypes import c_int32, c_uint32, c_double, c_char_p, byref, create_string_buffer

# Load the MEDAQLib DLL
# Replace 'MEDAQLib.dll' with the full path if necessary
medaq = ctypes.WinDLL('MEDAQLib.dll')

# Define constants (from MEDAQLib.h or documentation)
SENSOR_ILD1220 = 1016  # Example value; replace with actual value from MEDAQLib.h
ERR_NOERROR = 0

# Function prototypes
# You may need to adjust the argument and return types based on MEDAQLib.h

# uint32_t CreateSensorInstance(int sensorType);
medaq.CreateSensorInstance.argtypes = [c_int32]
medaq.CreateSensorInstance.restype = c_uint32

# int ReleaseSensorInstance(uint32_t hSensor);
medaq.ReleaseSensorInstance.argtypes = [c_uint32]
medaq.ReleaseSensorInstance.restype = c_int32

# int SetParameterString(uint32_t hSensor, const char* paramName, const char* paramValue);
medaq.SetParameterString.argtypes = [c_uint32, c_char_p, c_char_p]
medaq.SetParameterString.restype = c_int32

# int SetParameterInt(uint32_t hSensor, const char* paramName, int paramValue);
medaq.SetParameterInt.argtypes = [c_uint32, c_char_p, c_int32]
medaq.SetParameterInt.restype = c_int32

# int OpenSensor(uint32_t hSensor);
medaq.OpenSensor.argtypes = [c_uint32]
medaq.OpenSensor.restype = c_int32

# int CloseSensor(uint32_t hSensor);
medaq.CloseSensor.argtypes = [c_uint32]
medaq.CloseSensor.restype = c_int32

# int Poll(uint32_t hSensor, int32_t* pRawData, double* pScaledData, int arraySize);
medaq.Poll.argtypes = [c_uint32, ctypes.POINTER(c_int32), ctypes.POINTER(c_double), c_int32]
medaq.Poll.restype = c_int32

def main():
    # Create sensor instance
    hSensor = medaq.CreateSensorInstance(SENSOR_ILD1220)
    if hSensor == 0:
        print("Failed to create sensor instance.")
        return -1

    errorCode = ERR_NOERROR

    # Set communication parameters
    errorCode = medaq.SetParameterString(hSensor, b"Port", b"COM3")  # Replace "COM3" with your actual COM port
    if errorCode != ERR_NOERROR:
        print(f"Error setting Port: {errorCode}")
        goto_cleanup(hSensor)
        return

    errorCode = medaq.SetParameterInt(hSensor, b"BaudRate", 115200)  # Use your sensor's baud rate
    if errorCode != ERR_NOERROR:
        print(f"Error setting BaudRate: {errorCode}")
        goto_cleanup(hSensor)
        return

    errorCode = medaq.SetParameterString(hSensor, b"Interface", b"RS422")
    if errorCode != ERR_NOERROR:
        print(f"Error setting Interface: {errorCode}")
        goto_cleanup(hSensor)
        return

    # Open the sensor
    errorCode = medaq.OpenSensor(hSensor)
    if errorCode != ERR_NOERROR:
        print(f"Error opening sensor: {errorCode}")
        goto_cleanup(hSensor)
        return

    print("Sensor opened successfully.")

    # Read data from the sensor
    rawData = c_int32()
    scaledData = c_double()

    errorCode = medaq.Poll(hSensor, byref(rawData), byref(scaledData), 1)
    if errorCode != ERR_NOERROR:
        print(f"Error polling data: {errorCode}")
        goto_cleanup(hSensor)
        return

    print(f"Raw Data: {rawData.value} | Scaled Data: {scaledData.value} mm")

    # Cleanup
    goto_cleanup(hSensor)

def goto_cleanup(hSensor):
    # Close the sensor if it was opened
    medaq.CloseSensor(hSensor)
    # Release the sensor instance
    medaq.ReleaseSensorInstance(hSensor)

if __name__ == "__main__":
    main()
