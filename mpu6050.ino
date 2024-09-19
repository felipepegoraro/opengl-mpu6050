#include <Wire.h>

#define MPU_ADDR 0x68
#define PWR_MGMT_1 0x6B

#define CONFIG_REG 0x1A
#define GYRO_CONFIG_REG 0x1B
#define ACC_CONFIG_REG 0x1C

#define GYRO_XOUT_H_REG 0x43
#define ACCEL_XOUT 0x3B

const float GYRO_SCALE = 65.5f;
const float ACC_SCALE = 4096.0f;

float roll_rate, pitch_rate, yaw_rate;
float acc_x, acc_y, acc_z;
float roll_angle, pitch_angle, yaw_angle;

void read_mpu6050() {
    Wire.beginTransmission(MPU_ADDR); // comunicação i2c com mpu6050
    Wire.write(CONFIG_REG);
    Wire.write(0x05); // DLPF 10 bd 
    Wire.endTransmission();

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACC_CONFIG_REG);
    Wire.write(0x10); // AFS_SEL 2: +8g ~ 4096
    Wire.endTransmission();

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_XOUT);
    Wire.endTransmission(); 
    Wire.requestFrom(MPU_ADDR, 6); // (x,y,z) 2b cada

    int16_t acc_x_lsb = Wire.read() << 8 | Wire.read();
    int16_t acc_y_lsb = Wire.read() << 8 | Wire.read();
    int16_t acc_z_lsb = Wire.read() << 8 | Wire.read();

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(GYRO_CONFIG_REG); 
    Wire.write(0x8); // FS_SEL[1:0] (0000 1000) -> 500°/s ~ 65.5LSB°/s
    Wire.endTransmission();                                                   

    // leitura eixo x
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(GYRO_XOUT_H_REG);
    Wire.endTransmission();
    Wire.requestFrom(MPU_ADDR, 6);

    int16_t rawx = Wire.read() << 8 | Wire.read();
    int16_t rawy = Wire.read() << 8 | Wire.read();
    int16_t rawz = Wire.read() << 8 | Wire.read();

    roll_rate  = (float)rawx / GYRO_SCALE;
    pitch_rate = (float)rawy / GYRO_SCALE;
    yaw_rate   = (float)rawz / GYRO_SCALE;

    acc_x = (float)acc_x_lsb / ACC_SCALE - 0.01;
    acc_y = (float)acc_y_lsb / ACC_SCALE + 0.01;
    acc_z = (float)acc_z_lsb / ACC_SCALE + 0.01;

    roll_angle  =  atan(acc_y / sqrt(acc_x * acc_x + acc_z * acc_z)) * (180 / 3.142);
    pitch_angle = -atan(acc_x / sqrt(acc_y * acc_y + acc_z * acc_z)) * (180 / 3.142);
}

void setup() {
    Serial.begin(57600);
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);

    Wire.setClock(400000);

    Wire.begin();
    delay(250);
    Wire.beginTransmission(MPU_ADDR); 
    Wire.write(PWR_MGMT_1);
    Wire.write(0x00);

    Wire.endTransmission();
}

float prev_time;

void loop() {
    float dt = (millis() - prev_time) / 1000.0f;
    prev_time = millis();

    read_mpu6050();

    yaw_angle += yaw_rate * dt;

    Serial.print("Roll(");
    Serial.print(roll_angle);
    Serial.print(") Pitch(");
    Serial.print(pitch_angle);
    Serial.print(") Yaw(");
    Serial.print(yaw_angle);
    Serial.println(")");

    delay(50);
}

