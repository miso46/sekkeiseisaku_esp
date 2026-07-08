#include <Arduino.h>

// ===== Pin Assign =====
const int DRV_ENABLE_PIN = 4;   // GPIO4 (VCC + MODE)
const int PHASE_PIN      = 7;   // D5
const int ENABLE_PIN     = 5;   // D3

// ===== PWM =====
const int PWM_CH = 0;
const int PWM_FREQ = 20000;
const int PWM_RESOLUTION = 8;   // 0～255

// ===== Test Parameter =====
const int PWM_MAX = 250;
const int PWM_STEP = 10;

const int ACCEL_TIME = 1000;     // ms
const int BRAKE_TIME = 1000;     // ms

//--------------------------------------------------
// ブレーキ
//--------------------------------------------------
void brakeMotor()
{
    ledcWrite(PWM_CH, 0);
}

//--------------------------------------------------
// 正転
//--------------------------------------------------
void forward()
{
    digitalWrite(PHASE_PIN, LOW);
}

//--------------------------------------------------
// 指定PWMまで0.5秒で加速
//--------------------------------------------------
void rampTo(int targetPWM)
{
    const int interval = 10;               // ms
    int steps = ACCEL_TIME / interval;

    for (int i = 0; i <= steps; i++)
    {
        int pwm = map(i, 0, steps, 0, targetPWM);
        ledcWrite(PWM_CH, pwm);
        delay(interval);
    }
}

//--------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(DRV_ENABLE_PIN, OUTPUT);
    pinMode(PHASE_PIN, OUTPUT);

    digitalWrite(DRV_ENABLE_PIN, HIGH);    // DRV8835有効
    forward();

    ledcSetup(PWM_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(ENABLE_PIN, PWM_CH);

    brakeMotor();

    Serial.println();
    Serial.println("===== Motor Drive Test =====");
}

//--------------------------------------------------
void loop()
{
    for (int pwm = PWM_STEP; pwm <= PWM_MAX; pwm += PWM_STEP)
    {
        Serial.print("PWM = ");
        Serial.println(pwm);

        forward();

        rampTo(pwm);

        brakeMotor();

        delay(BRAKE_TIME);
    }

    Serial.println("Test Finished");

    while (true)
    {
        delay(1000);
    }
}