#include <Arduino.h>
#include <esp_system.h>
#include "photoreflector.hpp"
#include "motor.hpp"

static constexpr float TARGET_DISTANCE_MM = 2000.0f; // 目標距離(mm)
static constexpr float MM_PER_PULSE       = 5.0f;    // 1パルスあたりの距離(mm)

static constexpr float V_MAX   = 1.5f; // 最高巡航速度(m/s)
static constexpr float ACCEL   = 1.5f; // 加速の上限(m/s^2)
static constexpr float DECEL   = 1.0f; // 減速に使う仮定値(m/s^2)

static constexpr float STOP_DISTANCE_MM  = 5.0f;   // これ以下の残り距離で「到着」とみなす
static constexpr float STOP_VELOCITY_MPS = 0.02f;  // これ以下の速度で「停止」とみなす

// 速度PI制御のゲイン(duty per m/s)。
static constexpr float KP_V = 150.0f;
static constexpr float KI_V = 30.0f;

// 実測速度のローパスフィルタ係数(0〜1)。
// エンコーダのスリット間隔誤差による単発ノイズを均すために使う。
// 小さいほど滑らかになるが反応が遅れる。大きいほど反応は速いがノイズを拾いやすい。
static constexpr float V_FILTER_ALPHA = 0.25f;

// 1制御周期(LOOP_INTERVAL_MS)あたりのduty変化量の上限。
// ノイズが抜けても出力が急変しないようにする。
static constexpr int MAX_DUTY_STEP_PER_LOOP = 30;

static constexpr unsigned long LOOP_INTERVAL_MS   = 20;    // 制御周期
static constexpr unsigned long OVERALL_TIMEOUT_MS = 15000; // タイムアウト

// ----- CSVログバッファ -----
// 走行中はSerial出力せず貯めておき、停止した瞬間に一括出力する。
struct LogSample {
  unsigned long t_ms;
  float pos_mm;
  float remain_mm;
  float v_cmd;
  float v_act_raw;
  float v_act_filt;
  int duty;
  uint8_t state; // 0=RUN, 1=STOP
};

static constexpr int LOG_MAX = 1000; // 20ms周期 x 1000 = 20秒分
static LogSample log_buf[LOG_MAX];
static int log_count = 0;

static const char *CSV_HEADER =
    "t_ms,pos_mm,remain_mm,v_cmd_mps,v_act_raw_mps,v_act_filt_mps,duty,state";

void recordLog(float pos, float remain, float v_cmd, float v_act_raw, float v_act_filt,
               int duty, uint8_t state) {
  if (log_count < LOG_MAX) {
    log_buf[log_count++] = { millis(), pos, remain, v_cmd, v_act_raw, v_act_filt, duty, state };
  }
}

void dumpLogCsv() {
  Serial.println("---- CSV DUMP BEGIN ----");
  Serial.println(CSV_HEADER);
  for (int i = 0; i < log_count; i++) {
    const LogSample &s = log_buf[i];
    Serial.printf("%lu,%.1f,%.1f,%.3f,%.3f,%.3f,%d,%u\n",
                  s.t_ms, s.pos_mm, s.remain_mm, s.v_cmd, s.v_act_raw, s.v_act_filt,
                  s.duty, s.state);
  }
  Serial.println("---- CSV DUMP END ----");
}

// ----- 内部状態 -----
enum class State { RUNNING, STOPPED };
static State state = State::RUNNING;

static float total_distance_mm = 0.0f; // 積算位置
static float v_command = 0.0f;         // スルーレート制限後の目標速度(現在の指令速度)
static float integral_error = 0.0f;    // PI制御の積分項
static float v_actual_filt = 0.0f;     // ローパスフィルタ後の実速度
static int duty_prev = 0;              // スルーレート制限用の前回duty

static unsigned long start_ms = 0;
static unsigned long last_loop_ms = 0;

