#include <stdio.h>
#include <math.h>

/**
 * Calculates Dew Point using the Magnus-Tetens approximation.
 * @param T Temperature in Celsius
 * @param RH Relative Humidity as a percentage (0-100)
 * @return Dew Point temperature in Celsius
 */
double calculate_dew_point(double T, double RH) {
    // Standard coefficients for liquid water
    const double a = 17.27;
    const double b = 237.3;
    
    // Intermediate gamma calculation
    double gamma = log(RH / 100.0) + ((a * T) / (b + T));
    
    // Final dew point calculation
    return (b * gamma) / (a - gamma);
}

int main() {
    double temp = 22.4; // 25°C
    double humidity = 36; // 70% RH
    
    double dew_point = calculate_dew_point(temp, humidity);
    printf("Temperature: %.1f°C\n", temp);
    printf("Relative Humidity: %.0f%%\n", humidity);
    printf("Dew Point: %.1f°C\n", dew_point);
    
    return 0;
}
