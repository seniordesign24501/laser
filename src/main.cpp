#include <iostream>
#include "MEDAQLib.h"

int main() {
    uint32_t hSensor = CreateSensorInstance(SENSOR_ILD1220);
    if (hSensor == 0) {
        std::cerr << "Failed to create sensor instance." << std::endl;
        return -1;
    }

    int errorCode = 0;

    // Declare variables before any 'goto' statements
    int32_t rawData = 0;
    double scaledData = 0.0;

    // Set communication parameters
    errorCode = SetParameterString(hSensor, "Port", "COM3");  // Replace "COM3" with your actual COM port
    if (errorCode != ERR_NOERROR) {
        std::cerr << "Error setting Port: " << errorCode << std::endl;
        goto cleanup;
    }

    errorCode = SetParameterInt(hSensor, "BaudRate", 115200);  // Use your sensor's baud rate
    if (errorCode != ERR_NOERROR) {
        std::cerr << "Error setting BaudRate: " << errorCode << std::endl;
        goto cleanup;
    }

    errorCode = SetParameterString(hSensor, "Interface", "RS422");
    if (errorCode != ERR_NOERROR) {
        std::cerr << "Error setting Interface: " << errorCode << std::endl;
        goto cleanup;
    }

    // Open the sensor
    errorCode = OpenSensor(hSensor);
    if (errorCode != ERR_NOERROR) {
        std::cerr << "Error opening sensor: " << errorCode << std::endl;
        goto cleanup;
    }

    std::cout << "Sensor opened successfully." << std::endl;

    // Read data from the sensor
    errorCode = Poll(hSensor, &rawData, &scaledData, 1);
    if (errorCode != ERR_NOERROR) {
        std::cerr << "Error polling data: " << errorCode << std::endl;
        goto cleanup;
    }

    std::cout << "Raw Data: " << rawData << " | Scaled Data: " << scaledData << " mm" << std::endl;

cleanup:
    // Close the sensor if it was opened
    CloseSensor(hSensor);

    // Release the sensor instance
    ReleaseSensorInstance(hSensor);

    return 0;
}
