# OpenGL: MPU6050 Cube Rotation

Demonstraçao de como usar o modulo MPU6050 com OpenGL C++ e arduino. Usar os valores do sensor para rotacionar
um cubo usando matrizes de rotação. Falta adicionar um filtro de Kalman e um display simples.

- dependencias: g++ libglew-dev libglfw3-dev libglm-dev
- requisitos: um arduino qualquer e MPU6050, alem de sistema unix-like por conta do termios.h e fcntl.h

Adicionar mpu6050.ino no arduino, compilar via `./compile`.

Conexões: VCC-VCC, GND-GND, SCL-A5, SDA-A4
