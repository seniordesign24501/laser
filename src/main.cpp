#include <iostream>
#include "MEDAQLib.h"
#include <vector>
#include <cmath>
#include <cstdlib> // For std::abs

int main() {
    uint32_t hSensor = CreateSensorInstance(SENSOR_ILD1220);
    if (hSensor == 0) {
        std::cerr << "Failed to create sensor instance." << std::endl;
        return -1;
    }

    ERR_CODE errorCode = ERR_NOERROR;  // Use ERR_CODE type from MEDAQLib

    // Constants
    const int DATA_POINTS = 24;

    // Declare variables before any 'goto' statements
    int32_t rawData[DATA_POINTS] = {0};
    double scaledData[DATA_POINTS] = {0.0};

    // Set communication parameters
    errorCode = SetParameterString(hSensor, "Port", "/dev/ttyUSB0");  // Replace with your actual device
    if (errorCode != ERR_NOERROR) {
        char errText[256];
        GetError(hSensor, errText, sizeof(errText));
        std::cerr << "Error setting Port: " << errText << std::endl;
        goto cleanup;
    }

    errorCode = SetParameterString(hSensor, "Interface", "RS422");
    if (errorCode != ERR_NOERROR) {
        char errText[256];
        GetError(hSensor, errText, sizeof(errText));
        std::cerr << "Error setting Interface: " << errText << std::endl;
        goto cleanup;
    }

    // Baud rate testing loop
    int baudRates[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1000000};
    bool baudRateFound = false;

    for (size_t i = 0; i < sizeof(baudRates)/sizeof(baudRates[0]); ++i) {
        errorCode = SetParameterInt(hSensor, "BaudRate", baudRates[i]);
        if (errorCode != ERR_NOERROR) {
            char errText[256];
            GetError(hSensor, errText, sizeof(errText));
            std::cerr << "Error setting BaudRate to " << baudRates[i] << ": " << errText << std::endl;
            continue;
        }

        // Open the sensor
        errorCode = OpenSensor(hSensor);
        if (errorCode == ERR_NOERROR) {
            baudRateFound = true;
            std::cout << "Sensor opened successfully at baud rate: " << baudRates[i] << std::endl;

            // Proceed to read data
            errorCode = Poll(hSensor, rawData, scaledData, DATA_POINTS);
            if (errorCode != ERR_NOERROR) {
                char errText[256];
                GetError(hSensor, errText, sizeof(errText));
                std::cerr << "Error polling data at baud rate " << baudRates[i] << ": " << errText << std::endl;
                // Close sensor and try next baud rate
                CloseSensor(hSensor);
                continue;
            }

            // Display the collected data
            for (int j = 0; j < DATA_POINTS; ++j) {
                std::cout << "Data Point " << j+1 << ": Raw Data = " << rawData[j]
                          << ", Scaled Data = " << scaledData[j] << " mm" << std::endl;
            }

            // Exit the loop since we have successfully read data
            break;
        } else {
            char errText[256];
            GetError(hSensor, errText, sizeof(errText));
            std::cerr << "Error opening sensor at baud rate " << baudRates[i] << ": " << errText << std::endl;
            // Close sensor and try next baud rate
            CloseSensor(hSensor);
        }
    }

    if (!baudRateFound) {
        std::cerr << "Failed to open sensor at all tested baud rates." << std::endl;
        goto cleanup;
    }

    // Apply 4th-degree polynomial regression to the collected data

    // Prepare data vectors
    std::vector<double> x(DATA_POINTS);
    std::vector<double> y(DATA_POINTS);

    for (int i = 0; i < DATA_POINTS; ++i) {
        x[i] = static_cast<double>(i);  // Replace with actual x values if available
        y[i] = scaledData[i];           // Sensor readings as y
    }

    // Degree of the polynomial
    const int degree = 4;

    // Number of coefficients
    const int numCoeffs = degree + 1;

    // Initialize matrices and vectors for the normal equations
    std::vector<std::vector<double>> X(numCoeffs, std::vector<double>(numCoeffs, 0.0));
    std::vector<double> Y(numCoeffs, 0.0);
    std::vector<double> coefficients(numCoeffs, 0.0);

    // Construct the normal equations
    for (int row = 0; row < numCoeffs; ++row) {
        for (int col = 0; col < numCoeffs; ++col) {
            double sum = 0.0;
            for (int i = 0; i < DATA_POINTS; ++i) {
                sum += std::pow(x[i], row + col);
            }
            X[row][col] = sum;
        }
        double sumY = 0.0;
        for (int i = 0; i < DATA_POINTS; ++i) {
            sumY += std::pow(x[i], row) * y[i];
        }
        Y[row] = sumY;
    }

    // Solve the linear system X * coefficients = Y using Gaussian elimination
    // For small systems, this is acceptable

    // Forward elimination
    for (int k = 0; k < numCoeffs; ++k) {
        // Partial pivoting
        int maxRow = k;
        double maxVal = std::abs(X[k][k]);
        for (int i = k + 1; i < numCoeffs; ++i) {
            if (std::abs(X[i][k]) > maxVal) {
                maxVal = std::abs(X[i][k]);
                maxRow = i;
            }
        }
        if (maxRow != k) {
            // Swap rows in X
            std::swap(X[k], X[maxRow]);
            // Swap values in Y
            std::swap(Y[k], Y[maxRow]);
        }

        // Make sure the pivot element is not zero
        if (std::abs(X[k][k]) < 1e-12) {
            std::cerr << "Matrix is singular or nearly singular!" << std::endl;
            goto cleanup;
        }

        for (int i = k + 1; i < numCoeffs; ++i) {
            double factor = X[i][k] / X[k][k];
            for (int j = k; j < numCoeffs; ++j) {
                X[i][j] -= factor * X[k][j];
            }
            Y[i] -= factor * Y[k];
        }
    }

    // Back substitution
    for (int i = numCoeffs - 1; i >= 0; --i) {
        double sum = Y[i];
        for (int j = i + 1; j < numCoeffs; ++j) {
            sum -= X[i][j] * coefficients[j];
        }
        coefficients[i] = sum / X[i][i];
    }

    // Display the polynomial coefficients
    std::cout << "\n4th-Degree Polynomial Coefficients (from degree 0 to degree " << degree << "):" << std::endl;
    for (int i = 0; i <= degree; ++i) {
        std::cout << "Coefficient of x^" << i << " = " << coefficients[i] << std::endl;
    }

    // Optional: Evaluate the polynomial at the data points and calculate the fitted values
    std::vector<double> y_fitted(DATA_POINTS, 0.0);

    for (int i = 0; i < DATA_POINTS; ++i) {
        double y_fit = 0.0;
        for (int j = 0; j <= degree; ++j) {
            y_fit += coefficients[j] * std::pow(x[i], j);
        }
        y_fitted[i] = y_fit;
    }

    // Display fitted values
    std::cout << "\nFitted Values:" << std::endl;
    for (int i = 0; i < DATA_POINTS; ++i) {
        std::cout << "x = " << x[i] << ", y_actual = " << y[i]
                  << ", y_fitted = " << y_fitted[i] << std::endl;
    }

cleanup:
    // Close the sensor if it was opened
    CloseSensor(hSensor);

    // Release the sensor instance
    ReleaseSensorInstance(hSensor);

    return 0;
}
