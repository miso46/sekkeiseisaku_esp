#include <Arduino.h>

// ===== XIAO ESP32C3 ピン割り当て =====
const int RUN_SWITCH_PIN = 4;   // D2
const int PHASE_PIN      = 7;   // D5
const int ENABLE_PIN     = 5;   // D3

// ===== PWM設定 =====
const int ledcChannel = 0;
const int freq = 20000;
const int resolution = 8;

// ===== パラメータ =====
const int PWM_MIN  = 20;
const int PWM_MAX  = 230;
const int PWM_MIDDLE = 256/2;

const int ACCEL_TIME = 1000;   // 加速時間(ms)
const int RUN_TIME   = 1000;   // 走行時間(ms)
const int STOP_TIME  = 1000;   // 停止時間(ms)

const int ACCEL_INTERVAL = 100; // 加速分解能(ms)


//--------------------------------------------------
//モータ速度
//--------------------------------------------------
void motorWrite(int duty){
    //範囲を制限
    duty = constrain(duty, PWM_MIN, PWM_MAX);
    Serial.print("duty:");
    Serial.println(duty);
    ledcWrite(ledcChannel, duty);
}

//--------------------------------------------------
//モータ速度制御
//--------------------------------------------------
void rampControl(int targetduty){
    //開始時点のduty比を取得
    int startduty=ledcRead(ledcChannel);
    int duty=startduty;

    //ステップ数を計算
    int totalstep=ACCEL_TIME/ACCEL_INTERVAL;

    //1ステップのduty比の変更量
    int step=(targetduty-startduty)/totalstep;

    Serial.println("control start");

    for(int i=0;i<totalstep;i++){
        duty+=step;
        motorWrite(duty);
        delay(ACCEL_INTERVAL);
    }
    motorWrite(targetduty);
    Serial.println("control finish");
}
//--------------------------------------------------
//モータブレーキ
//--------------------------------------------------
void brakeMotor()
{
    ledcWrite(ledcChannel,0);
    Serial.println("Brake");
    delay(STOP_TIME);
}

//--------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(RUN_SWITCH_PIN, OUTPUT);
    pinMode(PHASE_PIN, OUTPUT);

    // モータ有効
    digitalWrite(RUN_SWITCH_PIN, HIGH);

    // 回転方向
    digitalWrite(PHASE_PIN, HIGH);

    ledcSetup(ledcChannel, freq, resolution);
    ledcAttachPin(ENABLE_PIN, ledcChannel);

    brakeMotor();

    Serial.println("===== Motor Test Start =====");
}

//--------------------------------------------------
void loop()
{
    if (Serial.available() > 0)
    {
        int targetDuty = Serial.parseInt();

        // 改行など受信バッファに残った文字を捨てる
        while (Serial.available())
        {
            Serial.read();
        }

        if (targetDuty >= PWM_MIN && targetDuty <= PWM_MAX)
        {
            Serial.print("Target Duty = ");
            Serial.println(targetDuty);

            rampControl(targetDuty);
        }
        else
        {
            Serial.println("Error : Duty must be 20 - 230");
        }
    }
}