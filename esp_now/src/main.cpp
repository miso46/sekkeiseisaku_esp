#include <WiFi.h>
#include <esp_now.h>

uint8_t targetAddress[] = {0xD4, 0xE9, 0xF4,
                           0xA7, 0xA0, 0x60}; // 文字の比率が正常な方
// uint8_t targetAddress[] = {0xA4, 0xF0, 0x0F,
//                            0x81, 0xE5, 0x34}; // 文字が縦長の方
esp_now_peer_info_t peerInfo; // ペア機の情報を格納

// 送信するデータ 中身を変えてもよい(250バイトまで)
struct Data {
  int num;
  char str[32];
};

// 送信時に呼び出される関数
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("送信: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "成功" : "失敗");
}

// 受信時に呼び出される関数
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  Data received;
  memcpy(&received, data, sizeof(received));
  Serial.println(received.num);
  Serial.println(received.str);
}

void setup() {
  Serial.begin(
      115200);         //`pio device monitor`で出力確認可能 115200で通信すること
  WiFi.mode(WIFI_STA); // ESP-NOWを使うためにwifiをSTAモードへ設定
  Serial.print("Mac address: ");
  Serial.println(WiFi.macAddress());

  // esp-nowの初期化が必要
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init error");
    return;
  }

  // ペア機の登録
  memcpy(peerInfo.peer_addr, targetAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // 受信・送信時に呼び出す関数を登録
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onReceive);
}

// 変数の登録
int num = 0;
void loop() {
  // 送信データ作成
  Data sent_data;
  sent_data.num = num;
  strncpy(sent_data.str, "hello esp-now", sizeof(sent_data.str));

  // 送信
  esp_now_send(targetAddress, (uint8_t *)&sent_data, sizeof(sent_data));
  num++;
  delay(200);
}
