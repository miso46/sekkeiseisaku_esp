photoreflector_debug

Minimal PlatformIO project to debug the photoreflector sensor. It prints the number of detected pulses (and clears them) and the last measured interval (us).

Usage:
1. cd photoreflector_debug
2. platformio run -t upload  (or use VSCode PlatformIO)
3. Open serial monitor at 115200 baud

Default pin: GPIO21 (PCN_PIN). Change include/photoreflector.hpp if your wiring differs.