// パルス間隔から実速度を推定する。
float estimateVelocityMps() {
  auto timing = PhotoReflector::getTimingSnapshot();
  unsigned long interval_us = timing.interval_us;
  unsigned long last_pulse_us = timing.last_pulse_us;

  if (last_pulse_us == 0) {
    return 0.0f; // まだ1パルスも来ていない
  }

  unsigned long elapsed_since_last = micros() - last_pulse_us;
  unsigned long effective_interval_us = max(interval_us, elapsed_since_last);

  if (effective_interval_us == 0) {
    return 0.0f;
  }

  float interval_s = effective_interval_us / 1000000.0f;
  float dist_per_pulse_m = MM_PER_PULSE / 1000.0f;
  return dist_per_pulse_m / interval_s;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.printf("reset_reason=%d\n", (int)esp_reset_reason());
  // ESP_RST_POWERON(1) 以外(特にESP_RST_BROWNOUT=10)が出る場合は
  // 走行中に電源不足でリセットしている可能性が高い。

  Serial.println("Position Control Start");
  Serial.printf("target=%.1fmm  V_MAX=%.2fm/s  ACCEL=%.2f  DECEL=%.2f\n",
                TARGET_DISTANCE_MM, V_MAX, ACCEL, DECEL);

  PhotoReflector::setup();
  Motor::setup();

  start_ms = millis();
  last_loop_ms = start_ms;
}

void loop() {
  // シリアルコマンド処理: いつでも 'd' を送ればCSVを再出力できる。
  // USBを繋がずに走らせた場合でも、後から接続してこれで履歴を取り出せる。
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'd' || c == 'D') {
      dumpLogCsv();
    }
  }

  unsigned long now = millis();
  if (now - last_loop_ms < LOOP_INTERVAL_MS) {
    return; // 制御周期に満たない間は何もしない(busy-waitだがdelayは使わない)
  }
  float dt_s = (now - last_loop_ms) / 1000.0f;
  last_loop_ms = now;

  // 位置の更新
  unsigned long count = PhotoReflector::getAndClearCount();
  total_distance_mm += count * MM_PER_PULSE;

  float remaining_mm = TARGET_DISTANCE_MM - total_distance_mm;
  float remaining_mm_for_brake = max(remaining_mm, 0.0f);

  float v_actual_raw = estimateVelocityMps();
  // ローパスフィルタ: エンコーダのスリット間隔誤差による単発ノイズを均す
  v_actual_filt = V_FILTER_ALPHA * v_actual_raw + (1.0f - V_FILTER_ALPHA) * v_actual_filt;
  int duty = 0;

  if (state == State::RUNNING) {
    // 1. 残り距離から「速度上限」を計算
    float v_limit_by_distance = sqrtf(2.0f * DECEL * (remaining_mm_for_brake / 1000.0f));
    float v_target = min(V_MAX, v_limit_by_distance);

    // 2. スルーレート制限は「加速方向」だけにかける。
    if (v_target > v_command) {
      float max_delta = ACCEL * dt_s;
      v_command = min(v_target, v_command + max_delta);
    } else {
      v_command = v_target;
    }

    // 3. 到着判定(フィルタ後の速度を使う)
    bool arrived = (remaining_mm <= STOP_DISTANCE_MM) &&
                   (fabsf(v_actual_filt) <= STOP_VELOCITY_MPS);
    bool timed_out = (now - start_ms) > OVERALL_TIMEOUT_MS;

    if (arrived || timed_out) {
      state = State::STOPPED;
      Motor::coast();
      duty = 0;
      duty_prev = 0;
      recordLog(total_distance_mm, remaining_mm, v_command, v_actual_raw, v_actual_filt, duty, 1);

      Serial.printf("STOPPED: reason=%s  total=%.1fmm  remaining=%.1fmm  elapsed=%lums\n",
                    arrived ? "arrived" : "timeout",
                    total_distance_mm, remaining_mm, now - start_ms);
      dumpLogCsv();
    } else {
      // 4. 速度PI制御でdutyを計算(フィルタ後の速度を使う)
      float error = v_command - v_actual_filt;
      integral_error += error * dt_s;
      // アンチワインドアップ(積分の暴走を防ぐ簡易クランプ)
      integral_error = constrain(integral_error, -2.0f, 2.0f);

      float duty_f = KP_V * error + KI_V * integral_error;
      duty = (int)duty_f;

      if (v_command <= 0.0f) {
        duty = 0; // 目標速度0なら完全にコースト
        integral_error = 0.0f;
      }

      // 5. dutyのスルーレート制限(出力の急変を抑える)
      duty = constrain(duty, duty_prev - MAX_DUTY_STEP_PER_LOOP, duty_prev + MAX_DUTY_STEP_PER_LOOP);
      duty = max(duty, 0);
      duty_prev = duty;

      Motor::driveForward(duty);
      recordLog(total_distance_mm, remaining_mm, v_command, v_actual_raw, v_actual_filt, duty, 0);
    }
  }
}
